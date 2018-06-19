#!/usr/bin/env bash

./map-server --run-once --load-plugin script_mapquit --load-plugin hashtable --load-script npc/dev/hash_unit_test.txt --load-script npc/dev/hash_ci_test.txt
