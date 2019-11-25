include_directories(../../lib)

set(SRC main.c)

add_executable(main ${SRC})
target_link_libraries(main  cube)