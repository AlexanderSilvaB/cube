include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

add_subdirectory(mles)
set_property(TARGET mles PROPERTY POSITION_INDEPENDENT_CODE ON)

include_directories(mles/src/lib)

set(SRC ml.cpp nn.cpp dataset.cpp)

add_library(ml SHARED ${SRC})
target_link_libraries(ml mles)
set_target_properties(ml PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
set_target_properties(ml PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)