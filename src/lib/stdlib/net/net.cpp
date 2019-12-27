#include "TcpUdpSocket/TcpUdpSocket.h"
#include <cube/cubeext.h>
#include <map>

#define BUFFER_SIZE 10240

using namespace std;

static map<int, TcpUdpSocket*> sockets;
static map<int, char*> buffers;

TcpUdpSocket* getSocket(int addr)
{
    if(sockets.find(addr) != sockets.end())
    {
        return sockets[addr];
    }
    return NULL;
}

char* getBuffer(int addr)
{
    if(sockets.find(addr) != sockets.end())
    {
        return buffers[addr];
    }
    return NULL;
}

extern "C"
{
    EXPORTED cube_native_var* tcp_udp_socket_create(cube_native_var* port, 
        cube_native_var* address, 
        cube_native_var* udp, 
        cube_native_var* broadcast,
        cube_native_var* reusesock, 
        cube_native_var* isServer, 
        cube_native_var* timeout)
    {
        TcpUdpSocket *socket = new TcpUdpSocket(AS_NATIVE_NUMBER(port), 
                AS_NATIVE_STRING(address), 
                AS_NATIVE_BOOL(udp), 
                AS_NATIVE_BOOL(broadcast), 
                AS_NATIVE_BOOL(reusesock), 
                AS_NATIVE_BOOL(isServer),
                AS_NATIVE_NUMBER(timeout));

        int addr = sockets.size();
        sockets[addr] = socket;
        buffers[addr] = new char[BUFFER_SIZE];

        cube_native_var* ret = NATIVE_NUMBER(addr);
        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_close(cube_native_var* pointer)
    {
        int addr = AS_NATIVE_NUMBER(pointer);
        cube_native_var* ret = NATIVE_BOOL(false);

        if(sockets.find(addr) != sockets.end())
        {
            if(sockets[addr] != NULL)
            {
                TcpUdpSocket *socket = sockets[addr];
                char *buffer = buffers[addr];

                socket->disconnect();
                delete socket;
                sockets[addr] = NULL;

                delete[] buffer;
                buffers[addr] = NULL;

                AS_NATIVE_BOOL(ret) = true;
            }
        }

        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_send(cube_native_var* pointer, cube_native_var* data)
    {
        cube_native_var* ret = NATIVE_NUMBER(0);

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;

        AS_NATIVE_NUMBER(ret) = socket->send((char*)AS_NATIVE_BYTES(data).bytes, AS_NATIVE_BYTES(data).length);
        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_send_to(cube_native_var* pointer, cube_native_var* address, cube_native_var* data)
    {
        cube_native_var* ret = NATIVE_NUMBER(0);

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;

        AS_NATIVE_NUMBER(ret) = socket->sendTo((char*)AS_NATIVE_BYTES(data).bytes, AS_NATIVE_BYTES(data).length, AS_NATIVE_STRING(address));
        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_receive(cube_native_var* pointer)
    {
        cube_native_var* ret = NATIVE_BYTES();

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;

        char* buffer = getBuffer(AS_NATIVE_NUMBER(pointer));
        
        int rec = socket->receive(buffer, BUFFER_SIZE);
        AS_NATIVE_BYTES(ret).bytes = (uint8_t*) malloc(sizeof(uint8_t) * rec);
        memcpy(AS_NATIVE_BYTES(ret).bytes, buffer, rec);
        AS_NATIVE_BYTES(ret).length = rec;

        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_wait(cube_native_var* pointer)
    {
        cube_native_var* ret = NATIVE_BOOL(false);

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;

        AS_NATIVE_BOOL(ret) = socket->wait();
        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_disconnect(cube_native_var* pointer)
    {
        cube_native_var* ret = NATIVE_BOOL(false);

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;
            
        socket->disconnect();
        AS_NATIVE_BOOL(ret) = true;

        return ret;
    }

    EXPORTED cube_native_var* tcp_udp_socket_client(cube_native_var* pointer)
    {
        cube_native_var* ret = NATIVE_NUMBER(0);

        TcpUdpSocket *socket = getSocket(AS_NATIVE_NUMBER(pointer));
        if(socket == NULL)
            return ret;

        const char *address = socket->getAddress(NULL);
        if(address != NULL)
        {
            char *str = (char*)malloc(sizeof(char) * 30);
            strcpy(str, address);

            AS_NATIVE_STRING(ret) = str;
        }
        else
        {
            AS_NATIVE_STRING(ret) = (char*)malloc(sizeof(char) * 1);
            AS_NATIVE_STRING(ret)[0] = '\0';
        }
        
        return ret;
    }
}
