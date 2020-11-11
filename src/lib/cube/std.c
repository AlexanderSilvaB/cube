#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/wait.h>
#endif

#include <errno.h>

#include "ansi_escapes.h"
#include "collections.h"
#include "compiler.h"
#include "external/cJSON/cJSON.h"
#include "files.h"
#include "gc.h"
#include "memory.h"
#include "mempool.h"
#include "object.h"
#include "std.h"
#include "strings.h"
#include "system.h"
#include "util.h"
#include "version.h"
#include "vm.h"

typedef union {
    bool b;
    char c;
    unsigned char uc;
    short s;
    unsigned short us;
    int i;
    unsigned int ui;
    float f;
    double d;
    long l;
    unsigned long ul;
    long long ll;
    unsigned long long ull;
    void *ptr;
} bin_val;

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif

Value hashNative(int argCount, Value *args)
{
    int code = 0;
    if (argCount > 0)
        code = (int)&args[0];
    else
        code = rand();

    char str[128];
    sprintf(str, "%d", code);

    uint32_t hash = 5381;
    int c;

    int i = 0;
    c = str[i++];
    while (c != '\0')
    {
        hash = ((hash << 5) + hash) + c;
        c = str[i++];
    }

    return NUMBER_VAL(hash);
}

Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL((cube_clock() * 1e-9));
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
    vm.exitCode = code;
    vm.running = false;
    vm.repl = NUMBER_VAL(code);
    // exit(code);
    return NULL_VAL;
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
    fflush(stdout);
    vm.print = true;
    vm.newLine = true;
    vm.repl = NULL_VAL;
    return NULL_VAL;
}

Value printlnNative(int argCount, Value *args)
{
    printNative(argCount, args);
    printf("\n");
    vm.newLine = false;
    return NULL_VAL;
}

Value throwNative(int argCount, Value *args)
{
    int sz = 128;
    int len = 0;
    char *error = ALLOCATE(char, sz);
    error[0] = '\0';
    for (int i = 0; i < argCount; i++)
    {
        char *str = valueToString(args[i], false);
        if (len + strlen(error) + 1 >= sz)
        {
            error = GROW_ARRAY(error, char, sz, sz * 2);
            sz *= 2;
        }
        strcat(error + len, str);
        mp_free(str);
        len = strlen(error);
    }
    runtimeError(error);
    FREE_ARRAY(char, error, sz);
    return NULL_VAL;
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

Value randNative(int argCount, Value *args)
{
    double max = 1.0;
    double min = 0.0;

    if (argCount > 0)
    {
        if (!IS_NUMBER(args[0]))
        {
            runtimeError("rand only accepts numbers as parameters.");
            return NULL_VAL;
        }

        max = AS_NUMBER(args[0]);
    }

    if (argCount > 1)
    {
        if (!IS_NUMBER(args[1]))
        {
            runtimeError("rand only accepts numbers as parameters.");
            return NULL_VAL;
        }

        min = max;
        max = AS_NUMBER(args[1]);
    }

    if (min > max)
    {
        double tmp = max;
        max = min;
        min = tmp;
    }

    int dv = 100000001;
    double r = min + ((rand() % dv) / (double)dv) * (max - min);
    return NUMBER_VAL(r);
}

double rand_gen()
{
    // return a uniformly distributed random value
    return ((double)(rand()) + 1.) / ((double)(RAND_MAX) + 1.);
}

double normalRandom()
{
    // return a normally distributed random value
    double v1 = rand_gen();
    double v2 = rand_gen();
    return cos(2 * 3.14 * v2) * sqrt(-2. * log(v1));
}

Value randnNative(int argCount, Value *args)
{
    double sigma = 1.0;
    double mu = 0.0;

    if (argCount > 0)
    {
        if (!IS_NUMBER(args[0]))
        {
            runtimeError("randn only accepts numbers as parameters.");
            return NULL_VAL;
        }

        sigma = AS_NUMBER(args[0]);
    }

    if (argCount > 1)
    {
        if (!IS_NUMBER(args[1]))
        {
            runtimeError("randn only accepts numbers as parameters.");
            return NULL_VAL;
        }

        mu = AS_NUMBER(args[1]);
    }

    return NUMBER_VAL((normalRandom() * sigma + mu));
}

Value seedNative(int argCount, Value *args)
{
    Value seed;
    if (argCount == 0)
        seed = NUMBER_VAL(cube_clock());
    else
        seed = toNumber(args[0]);

    srand(AS_NUMBER(seed));
    return seed;
}

Value waitNative(int argCount, Value *args)
{
    Value wait;
    if (argCount == 0)
        wait = NUMBER_VAL(1);
    else
    {
        if (IS_PROCESS(args[0]))
        {
            int status = -1;
#ifdef _WIN32
#else
            waitpid(AS_PROCESS(args[0])->pid, &status, 0);
            status = WEXITSTATUS(status);
#endif
            AS_PROCESS(args[0])->status = status;
            AS_PROCESS(args[0])->running = false;
            return NUMBER_VAL(status);
        }
        else
            wait = toNumber(args[0]);
    }

    uint64_t current = cube_clock();
    ThreadFrame *threadFrame = currentThread();
    threadFrame->ctf->startTime = current;
    threadFrame->ctf->endTime = current + (AS_NUMBER(wait) * 1e6);
    return wait;
}

Value sinNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(sin(AS_NUMBER(val)));
}

Value cosNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(cos(AS_NUMBER(val)));
}

Value tanNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(tan(AS_NUMBER(val)));
}

Value asinNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(asin(AS_NUMBER(val)));
}

Value acosNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(acos(AS_NUMBER(val)));
}

Value atanNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(atan(AS_NUMBER(val)));
}

Value atan2Native(int argCount, Value *args)
{
    Value y;
    if (argCount == 0)
        y = NUMBER_VAL(0);
    else
        y = toNumber(args[0]);

    Value x;
    if (argCount <= 1)
        x = NUMBER_VAL(0);
    else
        x = toNumber(args[1]);
    return NUMBER_VAL(atan2(AS_NUMBER(y), AS_NUMBER(x)));
}

Value sqrtNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(sqrt(AS_NUMBER(val)));
}

Value lnNative(int argCount, Value *args)
{
    Value val;
    if (argCount == 0)
        val = NUMBER_VAL(0);
    else
        val = toNumber(args[0]);
    return NUMBER_VAL(log(AS_NUMBER(val)));
}

Value logNative(int argCount, Value *args)
{
    Value x;
    if (argCount == 0)
        x = NUMBER_VAL(0);
    else
        x = toNumber(args[0]);

    Value b;
    if (argCount <= 1)
        b = NUMBER_VAL(10);
    else
        b = toNumber(args[1]);
    return NUMBER_VAL(log(AS_NUMBER(x)) / log(AS_NUMBER(b)));
}

Value isnanNative(int argCount, Value *args)
{
    if (argCount == 0)
        return FALSE_VAL;

    for (int i = 0; i < argCount; i++)
    {
        Value value = toNumber(args[i]);
        if (!isnan(AS_NUMBER(value)))
            return FALSE_VAL;
    }
    return TRUE_VAL;
}

Value isinfNative(int argCount, Value *args)
{
    if (argCount == 0)
        return FALSE_VAL;

    for (int i = 0; i < argCount; i++)
    {
        Value value = toNumber(args[i]);
        if (!isinf(AS_NUMBER(value)))
            return FALSE_VAL;
    }
    return TRUE_VAL;
}

Value isemptyNative(int argCount, Value *args)
{
    if (argCount == 0)
        return BOOL_VAL(false);
    Value arg = args[0];
    if (IS_NULL(arg))
    {
        return TRUE_VAL;
    }
    else if (IS_STRING(arg))
    {
        return BOOL_VAL(AS_STRING(arg)->length == 0);
    }
    else if (IS_BYTES(arg))
    {
        return BOOL_VAL(AS_BYTES(arg)->length == 0);
    }
    else if (IS_LIST(arg))
    {
        return BOOL_VAL(AS_LIST(arg)->values.count == 0);
    }
    else if (IS_DICT(arg))
    {
        return BOOL_VAL(AS_DICT(arg)->count == 0);
    }

    return BOOL_VAL(AS_STRING(toString(arg))->length == 0);
}

Value boolNative(int argCount, Value *args)
{
    if (argCount == 0)
        return BOOL_VAL(false);
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("bool")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("bool")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("bool")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    return toBool(args[0]);
}

Value numNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("num")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("num")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("num")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    return toNumber(args[0]);
}

