#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytes.h"
#include "memory.h"
#include "mempool.h"
#include "util.h"
#include "vm.h"

static bool splitBytes(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("split() takes 2 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Argument passed to split() must be bytes");
        return false;
    }

    DISABLE_GC;

    ObjBytes *delimiter = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    uint8_t *token;
    uint8_t *start = bytes->bytes;
    size_t len = bytes->length;

    ObjList *list = initList();

    do
    {
        token = cube_bytebyte(start, delimiter->bytes, len, delimiter->length);
        if (!token)
        {
            if (len > 0)
            {
                ObjBytes *b = AS_BYTES(BYTES_VAL(start, len));
                writeValueArray(&list->values, OBJ_VAL(b));
            }
            break;
        }

        ObjBytes *b = AS_BYTES(BYTES_VAL(start, token - start));
        writeValueArray(&list->values, OBJ_VAL(b));
        start += b->length + delimiter->length;
        len = bytes->length - (start - bytes->bytes);
    } while (token != NULL);

    push(OBJ_VAL(list));

    RESTORE_GC;

    return true;
}

static bool containsBytes(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("contains() takes 2 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Argument passed to contains() must be bytes");
        return false;
    }

    ObjBytes *delimiter = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    if (!cube_bytebyte(bytes->bytes, delimiter->bytes, bytes->length, delimiter->length))
    {
        push(FALSE_VAL);
        return true;
    }

    push(TRUE_VAL);
    return true;
}

bool bytesContains(Value bytesV, Value delimiterV, Value *result)
{
    ObjBytes *delimiter = AS_BYTES(delimiterV);
    ObjBytes *bytes = AS_BYTES(bytesV);

    if (!cube_bytebyte(bytes->bytes, delimiter->bytes, bytes->length, delimiter->length))
    {
        *result = FALSE_VAL;
        return true;
    }

    *result = TRUE_VAL;
    return true;
}

static bool findBytes(int argCount)
{
    if (argCount < 2 || argCount > 3)
    {
        runtimeError("find() takes either 2 or 3 arguments (%d given)", argCount);
        return false;
    }

    int index = 1;

    if (argCount == 3)
    {
        if (!IS_NUMBER(peek(0)))
        {
            runtimeError("Index passed to find() must be a number");
            return false;
        }

        index = AS_NUMBER(pop());
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Sub-bytes passed to find() must be bytes");
        return false;
    }

    ObjBytes *subb = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    int position = 0;
    int next = 0;

    for (int i = 0; i < index; ++i)
    {
        uint8_t *result = cube_bytebyte(bytes->bytes + next, subb->bytes, bytes->length, subb->length);
        if (!result)
        {
            position = -1;
            break;
        }

        position = (result - bytes->bytes);
        next = position + subb->length;
    }

    push(NUMBER_VAL(position));
    return true;
}

static bool replaceBytes(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("replace() takes 3 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Argument passed to replace() must be bytes");
        return false;
    }

    if (!IS_BYTES(peek(1)))
    {
        runtimeError("Argument passed to replace() must be bytes");
        return false;
    }

    ObjBytes *replace = AS_BYTES(pop());
    ObjBytes *to_replace = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    uint8_t *res = NULL;
    size_t rL = 0;

    uint8_t *token;
    uint8_t *start = bytes->bytes;
    size_t len = bytes->length;
    size_t bL = 0;

    do
    {
        token = cube_bytebyte(start, to_replace->bytes, len, to_replace->length);
        if (!token)
        {
            if (len > 0)
            {
                res = mp_realloc(res, rL + len);
                memcpy(res + rL, start, len);
                rL += len;
            }
            break;
        }

        bL = token - start;
        res = mp_realloc(res, rL + bL + replace->length);
        memcpy(res + rL, start, bL);
        rL += bL;
        memcpy(res + rL, replace->bytes, replace->length);
        rL += replace->length;

        start = token + to_replace->length;
        len = bytes->length - (start - bytes->bytes);
    } while (token != NULL);

    push(BYTES_VAL(res, rL));
    return true;
}

static bool subbytesBytes(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("sub() takes 3 argument (%d  given)", argCount);
        return false;
    }

    int length = AS_NUMBER(pop());
    int start = AS_NUMBER(pop());

    ObjBytes *bytes = AS_BYTES(pop());

    if (start >= bytes->length)
    {
        runtimeError("start index out of bounds", start);
        return false;
    }

    if (start + length > bytes->length)
    {
        length = bytes->length - start;
    }

    uint8_t *temp = mp_malloc(sizeof(uint8_t) * length);
    strncpy(temp, bytes->bytes + start, length);

    push(BYTES_VAL(temp, length));
    mp_free(temp);
    return true;
}

