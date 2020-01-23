include_directories(..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(SRC linenoise.cpp)

add_library(linenoise STATIC ${SRC})
         
install(TARGETS linenoise
    EXPORT cube_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})