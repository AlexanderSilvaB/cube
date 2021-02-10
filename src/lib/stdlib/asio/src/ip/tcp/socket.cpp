#include <asio_cube.hpp>
#include <functional>
#include <map>

using namespace std;
using namespace asio;
using ip::tcp;

static int socketId = 0;
static map<int, TCPSocketItem *> sockets;

int addTCPSocket(TCPSocketItem *socket)
{
    int sid = socketId;
    socketId++;
    sockets[sid] = socket;
    return sid;
}

TCPSocketItem *getTCPSocket(int sid)
{
    if (sockets.find(sid) == sockets.end())
        return NULL;
    return sockets[sid];
}

static void handle_read(const asio::error_code &error, std::size_t bytes_transferred, int sid, cube_native_var *handler)
{
    if (handler != NULL)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
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
        TCPSocketItem *socket = getTCPSocket(sid);
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
    EXPORTED int tcp_socket_create()
    {
        TCPSocketItem *socket = new TCPSocketItem(context);
        return addTCPSocket(socket);
    }

    EXPORTED bool tcp_socket_connect(int sid, char *ip, int port)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::error_code ec;
        socket->socket.connect(tcp::endpoint(asio::ip::address::from_string(ip), port), ec);
        if (ec)
            return false;
        return true;
    }

    EXPORTED bool tcp_socket_close(int sid)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        socket->socket.close();
        return true;
    }

    EXPORTED bool tcp_socket_connect_resolve(int sid, char *hostname, char *service)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::error_code ec;
        tcp::resolver resolver(context);
        tcp::resolver::query query(hostname, service);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        asio::connect(socket->socket, endpoint_iterator, ec);
        if (ec)
            return false;
        return true;
    }

    EXPORTED cube_native_var *tcp_socket_read_exactly(int sid, int n)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return NULL;

        asio::read(socket->socket, socket->buf, asio::transfer_exactly(n));

        cube_native_var *data =
            NATIVE_BYTES_COPY(socket->buf.size(), (unsigned char *)asio::buffer_cast<const void *>(socket->buf.data()));

        socket->buf.consume(socket->buf.size());
        return data;
    }

    EXPORTED bool tcp_socket_read_exactly_async(int sid, int n, cube_native_var *handler)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::async_read(socket->socket, socket->buf, asio::transfer_exactly(n),
                         std::bind(handle_read, std::placeholders::_1, std::placeholders::_2, sid, SAVE_FUNC(handler)));
        return true;
    }

    EXPORTED cube_native_var *tcp_socket_read_some(int sid, int n)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return NULL;

        asio::streambuf::mutable_buffers_type mutableBuffer = socket->buf.prepare(n);
        socket->socket.read_some(asio::buffer(mutableBuffer));

        cube_native_var *data =
            NATIVE_BYTES_COPY(socket->buf.size(), (unsigned char *)asio::buffer_cast<const void *>(socket->buf.data()));

        socket->buf.consume(socket->buf.size());
        return data;
    }

    EXPORTED bool tcp_socket_read_some_async(int sid, int n, cube_native_var *handler)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::streambuf::mutable_buffers_type mutableBuffer = socket->buf.prepare(n);
        socket->socket.async_read_some(
            asio::buffer(mutableBuffer),
            std::bind(handle_read, std::placeholders::_1, std::placeholders::_2, sid, SAVE_FUNC(handler)));
        return true;
    }

    EXPORTED cube_native_var *tcp_socket_read_until(int sid, char *end)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return NULL;

        asio::read_until(socket->socket, socket->buf, end);

        cube_native_var *data =
            NATIVE_BYTES_COPY(socket->buf.size(), (unsigned char *)asio::buffer_cast<const void *>(socket->buf.data()));

        socket->buf.consume(socket->buf.size());
        return data;
    }

    EXPORTED bool tcp_socket_read_until_async(int sid, char *end, cube_native_var *handler)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::async_read_until(
            socket->socket, socket->buf, end,
            std::bind(handle_read, std::placeholders::_1, std::placeholders::_2, sid, SAVE_FUNC(handler)));
        return true;
    }

    EXPORTED cube_native_var *tcp_socket_read_all(int sid)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return NULL;

        asio::error_code ec;
        asio::read(socket->socket, socket->buf, asio::transfer_all(), ec);

        if (ec && ec != asio::error::eof)
        {
            socket->buf.consume(socket->buf.size());
            return NATIVE_NULL();
        }

        cube_native_var *data =
            NATIVE_BYTES_COPY(socket->buf.size(), (unsigned char *)asio::buffer_cast<const void *>(socket->buf.data()));

        socket->buf.consume(socket->buf.size());
        return data;
    }

    EXPORTED bool tcp_socket_read_all_async(int sid, cube_native_var *handler)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        asio::async_read(socket->socket, socket->buf, asio::transfer_all(),
                         std::bind(handle_read, std::placeholders::_1, std::placeholders::_2, sid, SAVE_FUNC(handler)));
        return true;
    }

    EXPORTED int tcp_socket_write(int sid, cube_native_var *data)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return -1;

        cube_native_bytes bytes = AS_NATIVE_BYTES(data);
        return asio::write(socket->socket, asio::buffer(bytes.bytes, bytes.length));
    }

    EXPORTED bool tcp_socket_write_async(int sid, cube_native_var *data, cube_native_var *handler)
    {
        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        cube_native_bytes bytes = AS_NATIVE_BYTES(data);
        asio::async_write(
            socket->socket, asio::buffer(bytes.bytes, bytes.length),
            std::bind(handle_write, std::placeholders::_1, std::placeholders::_2, sid, SAVE_FUNC(handler)));
        return true;
    }
}