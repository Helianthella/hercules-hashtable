/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2015 - 2016 Evol Online
 * Copyright (C) 2017 - 2018 The Mana World
 * Copyright (C) 2014 - 2018 Hercules Dev Team
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common/hercules.h"
#include "map/map.h"
#include "map/script.h"

#include "common/db.h"
#include "common/memmgr.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"hashtable",     // Plugin name
	SERVER_TYPE_MAP,     // Which server types this plugin works with?
	"0.3.0",             // Plugin version
	HPM_VERSION,         // HPM Version (don't change, macro is automatically updated)
};

#define HT_MAX_KEY_LEN 32
static int64 htreg_last_id;
static int64 htreg_last_iterator_id;
static struct DBMap *htreg_htables;
static struct DBMap *htreg_iterators;


#define checkHashTableExists(id) \
	if (!i64db_exists(htreg_htables, id)) { \
		ShowError("%s: hashtable with id=%d does not exist\n", __func__, (int)id); \
		script_pushint(st, 0); \
		return false; \
	}

BUILDIN(htnew)
{
	int64 id = htreg_last_id++;
	struct DBMap *ht = strdb_alloc(DB_OPT_DUP_KEY | DB_OPT_RELEASE_DATA, HT_MAX_KEY_LEN);
	i64db_put(htreg_htables, id, ht);
	script_pushint(st, id);
	return true;
}

BUILDIN(htget)
{
	int64 id = script_getnum(st, 2);
	checkHashTableExists(id);
	struct DBMap *ht = i64db_get(htreg_htables, id);
	const char *key = script_getstr(st, 3);
	const struct DBData *result = ht->get(ht, DB->str2key(key));

	if (result != NULL) {
		switch(result->type) {
		case DB_DATA_INT:
		case DB_DATA_UINT:
			script_pushint(st, result->u.i);
			break;
		case DB_DATA_PTR:
			script_pushstrcopy(st, result->u.ptr);
			break;
		}
	} else if (script_hasdata(st, 4)) {
		script_pushcopy(st, 4);
	} else {
		script_pushint(st, 0);
	}

	return true;
}

BUILDIN(htput)
{
	int64 id = script_getnum(st, 2);
	checkHashTableExists(id);
	struct DBMap *ht = i64db_get(htreg_htables, id);

	struct DBData value;
	const char *key = script_getstr(st, 3);
	bool keep = false;

	if (script_isstringtype(st, 4)) {
		value.type = DB_DATA_PTR;
		value.u.ptr = (void *)aStrdup(script_getstr(st, 4));
		keep = value.u.ptr != NULL && ((char *)value.u.ptr)[0] != '\0';
	} else if (script_isinttype(st, 4)) {
		value.type = DB_DATA_INT;
		value.u.i = script_getnum(st, 4);
		keep = value.u.i != 0;
	} else {
		ShowError("script:htput: illegal data type!\n");
		script_pushint(st, 0);
		return false;
	}

	if (keep) {
		ht->put(ht, DB->str2key(key), value, NULL);
	} else {
		strdb_remove(ht, key);
	}

	script_pushint(st, 1);
	return true;
}

BUILDIN(htclear)
{
	int64 id = script_getnum(st, 2);
	checkHashTableExists(id);
	struct DBMap *ht = i64db_get(htreg_htables, id);

	db_clear(ht);
	script_pushint(st, 1);
	return true;
}

BUILDIN(htdelete)
{
	int64 id = script_getnum(st, 2);
	checkHashTableExists(id);
	struct DBMap *ht = i64db_get(htreg_htables, id);

	db_destroy(ht);
	i64db_remove(htreg_htables, id);
	script_pushint(st, 1);
	return true;
}

BUILDIN(htsize)
{
	int64 id = script_getnum(st, 2);
	checkHashTableExists(id);
	struct DBMap *ht = i64db_get(htreg_htables, id);

	script_pushint(st, db_size(ht));
	return true;
}

BUILDIN(htexists)
{
	script_pushint(st, i64db_exists(htreg_htables, script_getnum(st, 2)));
	return true;
}

