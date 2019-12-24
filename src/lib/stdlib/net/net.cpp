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
    EXPORTED cube_native_value tcp_udp_socket_create(cube_native_value port, 
        cube_native_value address, 
        cube_native_value udp, 
        cube_native_value broadcast,
        cube_native_value reusesock, 
        cube_native_value isServer, 
        cube_native_value timeout)
    {
        TcpUdpSocket *socket = new TcpUdpSocket(port._number, 
                address._string, 
                udp._bool, 
                broadcast._bool, 
                reusesock._bool, 
                isServer._bool,
                timeout._number);

        int addr = sockets.size();
        sockets[addr] = socket;
        buffers[addr] = new char[BUFFER_SIZE];

        cube_native_value ret;
        ret._number = addr;
        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_close(cube_native_value pointer)
    {
        int addr = pointer._number;
        cube_native_value ret;
        ret._bool = false;

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

                ret._bool = true;
            }
        }

        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_send(cube_native_value pointer, cube_native_value data)
    {
        cube_native_value ret;
        ret._number = 0;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;

        ret._number = socket->send((char*)data._bytes.bytes, data._bytes.length);
        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_send_to(cube_native_value pointer, cube_native_value address, cube_native_value data)
    {
        cube_native_value ret;
        ret._number = 0;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;

        ret._number = socket->sendTo((char*)data._bytes.bytes, data._bytes.length, address._string);
        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_receive(cube_native_value pointer)
    {
        cube_native_value ret;
        ret._number = 0;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;

        char* buffer = getBuffer(pointer._number);
        
        int rec = socket->receive(buffer, BUFFER_SIZE);
        ret._bytes.bytes = (uint8_t*) malloc(sizeof(uint8_t) * rec);
        memcpy(ret._bytes.bytes, buffer, rec);
        ret._bytes.length = rec;


        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_wait(cube_native_value pointer)
    {
        cube_native_value ret;
        ret._number = 0;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;

        ret._bool = socket->wait();
        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_disconnect(cube_native_value pointer)
    {
        cube_native_value ret;
        ret._bool = false;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;
            
        socket->disconnect();

        return ret;
    }

    EXPORTED cube_native_value tcp_udp_socket_client(cube_native_value pointer)
    {
        cube_native_value ret;
        ret._number = 0;

        TcpUdpSocket *socket = getSocket(pointer._number);
        if(socket == NULL)
            return ret;

        const char *address = socket->getAddress(NULL);
        if(address != NULL)
        {
            char *str = (char*)malloc(sizeof(char) * 30);
            strcpy(str, address);

            ret._string = str;
        }
        else
        {
            ret._string = (char*)malloc(sizeof(char) * 1);
            ret._string[0] = '\0';
        }
        
        return ret;
    }
}
