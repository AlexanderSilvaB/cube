include_directories(..)

set(SRC calc.cpp)

add_library(calc SHARED ${SRC})

set_target_properties(calc PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/examples
)