static bool fromBytes(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("from() takes 2 argument (%d  given)", argCount);
        return false;
    }

    int start = AS_NUMBER(pop());

    ObjBytes *bytes = AS_BYTES(pop());

    if (start > bytes->length)
    {
        runtimeError("start index out of bounds", start);
        return false;
    }

    int length = bytes->length - start;

    uint8_t *temp = mp_malloc(sizeof(uint8_t) * length);
    memcpy(temp, bytes->bytes + start, length);

    push(BYTES_VAL(temp, length));
    mp_free(temp);
    return true;
}

static bool startsWithBytes(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("startsWith() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Argument passed to startsWith() must be bytes");
        return false;
    }

    ObjBytes *start = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    uint8_t *result = cube_bytebyte(bytes->bytes, start->bytes, bytes->length, start->length);

    push(BOOL_VAL(bytes->bytes == result));
    return true;
}

static bool endsWithBytes(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("endsWith() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    if (!IS_BYTES(peek(0)))
    {
        runtimeError("Argument passed to endsWith() must be bytes");
        return false;
    }

    ObjBytes *suffix = AS_BYTES(pop());
    ObjBytes *bytes = AS_BYTES(pop());

    if (bytes->length < suffix->length)
    {
        push(FALSE_VAL);
        return true;
    }

    uint8_t *result = cube_bytebyte(bytes->bytes, suffix->bytes, bytes->length, suffix->length);

    push(BOOL_VAL(result == &bytes->bytes[bytes->length - suffix->length]));
    return true;
}

static bool numBytes(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("num() takes 0 arguments (%d  given)", argCount - 1);
        return false;
    }

    ObjBytes *bytes = AS_BYTES(pop());
    if (bytes->length == 0)
    {
        push(NUMBER_VAL(0));
        return true;
    }

    char b[sizeof(uint32_t)];
    size_t len = bytes->length > sizeof(uint32_t) ? sizeof(uint32_t) : bytes->length;
    memset(b, '\0', sizeof(uint32_t));
    if (bytes->length >= len)
        memcpy(b, bytes->bytes, len);
    else
        memcpy(b, bytes->bytes, bytes->length);
    uint32_t value = *((uint32_t *)b);
    push(NUMBER_VAL(value));

    size_t L = bytes->length - len;
    memcpy(bytes->bytes, bytes->bytes + len, L);
    bytes->bytes = GROW_ARRAY(bytes->bytes, uint8_t, bytes->length, L);
    bytes->length = L;

    return true;
}

static bool boolBytes(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("bool() takes 0 arguments (%d  given)", argCount - 1);
        return false;
    }

    ObjBytes *bytes = AS_BYTES(pop());

    if (bytes->length == 0)
    {
        push(BOOL_VAL(false));
        return true;
    }

    push(BOOL_VAL((bytes->bytes[0] > 0)));

    size_t L = bytes->length - 1;
    memcpy(bytes->bytes, bytes->bytes + 1, L);
    bytes->bytes = GROW_ARRAY(bytes->bytes, uint8_t, bytes->length, L);
    bytes->length = L;

    return true;
}

bool bytesMethods(char *method, int argCount)
{
    if (strcmp(method, "split") == 0)
        return splitBytes(argCount);
    else if (strcmp(method, "contains") == 0)
        return containsBytes(argCount);
    else if (strcmp(method, "find") == 0)
        return findBytes(argCount);
    else if (strcmp(method, "replace") == 0)
        return replaceBytes(argCount);
    else if (strcmp(method, "startsWith") == 0)
        return startsWithBytes(argCount);
    else if (strcmp(method, "endsWith") == 0)
        return endsWithBytes(argCount);
    else if (strcmp(method, "from") == 0)
        return fromBytes(argCount);
    else if (strcmp(method, "sub") == 0)
        return subbytesBytes(argCount);
    else if (strcmp(method, "num") == 0)
        return numBytes(argCount);
    else if (strcmp(method, "bool") == 0)
        return boolBytes(argCount);

    runtimeError("Bytes has no method %s()", method);
    return false;
}