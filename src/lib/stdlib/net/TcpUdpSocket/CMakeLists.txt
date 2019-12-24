cmake_minimum_required(VERSION 2.8)
project(TcpUdpSocket)

include_directories(.)

set(LIB_SRC
    TcpUdpSocket.h
    TcpUdpSocket.cpp)

if(WIN32)
    set(ADDITIONAL_LIBRARIES wsock32)
else()
    set(ADDITIONAL_LIBRARIES "")
endif()

add_library(TcpUdpSocket STATIC ${LIB_SRC} )
target_link_libraries(TcpUdpSocket ${ADDITIONAL_LIBRARIES})
