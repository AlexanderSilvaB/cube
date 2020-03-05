#include <cube/cube.h>

#include <stdio.h>

int main(int argc, const char *argv[])
{
    startCube(argc, argv);

    int rc = runCube(argc, argv);

    stopCube();
    return rc;
}