BUILDIN(htiterator)
{
	int64 htId = script_getnum(st, 2);
	checkHashTableExists(htId);
	struct DBMap *ht = i64db_get(htreg_htables, htId);

	int64 id = htreg_last_iterator_id++;
	struct DBIterator *it = db_iterator(ht);
	i64db_put(htreg_iterators, id, it);

	script_pushint(st, id);
	return true;
}

#undef checkHashTableExists

#define checkHtIteratorExists(id) \
	if (!i64db_exists(htreg_iterators, id)) { \
		ShowError("%s: htIterator with id=%d does not exist\n", __func__, (int)id); \
		script_pushint(st, 0); \
		return false; \
	}

BUILDIN(htifirstkey)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	union DBKey key;
	it->first(it, &key);

	if (dbi_exists(it)) {
		script_pushstrcopy(st, key.str);
	} else {
		script_pushstrcopy(st, "");
	}

	return true;
}

BUILDIN(htilastkey)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	union DBKey key;
	it->last(it, &key);

	if (dbi_exists(it)) {
		script_pushstrcopy(st, key.str);
	} else {
		script_pushstrcopy(st, "");
	}

	return true;
}

BUILDIN(htinextkey)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	union DBKey key;
	it->next(it, &key);

	if (dbi_exists(it)) {
		script_pushstrcopy(st, key.str);
	} else {
		script_pushstrcopy(st, "");
	}

	return true;
}

BUILDIN(htiprevkey)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	union DBKey key;
	it->prev(it, &key);

	if (dbi_exists(it)) {
		script_pushstrcopy(st, key.str);
	} else {
		script_pushstrcopy(st, "");
	}

	return true;
}

BUILDIN(hticheck)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	if (dbi_exists(it)) {
		script_pushint(st, 1);
	} else {
		script_pushint(st, 0);
	}

	return true;
}

BUILDIN(htidelete)
{
	int64 id = script_getnum(st, 2);
	checkHtIteratorExists(id);
	struct DBIterator *it = i64db_get(htreg_iterators, id);

	dbi_destroy(it);
	i64db_remove(htreg_iterators, id);
	script_pushint(st, 1);
	return true;
}

#undef checkHtIteratorExists


/**
 * Initializer.
 */
static void htreg_init(void)
{
	htreg_htables = i64db_alloc(DB_OPT_BASE);
	htreg_iterators = i64db_alloc(DB_OPT_BASE);
}

/**
 * Finalizer.
 */
static void htreg_final(void)
{

	if (htreg_iterators != NULL) {
		struct DBIterator *it;
		struct DBIterator *iter = db_iterator(htreg_iterators);

		for (it = dbi_first(iter); dbi_exists(iter); it = dbi_next(iter)) {
			dbi_destroy(it);
		}

		dbi_destroy(iter);
	}

	if (htreg_htables != NULL) {
		struct DBMap *ht;
		struct DBIterator *iter = db_iterator(htreg_htables);

		for (ht = dbi_first(iter); dbi_exists(iter); ht = dbi_next(iter)) {
			db_destroy(ht);
		}

		dbi_destroy(iter);
	}

	db_destroy(htreg_htables);
	db_destroy(htreg_iterators);
}

/**
 * defaults initializer.
 */
static void htreg_defaults(void)
{
	htreg_last_id = 1;
	htreg_htables = NULL;

	htreg_last_iterator_id = 1;
	htreg_iterators = NULL;
}



HPExport void server_preinit (void)
{
	htreg_defaults();
}

HPExport void plugin_init (void)
{
	htreg_init();

	addScriptCommand("htnew", "", htnew);
	addScriptCommand("htget", "is?", htget);
	addScriptCommand("htput", "isv", htput);
	addScriptCommand("htclear", "i", htclear);
	addScriptCommand("htdelete", "i", htdelete);
	addScriptCommand("htsize", "i", htsize);
	addScriptCommand("htexists", "i", htexists);

	addScriptCommand("htiterator", "i", htiterator);
	addScriptCommand("htifirstkey", "i", htifirstkey);
	addScriptCommand("htilastkey", "i", htilastkey);
	addScriptCommand("htinextkey", "i", htinextkey);
	addScriptCommand("htiprevkey", "i", htiprevkey);
	addScriptCommand("hticheck", "i", hticheck);
	addScriptCommand("htidelete", "i", htidelete);
}

HPExport void plugin_final (void)
{
	htreg_final();
}
