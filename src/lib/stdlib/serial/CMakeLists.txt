include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

if (UNIX)
        set(SRC serial-unix.c)
else(UNIX)
        set(SRC serial-win.c)
endif(UNIX)

add_library(serial SHARED ${SRC})
set_target_properties(serial PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
set_target_properties(serial PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)