Value intNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("int")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("int")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("int")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    return NUMBER_VAL((int)AS_NUMBER(toNumber(args[0])));
}

Value strNative(int argCount, Value *args)
{
    if (argCount == 0)
        return STRING_VAL("");
    if (IS_BYTES(args[0]))
    {
        ObjBytes *bytes = AS_BYTES(args[0]);
        int len = bytes->length;
        if (len < 0)
            len = 10;
        char *str = NULL;
        if (len != bytes->length)
            str = mp_malloc(len + 5);
        else
            str = mp_malloc(len + 1);
        int j = 0;
        for (int i = 0; i < len; i++)
        {
            if (isprint(bytes->bytes[i]) != 0 || iscntrl(bytes->bytes[i]) != 0)
                str[j++] = bytes->bytes[i];
        }
        if (len != bytes->length)
        {
            str[j++] = ' ';
            str[j++] = '.';
            str[j++] = '.';
            str[j++] = '.';
        }
        str[j] = '\0';
        Value ret = STRING_VAL(str);
        mp_free(str);
        return ret;
    }
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("str")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("str")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("str")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    return toString(args[0]);
}

Value hexNative(int argCount, Value *args)
{
    if (argCount == 0)
        return STRING_VAL("FF");
    if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("hex")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("hex")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("hex")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }

    int hex = (int)AS_NUMBER(toNumber(args[0]));
    char hex_string[20];
    sprintf(hex_string, "%X", hex);

    return STRING_VAL(hex_string);
}

static void appendBits(char *str, bin_val val, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if ((val.l >> i) & 0x1)
            strcat(str, "1");
        else
            strcat(str, "0");
    }
}

static void fromBits(char *str, bin_val *val, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (str[i] == '1')
            val->l |= 0x1 << (len - i - 1);
    }
}

Value binNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    char fmt = 'b';
    if (argCount > 1 && IS_STRING(args[1]))
        fmt = AS_CSTRING(args[1])[0];

    if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("bin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("bin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("bin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }

    char *str = NULL;
    bin_val *bval = NULL;
    int len = 0;
    int num = 1;
    switch (fmt)
    {
        case 'x':
            len = 0;
            num = 0;
            break;
        case 'c': {
            len = 8;
            num = 1;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].c = AS_CSTRING(toString(args[0]))[0];
            break;
        }
        case 'b': {
            num = 1;
            len = 8;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].c = (char)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'B': {
            num = 1;
            len = 8;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].uc = (unsigned char)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case '?': {
            num = 1;
            len = 1;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].b = (bool)AS_BOOL(toBool(args[0]));
            break;
        }
        case 'h': {
            num = 1;
            len = 16;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].s = (short)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'H': {
            num = 1;
            len = 16;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].us = (unsigned short)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'i': {
            num = 1;
            len = 32;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].i = (int)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'I': {
            num = 1;
            len = 32;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].ui = (unsigned int)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'l': {
            num = 1;
            len = 64;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].l = (long)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'L': {
            num = 1;
            len = 64;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].ul = (unsigned long)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'q': {
            num = 1;
            len = 64;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].ll = (long long)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'Q': {
            num = 1;
            len = 64;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].ull = (unsigned long long)AS_NUMBER(toNumber(args[0]));
            break;
        }
        case 'f': {
            num = 1;
            len = 32;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].f = (float)AS_NUMBER(toNumber(args[0]));
            break;
        }

        case 'd': {
            num = 1;
            len = 64;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].d = (double)AS_NUMBER(toNumber(args[0]));
            break;
        }

        case 's':
        case 'p': {
            ObjString *s = AS_STRING(toString(args[0]));
            num = s->length;
            len = 8;
            bval = mp_malloc(sizeof(bin_val) * num);
            for (int i = 0; i < num; i++)
            {
                bval[i].c = s->chars[i];
            }
            break;
        }

        case 'P': {
            int p = AS_NUMBER(toNumber(args[0]));

            num = 1;
            len = 32;
            bval = mp_malloc(sizeof(bin_val) * num);
            bval[0].ptr = (void *)p;
            break;
        }
    }

    if (len > 0 && num > 0)
    {
        str = mp_calloc((num * len) + 1, sizeof(char));
        for (int i = 0; i < num; i++)
            appendBits(str, bval[i], len);
    }

    mp_free(bval);

    Value val = NULL_VAL;
    if (str != NULL)
    {
        val = STRING_VAL(str);
        mp_free(str);
    }
    return val;
}

Value unbinNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    char fmt = 'b';
    if (argCount > 1 && IS_STRING(args[1]))
        fmt = AS_CSTRING(args[1])[0];

    if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("unbin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("unbin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("unbin")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }

    if (!IS_STRING(args[0]))
        return NULL_VAL;

    char *str = AS_CSTRING(args[0]);
    bin_val *bval = NULL;
    int len = 0;
    int num = 1;
    switch (fmt)
    {
        case 'x':
            len = 0;
            num = 0;
            break;
        case 'c': {
            len = 8;
            num = 1;
            break;
        }
        case 'b': {
            num = 1;
            len = 8;
            break;
        }
        case 'B': {
            num = 1;
            len = 8;
            break;
        }
        case '?': {
            num = 1;
            len = 1;
            break;
        }
        case 'h': {
            num = 1;
            len = 16;
            break;
        }
        case 'H': {
            num = 1;
            len = 16;
            break;
        }
        case 'i': {
            num = 1;
            len = 32;
            break;
        }
        case 'I': {
            num = 1;
            len = 32;
            break;
        }
        case 'l': {
            num = 1;
            len = 64;
            break;
        }
        case 'L': {
            num = 1;
            len = 64;
            break;
        }
        case 'q': {
            num = 1;
            len = 64;
            break;
        }
        case 'Q': {
            num = 1;
            len = 64;
            break;
        }
        case 'f': {
            num = 1;
            len = 32;
            break;
        }

        case 'd': {
            num = 1;
            len = 64;
            break;
        }

        case 's':
        case 'p': {
            num = strlen(str) / 8;
            len = 8;
            break;
        }

        case 'P': {
            num = 1;
            len = 32;
            break;
        }
    }

    bval = mp_calloc(num, sizeof(bin_val));
    for (int i = 0; i < num; i++)
    {
        fromBits(str + (i * len), &bval[i], len);
    }

    Value val = NULL_VAL;
    switch (fmt)
    {
        case 'x':
            break;
        case 'c': {
            char c[2];
            c[0] = bval[0].c;
            c[1] = '\0';
            val = STRING_VAL(c);
            break;
        }
        case 'b': {
            val = NUMBER_VAL(bval[0].c);
            break;
        }
        case 'B': {
            val = NUMBER_VAL(bval[0].uc);
            break;
        }
        case '?': {
            val = BOOL_VAL(bval[0].b);
            break;
        }
        case 'h': {
            val = NUMBER_VAL(bval[0].s);
            break;
        }
        case 'H': {
            val = NUMBER_VAL(bval[0].us);
            break;
        }
        case 'i': {
            val = NUMBER_VAL(bval[0].i);
            break;
        }
        case 'I': {
            val = NUMBER_VAL(bval[0].ui);
            break;
        }
        case 'l': {
            val = NUMBER_VAL(bval[0].l);
            break;
        }
        case 'L': {
            val = NUMBER_VAL(bval[0].ul);
            break;
        }
        case 'q': {
            val = NUMBER_VAL(bval[0].ll);
            break;
        }
        case 'Q': {
            val = NUMBER_VAL(bval[0].ull);
            break;
        }
        case 'f': {
            val = NUMBER_VAL(bval[0].f);
            break;
        }

        case 'd': {
            val = NUMBER_VAL(bval[0].d);
            break;
        }

        case 's':
        case 'p': {
            char *s = mp_calloc(num, sizeof(char));
            for (int i = 0; i < num; i++)
            {
                s[i] = bval[i].c;
            }
            val = STRING_VAL(s);
            mp_free(s);
            break;
        }

        case 'P': {
            int p = bval[0].ptr;
            val = NUMBER_VAL(p);
            break;
        }
    }

    mp_free(bval);
    return val;
}

