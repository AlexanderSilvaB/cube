#include "tasks.h"
#include "memory.h"
#include "mempool.h"
#include "vm.h"

extern void destroyTaskFrame(const char *name);
extern TaskFrame *findTaskFrame(const char *name);

static bool getTaskName(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("name() takes 1 arguments (%d given)", argCount);
        return false;
    }

    ObjTask *task = AS_TASK(pop());
    push(OBJ_VAL(task->name));
    return true;
}

static bool stopTask(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("stop() takes 1 arguments (%d given)", argCount);
        return false;
    }

    ObjTask *task = AS_TASK(pop());
    destroyTaskFrame(task->name->chars);
    push(TRUE_VAL);
    return true;
}

static bool doneTask(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("done() takes 1 arguments (%d given)", argCount);
        return false;
    }

    ObjTask *task = AS_TASK(pop());
    TaskFrame *tf = findTaskFrame(task->name->chars);

    if (tf == NULL)
        push(TRUE_VAL);
    else
        push(BOOL_VAL(tf->finished));
    return true;
}

static bool resultTask(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("result() takes 1 arguments (%d given)", argCount);
        return false;
    }

    ObjTask *task = AS_TASK(pop());
    TaskFrame *tf = findTaskFrame(task->name->chars);

    if (tf == NULL)
        push(NULL_VAL);
    else
    {
        if (tf->finished)
            push(tf->result);
        else
            push(NULL_VAL);
    }
    return true;
}

bool taskMethods(char *method, int argCount)
{
    if (strcmp(method, "name") == 0)
        return getTaskName(argCount);
    else if (strcmp(method, "stop") == 0)
        return stopTask(argCount);
    else if (strcmp(method, "done") == 0)
        return doneTask(argCount);
    else if (strcmp(method, "result") == 0)
        return resultTask(argCount);

    runtimeError("Task has no method %s()", method);
    return false;
}
