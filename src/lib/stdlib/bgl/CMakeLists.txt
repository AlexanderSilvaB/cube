include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

add_subdirectory(basicgl)
set_property(TARGET basicgl PROPERTY POSITION_INDEPENDENT_CODE ON)

include_directories(basicgl/src/basicgl)

set(SRC bgl.cpp)

add_library(bgl SHARED ${SRC})
target_link_libraries(bgl basicgl)
set_target_properties(bgl PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
set_target_properties(bgl PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)