Value mbinNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);

    char fmt = 'b';
    if (argCount > 1 && IS_STRING(args[1]))
        fmt = AS_CSTRING(args[1])[0];

    if (!IS_STRING(args[0]))
        return NUMBER_VAL(0);

    char *str = AS_CSTRING(args[0]);
    int len = 0;
    int num = 1;
    switch (fmt)
    {
        case 'x':
            len = 0;
            num = 0;
            break;
        case 'c': {
            len = 8;
            num = 1;
            break;
        }
        case 'b': {
            num = 1;
            len = 8;
            break;
        }
        case 'B': {
            num = 1;
            len = 8;
            break;
        }
        case '?': {
            num = 1;
            len = 1;
            break;
        }
        case 'h': {
            num = 1;
            len = 16;
            break;
        }
        case 'H': {
            num = 1;
            len = 16;
            break;
        }
        case 'i': {
            num = 1;
            len = 32;
            break;
        }
        case 'I': {
            num = 1;
            len = 32;
            break;
        }
        case 'l': {
            num = 1;
            len = 64;
            break;
        }
        case 'L': {
            num = 1;
            len = 64;
            break;
        }
        case 'q': {
            num = 1;
            len = 64;
            break;
        }
        case 'Q': {
            num = 1;
            len = 64;
            break;
        }
        case 'f': {
            num = 1;
            len = 32;
            break;
        }

        case 'd': {
            num = 1;
            len = 64;
            break;
        }

        case 's':
        case 'p': {
            num = strlen(str) / 8;
            len = 8;
            break;
        }

        case 'P': {
            num = 1;
            len = 32;
            break;
        }
    }

    return NUMBER_VAL(num * len);
}

Value charNative(int argCount, Value *args)
{
    char str[2];
    str[0] = 0xFE;
    str[1] = '\0';
    if (argCount == 0)
    {
        return STRING_VAL(str);
    }
    if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("char")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("char")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("char")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }

    int value = (int)AS_NUMBER(toNumber(args[0]));
    if (isprint(value))
        str[0] = (char)value;

    return STRING_VAL(str);
}

Value xorNative(int argCount, Value *args)
{
    Value a = intNative(argCount, args);
    Value b = intNative(argCount - 1, args + 1);
    return NUMBER_VAL((int)AS_NUMBER(a) ^ (int)AS_NUMBER(b));
}

Value listNative(int argCount, Value *args)
{
    ObjList *list = NULL;

    if (argCount == 1)
    {
        Value arg = args[0];
        if (IS_STRING(arg))
        {
            ObjString *str = AS_STRING(arg);
            list = initList();
            for (int i = 0; i < str->length; i++)
            {
                char s[2];
                s[0] = str->chars[i];
                s[1] = '\0';
                writeValueArray(&list->values, STRING_VAL(s));
            }
        }
        else if (IS_DICT(arg))
        {
            ObjDict *dict = AS_DICT(arg);
            list = initList();
            int len;
            if (dict->count != 0)
            {
                for (int i = 0; i < dict->capacity; ++i)
                {
                    dictItem *item = dict->items[i];

                    if (!item || item->deleted)
                    {
                        continue;
                    }

                    len = strlen(item->key);
                    char *key = mp_malloc(sizeof(char) * (len + 1));
                    strcpy(key, item->key);
                    writeValueArray(&list->values, STRING_VAL(key));
                    mp_free(key);

                    writeValueArray(&list->values, copyValue(item->item));
                }
            }
        }
        else if (IS_ENUM(arg))
        {
            ObjEnum *enume = AS_ENUM(arg);
            list = initList();

            int i = 0;
            Entry entry;
            Table *table = &enume->members;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                writeValueArray(&list->values, copyValue(OBJ_VAL(entry.key)));
            }
        }
        else if (IS_INSTANCE(arg))
        {
            Value method;
            if (tableGet(&AS_INSTANCE(arg)->klass->methods, AS_STRING(STRING_VAL("list")), &method))
            {
                ObjRequest *request = newRequest();
                request->fn = method;
                request->pops = 1;
                return OBJ_VAL(request);
            }

            if (tableGet(&AS_INSTANCE(arg)->klass->staticFields, AS_STRING(STRING_VAL("list")), &method))
            {
                ObjRequest *request = newRequest();
                request->fn = method;
                request->pops = 1;
                return OBJ_VAL(request);
            }
        }
        else if (IS_CLASS(arg))
        {
            Value method;
            if (tableGet(&AS_CLASS(arg)->staticFields, AS_STRING(STRING_VAL("list")), &method))
            {
                ObjRequest *request = newRequest();
                request->fn = method;
                request->pops = 1;
                return OBJ_VAL(request);
            }
        }
    }

    if (list == NULL)
    {
        list = initList();
        for (int i = 0; i < argCount; i++)
        {
            writeValueArray(&list->values, copyValue(args[i]));
        }
    }
    return OBJ_VAL(list);
}

Value jsonToValue(cJSON *json)
{
    if (cJSON_IsTrue(json))
        return TRUE_VAL;
    else if (cJSON_IsFalse(json))
        return FALSE_VAL;
    else if (cJSON_IsNull(json))
        return NULL_VAL;
    else if (cJSON_IsNumber(json))
        return NUMBER_VAL(json->valuedouble);
    else if (cJSON_IsString(json))
        return STRING_VAL(json->valuestring);
    else if (cJSON_IsArray(json))
    {
        ObjList *list = initList();
        cJSON *child = json->child;
        while (child != NULL)
        {
            addList(list, jsonToValue(child));
            child = child->next;
        }
        return OBJ_VAL(list);
    }
    else if (cJSON_IsObject(json))
    {
        ObjDict *dict = initDict();
        cJSON *child = json->child;
        while (child != NULL)
        {
            insertDict(dict, child->string, jsonToValue(child));
            child = child->next;
        }
        return OBJ_VAL(dict);
    }
    return NULL_VAL;
}

Value dictNative(int argCount, Value *args)
{
    if (argCount != 3)
    {
        if (argCount == 1)
        {
            if (IS_ENUM(args[0]))
            {
                ObjEnum *enume = AS_ENUM(args[0]);
                ObjDict *dict = initDict();

                int i = 0;
                Entry entry;
                Table *table = &enume->members;
                while (iterateTable(table, &entry, &i))
                {
                    if (entry.key == NULL)
                        continue;

                    insertDict(dict, entry.key->chars, copyValue(AS_ENUM_VALUE(entry.value)->value));
                }

                return OBJ_VAL(dict);
            }
            else if (IS_INSTANCE(args[0]))
            {
                Value method;
                if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("dict")), &method))
                {
                    ObjRequest *request = newRequest();
                    request->fn = method;
                    request->pops = 1;
                    return OBJ_VAL(request);
                }

                if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("dict")), &method))
                {
                    ObjRequest *request = newRequest();
                    request->fn = method;
                    request->pops = 1;
                    return OBJ_VAL(request);
                }
            }
            else if (IS_CLASS(args[0]))
            {
                Value method;
                if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("dict")), &method))
                {
                    ObjRequest *request = newRequest();
                    request->fn = method;
                    request->pops = 1;
                    return OBJ_VAL(request);
                }
            }
            else if (IS_STRING(args[0]))
            {
                char *str = AS_CSTRING(args[0]);
                cJSON *json = cJSON_Parse(str);
                if (json == NULL || !cJSON_IsObject(json))
                {
                    runtimeError("Invalid dict string.");
                    return NULL_VAL;
                }

                Value value = jsonToValue(json);

                cJSON_Delete(json);
                return value;
            }
        }
        else if (argCount % 2 == 0)
        {
            ObjDict *dict = initDict();
            for (int i = 0; i < argCount; i += 2)
            {
                ObjString *str = AS_STRING(toString(args[i]));
                char *key = str->chars;
                insertDict(dict, key, args[i + 1]);
            }
            return OBJ_VAL(dict);
        }

        runtimeError("dict(str, delimiter, innerDelimiter) takes exactly 3 "
                     "arguments (%d given).",
                     argCount);
        return NULL_VAL;
    }

    for (int i = 0; i < argCount; i++)
    {
        if (!IS_STRING(args[i]))
        {
            runtimeError("'dict' argument must be a string.");
            return NULL_VAL;
        }
    }

    Value listV = stringSplit(args[0], args[1]);
    if (!IS_LIST(listV))
    {
        runtimeError("dict(): Failed to split '%s' by '%s'.", AS_CSTRING(args[0]), AS_CSTRING(args[1]));
        return NULL_VAL;
    }

    ObjList *list = AS_LIST(listV);

    ObjDict *dict = initDict();

    for (int i = 0; i < list->values.count; i++)
    {
        Value innerV = stringSplit(list->values.values[i], args[2]);
        if (!IS_LIST(innerV))
        {
            runtimeError("dict(): Failed to split '%s' by '%s'.", AS_CSTRING(list->values.values[i]),
                         AS_CSTRING(args[2]));
            return NULL_VAL;
        }

        ObjList *inner = AS_LIST(innerV);
        if (inner->values.count == 0)
            continue;
        if (AS_STRING(inner->values.values[0])->length == 0)
            continue;

        char *key = AS_CSTRING(inner->values.values[0]);
        Value val;
        if (inner->values.count > 1)
            val = inner->values.values[1];
        else
            val = STRING_VAL("");
        insertDict(dict, key, val);
    }

    return OBJ_VAL(dict);
}

