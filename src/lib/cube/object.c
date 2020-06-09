#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#endif

#include "collections.h"
#include "memory.h"
#include "mempool.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %ld for %d\n", object, size, type);
#endif

    return object;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method)
{
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name)
{
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->package = NULL;
    klass->super = NULL;
    initTable(&klass->methods);
    initTable(&klass->fields);
    initTable(&klass->staticFields);
    return klass;
}

ObjEnum *newEnum(ObjString *name)
{
    ObjEnum *enume = ALLOCATE_OBJ(ObjEnum, OBJ_ENUM);
    enume->name = name;
    enume->last = NULL_VAL;
    initTable(&enume->members);
    return enume;
}

ObjEnumValue *newEnumValue(ObjEnum *enume, ObjString *name, Value value)
{
    ObjEnumValue *enumValue = ALLOCATE_OBJ(ObjEnumValue, OBJ_ENUM_VALUE);
    enumValue->name = name;
    enumValue->value = value;
    enumValue->enume = enume;
    return enumValue;
}

ObjTask *newTask(ObjString *name)
{
    ObjTask *task = ALLOCATE_OBJ(ObjTask, OBJ_TASK);
    task->name = name;
    return task;
}

ObjPackage *newPackage(ObjString *name)
{
    ObjPackage *package = ALLOCATE_OBJ(ObjPackage, OBJ_PACKAGE);
    package->name = name;
    initTable(&package->symbols);
    package->parent = NULL;
    return package;
}

ObjClosure *newClosure(ObjFunction *function)
{
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++)
    {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    closure->package = NULL;
    return closure;
}

ObjFunction *newFunction(bool isStatic)
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    function->staticMethod = isStatic;
    function->path = NULL;
    function->doc = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance *newInstance(ObjClass *klass)
{
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    tableAddAll(&klass->fields, &instance->fields);
    return instance;
}

ObjNative *newNative(NativeFn function)
{
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjRequest *newRequest()
{
    ObjRequest *request = ALLOCATE_OBJ(ObjRequest, OBJ_REQUEST);
    request->pops = 0;
    request->fn = NULL_VAL;
    return request;
}

ObjProcess *defaultProcess()
{
    ObjProcess *process = ALLOCATE_OBJ(ObjProcess, OBJ_PROCESS);
    process->path = AS_STRING(STRING_VAL("std.io"));
    process->running = true;
    process->status = 0;
    process->closed = false;
    process->pid = 0;
#ifndef _WIN32
    process->in = STDIN_FILENO;
    process->out = STDOUT_FILENO;
#else
    process->in = _fileno(stdin);
    process->out = _fileno(stdout);
#endif
    process->protected = true;

    return process;
}

ObjProcess *newProcess(ObjString *path, int argCount, Value *args)
{
#ifndef _WIN32

    ObjProcess *process = ALLOCATE_OBJ(ObjProcess, OBJ_PROCESS);
    process->path = path;
    process->running = false;
    process->status = 0;
    process->closed = true;
    process->protected = false;
    int pipe1[2];
    int pipe2[2];
    int child;

    // Create two pipes
    pipe(pipe1);
    pipe(pipe2);

    if ((child = fork()) == 0)
    {
        // In child
        // // Close current `stdin` and `stdout` file handles
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        // // Duplicate pipes as new `stdin` and `stdout`
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);

        // // We don't need the other ends of the pipes, so close them
        close(pipe1[1]);
        close(pipe2[0]);

        // Construct args
        char **_args = (char **)malloc(sizeof(char *) * (argCount + 2));
        _args[0] = path->chars;
        int i = 1;
        for (; i <= argCount; i++)
        {
            ObjString *str = AS_STRING(toString(args[i - 1]));
            _args[i] = (char *)malloc(sizeof(char) * (str->length + 1));
            strcpy(_args[i], str->chars);
        }
        _args[i] = NULL;

        // Run the external program
        execvp(path->chars, _args);
    }
    else
    {
        if (child < 0)
            return NULL;

        // We don't need the read-end of the first pipe (the childs `stdin`)
        // or the write-end of the second pipe (the childs `stdout`)
        close(pipe1[0]);
        close(pipe2[1]);

        // Now you can write to `pipe1[1]` and it will end up as `stdin` in the child
        // Read from `pipe2[0]` to read from the childs `stdout`

        process->pid = child;
        process->in = pipe2[0];
        process->out = pipe1[1];
        process->running = true;
        process->closed = false;

        // fcntl(process->in,  F_SETFL, fcntl(process->in,  F_GETFL) | O_NONBLOCK);
        // fcntl(process->out, F_SETFL, fcntl(process->out, F_GETFL) | O_NONBLOCK);

        return process;
    }

#else
    return NULL;
#endif
}

