include_directories(..)

set(SRC cube.c chunk.c memory.c debug.c value.c vm.c compiler.c scanner.c object.c table.c std.c)

add_library(cube ${SRC})