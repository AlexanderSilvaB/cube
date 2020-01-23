include_directories(..)

find_package (Threads)



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
        gc.c
        threads.c)

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/cube.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cubeext.h)

add_library(cube ${SRC})
target_link_libraries(cube m linenoise)

target_link_libraries(cube dl)

target_link_libraries (cube ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(cube PROPERTIES PUBLIC_HEADER "${INCLUDES}")