Value bytesNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        char c = 0;
        return BYTES_VAL(&c, 0);
    }
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("bytes")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("bytes")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("bytes")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    return toBytes(args[0]);
}

Value makeNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    ValueType vType = VAL_NULL;
    ObjType oType = OBJ_STRING;
    bool instance = false;
    if (IS_STRING(args[0]))
    {
        char *name = AS_CSTRING(args[0]);
        if (strcmp(name, "null") == 0)
            vType = VAL_NULL;
        else if (strcmp(name, "bool") == 0)
            vType = VAL_BOOL;
        else if (strcmp(name, "num") == 0)
            vType = VAL_NUMBER;
        else if (strcmp(name, "string") == 0 || strcmp(name, "str") == 0)
        {
            vType = VAL_OBJ;
            oType = OBJ_STRING;
        }
        else if (strcmp(name, "list") == 0 || strcmp(name, "array") == 0)
        {
            vType = VAL_OBJ;
            oType = OBJ_LIST;
        }
        else if (strcmp(name, "byte") == 0 || strcmp(name, "bytes") == 0)
        {
            vType = VAL_OBJ;
            oType = OBJ_BYTES;
        }
        else
        {
            vType = VAL_OBJ;
            oType = OBJ_STRING;
            instance = true;
        }
    }
    else
    {
        if (IS_NULL(args[0]))
            vType = VAL_NULL;
        else if (IS_BOOL(args[0]))
            vType = VAL_BOOL;
        else if (IS_NUMBER(args[0]))
            vType = VAL_NUMBER;
        else
            vType = VAL_OBJ;
        if (vType != VAL_OBJ)
        {
            if (IS_LIST(args[0]))
            {
                oType = OBJ_LIST;
            }
        }
    }

    if (vType == VAL_NULL)
        return NULL_VAL;
    else if (vType == VAL_BOOL)
    {
        return argCount == 1 ? FALSE_VAL : toBool(args[1]);
    }
    else if (vType == VAL_NUMBER)
    {
        return argCount == 1 ? NUMBER_VAL(0) : toNumber(args[1]);
    }
    else if (vType == VAL_OBJ)
    {
        if (oType == OBJ_STRING && !instance)
        {
            return argCount == 1 ? STRING_VAL("") : toString(args[1]);
        }
        else if (oType == OBJ_LIST)
        {
            if (argCount == 1)
                return OBJ_VAL(initList());

            int sz = (int)AS_NUMBER(toNumber(args[1]));
            Value def = NULL_VAL;
            if (argCount > 2)
                def = args[2];

            ObjList *list = initList();
            for (int i = 0; i < sz; i++)
            {
                writeValueArray(&list->values, copyValue(def));
            }

            return OBJ_VAL(list);
        }
        else if (oType == OBJ_BYTES)
        {
            if (argCount == 1)
                return OBJ_VAL(initBytes());

            int sz = (int)AS_NUMBER(toNumber(args[1]));
            Value def = FALSE_VAL;
            if (argCount > 2)
                def = args[2];

            Value b = toBytes(def);
            ObjBytes *bytes = initBytes();
            for (int i = 0; i < sz; i++)
            {
                appendBytes(bytes, AS_BYTES(b));
            }

            return OBJ_VAL(bytes);
        }
        else
        {
            ObjString *name = AS_STRING(args[0]);
            int len = name->length + 2 + 32;
            int cur = 0;
            char *code = mp_malloc(len);
            strcpy(code, name->chars);
            strcat(code, "(");
            cur = strlen(code);
            for (int i = 1; i < argCount; i++)
            {
                char *arg = valueToString(args[i], true);
                int l = strlen(arg);
                if (l + cur + 4 > len)
                {
                    len += l * 2 + 4;
                    code = mp_realloc(code, len);
                }
                strcat(code, arg);
                mp_free(arg);
                if (i < argCount - 1)
                    strcat(code, ",");
            }
            strcat(code, ")");

            ObjFunction *fn = eval(code);
            mp_free(code);
            if (fn == NULL)
            {
                return NULL_VAL;
            }

            ObjRequest *request = newRequest();
            request->fn = OBJ_VAL(newClosure(fn));
            request->pops = argCount;
            return OBJ_VAL(request);
        }
    }
    return NULL_VAL;
}

static int colorCode(char *color)
{
    if (strcmp(color, "black") == 0)
        return 30;
    else if (strcmp(color, "red") == 0)
        return 31;
    else if (strcmp(color, "green") == 0)
        return 32;
    else if (strcmp(color, "yellow") == 0)
        return 33;
    else if (strcmp(color, "blue") == 0)
        return 34;
    else if (strcmp(color, "magenta") == 0)
        return 35;
    else if (strcmp(color, "cyan") == 0)
        return 36;
    else if (strcmp(color, "white") == 0)
        return 37;
    return 30;
}

Value colorNative(int argCount, Value *args)
{
    char *format = mp_malloc(sizeof(char) * 30);
    strcpy(format, "\033[0;");

    int code = 30;

    if (argCount > 0)
    {
        code = 37;
        if (IS_STRING(args[0]))
            code = colorCode(AS_CSTRING(args[0]));
        else if (IS_NUMBER(args[0]))
            code = AS_NUMBER(args[0]);
        sprintf(format + strlen(format), "%d", code);
    }

    if (argCount > 1)
    {
        code = 40;
        if (IS_STRING(args[1]))
            code = colorCode(AS_CSTRING(args[1])) + 10;
        else if (IS_NUMBER(args[1]))
            code = AS_NUMBER(args[1]);
        sprintf(format + strlen(format), ";%d", code);
    }

    sprintf(format + strlen(format), "m");

    Value res = STRING_VAL(format);
    mp_free(format);

    return res;
}

Value dateNative(int argCount, Value *args)
{
    char *format;
    int len;
    if (argCount == 0)
    {
        len = 4;
        format = ALLOCATE(char, len);
        strcpy(format, "%c");
    }
    else
    {
        ObjString *str = AS_STRING(toString(args[0]));
        len = str->length * 2;
        format = ALLOCATE(char, len);
        strcpy(format, str->chars);
    }

    replaceString(format, "YYYY", "%Y");
    replaceString(format, "YY", "%y");
    replaceString(format, "MM", "%m");
    replaceString(format, "mm", "%M");
    replaceString(format, "ss", "%S");
    replaceString(format, "hh", "%I");
    replaceString(format, "HH", "%H");
    replaceString(format, "dd", "%d");
    replaceString(format, "D", "%e");

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[256];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 256, format, timeinfo);
    Value ret = STRING_VAL(buffer);
    FREE_ARRAY(char, format, len);
    return ret;
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
        runtimeError("len() takes exactly 1 argument (%d given).", argCount);
        return NULL_VAL;
    }

    if (IS_STRING(args[0]))
        return NUMBER_VAL(AS_STRING(args[0])->length);
    else if (IS_BYTES(args[0]))
        return NUMBER_VAL(AS_BYTES(args[0])->length);
    else if (IS_LIST(args[0]))
        return NUMBER_VAL(AS_LIST(args[0])->values.count);
    else if (IS_DICT(args[0]))
        return NUMBER_VAL(AS_DICT(args[0])->count);
    else if (IS_ENUM(args[0]))
        return NUMBER_VAL(AS_ENUM(args[0])->members.count);
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("len")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("len")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("len")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_NULL(args[0]))
        return NUMBER_VAL(0);

    // runtimeError("Unsupported type passed to len()", argCount);
    return NUMBER_VAL(1);
}

Value typeNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    char *str = valueType(args[0]);
    Value res = STRING_VAL(str);
    mp_free(str);
    return res;
}

