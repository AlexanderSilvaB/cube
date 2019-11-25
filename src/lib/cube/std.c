#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "std.h"
#include "object.h"

Value clockNative(int argCount, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value timeNative(int argCount, Value* args)
{
    time_t seconds;
    time(&seconds);

    return NUMBER_VAL((double)seconds);
}

Value exitNative(int argCount, Value* args)
{
    int code = 0;
    if(argCount > 0)
    {
        code = (int)AS_NUMBER(args[0]);
    }
    exit(code);
    return NONE_VAL;
}

Value inputNative(int argCount, Value* args)
{
    printNative(argCount, args);

    char str[MAX_STRING_INPUT];
    fgets(str, MAX_STRING_INPUT, stdin);

    return OBJ_VAL(copyString(str, strlen(str)));
}

Value printNative(int argCount, Value* args)
{
    for(int i = 0; i < argCount; i++)
    {
        printValue(args[i]);
    }
    return NONE_VAL;
}

Value printlnNative(int argCount, Value* args)
{
    for(int i = 0; i < argCount; i++)
    {
        printValue(args[i]);
    }
    printf("\n");
    return NONE_VAL;
}

Value randomNative(int argCount, Value* args)
{
    int dv = 100000001;
    double r = rand() % dv;
    return NUMBER_VAL(r / dv);
}