bool processAlive(ObjProcess *process)
{
    if (process->protected)
        return true;
    if (!process->running)
        return false;

    bool alive = true;
#ifdef _WIN32

#else
    int status, rc;
    rc = waitpid(process->pid, &status, WNOHANG);
    if (rc != 0 && WIFEXITED(status))
    {
        process->status = WEXITSTATUS(status);
        alive = false;
    }
#endif

    process->running = alive;

    return alive;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NULL_VAL);
    pop();
    return string;
}

ObjList *initList()
{
    ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    initValueArray(&list->values);
    return list;
}

ObjBytes *initBytes()
{
    ObjBytes *bytes = ALLOCATE_OBJ(ObjBytes, OBJ_BYTES);
    bytes->length = 0;
    bytes->bytes = NULL;
    return bytes;
}

ObjDict *initDict()
{
    ObjDict *dict = ALLOCATE_OBJ(ObjDict, OBJ_DICT);
    dict->capacity = 8;
    dict->count = 0;
    dict->items = mp_calloc(dict->capacity, sizeof(*dict->items));

    return dict;
}

ObjNativeFunc *initNativeFunc()
{
    ObjNativeFunc *nativeFunc = ALLOCATE_OBJ(ObjNativeFunc, OBJ_NATIVE_FUNC);
    initValueArray(&nativeFunc->params);
    nativeFunc->name = NULL;
    nativeFunc->returnType = NULL;
    nativeFunc->lib = NULL;
    return nativeFunc;
}

ObjNativeStruct *initNativeStruct()
{
    ObjNativeStruct *nativeStruct = ALLOCATE_OBJ(ObjNativeStruct, OBJ_NATIVE_STRUCT);
    initValueArray(&nativeStruct->types);
    initValueArray(&nativeStruct->names);
    nativeStruct->name = NULL;
    nativeStruct->lib = NULL;
    return nativeStruct;
}

ObjNativeLib *initNativeLib()
{
    ObjNativeLib *nativeLib = ALLOCATE_OBJ(ObjNativeLib, OBJ_NATIVE_LIB);
    initValueArray(&nativeLib->objs);
    nativeLib->name = NULL;
    nativeLib->functions = 0;
    nativeLib->structs = 0;
    nativeLib->handle = NULL;
    return nativeLib;
}

ObjFile *initFile()
{
    ObjFile *file = ALLOCATE_OBJ(ObjFile, OBJ_FILE);
    file->isOpen = false;
    file->mode = 0;
    return file;
}

static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }

    return hash;
}

ObjString *takeString(char *chars, int length)
{
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
    {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length)
{
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length, hash);
}

ObjBytes *copyBytes(const void *bytes, int length)
{
    ObjBytes *obj = initBytes();
    obj->bytes = ALLOCATE(unsigned char, length);
    obj->length = length;
    memcpy(obj->bytes, bytes, length);
    return obj;
}

ObjBytes *transferBytes(void *bytes)
{
    ObjBytes *obj = initBytes();
    obj->bytes = bytes;
    obj->length = -1;
    return obj;
}

