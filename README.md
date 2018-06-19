# Hash table plugin for Hercules
[![Build Status](https://semaphoreci.com/api/v1/mekolat/hercules-hashtable/branches/master/badge.svg)](https://semaphoreci.com/mekolat/hercules-hashtable)

The documentation is in [doc] and the plugin is in [src].<br>
You do not need [.tools], as this is only used for unit tests.

<br>

## Installation
1. Put [hashtable.c] in `Hercules/src/plugins`
2. Run `make plugin.hashtable`
3. You can now load it with `./map-server --load-plugin hashtable`

## Usage
see [doc/script_commands.txt](doc/script_commands.txt)



[doc]: doc
[src]: src
[.tools]: .tools
[hashtable.c]: src/hashtable.c
