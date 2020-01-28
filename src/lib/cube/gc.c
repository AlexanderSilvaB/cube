#include "gc.h"
#include "vm.h"
#include "compiler.h"
#include "table.h"
#include "memory.h"

void mark_roots();
void mark();
void mark_object(Obj* obj);
void mark_array(ValueArray *array);
void sweep();

void gc_maybe_collect()
{
    if (vm.bytesAllocated > vm.nextGC)
    {
        gc_collect();
    }
}

void gc_collect()
{
    #ifdef GC_DISABLED
    return;
    #endif
    if(!vm.gc)
        return;

    #ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
    #endif

    mark();
    sweep();

    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

    #ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %ld bytes (from %ld to %ld) next at %ld\n",
        before - vm.bytesAllocated, before, vm.bytesAllocated,
        vm.nextGC);
    #endif
}

void mark()
{
    mark_roots();
}

void sweep()
{   
    tableRemoveWhite(&vm.strings);

    Obj *previous = NULL;
    Obj *object = vm.objects;
    while (object != NULL)
    {
        if (object->isMarked)
        {
            object->isMarked = false;
            previous = object;
            object = object->next;
        }
        else
        {
            Obj *unreached = object;

            object = object->next;
            if (previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void mark_task_frame(TaskFrame *tf)
{
    // Mark stack
    for(Value *v = tf->stack; v < tf->stackTop; v++)
    {
        mark_value(*v);
    }

    // Mark frames
    for (int i = 0; i < tf->frameCount; i++)
    {
        mark_object((Obj *)tf->frames[i].closure);
    }

    // Mark open upvalues
    for (ObjUpvalue *upvalue = tf->openUpvalues; upvalue != NULL; upvalue = upvalue->next)
    {
        mark_object((Obj *)upvalue);
    }

    mark_value(tf->currentArgs);
    mark_value(tf->result);
}

void mark_roots()
{
    // for(int i = 0; i < MAX_THREADS; i++)
    // {
    //     TaskFrame *tf = vm.threadFrames[i].taskFrame;
    //     while(tf != NULL)
    //     {
    //         mark_task_frame(tf);
    //         tf = tf->next;
    //     }
    // }


    TaskFrame *tf = vm.taskFrame;
    while(tf != NULL)
    {
        mark_task_frame(tf);
        tf = tf->next;
    }

    markTable(&vm.globals);
    markTable(&vm.extensions);
    markCompilerRoots();
    mark_object((Obj *)vm.initString);
    mark_object((Obj *)vm.paths);
    mark_value(vm.repl);
}


void mark_value(Value val)
{
    if(!IS_OBJ(val))
        return;
    mark_object(AS_OBJ(val));
}

void mark_object(Obj *object)
{
    if(object == NULL)
        return;

    if(object->isMarked)
        return;

    #ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)object);
    printValue(OBJ_VAL(object));
    printf("\n");
    #endif

    object->isMarked = true;

    switch (object->type)
    {
        case OBJ_BOUND_METHOD:
        {
            ObjBoundMethod *bound = (ObjBoundMethod *)object;
            mark_value(bound->receiver);
            mark_object((Obj *)bound->method);
            break;
        }

        case OBJ_CLASS:
        {
            ObjClass *klass = (ObjClass *)object;
            mark_object((Obj *)klass->name);
            markTable(&klass->methods);
            markTable(&klass->fields);
            markTable(&klass->staticFields);

            break;
        }

        case OBJ_PACKAGE:
        {
            ObjPackage *package = (ObjPackage *)object;
            mark_object((Obj *)package->name);
            markTable(&package->symbols);
            break;
        }

        case OBJ_CLOSURE:
        {
            ObjClosure *closure = (ObjClosure *)object;
            mark_object((Obj *)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++)
            {
                mark_object((Obj *)closure->upvalues[i]);
            }
            break;
        }

        case OBJ_FUNCTION:
        {
            ObjFunction *function = (ObjFunction *)object;
            mark_object((Obj *)function->name);
            mark_array(&function->chunk.constants);
            break;
        }

        case OBJ_REQUEST:
        {
            ObjRequest *request = (ObjRequest *)object;
            mark_value(request->fn);
            break;
        }

        case OBJ_INSTANCE:
        {
            ObjInstance *instance = (ObjInstance *)object;
            mark_object((Obj *)instance->klass);
            markTable(&instance->fields);
            break;
        }

        case OBJ_LIST:
        {
            ObjList *list = (ObjList *)object;
            mark_array(&list->values);
            break;
        }

        case OBJ_DICT:
        {
            ObjDict *dict = (ObjDict *)object;
            for (int i = 0; i < dict->capacity; ++i)
            {
                if (!dict->items[i])
                    continue;

                mark_value(dict->items[i]->item);
            }
            break;
        }

        case OBJ_NATIVE_FUNC:
        {
            ObjNativeFunc *func = (ObjNativeFunc *)object;
            mark_object((Obj *)func->name);
            mark_object((Obj *)func->returnType);
            mark_object((Obj *)func->lib);
            mark_array(&func->params);
            break;
        }

        case OBJ_NATIVE_LIB:
        {
            ObjNativeLib *lib = (ObjNativeLib *)object;
            mark_object((Obj *)lib->name);
            break;
        }

        case OBJ_UPVALUE:
            mark_value(((ObjUpvalue *)object)->closed);
            break;

        default:
            break;
    }
}

void mark_array(ValueArray *array)
{
    for (int i = 0; i < array->count; i++)
    {
        mark_value(array->values[i]);
    }
}
