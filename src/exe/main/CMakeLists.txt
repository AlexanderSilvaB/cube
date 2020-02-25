include_directories(../../lib)

set(SRC main.c)

add_executable(main ${SRC})
target_link_libraries(main  cube)
set_target_properties(main PROPERTIES OUTPUT_NAME cube)

install(TARGETS main
    EXPORT main_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})