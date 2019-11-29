#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "std.h"
#include "object.h"
#include "vm.h"

Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value timeNative(int argCount, Value *args)
{
    time_t seconds;
    time(&seconds);

    return NUMBER_VAL((double)seconds);
}

Value exitNative(int argCount, Value *args)
{
    int code = 0;
    if (argCount > 0)
    {
        code = (int)AS_NUMBER(args[0]);
    }
    exit(code);
    return NONE_VAL;
}

Value inputNative(int argCount, Value *args)
{
    printNative(argCount, args);

    char str[MAX_STRING_INPUT];
    fgets(str, MAX_STRING_INPUT, stdin);
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL)
        *pos = '\0';

    vm.newLine = false;
    return OBJ_VAL(copyString(str, strlen(str)));
}

Value printNative(int argCount, Value *args)
{
    for (int i = 0; i < argCount; i++)
    {
        Value value = args[i];
        if (IS_STRING(value))
            printf("%s", AS_CSTRING(value));
        else
            printValue(value);
    }
    vm.newLine = true;
    return NONE_VAL;
}

Value printlnNative(int argCount, Value *args)
{
    for (int i = 0; i < argCount; i++)
    {
        Value value = args[i];
        if (IS_STRING(value))
            printf("%s", AS_CSTRING(value));
        else
            printValue(value);
    }
    printf("\n");
    vm.newLine = false;
    return NONE_VAL;
}

Value randomNative(int argCount, Value *args)
{
    srand(time(NULL));
    int dv = 100000001;
    double r = rand() % dv;
    return NUMBER_VAL(r / dv);
}

Value boolNative(int argCount, Value *args)
{
    if (argCount == 0)
        return BOOL_VAL(false);
    return toBool(args[0]);
}

Value numNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    return toNumber(args[0]);
}

Value strNative(int argCount, Value *args)
{
    if (argCount == 0)
        return STRING_VAL("");
    return toString(args[0]);
}

Value ceilNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(ceil(AS_NUMBER(num)));
}

Value floorNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(floor(AS_NUMBER(num)));
}

Value roundNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    return NUMBER_VAL(round(AS_NUMBER(num)));
}

Value powNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    Value num = toNumber(args[0]);
    Value power = NUMBER_VAL(1);
    if (argCount > 1)
        power = toNumber(args[1]);

    return NUMBER_VAL(pow(AS_NUMBER(num), AS_NUMBER(power)));
}

Value expNative(int argCount, Value *args)
{
    Value power = NUMBER_VAL(1);
    if (argCount > 0)
        power = toNumber(args[0]);

    return NUMBER_VAL(exp(AS_NUMBER(power)));
}

Value lenNative(int argCount, Value *args)
{
    if (argCount != 1)
    {
        //runtimeError("len() takes exactly 1 argument (%d given).", argCount);
        printf("len() takes exactly 1 argument (%d given).\n", argCount);
        return NONE_VAL;
    }

    if (IS_STRING(args[0]))
        return NUMBER_VAL(AS_STRING(args[0])->length);
    else if (IS_LIST(args[0]))
        return NUMBER_VAL(AS_LIST(args[0])->values.count);
    else if (IS_DICT(args[0]))
        return NUMBER_VAL(AS_DICT(args[0])->count);

    //runtimeError("Unsupported type passed to len()", argCount);
    printf("Unsupported type passed to len()\n");
    return NONE_VAL;
}

Value typeNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;
    char *str = valueType(args[0]);
    Value res = STRING_VAL(str);
    free(str);
    return res;
}

Value envNative(int argCount, Value *args)
{
    bool showNative = false;
    if (argCount > 0)
    {
        showNative = !isFalsey(args[0]);
    }

    int i = 0;
    Entry entry;
    while (iterateTable(&vm.globals, &entry, &i))
    {
        if (entry.key == NULL)
            continue;
        if (!showNative && IS_NATIVE(entry.value))
            continue;

        printf("%s : ", entry.key->chars);
        printValue(entry.value);
        printf("\n");
    }

    vm.newLine = false;
    return NONE_VAL;
}

Value openNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        //runtimeError("open requires at least a file name");
        printf("'open' requires at least a file name\n");
        return NONE_VAL;
    }

    Value fileName = args[0];
    Value openType;
    if (argCount > 1)
        openType = args[1];
    else
        openType = STRING_VAL("r");

    if (!IS_STRING(openType))
    {
        //runtimeError("File open type must be a string");
        printf("File open type must be a string\n");
        return NONE_VAL;
    }

    if (!IS_STRING(fileName))
    {
        //runtimeError("Filename must be a string");
        printf("Filename must be a string\n");
        return NONE_VAL;
    }

    ObjString *openTypeString = AS_STRING(openType);
    ObjString *fileNameString = AS_STRING(fileName);

    ObjFile *file = initFile();
    file->file = fopen(fileNameString->chars, openTypeString->chars);
    file->path = fileNameString->chars;
    file->openType = openTypeString->chars;

    if (file->file == NULL)
    {
        //runtimeError("Unable to open file");
        printf("Unable to open file\n");
        return NONE_VAL;
    }

    file->isOpen = true;

    return OBJ_VAL(file);
}