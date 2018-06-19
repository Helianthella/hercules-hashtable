#!/usr/bin/env bash

uname -a
sudo apt-get -y install git zlib1g-dev make gcc libpcre3-dev
git clone --depth 1 git://github.com/HerculesWS/Hercules.git herc

cp src/hashtable.c herc/src/plugins
cp .tools/scripts/unit_test.txt herc/npc/dev/hash_unit_test.txt
cp .tools/scripts/ci_test.txt herc/npc/dev/hash_ci_test.txt

pushd herc
./tools/ci/travis.sh createdb ragnarok $DATABASE_MYSQL_USERNAME $DATABASE_MYSQL_PASSWORD
./tools/ci/travis.sh importdb ragnarok $DATABASE_MYSQL_USERNAME $DATABASE_MYSQL_PASSWORD
./tools/ci/travis.sh adduser ragnarok ragnarok ragnarok $DATABASE_MYSQL_USERNAME $DATABASE_MYSQL_PASSWORD
./tools/ci/travis.sh build --enable-debug --enable-Werror --enable-buildbot
make plugin.hashtable

echo -e "sql_connection: {\n\tdb_hostname: \"localhost\"\n\tdb_username: \"ragnarok\"\n\tdb_password: \"ragnarok\"\n\tdb_database: \"ragnarok\"\n}" > conf/semaphore_sql_connection.conf
echo -e "map_configuration: {\n\t@include \"conf/semaphore_sql_connection.conf\"\n}" > conf/import/map-server.conf
echo -e "inter_configuration: {\n\t@include \"conf/semaphore_sql_connection.conf\"\n}" > conf/import/inter-server.conf
popd
