include_directories(../../lib)

set(SRC main.c debug.c)

add_executable(dcube ${SRC})
target_link_libraries (dcube cube)

install(TARGETS dcube
    EXPORT dcube_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})