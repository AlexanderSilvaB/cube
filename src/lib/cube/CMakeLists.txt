include_directories(..)

add_subdirectory(external)

include(GNUInstallDirs)
find_package(Threads)

include_directories(../libffi-3.3/include)
include_directories(../libuv/include)
include_directories(${CMAKE_BINARY_DIR}/src/lib/libffi-3.3/include)

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
        bytes.c
        collections.c
        files.c
        processes.c
        enums.c
        tasks.c
        linkedList.c
        native.c
        gc.c
        threads.c
        mempool.c
        ansi_escapes.c
        packer.c)

if (UNIX)
    list(APPEND SRC native_posix.c)
else(UNIX)
	list(APPEND SRC native_w32.c)
endif (UNIX)

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/cube.h
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cubeext.h)

add_library(cube ${SRC})
target_link_libraries(cube cube_linenoise cjson ffi_static uv_a)
IF (NOT WIN32)
target_link_libraries(cube m)
else()
target_link_libraries(cube Imagehlp)
endif()

if (UNIX)
    target_link_libraries(cube dl)
endif (UNIX)

target_link_libraries (cube ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(cube PROPERTIES PUBLIC_HEADER "${INCLUDES}")

#install(FILES ${INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS cube
    EXPORT cube_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cube)
