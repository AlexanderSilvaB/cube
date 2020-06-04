include_directories(..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(SRC linenoise.cpp)

add_definitions(-DWIN_EXPORT)

add_library(cube_linenoise STATIC ${SRC})
         
install(TARGETS cube_linenoise
    EXPORT cube_linenoise_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})