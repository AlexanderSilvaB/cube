include_directories(..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(SRC calc.c)


add_library(calc SHARED ${SRC})
set_target_properties(calc PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/)