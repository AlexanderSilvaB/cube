#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#include "std.h"
#include "object.h"
#include "vm.h"
#include "files.h"
#include "memory.h"
#include "util.h"
#include "compiler.h"
#include "strings.h"
#include "gc.h"
#include "system.h"

Value clockNative(int argCount, Value *args)
{
    return NUMBER_VAL( ( cube_clock() * 1e-9 ) );
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
    //exit(code);
    return NONE_VAL;
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
    vm.print = true;
    vm.newLine = true;
    return NONE_VAL;
}

Value printlnNative(int argCount, Value *args)
{
    printNative(argCount, args);
    printf("\n");
    vm.newLine = false;
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

Value randomNative(int argCount, Value *args)
{
    int dv = 100000001;
    double r = rand() % dv;
    return NUMBER_VAL(r / dv);
}

Value seedNative(int argCount, Value *args)
{
    Value seed;
    if (argCount == 0)
        seed = NUMBER_VAL(time(NULL));
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
        wait = toNumber(args[0]);

    uint64_t current = cube_clock();
    vm.ctf->startTime = current;
    vm.ctf->endTime = current + ( AS_NUMBER(wait) * 1e6 );
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
    if (IS_NONE(arg))
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
    return toBool(args[0]);
}

Value numNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    return toNumber(args[0]);
}

Value intNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NUMBER_VAL(0);
    return NUMBER_VAL((int)AS_NUMBER(toNumber(args[0])));
}

Value strNative(int argCount, Value *args)
{
    if (argCount == 0)
        return STRING_VAL("");
    if (IS_BYTES(args[0]))
    {
        ObjBytes *bytes = AS_BYTES(args[0]);
        char *str = malloc(bytes->length + 1);
        int j = 0;
        for (int i = 0; i < bytes->length; i++)
        {
            if (isprint(bytes->bytes[i]) != 0 || iscntrl(bytes->bytes[i]) != 0)
                str[j++] = bytes->bytes[i];
        }
        str[j] = '\0';
        Value ret = STRING_VAL(str);
        free(str);
        return ret;
    }
    return toString(args[0]);
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
                    char *key = malloc(sizeof(char) * len);
                    strcpy(key, item->key);
                    writeValueArray(&list->values, STRING_VAL(key));
                    free(key);

                    writeValueArray(&list->values, copyValue(item->item));
                }
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

Value dictNative(int argCount, Value *args)
{
    if(argCount != 3)
    {
        printf("dict(str, delimiter, innerDelimiter) takes exactly 3 arguments (%d given).\n", argCount);
        return NONE_VAL;
    }

    for(int i = 0; i < argCount; i++)
    {
        if(!IS_STRING(args[i]))
        {
            printf("'dict' argument must be a string.\n");
            return NONE_VAL;
        }
    }

    Value listV = stringSplit(args[0], args[1]);
    if(!IS_LIST(listV))
    {
        printf("dict(): Failed to split '%s' by '%s'.\n", AS_CSTRING(args[0]), AS_CSTRING(args[1]));
        return NONE_VAL;
    }

    ObjList *list = AS_LIST(listV);
    
    ObjDict *dict = initDict();

    for(int i = 0; i < list->values.count; i++)
    {
        Value innerV = stringSplit(list->values.values[i], args[2]);
        if(!IS_LIST(innerV))
        {
            printf("dict(): Failed to split '%s' by '%s'.\n", AS_CSTRING(list->values.values[i]), AS_CSTRING(args[2]));
            return NONE_VAL;
        }

        ObjList *inner = AS_LIST(innerV);      
        if(inner->values.count == 0)
            continue;
        if(AS_STRING(inner->values.values[0])->length == 0)
            continue;
            
        char *key = AS_CSTRING(inner->values.values[0]);
        Value val;
        if(inner->values.count > 1)
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
    return toBytes(args[0]);
}

Value makeNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;

    ValueType vType = VAL_NONE;
    ObjType oType = OBJ_STRING;
    if (IS_STRING(args[0]))
    {
        char *name = AS_CSTRING(args[0]);
        if (strcmp(name, "none") == 0)
            vType = VAL_NONE;
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
        }
    }
    else
    {
        vType = args[0].type;
        if (vType != VAL_OBJ)
        {
            if (IS_LIST(args[0]))
            {
                oType = OBJ_LIST;
            }
        }
    }

    if (vType == VAL_NONE)
        return NONE_VAL;
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
        if (oType == OBJ_STRING)
        {
            return argCount == 1 ? STRING_VAL("") : toString(args[1]);
        }
        else if (oType == OBJ_LIST)
        {
            if (argCount == 1)
                return OBJ_VAL(initList());

            int sz = (int)AS_NUMBER(toNumber(args[1]));
            Value def = NONE_VAL;
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
    }
    return NONE_VAL;
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
    char *format = malloc(sizeof(char) * 30);
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
    free(format);

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
    printf("Unsupported type passed to len(): ");
    printValue(args[0]);
    printf("\n");
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
    Table *table = &vm.globals;

    if(IS_PACKAGE(args[0]))
    {
        table = &(AS_PACKAGE(args[0])->symbols);
    }

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;
        if (!showNative && IS_NATIVE(entry.value))
            continue;

        printf("%s : ", entry.key->chars);
        printValue(entry.value);
        printf("\n");
    }

    vm.print = true;
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

    ObjFile *file = openFile(fileNameString->chars, openTypeString->chars);
    if (file == NULL)
    {
        //runtimeError("Unable to open file");
        printf("Unable to open file\n");
        return NONE_VAL;
    }

    return OBJ_VAL(file);
}

