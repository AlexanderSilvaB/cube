include_directories(..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(SRC calc.c)

add_library(calc SHARED ${SRC})
set_target_properties(calc PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../libs/)
set_target_properties(calc PROPERTIES
         RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../libs/)
         
install(TARGETS calc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/libs)