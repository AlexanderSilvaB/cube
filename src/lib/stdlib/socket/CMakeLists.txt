include_directories(../..)

add_subdirectory(TcpUdpSocket)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(SRC socket.cpp)

add_library(socket SHARED ${SRC})
target_link_libraries(socket TcpUdpSocket)
set_target_properties(socket PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
set_target_properties(socket PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)