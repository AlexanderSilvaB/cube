include_directories(../../lib)

set(SRC main.c)

add_executable(main ${SRC})
target_link_libraries(main  cube)
set_target_properties(main PROPERTIES OUTPUT_NAME cube)