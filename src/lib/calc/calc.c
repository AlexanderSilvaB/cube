#include <cube/cubeext.h>
#include <stdio.h>

EXPORTED cube_native_var *add(cube_native_var *a, cube_native_var *b)
{
    double v = AS_NATIVE_NUMBER(a) + AS_NATIVE_NUMBER(b);
    cube_native_var *ret = NATIVE_NUMBER(v);
    return ret;
}

EXPORTED cube_native_var *sub(cube_native_var *a, cube_native_var *b)
{
    double v = AS_NATIVE_NUMBER(a) - AS_NATIVE_NUMBER(b);
    cube_native_var *ret = NATIVE_NUMBER(v);
    return ret;
}

EXPORTED cube_native_var *mul(cube_native_var *a, cube_native_var *b)
{
    double v = AS_NATIVE_NUMBER(a) * AS_NATIVE_NUMBER(b);
    cube_native_var *ret = NATIVE_NUMBER(v);
    return ret;
}

EXPORTED cube_native_var *divi(cube_native_var *a, cube_native_var *b)
{
    double v = AS_NATIVE_NUMBER(a) / AS_NATIVE_NUMBER(b);
    cube_native_var *ret = NATIVE_NUMBER(v);
    return ret;
}
