#include <stdio.h>
#include <cube/cubeext.h>

EXPORTED cube_native_value add(cube_native_value a, cube_native_value b)
{
    cube_native_value ret;
    ret._number = a._number + b._number;
    return ret;
}

EXPORTED cube_native_value sub(cube_native_value a, cube_native_value b)
{
    cube_native_value ret;
    ret._number = a._number - b._number;
    return ret;
}

EXPORTED cube_native_value mul(cube_native_value a, cube_native_value b)
{
    cube_native_value ret;
    ret._number = a._number * b._number;
    return ret;
}

EXPORTED cube_native_value div(cube_native_value a, cube_native_value b)
{
    cube_native_value ret;
    ret._number = a._number / b._number;
    return ret;
}
