#include "asio_cube.hpp"
#include <cube/cubeext.h>
#include <iostream>

asio::io_context context;

extern "C"
{
    void run()
    {
        context.run();
    }

    void runOne()
    {
        context.run_one();
    }

    void poll_()
    {
        context.poll();
    }

    void pollOne()
    {
        context.poll_one();
    }

    void stop()
    {
        context.stop();
    }

    void restart()
    {
        context.restart();
    }
}