#include <asio_cube.hpp>
#include <functional>
#include <iostream>
#include <map>

using namespace std;
using namespace asio;
using ip::tcp;

int acceptorId = 0;
map<int, tcp::acceptor *> acceptors;

int addAcceptor(tcp::acceptor *acceptor)
{
    int aid = acceptorId;
    acceptorId++;
    acceptors[aid] = acceptor;
    return aid;
}

tcp::acceptor *getAcceptor(int aid)
{
    if (acceptors.find(aid) == acceptors.end())
        return NULL;
    return acceptors[aid];
}

void handle_accept(const asio::error_code &error, int aid, int sid, cube_native_var *handler)
{
    if (!error)
    {
        if (handler != NULL)
        {
            cube_native_var *args = NATIVE_LIST();
            ADD_NATIVE_LIST(args, NATIVE_NUMBER(sid));
            CALL_NATIVE_FUNC(handler, args);
        }
    }
    if (handler != NULL)
    {
        tcp::acceptor *acceptor = getAcceptor(aid);
        if (!acceptor)
            return;

        TCPSocketItem *socket = new TCPSocketItem(context);
        sid = addTCPSocket(socket);

        acceptor->async_accept(socket->socket, std::bind(handle_accept, std::placeholders::_1, aid, sid, handler));
    }
}

extern "C"
{
    EXPORTED int acceptor_create(int port, bool reuse)
    {
        tcp::acceptor *acceptor = new tcp::acceptor(context);
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
        acceptor->open(endpoint.protocol());
        acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(reuse));
        acceptor->bind(endpoint);
        acceptor->listen();
        return addAcceptor(acceptor);
    }

    EXPORTED bool acceptor_accept(int aid, int sid)
    {
        tcp::acceptor *acceptor = getAcceptor(aid);
        if (!acceptor)
            return false;

        TCPSocketItem *socket = getTCPSocket(sid);
        if (!socket)
            return false;

        acceptor->accept(socket->socket);
        return true;
    }

    EXPORTED void acceptor_cancel(int aid)
    {
        tcp::acceptor *acceptor = getAcceptor(aid);
        if (!acceptor)
            return;

        acceptor->cancel();
    }

    EXPORTED int acceptor_accept_new(int aid)
    {
        tcp::acceptor *acceptor = getAcceptor(aid);
        if (!acceptor)
            return false;

        TCPSocketItem *socket = new TCPSocketItem(context);
        int sid = addTCPSocket(socket);

        acceptor->accept(socket->socket);
        return sid;
    }

    EXPORTED bool acceptor_accept_async(int aid, cube_native_var *handler)
    {
        tcp::acceptor *acceptor = getAcceptor(aid);
        if (!acceptor)
            return false;

        TCPSocketItem *socket = new TCPSocketItem(context);
        int sid = addTCPSocket(socket);

        acceptor->async_accept(socket->socket,
                               std::bind(handle_accept, std::placeholders::_1, aid, sid, SAVE_FUNC(handler)));
        return true;
    }
}