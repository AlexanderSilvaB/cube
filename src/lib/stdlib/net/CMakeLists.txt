include_directories(../..)

add_subdirectory(TcpUdpSocket)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(SRC net.cpp)

add_library(net SHARED ${SRC})
target_link_libraries(net TcpUdpSocket)
set_target_properties(net PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../stdlib/libs/)