Value varsNative(int argCount, Value *args)
{
    int i = 0;
    Entry entry;
    int K = 0;
    Table *tables[10];
    char *titles[10];
    Table *table;
    bool isGlobal = false;
    bool showLocals = false;

    if (argCount > 0 && IS_MODULE(args[0]))
    {
        tables[K++] = &(AS_MODULE(args[0])->symbols);
        titles[K - 1] = "Symbols";
    }
    else if (argCount > 0 && IS_CLASS(args[0]))
    {
        tables[K++] = &(AS_CLASS(args[0])->staticFields);
        titles[K - 1] = "Static fields";
        tables[K++] = &(AS_CLASS(args[0])->fields);
        titles[K - 1] = "Fields";
        tables[K++] = &(AS_CLASS(args[0])->methods);
        titles[K - 1] = "Methods";
    }
    else if (argCount > 0 && IS_INSTANCE(args[0]))
    {
        tables[K++] = &(AS_INSTANCE(args[0])->fields);
        titles[K - 1] = "Fields";
        tables[K++] = &(AS_INSTANCE(args[0])->klass->staticFields);
        titles[K - 1] = "Static fields";
        tables[K++] = &(AS_INSTANCE(args[0])->klass->methods);
        titles[K - 1] = "Methods";
    }
    else if (argCount > 0 && IS_ENUM(args[0]))
    {
        tables[K++] = &(AS_ENUM(args[0])->members);
        titles[K - 1] = "Members";
    }
    else
    {
        isGlobal = true;
        tables[K++] = &vm.globals;
        titles[K - 1] = "Global variables";
        tables[K++] = &vm.extensions;
        titles[K - 1] = "Extensions";

        if (argCount > 0 && IS_BOOL(args[0]))
        {
            showLocals = AS_BOOL(args[0]);
        }
    }

    for (int k = 0; k < K; k++)
    {
        table = tables[k];
        i = 0;
        if (table->count <= 0)
            continue;
        printf("-----------------\n%s\n-----------------\n", titles[k]);

        if (isGlobal && k == 0)
        {
            ThreadFrame *tf = currentThread();

            printf("__name__ : ");
            printValue(STRING_VAL(tf->ctf->currentScriptName));
            printf("\n");

            printf("%s : ", vm.argsString);
            printValue(tf->ctf->currentArgs);
            printf("\n");

            printf("__ans__ : ");
            printValue(vm.repl);
            printf("\n");

            printf("__std__ : ");
            printValue(OBJ_VAL(vm.stdModule));
            printf("\n");
        }

        while (iterateTable(table, &entry, &i))
        {
            if (entry.key == NULL)
                continue;
            if (isGlobal && IS_NATIVE(entry.value))
                continue;

            printf("%s : ", entry.key->chars);
            printValue(entry.value);
            printf("\n");
        }
    }

    if (showLocals)
    {
        ThreadFrame *tf = currentThread();
        Value *value = tf->ctf->stack;
        if (value < tf->ctf->stackTop - 1)
        {
            printf("-----------------\nLocal variables\n-----------------\n");
            int i = 0;
            while (value < tf->ctf->stackTop - 1)
            {
                printf("%d : ", i++);
                printValue(*value);
                printf("\n");
                value++;
            }
        }
    }

    vm.print = true;
    vm.newLine = false;
    vm.repl = NULL_VAL;
    return NULL_VAL;
}

Value dirNative(int argCount, Value *args)
{
    int i = 0;
    Entry entry;
    int K = 0;
    Table *tables[10];
    Table *table;
    bool isGlobal = false;
    ObjList *list = initList();

    if (argCount > 0 && IS_MODULE(args[0]))
    {
        tables[K++] = &(AS_MODULE(args[0])->symbols);
    }
    else if (argCount > 0 && IS_CLASS(args[0]))
    {
        tables[K++] = &(AS_CLASS(args[0])->staticFields);
        tables[K++] = &(AS_CLASS(args[0])->fields);
        tables[K++] = &(AS_CLASS(args[0])->methods);
    }
    else if (argCount > 0 && IS_INSTANCE(args[0]))
    {
        tables[K++] = &(AS_INSTANCE(args[0])->fields);
        tables[K++] = &(AS_INSTANCE(args[0])->klass->staticFields);
        tables[K++] = &(AS_INSTANCE(args[0])->klass->methods);
    }
    else if (argCount > 0 && IS_ENUM(args[0]))
    {
        tables[K++] = &(AS_ENUM(args[0])->members);
    }
    else
    {
        isGlobal = true;
        tables[K++] = &vm.globals;
        tables[K++] = &vm.extensions;
    }

    for (int k = 0; k < K; k++)
    {
        table = tables[k];
        i = 0;
        if (table->count <= 0)
            continue;

        if (isGlobal && k == 0)
        {
            writeValueArray(&list->values, STRING_VAL("__name__"));
            writeValueArray(&list->values, STRING_VAL(vm.argsString));
            writeValueArray(&list->values, STRING_VAL("__ans__"));
            writeValueArray(&list->values, STRING_VAL("__std__"));
        }

        while (iterateTable(table, &entry, &i))
        {
            if (entry.key == NULL)
                continue;
            if (isGlobal && IS_NATIVE(entry.value))
                continue;

            writeValueArray(&list->values, STRING_VAL(entry.key->chars));
        }
    }

    return OBJ_VAL(list);
}

Value openNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("open requires at least a file name");
        return NULL_VAL;
    }

    Value fileName = args[0];
    Value openType;
    if (argCount > 1)
        openType = args[1];
    else
        openType = STRING_VAL("r");

    if (!IS_STRING(openType))
    {
        runtimeError("File open type must be a string");
        return NULL_VAL;
    }

    if (!IS_STRING(fileName))
    {
        runtimeError("Filename must be a string");
        return NULL_VAL;
    }

    ObjString *openTypeString = AS_STRING(openType);
    ObjString *fileNameString = AS_STRING(fileName);

    char *path = fixPath(fileNameString->chars);

    ObjFile *file = openFile(path, openTypeString->chars);
    mp_free(path);
    if (file == NULL)
    {
        // runtimeError("Unable to open file");
        return NULL_VAL;
    }

    return OBJ_VAL(file);
}

Value pwdNative(int argCount, Value *args)
{
    char cCurrentPath[FILENAME_MAX];
    Value ret = NULL_VAL;
    if (GetCurrentDir(cCurrentPath, FILENAME_MAX))
    {
        cCurrentPath[FILENAME_MAX - 1] = '\0';
        ret = STRING_VAL(cCurrentPath);
    }
    return ret;
}

Value lsNative(int argCount, Value *args)
{

    char *pathRaw = ".";
    if (argCount > 0 && IS_STRING(args[0]))
        pathRaw = AS_CSTRING(args[0]);

    char *path = fixPath(pathRaw);

    ObjList *list = initList();
#ifdef _WIN32
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char sPath[2048];
    sprintf(sPath, "%s\\*.*", path);
    if ((hFind = FindFirstFile(sPath, &fdFile)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            writeValueArray(&list->values, STRING_VAL(fdFile.cFileName));
        } while (FindNextFile(hFind, &fdFile)); // Find the next file.

        FindClose(hFind); // Always, Always, clean things up!
    }

#else
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            writeValueArray(&list->values, STRING_VAL(dir->d_name));
        }
        closedir(d);
    }
#endif
    mp_free(path);
    return OBJ_VAL(list);
}

Value isdirNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);

    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
    {
        mp_free(path);
        return FALSE_VAL;
    }
    mp_free(path);

    int rc = S_ISDIR(statbuf.st_mode);

    return BOOL_VAL(rc != 0);
}

Value cdNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);

    int rc = chdir(path);
    mp_free(path);
    return BOOL_VAL(rc == 0);
}

Value existsNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);
    Value ret = BOOL_VAL(existsFile(path));
    mp_free(path);
    return ret;
}

Value findNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);
    char *found = findFile(path);
    mp_free(path);

    if (found == NULL)
        return NULL_VAL;
    Value ret = STRING_VAL(found);
    mp_free(found);
    return ret;
}

Value removeNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);
    int rc = remove(path);
    mp_free(path);
    return BOOL_VAL(rc == 0);
}

static int cube_mkdir(const char *dir, int mode)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    mode = 0775;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
#ifdef _WIN32
            mkdir(tmp);
#else
            mkdir(tmp, mode);
#endif
            *p = '/';
        }
    }
#ifdef _WIN32
    int rc = mkdir(tmp);
#else
    int rc = mkdir(tmp, mode);
#endif
    // if (rc < 0)
    // {
    //     char cmd[512];
    //     sprintf(cmd, "mkdir -m %d %s", mode, tmp);
    //     rc = system(cmd);
    // }
    return rc;
}

