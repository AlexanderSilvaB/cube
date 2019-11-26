#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "std.h"
#include "object.h"
#include "vm.h"

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
        Value value = args[i];
        if (IS_STRING(value))
            printf("%s", AS_CSTRING(value));
        else
            printValue(args[i]);
    }
    return NONE_VAL;
}

Value printlnNative(int argCount, Value* args)
{
    for(int i = 0; i < argCount; i++)
    {
        Value value = args[i];
        if (IS_STRING(value))
            printf("%s", AS_CSTRING(value));
        else
            printValue(args[i]);
    }
    printf("\n");
    return NONE_VAL;
}

Value randomNative(int argCount, Value* args)
{
    srand(time(NULL));
    int dv = 100000001;
    double r = rand() % dv;
    return NUMBER_VAL(r / dv);
}

Value boolNative(int argCount, Value* args)
{
    if(argCount == 0)
        return BOOL_VAL(false);
    return toBool(args[0]);
}

Value numNative(int argCount, Value* args)
{
    if(argCount == 0)
        return NUMBER_VAL(0);
    return toNumber(args[0]);
}

Value strNative(int argCount, Value* args)
{
    if(argCount == 0)
        return STRING_VAL("");
    return toString(args[0]);
}

Value ceilNative(int argCount, Value* args)
{
    if(argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(ceil(AS_NUMBER(num)));
}

Value floorNative(int argCount, Value* args)
{
    if(argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(floor(AS_NUMBER(num)));
}

Value roundNative(int argCount, Value* args)
{
    if(argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(round(AS_NUMBER(num)));
}

Value powNative(int argCount, Value* args)
{
    if(argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    Value power = NUMBER_VAL(1);
    if(argCount > 1)
        power = toNumber(args[1]);

    return NUMBER_VAL(pow(AS_NUMBER(num), AS_NUMBER(power)));
}

Value expNative(int argCount, Value* args)
{
    Value power = NUMBER_VAL(1);
    if(argCount > 0)
        power = toNumber(args[0]);

    return NUMBER_VAL(exp(AS_NUMBER(power)));
}

Value lenNative(int argCount, Value* args)
{
	if (argCount != 1)
    {
        runtimeError("len() takes exactly 1 argument (%d given).", argCount);
        return NONE_VAL;
    }

    if (IS_STRING(args[0]))
        return NUMBER_VAL(AS_STRING(args[0])->length);
    else if (IS_LIST(args[0]))
        return NUMBER_VAL(AS_LIST(args[0])->values.count);
    else if (IS_DICT(args[0]))
        return NUMBER_VAL(AS_DICT(args[0])->count);

    runtimeError("Unsupported type passed to len()", argCount);
    return NONE_VAL;
}