include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(CMAKE_CXX_STANDARD 11)

set(SRC sciplot.cpp)

include_directories(.)

add_library(sciplot SHARED ${SRC})
set_target_properties(sciplot PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)
set_target_properties(sciplot PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)




