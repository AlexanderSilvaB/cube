#include <asio_cube.hpp>
#include <functional>
#include <map>

using namespace std;
using namespace asio;
using ip::udp;

static int socketId = 0;
static map<int, UDPSocketItem *> sockets;

int addUDPSocket(UDPSocketItem *socket)
{
    int sid = socketId;
    socketId++;
    sockets[sid] = socket;
    return sid;
}

UDPSocketItem *getUDPSocket(int sid)
{
    if (sockets.find(sid) == sockets.end())
        return NULL;
    return sockets[sid];
}

static void handle_read(const asio::error_code &error, std::size_t bytes_transferred, int sid, cube_native_var *handler)
{
    if (handler != NULL)
    {
        UDPSocketItem *socket = getUDPSocket(sid);
        if (!socket)
            return;

        cube_native_var *args = NATIVE_LIST();
        if (!error || error == asio::error::eof)
        {
            cube_native_var *data = NATIVE_BYTES_COPY(
                socket->buf.size(), (unsigned char *)asio::buffer_cast<const void *>(socket->buf.data()));
            ADD_NATIVE_LIST(args, data);
        }
        else
        {
            ADD_NATIVE_LIST(args, NATIVE_NULL());
        }
        socket->buf.consume(socket->buf.size());
        CALL_NATIVE_FUNC(handler, args);
    }
}

static void handle_write(const asio::error_code &error, std::size_t bytes_transferred, int sid,
                         cube_native_var *handler)
{
    if (handler != NULL)
    {
        UDPSocketItem *socket = getUDPSocket(sid);
        if (!socket)
            return;

        cube_native_var *args = NATIVE_LIST();
        if (!error || error == asio::error::eof)
        {
            ADD_NATIVE_LIST(args, NATIVE_NUMBER(bytes_transferred));
        }
        else
        {
            ADD_NATIVE_LIST(args, NATIVE_NUMBER(-1));
        }
        CALL_NATIVE_FUNC(handler, args);
    }
}

extern "C"
{
    EXPORTED int udp_socket_create()
    {
        UDPSocketItem *socket = new UDPSocketItem(context);
        return addUDPSocket(socket);
    }

    EXPORTED bool udp_socket_open(int sid, int port, bool reuse)
    {
        UDPSocketItem *socket = getUDPSocket(sid);
        if (!socket)
            return false;

        asio::error_code ec;
        udp::endpoint rx_endpoint_(udp::v4(), port);
        socket->socket.open(rx_endpoint_.protocol(), ec);
        if (ec)
            return false;

        socket->socket.set_option(udp::socket::reuse_address(true));
        socket->socket.bind(rx_endpoint_, ec);
        if (ec)
            return false;

        return true;
    }
}