Value mkdirNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    int mode = 0777;
    // if (argCount > 1 && IS_NUMBER(args[1]))
    // {
    //     mode = AS_NUMBER(args[1]);
    // }

    char *path = fixPath(str);
    int rc = cube_mkdir(path, mode);
    mp_free(path);

    return BOOL_VAL(rc == 0);
}

Value envNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    if (!IS_STRING(args[0]))
        return NULL_VAL;
    char *str = AS_CSTRING(args[0]);
    char *env = getEnv(str);
    if (env == NULL)
        return NULL_VAL;
    return STRING_VAL(env);
}

Value copyNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;
    return copyValue(args[0]);
}

Value evalNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    Value arg = args[0];
    if (!IS_STRING(arg))
    {
        runtimeError("'eval' requires a string parameter\n");
        return NULL_VAL;
    }

    ObjString *str = AS_STRING(arg);
    ObjFunction *function = eval(str->chars);

    if (function == NULL)
    {
        runtimeError("Failed to evaluate.\n");
        return NULL_VAL;
    }

    ThreadFrame *threadFrame = currentThread();
    threadFrame->ctf->eval = true;
    return OBJ_VAL(function);
}

Value memNative(int argCount, Value *args)
{
    bool human = false;
    if (argCount > 0)
    {
        human = AS_BOOL(toBool(args[0]));
    }

    if (!human)
    {
        return NUMBER_VAL(vm.bytesAllocated);
    }

    int sz = vm.bytesAllocated;
    char *comp = (char *)mp_malloc(sizeof(char) * 4);
    if (sz > 1024 * 1024 * 1024)
    {
        sz /= (1024 * 1024 * 1024);
        strcpy(comp, " Gb");
    }
    else if (sz > 1024 * 1024)
    {
        sz /= (1024 * 1024);
        strcpy(comp, " Mb");
    }
    else if (sz > 1024)
    {
        sz /= 1024;
        strcpy(comp, " Kb");
    }
    else
        strcpy(comp, " b");

    char *vStr = valueToString(NUMBER_VAL(sz), false);

    char *str = (char *)mp_malloc(sizeof(char) * (strlen(comp) + strlen(vStr) + 1));
    strcpy(str, vStr);
    strcat(str, comp);

    Value ret = STRING_VAL(str);
    mp_free(comp);
    mp_free(vStr);
    mp_free(str);

    return ret;
}

Value gcCollectNative(int argCount, Value *args)
{
    gc_collect();
    return NUMBER_VAL(vm.bytesAllocated);
}

Value autoGCNative(int argCount, Value *args)
{
    if (argCount == 0)
        return BOOL_VAL(vm.autoGC);
    bool autoGC = AS_BOOL(toBool(args[0]));
    vm.autoGC = autoGC;
    return BOOL_VAL(vm.autoGC);
}

Value systemInfoNative(int argCount, Value *args)
{
    ObjDict *dict = initDict();

    Value os = STRING_VAL(PLATFORM_NAME);
    insertDict(dict, "os", os);
    insertDict(dict, "version", STRING_VAL(version_string));

    int columns, rows;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize max;
    ioctl(0, TIOCGWINSZ, &max);
    rows = max.ws_row;
    columns = max.ws_col;
#endif

    insertDict(dict, "cols", NUMBER_VAL(columns));
    insertDict(dict, "rows", NUMBER_VAL(rows));

    if ((size_t)-1 > 0xffffffffUL)
        insertDict(dict, "arch", NUMBER_VAL(64));
    else
        insertDict(dict, "arch", NUMBER_VAL(32));
    return OBJ_VAL(dict);
}

Value printStackNative(int argCount, Value *args)
{
    ThreadFrame *threadFrame = currentThread();
    printf("Task(%s)\n", threadFrame->ctf->name);
    for (Value *slot = threadFrame->ctf->stack; slot < threadFrame->ctf->stackTop - 1; slot++)
    {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }

    vm.newLine = true;
    vm.print = true;
    vm.repl = NULL_VAL;
    return NULL_VAL;
}

Value isdigitNative(int argCount, Value *args)
{
    if (argCount != 1)
        return FALSE_VAL;
    if (!IS_STRING(args[0]))
        return FALSE_VAL;
    ObjString *str = AS_STRING(args[0]);
    if (str->length != 1)
        return FALSE_VAL;

    char c = str->chars[0];
    return BOOL_VAL(isdigit(c));
}

Value ischarNative(int argCount, Value *args)
{
    if (argCount != 1)
        return FALSE_VAL;
    if (!IS_STRING(args[0]))
        return FALSE_VAL;
    ObjString *str = AS_STRING(args[0]);
    if (str->length != 1)
        return FALSE_VAL;

    char c = str->chars[0];
    return BOOL_VAL(((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'));
}

Value isspaceNative(int argCount, Value *args)
{
    if (argCount != 1)
        return FALSE_VAL;
    if (!IS_STRING(args[0]))
        return FALSE_VAL;
    ObjString *str = AS_STRING(args[0]);
    if (str->length != 1)
        return FALSE_VAL;

    char c = str->chars[0];
    return BOOL_VAL((c == ' ' || c == '\n' || c == '\r' || c == '\t'));
}

Value processNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("No process specified.\n");
        return NULL_VAL;
    }

    if (!IS_STRING(args[0]))
    {
        runtimeError("Invalid process.\n");
        return NULL_VAL;
    }

    char *str = AS_CSTRING(args[0]);
    char *path = fixPath(str);
    char *found = findFile(path);

    if (found == NULL)
        found = path;
    else
        mp_free(path);

    ObjProcess *process = newProcess(AS_STRING(STRING_VAL(found)), argCount - 1, args + 1);
    mp_free(found);
    if (process == NULL)
    {
        runtimeError("Could not start the process.\n");
        return NULL_VAL;
    }

    return OBJ_VAL(process);
}

Value readNative(int argCount, Value *args)
{
    int maxSize = MAX_STRING_INPUT;
    if (argCount == 0)
    {
        runtimeError("No source provided.\n");
        return NULL_VAL;
    }

    if (argCount == 2 && IS_NUMBER(args[1]))
    {
        maxSize = AS_NUMBER(args[1]);
    }

    if (IS_PROCESS(args[0]))
    {
        ObjProcess *process = AS_PROCESS(args[0]);
        if (process->closed)
        {
            runtimeError("Cannot read from a finished process.\n");
            return NULL_VAL;
        }

        char *buffer = NULL;
        int size = 0;
        int rd = maxSize - size;
        if (rd > 256)
            rd = 256;

        char buf[256];
        int rc = readFd(process->in, rd, buf);
        while (rc > 0 && size <= maxSize)
        {
            buffer = mp_realloc(buffer, size + rc + 1);
            memcpy(buffer + size, buf, rc);
            size += rc;
            rd = maxSize - size;
            if (rd > 256)
                rd = 256;
#ifndef _WIN32
            if (process->in == STDIN_FILENO)
#else
            if (process->in == _fileno(stdin))
#endif

                break;
            rc = readFd(process->in, rd, buf);
        }

        if (rc < 0 && size == 0)
        {
            mp_free(buffer);
            if (errno != EAGAIN)
                runtimeError("Could not read the process.\n");
            return NULL_VAL;
        }

        Value ret;
        if (buffer == NULL)
            ret = STRING_VAL("");
        else
        {
            buffer[size] = '\0';
            ret = STRING_VAL(buffer);
            mp_free(buffer);
        }
        return ret;
    }
    else if (IS_FILE(args[0]))
    {
        ObjFile *file = AS_FILE(args[0]);
        if (!file->isOpen)
        {
            runtimeError("Cannot read from a closed file.\n");
            return NULL_VAL;
        }

        char *buffer = NULL;
        int size = 0;
        int rd = maxSize - size;
        if (rd > 256)
            rd = 256;

        char buf[256];
        int rc = readFileRaw(file->file, rd, buf);
        while (rc > 0 && size <= maxSize)
        {
            buffer = mp_realloc(buffer, size + rc + 1);
            memcpy(buffer + size, buf, rc);
            size += rc;
            rd = maxSize - size;
            if (rd > 256)
                rd = 256;
        }

        if (rc < 0)
        {
            mp_free(buffer);
            runtimeError("Could not read the process.\n");
            return NULL_VAL;
        }

        Value ret;
        if (buffer == NULL)
            ret = STRING_VAL("");
        else
        {
            buffer[size] = '\0';
            ret = STRING_VAL(buffer);
            mp_free(buffer);
        }
        return ret;
    }
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("read")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("read")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("read")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    runtimeError("Invalid source.\n");
    return NULL_VAL;
}

Value writeNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("No source provided.\n");
        return NULL_VAL;
    }

    if (IS_PROCESS(args[0]))
    {
        ObjProcess *process = AS_PROCESS(args[0]);
        if (process->closed)
        {
            runtimeError("Cannot write to a finished process.\n");
            return NULL_VAL;
        }

        int size = 0;
        int rc;
        for (int i = 1; i < argCount; i++)
        {
            if (IS_BYTES(args[i]))
            {
                ObjBytes *bytes = AS_BYTES(args[i]);
                if (bytes->length >= 0)
                    rc = writeFd(process->out, bytes->length, (char *)bytes->bytes);
                else
                {
                    runtimeError("Cannot write unsafe bytes.\n");
                    return NULL_VAL;
                }
            }
            else
            {
                ObjString *str = AS_STRING(toString(args[i]));
                rc = writeFd(process->out, str->length, str->chars);
#ifndef _WIN32
                if (process->out == STDOUT_FILENO)
#else
                if (process->out == _fileno(stdout))
#endif

                    vm.newLine = str->chars[str->length - 1] != '\n';
            }

            if (rc < 0)
            {
                char *str = objectToString(args[i], false);
                runtimeError("Could not write '%s' to the process.\n", str);
                mp_free(str);
                return NULL_VAL;
            }
            size += rc;
        }

        return NUMBER_VAL(size);
    }
    else if (IS_FILE(args[0]))
    {
        ObjFile *file = AS_FILE(args[0]);
        if (!file->isOpen)
        {
            runtimeError("Cannot write to a closed file.\n");
            return NULL_VAL;
        }

        int size = 0;
        int rc;
        for (int i = 1; i < argCount; i++)
        {
            ObjString *str = AS_STRING(toString(args[i]));
            rc = writeFileRaw(file->file, str->length, str->chars);
            if (rc < 0)
            {
                runtimeError("Could not write '%s' to the file.\n", str->chars);
                return NULL_VAL;
            }
            size += rc;
        }

        return NUMBER_VAL(size);
    }
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("write")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("write")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("write")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    runtimeError("Invalid source.\n");
    return NULL_VAL;
}

