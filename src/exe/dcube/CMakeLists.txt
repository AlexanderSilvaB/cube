include_directories(../../lib)

set(SRC main.c debug.c)

add_executable(dcube ${SRC})
target_link_libraries (dcube cube)