Value pwdNative(int argCount, Value *args)
{
    char cCurrentPath[FILENAME_MAX];
    Value ret = NONE_VAL;
    if (GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
    {
        cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
        ret = STRING_VAL(cCurrentPath);
    }
    return ret;
}

Value lsNative(int argCount, Value *args)
{
    char *path = ".";
    if (argCount > 0 && IS_STRING(args[0]))
        path = AS_CSTRING(args[0]);

    ObjList *list = initList();
#ifdef WINDOWS

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
    return OBJ_VAL(list);
}

Value cdNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;
    if (!IS_STRING(args[0]))
        return NONE_VAL;
    char *str = AS_CSTRING(args[0]);
    int rc = chdir(str);
    return BOOL_VAL(rc == 0);
}

Value existsNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;
    if (!IS_STRING(args[0]))
        return NONE_VAL;
    char *str = AS_CSTRING(args[0]);
    return BOOL_VAL(existsFile(str));
}

Value findNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;
    if (!IS_STRING(args[0]))
        return NONE_VAL;
    char *str = AS_CSTRING(args[0]);
    char *found = findFile(str);
    if(found == NULL)
        return NONE_VAL;
    Value ret = STRING_VAL(found);
    free(found);
    return ret;
}

Value copyNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;
    return copyValue(args[0]);
}

Value evalNative(int argCount, Value *args)
{
    if (argCount == 0)
        return NONE_VAL;

    Value arg = args[0];
    if (!IS_STRING(arg))
    {
        printf("'eval' requires a string parameter\n");
        return NONE_VAL;
    }

    ObjString *str = AS_STRING(arg);
    ObjFunction *function = eval(str->chars);

    if (function == NULL)
    {
        printf("Failed to evaluate.\n");
        return NONE_VAL;
    }

    vm.ctf->eval = true;
    return OBJ_VAL(function);
}

Value memNative(int argCount, Value *args)
{
    bool human = false;
    if (argCount > 0)
    {
        human = AS_BOOL(toBool(args[0]));
    }

    if(!human)
    {
        return NUMBER_VAL(vm.bytesAllocated);
    }

    int sz = vm.bytesAllocated;
    char *comp = (char*)malloc(sizeof(char) * 4);
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

    char *str = (char*)malloc(sizeof(char) * ( strlen(comp) + strlen(vStr) + 1 ));
    strcpy(str, vStr);
    strcat(str, comp);

    Value ret = STRING_VAL(str);
    free(comp);
    free(vStr);
    free(str);

    return ret;
}

Value gcNative(int argCount, Value *args)
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

Value getSystemInfoNative(int argCount, Value *args)
{
    ObjDict *dict = initDict();

    Value os = STRING_VAL(PLATFORM_NAME);
    insertDict(dict, "os", os);

    return OBJ_VAL(dict);
}

Value printStackNative(int argCount, Value *args)
{
    printf("Task(%s)\n", vm.ctf->name);
    for (Value *slot = vm.ctf->stack; slot < vm.ctf->stackTop - 1; slot++)
    {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }

    vm.newLine = true;
    vm.print = true;
    return NONE_VAL;
}


// Register
linked_list *stdFnList;
#define ADD_STD(name, fn) linked_list_add(stdFnList, createStdFn(name, fn))

std_fn *createStdFn(const char *name, NativeFn fn)
{
    std_fn *stdFn = (std_fn *)malloc(sizeof(std_fn));
    stdFn->name = name;
    stdFn->fn = fn;
    return stdFn;
}

void initStd()
{
    stdFnList = linked_list_create();

    ADD_STD("clock", clockNative);
    ADD_STD("time", timeNative);
    ADD_STD("exit", exitNative);
    ADD_STD("input", inputNative);
    ADD_STD("print", printNative);
    ADD_STD("println", printlnNative);
    ADD_STD("random", randomNative);
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
    ADD_STD("len", lenNative);
    ADD_STD("type", typeNative);
    ADD_STD("env", envNative);
    ADD_STD("open", openNative);
    ADD_STD("cd", cdNative);
    ADD_STD("pwd", pwdNative);
    ADD_STD("ls", lsNative);
    ADD_STD("exists", existsNative);
    ADD_STD("find", findNative);
    ADD_STD("make", makeNative);
    ADD_STD("copy", copyNative);
    ADD_STD("eval", evalNative);
    ADD_STD("mem", memNative);
    ADD_STD("gc", gcNative);
    ADD_STD("enableAutoGC", autoGCNative);
    ADD_STD("getSystemInfo", getSystemInfoNative);
    ADD_STD("printStack", printStackNative);
}

void destroyStd()
{
    linked_list_destroy(stdFnList, true);
}