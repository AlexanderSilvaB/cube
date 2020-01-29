include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(SRC zip.c kuba-zip.c)

add_library(zip SHARED ${SRC})
set_target_properties(zip PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)