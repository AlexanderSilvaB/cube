include_directories(../../lib)

set(SRC main.cpp)

add_executable(cube ${SRC})
target_link_libraries(cube  libcube)