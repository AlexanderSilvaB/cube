#ifndef _CUBE_ASIO_HPP_
#define _CUBE_ASIO_HPP_

#include <asio.hpp>
#include <cube/cubeext.h>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

extern asio::io_context context;

typedef struct TCPSocketItem_st
{
    asio::ip::tcp::socket socket;
    asio::streambuf buf;
    TCPSocketItem_st(asio::io_context &context) : socket(context)
    {
    }

} TCPSocketItem;

typedef struct UDPSocketItem_st
{
    asio::ip::udp::socket socket;
    asio::streambuf buf;
    UDPSocketItem_st(asio::io_context &context) : socket(context)
    {
    }

} UDPSocketItem;

int addTCPSocket(TCPSocketItem *socket);
TCPSocketItem *getTCPSocket(int sid);

int addUDPSocket(UDPSocketItem *socket);
UDPSocketItem *getUDPSocket(int sid);

int addAcceptor(asio::ip::tcp::acceptor *acceptor);
asio::ip::tcp::acceptor *getAcceptor(int aid);

#endif
