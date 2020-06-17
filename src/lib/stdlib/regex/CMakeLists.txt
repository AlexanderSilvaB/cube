include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

include_directories(tiny-regex-c)

set(SRC regex.c tiny-regex-c/re.c)

add_library(regex SHARED ${SRC})
set_target_properties(regex PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
set_target_properties(regex PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)