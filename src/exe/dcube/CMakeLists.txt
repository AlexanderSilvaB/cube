include_directories(../../lib)

set(SRC main.c debug.c)

add_executable(dcube ${SRC})
target_link_libraries (dcube cube)

install(TARGETS dcube
    EXPORT dcube_Targets
    ARCHIVE DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin ${CMAKE_INSTALL_BINDIR})