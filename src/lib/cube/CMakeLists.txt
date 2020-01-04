include_directories(..)

set(SRC cube.c 
        chunk.c 
        memory.c 
        debug.c 
        value.c 
        vm.c 
        compiler.c 
        scanner.c 
        object.c 
        table.c 
        std.c 
        util.c 
        strings.c
        collections.c
        files.c
        linkedList.c
        native.c
        gc.c)

add_library(cube ${SRC})
target_link_libraries(cube m dl)