Value closeNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("No source provided.\n");
        return NULL_VAL;
    }

    if (IS_PROCESS(args[0]))
    {
        ObjProcess *process = AS_PROCESS(args[0]);
        if (process->protected)
        {
            return FALSE_VAL;
        }

        if (process->closed)
        {
            return TRUE_VAL;
        }

#ifdef _WIN32

#else
        close(process->in);
        close(process->out);
#endif
        process->closed = true;
        return TRUE_VAL;
    }
    else if (IS_FILE(args[0]))
    {
        ObjFile *file = AS_FILE(args[0]);
        if (!file->isOpen)
        {
            return TRUE_VAL;
        }

        fclose(file->file);
        file->isOpen = false;
        return TRUE_VAL;
    }
    else if (IS_INSTANCE(args[0]))
    {
        Value method;
        if (tableGet(&AS_INSTANCE(args[0])->klass->methods, AS_STRING(STRING_VAL("close")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }

        if (tableGet(&AS_INSTANCE(args[0])->klass->staticFields, AS_STRING(STRING_VAL("close")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    else if (IS_CLASS(args[0]))
    {
        Value method;
        if (tableGet(&AS_CLASS(args[0])->staticFields, AS_STRING(STRING_VAL("close")), &method))
        {
            ObjRequest *request = newRequest();
            request->fn = method;
            request->pops = 1;
            return OBJ_VAL(request);
        }
    }
    runtimeError("Invalid source.\n");
    return FALSE_VAL;
}

static Value getFieldNative(int argCount, Value *args)
{
    if (argCount != 2)
        return NULL_VAL;
    if (!IS_INSTANCE(args[0]))
        return NULL_VAL;
    if (!IS_STRING(args[1]))
        return NULL_VAL;

    ObjInstance *instance = AS_INSTANCE(args[0]);
    Value value = NULL_VAL;
    tableGet(&instance->fields, AS_STRING(args[1]), &value);
    return value;
}

static Value setFieldNative(int argCount, Value *args)
{
    if (argCount != 3)
        return FALSE_VAL;
    if (!IS_INSTANCE(args[0]))
        return FALSE_VAL;
    if (!IS_STRING(args[1]))
        return FALSE_VAL;

    ObjInstance *instance = AS_INSTANCE(args[0]);
    tableSet(&instance->fields, AS_STRING(args[1]), args[2]);
    return args[2];
}

static Value classNameNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    if (IS_INSTANCE(args[0]))
    {
        ObjInstance *instance = AS_INSTANCE(args[0]);
        return STRING_VAL(instance->klass->name->chars);
    }
    else if (IS_CLASS(args[0]))
    {
        ObjClass *klass = AS_CLASS(args[0]);
        return STRING_VAL(klass->name->chars);
    }

    return NULL_VAL;
}

static Value superNameNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NULL_VAL;

    if (IS_INSTANCE(args[0]))
    {
        ObjInstance *instance = AS_INSTANCE(args[0]);
        if (instance->klass->super == NULL)
            return NULL_VAL;
        return STRING_VAL(instance->klass->super->name->chars);
    }
    else if (IS_CLASS(args[0]))
    {
        ObjClass *klass = AS_CLASS(args[0]);
        if (klass->super == NULL)
            return NULL_VAL;
        return STRING_VAL(klass->super->name->chars);
    }

    return NULL_VAL;
}

static Value getFieldsNative(int argCount, Value *args)
{
    ObjList *list = initList();
    if (argCount != 0)
    {
        if (IS_INSTANCE(args[0]))
        {
            ObjInstance *instance = AS_INSTANCE(args[0]);
            int i = 0;
            Entry entry;
            Table *table = &instance->fields;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }
        }
        else if (IS_CLASS(args[0]))
        {
            ObjClass *klass = AS_CLASS(args[0]);
            int i = 0;
            Entry entry;
            Table *table = &klass->fields;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }

            i = 0;
            table = &klass->staticFields;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                if (!IS_FUNCTION(entry.value))
                    writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }
        }
    }

    return OBJ_VAL(list);
}

static Value getMethodsNative(int argCount, Value *args)
{
    ObjList *list = initList();
    if (argCount != 0)
    {
        if (IS_INSTANCE(args[0]))
        {
            ObjInstance *instance = AS_INSTANCE(args[0]);
            int i = 0;
            Entry entry;
            Table *table = &instance->klass->methods;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }
        }
        else if (IS_CLASS(args[0]))
        {
            ObjClass *klass = AS_CLASS(args[0]);
            int i = 0;
            Entry entry;
            Table *table = &klass->methods;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }

            i = 0;
            table = &klass->staticFields;
            while (iterateTable(table, &entry, &i))
            {
                if (entry.key == NULL)
                    continue;

                if (IS_FUNCTION(entry.value))
                    writeValueArray(&list->values, STRING_VAL(entry.key->chars));
            }
        }
    }

    return OBJ_VAL(list);
}

static Value skipWaitingTasksNative(int argCount, Value *args)
{
    if (argCount > 0)
    {
        bool v = AS_BOOL(toBool(args[0]));
        vm.skipWaitingTasks = v;
    }
    return BOOL_VAL(vm.skipWaitingTasks);
}

static Value dlopenNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("dlopen requires a file name");
        return NULL_VAL;
    }

    if (!IS_STRING(args[0]))
    {
        runtimeError("The library file name must be a string");
        return NULL_VAL;
    }

    ObjString *name = AS_STRING(pop());
    int len = name->length + strlen(vm.nativeExtension) + 1;
    char *str = mp_malloc(len);
    sprintf(str, "%s%s", name->chars, vm.nativeExtension);

    ObjNativeLib *lib = initNativeLib();
    lib->functions = 0;
    lib->name = AS_STRING(STRING_VAL(str));

    mp_free(str);

    return OBJ_VAL(lib);
}

static Value assertNative(int argCount, Value *args)
{
    bool valid = false;
    if (argCount < 2)
        runtimeError("assert()");
    else
    {
        if (AS_BOOL(toBool(args[1])) == false)
        {
            if (argCount > 2)
            {
                int sz = 128;
                int len = 0;
                char *error = ALLOCATE(char, sz);
                error[0] = '\0';
                for (int i = 2; i < argCount; i++)
                {
                    char *str = valueToString(args[i], false);
                    if (len + strlen(error) + 1 >= sz)
                    {
                        error = GROW_ARRAY(error, char, sz, sz * 2);
                        sz *= 2;
                    }
                    strcat(error + len, str);
                    mp_free(str);
                    len = strlen(error);
                }
                runtimeError("assert(%s): %s", AS_CSTRING(toString(args[0])), error);
                FREE_ARRAY(char, error, sz);
            }
            else
                runtimeError("assert(%s)", AS_CSTRING(toString(args[0])));
        }
        else
            valid = true;
    }
    return BOOL_VAL(valid);
}