ObjUpvalue *newUpvalue(Value *slot)
{
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NULL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void printFunction(ObjFunction *function)
{
    if (function->name == NULL)
    {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

char *objectToString(Value value, bool literal)
{
    Obj *obj = AS_OBJ(value);
    ObjType tp = obj->type;
    // printf("TP: %d\n", tp);
    switch (OBJ_TYPE(value))
    {
        case OBJ_CLASS: {
            ObjClass *klass = AS_CLASS(value);
            char *classString = mp_malloc(sizeof(char) * (klass->name->length + 10));
            snprintf(classString, klass->name->length + 9, "<class %s>", klass->name->chars);
            return classString;
        }

        case OBJ_ENUM: {
            ObjEnum *enume = AS_ENUM(value);
            char *enumString = mp_malloc(sizeof(char) * (enume->name->length + 10));
            snprintf(enumString, enume->name->length + 9, "<enum %s>", enume->name->chars);
            return enumString;
        }

        case OBJ_ENUM_VALUE: {
            ObjEnumValue *enumValue = AS_ENUM_VALUE(value);
            int len = enumValue->enume->name->length + 10;
            len += enumValue->name->length;
            char *valueStr = valueToString(enumValue->value, false);
            len += strlen(valueStr);
            char *enumValueString = mp_malloc(sizeof(char) * len);
            snprintf(enumValueString, len - 1, "%s.%s<%s>", enumValue->enume->name->chars, enumValue->name->chars,
                     valueStr);
            mp_free(valueStr);
            return enumValueString;
        }

        case OBJ_TASK: {
            ObjClass *klass = AS_CLASS(value);
            char *classString = mp_malloc(sizeof(char) * (klass->name->length + 10));
            snprintf(classString, klass->name->length + 9, "<task %s>", klass->name->chars);
            return classString;
        }

        case OBJ_PACKAGE: {
            ObjPackage *package = AS_PACKAGE(value);
            char *packageString = mp_malloc(sizeof(char) * (package->name->length + 14));
            snprintf(packageString, package->name->length + 13, "<package %s>", package->name->chars);
            return packageString;
        }

        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *method = AS_BOUND_METHOD(value);
            char *methodString = mp_malloc(sizeof(char) * (method->method->function->name->length + 17));
            char *methodType = method->method->function->staticMethod ? "<static method %s>" : "<bound method %s>";
            snprintf(methodString, method->method->function->name->length + 17, methodType,
                     method->method->function->name->chars);
            return methodString;
        }

        case OBJ_CLOSURE: {
            ObjClosure *closure = AS_CLOSURE(value);

            if (closure->function && closure->function->name)
            {
                return objectToString(OBJ_VAL(closure->function), literal);
            }
            else
            {
                char *nativeString = mp_malloc(sizeof(char) * 30);
                snprintf(nativeString, 29, "<fn uninit%ju>", (uintmax_t)(uintptr_t)AS_CLOSURE(value));
                return nativeString;
            }
        }

        case OBJ_FUNCTION: {
            ObjFunction *function = AS_FUNCTION(value);
            char *nameStr = NULL;
            int nameLen = 0;
            if (function->name != NULL && function->name->chars != NULL)
            {
                nameStr = function->name->chars;
                nameLen = function->name->length;
            }
            else
            {
                nameStr = (char *)vm.scriptName;
                nameLen = strlen(nameStr);
            }

            char *functionString = mp_malloc(sizeof(char) * (nameLen + 8));
            snprintf(functionString, nameLen + 8, "<func %s>", nameStr);
            return functionString;
        }

        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(value);
            char *instanceString = mp_malloc(sizeof(char) * (instance->klass->name->length + 12));
            snprintf(instanceString, instance->klass->name->length + 12, "<%s instance>", instance->klass->name->chars);
            return instanceString;
        }

        // case OBJ_NATIVE_VOID:
        case OBJ_NATIVE: {
            char *nativeString = mp_malloc(sizeof(char) * 15);
            snprintf(nativeString, 14, "%s", "<native func>");
            return nativeString;
        }

        case OBJ_STRING: {
            ObjString *stringObj = AS_STRING(value);
            char *string = mp_malloc(sizeof(char) * stringObj->length + 4);
            if (literal)
                snprintf(string, stringObj->length + 3, "\"%s\"", stringObj->chars);
            else
                snprintf(string, stringObj->length + 3, "%s", stringObj->chars);
            return string;
        }

        case OBJ_FILE: {
            ObjFile *file = AS_FILE(value);
            char *fileString = mp_malloc(sizeof(char) * (strlen(file->path) + 10));
            snprintf(fileString, strlen(file->path) + 9, "<file %s%s>", file->path, file->isOpen ? "*" : "");
            return fileString;
        }

        case OBJ_PROCESS: {
            ObjProcess *process = AS_PROCESS(value);
            char *processString = mp_malloc(sizeof(char) * (process->path->length + 32));
            snprintf(processString, process->path->length + 31, "<process %s'%s'", process->running ? "*" : "",
                     process->path->chars);
            if (process->running)
                strcat(processString, ">");
            else
                sprintf(processString + strlen(processString), "[%d]>", process->status);

            return processString;
        }

        case OBJ_NATIVE_FUNC: {
            ObjNativeFunc *func = AS_NATIVE_FUNC(value);

            int len = 4 + func->name->length + func->returnType->length;
            for (int i = 0; i < func->params.count; i++)
                len += AS_STRING(func->params.values[i])->length + 2;
            len += 3;
            if (func->lib != NULL)
            {
                len += func->lib->name->length;
            }

            char *str = mp_malloc(sizeof(char) * len);
            sprintf(str, "%s %s(", func->returnType->chars, func->name->chars);
            for (int i = 0; i < func->params.count; i++)
            {
                if (i < func->params.count - 1)
                    sprintf(str + strlen(str), "%s, ", AS_STRING(func->params.values[i])->chars);
                else
                    sprintf(str + strlen(str), "%s", AS_STRING(func->params.values[i])->chars);
            }
            sprintf(str + strlen(str), ") [");
            if (func->lib != NULL)
            {
                sprintf(str + strlen(str), "%s", func->lib->name->chars);
            }
            sprintf(str + strlen(str), "]");
            return str;
        }

        case OBJ_NATIVE_STRUCT: {
            ObjNativeStruct *func = AS_NATIVE_STRUCT(value);

            int len = 8 + func->name->length;
            for (int i = 0; i < func->types.count; i++)
            {
                len += AS_STRING(func->types.values[i])->length + 2;
                len += AS_STRING(func->names.values[i])->length + 3;
            }
            len += 4;
            if (func->lib != NULL)
            {
                len += func->lib->name->length;
            }

            char *str = mp_malloc(sizeof(char) * len);
            sprintf(str, "struct %s { ", func->name->chars);
            for (int i = 0; i < func->types.count; i++)
            {
                if (i < func->types.count - 1)
                    sprintf(str + strlen(str), "%s %s, ", AS_STRING(func->types.values[i])->chars,
                            AS_STRING(func->names.values[i])->chars);
                else
                    sprintf(str + strlen(str), "%s %s", AS_STRING(func->types.values[i])->chars,
                            AS_STRING(func->names.values[i])->chars);
            }
            sprintf(str + strlen(str), " } [");
            if (func->lib != NULL)
            {
                sprintf(str + strlen(str), "%s", func->lib->name->chars);
            }
            sprintf(str + strlen(str), "]");
            return str;
        }

        case OBJ_NATIVE_LIB: {
            ObjNativeLib *lib = AS_NATIVE_LIB(value);

            int len = 18 + lib->name->length;

            char *str = mp_malloc(sizeof(char) * len);
            sprintf(str, "<native lib '%s'>", lib->name->chars);
            return str;
        }

        case OBJ_BYTES: {
            ObjBytes *bytes = AS_BYTES(value);
            int len = bytes->length;
            if (len < 0)
                len = 10;
            char *bytesString = mp_malloc(sizeof(unsigned char) * 8 * len);
            bytesString[0] = '\0';
            for (int i = 0; i < len; i++)
            {
                snprintf(bytesString + strlen(bytesString), 6, "0x%X ", (unsigned char)bytes->bytes[i]);
            }
            if (len != bytes->length)
                snprintf(bytesString + strlen(bytesString), 6, "... ");
            return bytesString;
        }

        case OBJ_LIST: {
            int size = 50;
            ObjList *list = AS_LIST(value);
            char *listString = mp_malloc(sizeof(char) * size);
            snprintf(listString, 2, "%s", "[");

            int listStringSize;

            for (int i = 0; i < list->values.count; ++i)
            {
                char *element = valueToString(list->values.values[i], true);

                int elementSize = strlen(element);
                listStringSize = strlen(listString);

            resize:
                if ((elementSize + 12) >= (size - listStringSize - 1))
                {
                    if (elementSize > size * 2)
                        size += elementSize * 2;
                    else
                        size *= 2;

                    char *newB = mp_realloc(listString, sizeof(char) * size);

                    if (newB == NULL)
                    {
                        printf("Unable to allocate memory\n");
                        exit(71);
                    }

                    listString = newB;
                    goto resize;
                }

                strncat(listString, element, size - listStringSize - 1);

                mp_free(element);

                if (i != list->values.count - 1)
                    strncat(listString, ", ", size - listStringSize - 1);
            }

            listStringSize = strlen(listString);
            strncat(listString, "]", size - listStringSize - 1);
            return listString;
        }

        case OBJ_DICT: {
            int count = 0;
            int size = 50;
            ObjDict *dict = AS_DICT(value);
            char *dictString = mp_malloc(sizeof(char) * size);
            snprintf(dictString, 2, "%s", "{");

            for (int i = 0; i < dict->capacity; ++i)
            {
                dictItem *item = dict->items[i];
                if (!item || item->deleted)
                    continue;

                count++;

                int keySize = strlen(item->key);
                int dictStringSize = strlen(dictString);

            resizeKey:
                if ((keySize + 6) >= (size - dictStringSize - 1))
                {
                    if (keySize > size * 2)
                        size += keySize * 2;
                    else
                        size *= 2;

                    char *newB = mp_realloc(dictString, sizeof(char) * size);

                    if (newB == NULL)
                    {
                        printf("Unable to allocate memory\n");
                        exit(71);
                    }

                    dictString = newB;
                    goto resizeKey;
                }

                char *dictKeyString = mp_malloc(sizeof(char) * (strlen(item->key) + 5));
                snprintf(dictKeyString, (strlen(item->key) + 5), "\"%s\": ", item->key);

                strncat(dictString, dictKeyString, size - dictStringSize - 1);
                mp_free(dictKeyString);

                dictStringSize = strlen(dictString);

                char *element = valueToString(item->item, true);
                int elementSize = strlen(element);

            resizeElement:
                if (elementSize + 8 >= (size - dictStringSize - 1))
                {
                    if (elementSize > size * 2)
                        size += elementSize * 2;
                    else
                        size *= 2;

                    char *newB = mp_realloc(dictString, sizeof(char) * size);

                    if (newB == NULL)
                    {
                        printf("Unable to allocate memory\n");
                        exit(71);
                    }

                    dictString = newB;
                    goto resizeElement;
                }

                strncat(dictString, element, size - dictStringSize - 1);

                mp_free(element);

                if (count != dict->count)
                    strncat(dictString, ", ", size - dictStringSize - 1);
            }

            strncat(dictString, "}", size - strlen(dictString) - 1);
            return dictString;
        }

        case OBJ_UPVALUE: {
            char *nativeString = mp_malloc(sizeof(char) * 8);
            snprintf(nativeString, 8, "%s", "upvalue");
            return nativeString;
        }
    }

    char *unknown = mp_malloc(sizeof(char) * 9);
    snprintf(unknown, 8, "%s", "unknown");
    return unknown;
}

char *objectType(Value value)
{
    Obj *obj = AS_OBJ(value);
    ObjType tp = obj->type;
    switch (OBJ_TYPE(value))
    {
        case OBJ_CLASS: {
            char *str = mp_malloc(sizeof(char) * 7);
            snprintf(str, 6, "class");
            return str;
        }

        case OBJ_ENUM: {
            char *str = mp_malloc(sizeof(char) * 7);
            snprintf(str, 6, "enum");
            return str;
        }

        case OBJ_ENUM_VALUE: {
            ObjEnumValue *enumValue = AS_ENUM_VALUE(value);
            char *str = mp_malloc(sizeof(char) * (8 + enumValue->enume->name->length));
            snprintf(str, (7 + enumValue->enume->name->length), "enum<%s>", enumValue->enume->name->chars);
            return str;
        }

        case OBJ_TASK: {
            char *str = mp_malloc(sizeof(char) * 7);
            snprintf(str, 6, "task");
            return str;
        }

        case OBJ_PROCESS: {
            char *str = mp_malloc(sizeof(char) * 10);
            snprintf(str, 9, "process");
            return str;
        }

        case OBJ_PACKAGE: {
            char *str = mp_malloc(sizeof(char) * 9);
            snprintf(str, 8, "package");
            return str;
        }

        case OBJ_BOUND_METHOD: {
            char *str = mp_malloc(sizeof(char) * 8);
            snprintf(str, 7, "method");
            return str;
        }

        case OBJ_CLOSURE: {
            char *str = mp_malloc(sizeof(char) * 6);
            snprintf(str, 5, "func");
            return str;
        }

        case OBJ_FUNCTION: {
            char *str = mp_malloc(sizeof(char) * 6);
            snprintf(str, 5, "func");
            return str;
        }

        case OBJ_INSTANCE: {
            char *str = mp_malloc(sizeof(char) * 10);
            snprintf(str, 9, "instance");
            return str;
        }

        // case OBJ_NATIVE_VOID:
        case OBJ_NATIVE: {
            char *str = mp_malloc(sizeof(char) * 8);
            snprintf(str, 7, "native");
            return str;
        }

        case OBJ_STRING: {
            char *str = mp_malloc(sizeof(char) * 8);
            snprintf(str, 7, "str");
            return str;
        }

        case OBJ_FILE: {
            char *str = mp_malloc(sizeof(char) * 6);
            snprintf(str, 5, "file");
            return str;
        }

        case OBJ_BYTES: {
            char *str = mp_malloc(sizeof(char) * 7);
            snprintf(str, 6, "bytes");
            return str;
        }

        case OBJ_LIST: {
            char *str = mp_malloc(sizeof(char) * 6);
            snprintf(str, 5, "list");
            return str;
        }

        case OBJ_DICT: {
            char *str = mp_malloc(sizeof(char) * 6);
            snprintf(str, 5, "dict");
            return str;
        }

        case OBJ_UPVALUE: {
            char *str = mp_malloc(sizeof(char) * 9);
            snprintf(str, 8, "upvalue");
            return str;
        }

        case OBJ_NATIVE_FUNC: {
            char *str = mp_malloc(sizeof(char) * 12);
            sprintf(str, "nativefunc");
            return str;
        }

        case OBJ_NATIVE_STRUCT: {
            char *str = mp_malloc(sizeof(char) * 16);
            sprintf(str, "nativestruct");
            return str;
        }

        case OBJ_NATIVE_LIB: {
            char *str = mp_malloc(sizeof(char) * 12);
            sprintf(str, "nativelib");
            return str;
        }
    }

    char *unknown = mp_malloc(sizeof(char) * 9);
    snprintf(unknown, 8, "%s", "unknown");
    return unknown;
}

bool dictComparison(Value a, Value b)
{
    ObjDict *dict = AS_DICT(a);
    ObjDict *dictB = AS_DICT(b);

    // Different lengths, not the same
    if (dict->capacity != dictB->capacity)
        return false;

    // Lengths are the same, and dict 1 has 0 length
    // therefore both are empty
    if (dict->count == 0)
        return true;

    for (int i = 0; i < dict->capacity; ++i)
    {
        dictItem *item = dict->items[i];
        dictItem *itemB = dictB->items[i];

        if (!item || !itemB)
        {
            continue;
        }

        if (strcmp(item->key, itemB->key) != 0)
            return false;

        if (!valuesEqual(item->item, itemB->item))
            return false;
    }

    return true;
}

bool listComparison(Value a, Value b)
{
    ObjList *list = AS_LIST(a);
    ObjList *listB = AS_LIST(b);

    if (list->values.count != listB->values.count)
        return false;

    for (int i = 0; i < list->values.count; ++i)
    {
        if (!valuesEqual(list->values.values[i], listB->values.values[i]))
            return false;
    }

    return true;
}

static bool bytesComparison(Value a, Value b)
{
    ObjBytes *A = AS_BYTES(a);
    ObjBytes *B = AS_BYTES(b);
    if (A->length != B->length)
        return false;
    return memcmp(A->bytes, B->bytes, A->length) == 0;
}

bool objectComparison(Value a, Value b)
{
    if (!IS_OBJ(a) || !IS_OBJ(b))
    {
        return false;
    }
    else if (AS_OBJ(a) == NULL || AS_OBJ(b) == NULL)
    {
        return false;
    }
    else if (IS_STRING(a) && IS_STRING(b))
    {
        return strcmp(AS_CSTRING(a), AS_CSTRING(b)) == 0;
    }
    else if (IS_DICT(a) && IS_DICT(b))
    {
        return dictComparison(a, b);
    }
    else if (IS_LIST(a) && IS_LIST(b))
    {
        return listComparison(a, b);
    }
    else if (IS_BYTES(a) && IS_BYTES(b))
    {
        return bytesComparison(a, b);
    }
    else
    {
#ifdef NAN_TAGGING
        return a == b;
#else
        return a.as.number == b.as.number;
#endif
    }
    return false;
}

Value copyObject(Value value)
{
    if (IS_LIST(value))
    {
        ObjList *newList = copyList(AS_LIST(value), true);
        return OBJ_VAL(newList);
    }
    else if (IS_DICT(value))
    {
        ObjDict *newDict = copyDict(AS_DICT(value), true);
        return OBJ_VAL(newDict);
    }
    else if (IS_DICT(value))
    {
        ObjDict *newDict = copyDict(AS_DICT(value), true);
        return OBJ_VAL(newDict);
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *oldBytes = AS_BYTES(value);
        ObjBytes *newBytes = NULL;
        if (oldBytes->length >= 0)
            newBytes = copyBytes(oldBytes->bytes, oldBytes->length);
        else
            newBytes = transferBytes(oldBytes->bytes);
        return OBJ_VAL(newBytes);
    }
    return value;
}

ObjBytes *objectToBytes(Value value)
{
    ObjBytes *bytes = NULL;
    if (IS_STRING(value))
    {
        ObjString *str = AS_STRING(value);
        bytes = copyBytes(str->chars, str->length);
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *old = AS_BYTES(value);
        if (old->length >= 0)
            bytes = copyBytes(old->bytes, old->length);
        else
            bytes = transferBytes(old->bytes);
    }
    else if (IS_LIST(value))
    {
        bytes = initBytes();
        ObjList *list = AS_LIST(value);
        for (int i = 0; i < list->values.count; i++)
        {
            ObjBytes *partial = AS_BYTES(toBytes(list->values.values[i]));
            appendBytes(bytes, partial);
        }
    }
    return bytes;
}

void appendBytes(ObjBytes *dest, ObjBytes *src)
{
    if (dest->length < 0 || src->length < 0)
        return;
    dest->bytes = GROW_ARRAY(dest->bytes, unsigned char, dest->length, dest->length + src->length);
    memcpy(dest->bytes + dest->length, src->bytes, src->length);
    dest->length += src->length;
}