Value clearNative(int argCount, Value *args)
{
    clearScreen();
    return NULL_VAL;
}

Value clearbNative(int argCount, Value *args)
{
    clearScreenToBottom();
    return NULL_VAL;
}

Value cleartNative(int argCount, Value *args)
{
    clearScreenToTop();
    return NULL_VAL;
}

Value clearlNative(int argCount, Value *args)
{
    clearLine();
    return NULL_VAL;
}

Value clearlrNative(int argCount, Value *args)
{
    clearLineToRight();
    return NULL_VAL;
}

Value clearllNative(int argCount, Value *args)
{
    clearLineToLeft();
    return NULL_VAL;
}

Value getPathNative(int argCount, Value *args)
{
    return copyObject(OBJ_VAL(vm.paths));
}

Value setPathNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("setPath requires a list of paths");
        return NULL_VAL;
    }

    if (!IS_LIST(args[0]))
    {
        runtimeError("setPath requires a list of paths");
        return NULL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    for (int i = 0; i < list->values.count; i++)
    {
        if (!IS_STRING(list->values.values[i]))
        {
            runtimeError("A path must be a string. %s is not valid.", valueToString(list->values.values[i], true));
            return NULL_VAL;
        }
    }

    vm.paths->values.count = 0;

    for (int i = 0; i < list->values.count; i++)
    {
        addPath(AS_CSTRING(list->values.values[i]));
    }

    return copyObject(OBJ_VAL(vm.paths));
}

Value addPathNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("addPath requires one or more paths.");
        return NULL_VAL;
    }

    for (int i = 0; i < argCount; i++)
    {
        if (!IS_STRING(args[i]))
        {
            runtimeError("A path must be a string. %s is not valid.", valueToString(args[i], true));
            return NULL_VAL;
        }
    }

    ObjList *list = initList();
    for (int i = 0; i < argCount; i++)
    {
        addPath(AS_CSTRING(args[i]));
        writeValueArray(&list->values, args[i]);
    }
    return OBJ_VAL(list);
}

Value rmPathNative(int argCount, Value *args)
{
    if (argCount == 0)
    {
        runtimeError("rmPath requires one or more paths.");
        return NULL_VAL;
    }

    for (int i = 0; i < argCount; i++)
    {
        if (!IS_STRING(args[i]))
        {
            runtimeError("A path must be a string. %s is not valid.", valueToString(args[i], true));
            return NULL_VAL;
        }
    }

    ObjList *list = initList();
    for (int i = 0; i < argCount; i++)
    {
        rmList(vm.paths, args[i]);
        writeValueArray(&list->values, args[i]);
    }
    return OBJ_VAL(list);
}

Value modulesNative(int argCount, Value *args)
{
    ObjList *list = initList();
    char name[128];

    for (int i = 0; i < vm.paths->values.count; i++)
    {
        if (IS_STRING(vm.paths->values.values[i]))
        {
            ObjList *items = AS_LIST(lsNative(1, &vm.paths->values.values[i]));
            for (int j = 0; j < items->values.count; j++)
            {
                if (IS_STRING(items->values.values[j]))
                {
                    if (stringEndsWith(AS_CSTRING(items->values.values[j]), vm.extension))
                    {
                        strcpy(name, AS_CSTRING(items->values.values[j]));
                        replaceString(name, vm.extension, "\0");
                        writeValueArray(&list->values, STRING_VAL(name));
                    }
                }
            }
        }
    }

    return OBJ_VAL(list);
}

// Register
linked_list *stdFnList;
#define ADD_STD(name, fn) linked_list_add(stdFnList, createStdFn(name, fn))

std_fn *createStdFn(const char *name, NativeFn fn)
{
    std_fn *stdFn = (std_fn *)mp_malloc(sizeof(std_fn));
    stdFn->name = name;
    stdFn->fn = fn;
    return stdFn;
}

void initStd()
{
    stdFnList = linked_list_create();

    ADD_STD("hash", hashNative);
    ADD_STD("clock", clockNative);
    ADD_STD("time", timeNative);
    ADD_STD("exit", exitNative);
    ADD_STD("input", inputNative);
    ADD_STD("print", printNative);
    ADD_STD("println", printlnNative);
    ADD_STD("throw", throwNative);
    ADD_STD("rand", randNative);
    ADD_STD("randn", randnNative);
    ADD_STD("seed", seedNative);
    ADD_STD("wait", waitNative);
    ADD_STD("sin", sinNative);
    ADD_STD("cos", cosNative);
    ADD_STD("tan", tanNative);
    ADD_STD("asin", asinNative);
    ADD_STD("acos", acosNative);
    ADD_STD("atan", atanNative);
    ADD_STD("atan2", atan2Native);
    ADD_STD("sqrt", sqrtNative);
    ADD_STD("log", logNative);
    ADD_STD("ln", lnNative);
    ADD_STD("isnan", isnanNative);
    ADD_STD("isinf", isinfNative);
    ADD_STD("isempty", isemptyNative);
    ADD_STD("bool", boolNative);
    ADD_STD("num", numNative);
    ADD_STD("int", intNative);
    ADD_STD("str", strNative);
    ADD_STD("hex", hexNative);
    ADD_STD("bin", binNative);
    ADD_STD("unbin", unbinNative);
    ADD_STD("mbin", mbinNative);
    ADD_STD("char", charNative);
    ADD_STD("list", listNative);
    ADD_STD("dict", dictNative);
    ADD_STD("bytes", bytesNative);
    ADD_STD("color", colorNative);
    ADD_STD("date", dateNative);
    ADD_STD("ceil", ceilNative);
    ADD_STD("floor", floorNative);
    ADD_STD("round", roundNative);
    ADD_STD("pow", powNative);
    ADD_STD("exp", expNative);
    ADD_STD("xor", xorNative);
    ADD_STD("len", lenNative);
    ADD_STD("type", typeNative);
    ADD_STD("vars", varsNative);
    ADD_STD("dir", dirNative);
    ADD_STD("open", openNative);
    ADD_STD("read", readNative);
    ADD_STD("write", writeNative);
    ADD_STD("close", closeNative);
    ADD_STD("isdir", isdirNative);
    ADD_STD("cd", cdNative);
    ADD_STD("pwd", pwdNative);
    ADD_STD("ls", lsNative);
    ADD_STD("exists", existsNative);
    ADD_STD("find", findNative);
    ADD_STD("remove", removeNative);
    ADD_STD("mkdir", mkdirNative);
    ADD_STD("env", envNative);
    ADD_STD("make", makeNative);
    ADD_STD("copy", copyNative);
    ADD_STD("eval", evalNative);
    ADD_STD("mem", memNative);
    ADD_STD("gcCollect", gcCollectNative);
    ADD_STD("enableAutoGC", autoGCNative);
    ADD_STD("systemInfo", systemInfoNative);
    ADD_STD("printStack", printStackNative);
    ADD_STD("isdigit", isdigitNative);
    ADD_STD("isspace", isspaceNative);
    ADD_STD("ischar", ischarNative);
    ADD_STD("process", processNative);
    ADD_STD("getField", getFieldNative);
    ADD_STD("setField", setFieldNative);
    ADD_STD("getClassName", classNameNative);
    ADD_STD("getSuperName", superNameNative);
    ADD_STD("getFields", getFieldsNative);
    ADD_STD("getMethods", getMethodsNative);
    ADD_STD("skipWaitingTasks", skipWaitingTasksNative);
    ADD_STD("dlopen", dlopenNative);
    ADD_STD("assert", assertNative);
    ADD_STD("clear", clearNative);
    ADD_STD("clearb", clearbNative);
    ADD_STD("cleart", cleartNative);
    ADD_STD("clearl", clearlNative);
    ADD_STD("clearlr", clearlrNative);
    ADD_STD("clearll", clearllNative);
    ADD_STD("getPath", getPathNative);
    ADD_STD("setPath", setPathNative);
    ADD_STD("addPath", addPathNative);
    ADD_STD("rmPath", rmPathNative);
    ADD_STD("modules", modulesNative);
}

void destroyStd()
{
    linked_list_destroy(stdFnList, true);
}