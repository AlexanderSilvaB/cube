#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "vm.h"
#include "native.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *previous, size_t oldSize, size_t newSize)
{
  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize)
  {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

    if (vm.bytesAllocated > vm.nextGC)
    {
      collectGarbage();
    }
  }

  if (newSize == 0)
  {
    free(previous);
    return NULL;
  }

  return realloc(previous, newSize);
}

void markObject(Obj *object)
{
  if (object == NULL)
    return;
  if (object->isMarked)
    return;

  // #ifdef DEBUG_LOG_GC
  //   printf("%p mark ", (void *)object);
  //   printValue(OBJ_VAL(object));
  //   printf("\n");
  // #endif

  object->isMarked = true;

  if (vm.grayCapacity < vm.grayCount + 1)
  {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = realloc(vm.grayStack, sizeof(Obj *) * vm.grayCapacity);
  }

  vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value)
{
  if (!IS_OBJ(value))
    return;
  markObject(AS_OBJ(value));
}

static void markArray(ValueArray *array)
{
  for (int i = 0; i < array->count; i++)
  {
    markValue(array->values[i]);
  }
}

static void blackenObject(Obj *object)
{
  // #ifdef DEBUG_LOG_GC
  //   printf("%p blacken ", object);
  //   printValue(OBJ_VAL(object));
  //   printf("\n");
  // #endif

  switch (object->type)
  {
  case OBJ_BOUND_METHOD:
  {
    ObjBoundMethod *bound = (ObjBoundMethod *)object;
    markValue(bound->receiver);
    markObject((Obj *)bound->method);
    break;
  }

  case OBJ_CLASS:
  {
    ObjClass *klass = (ObjClass *)object;
    markObject((Obj *)klass->name);
    markTable(&klass->methods);
    markTable(&klass->fields);
    markTable(&klass->staticFields);

    break;
  }

  case OBJ_NAMESPACE:
  {
    ObjNamespace *namespace = (ObjNamespace *)object;
    markObject((Obj *)namespace->name);
    markTable(&namespace->methods);
    markTable(&namespace->fields);
    break;
  }

  case OBJ_CLOSURE:
  {
    ObjClosure *closure = (ObjClosure *)object;
    markObject((Obj *)closure->function);
    for (int i = 0; i < closure->upvalueCount; i++)
    {
      markObject((Obj *)closure->upvalues[i]);
    }
    break;
  }

  case OBJ_FUNCTION:
  {
    ObjFunction *function = (ObjFunction *)object;
    markObject((Obj *)function->name);
    markArray(&function->chunk.constants);
    break;
  }

  case OBJ_INSTANCE:
  {
    ObjInstance *instance = (ObjInstance *)object;
    markObject((Obj *)instance->klass);
    markTable(&instance->fields);
    break;
  }

  case OBJ_UPVALUE:
    markValue(((ObjUpvalue *)object)->closed);
    break;

  case OBJ_LIST:
  {
    ObjList *list = (ObjList *)object;
    markArray(&list->values);
    break;
  }

  case OBJ_DICT:
  {
    ObjDict *dict = (ObjDict *)object;
    for (int i = 0; i < dict->capacity; ++i)
    {
      if (!dict->items[i])
        continue;

      markValue(dict->items[i]->item);
    }
    break;
  }

  case OBJ_NATIVE_FUNC:
  {
    ObjNativeFunc *func = (ObjNativeFunc *)object;
    markObject((Obj *)func->name);
    markObject((Obj *)func->returnType);
    markObject((Obj *)func->lib);
    markArray(&func->params);
    break;
  }

  case OBJ_NATIVE_LIB:
  {
    ObjNativeLib *lib = (ObjNativeLib *)object;
    markObject((Obj *)lib->name);
    break;
  }

  case OBJ_NATIVE:
  case OBJ_STRING:
  case OBJ_FILE:
  case OBJ_BYTES:
    break;
  }
}

static void markRoots()
{
  for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
  {
    markValue(*slot);
  }

  for (int i = 0; i < vm.frameCount; i++)
  {
    markObject((Obj *)vm.frames[i].closure);
  }

  for (ObjUpvalue *upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next)
  {
    markObject((Obj *)upvalue);
  }

  markTable(&vm.globals);
  markCompilerRoots();
  markObject((Obj *)vm.initString);
  markObject((Obj *)vm.paths);
}

static void traceReferences()
{
  while (vm.grayCount > 0)
  {
    Obj *object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}

static void freeObject(Obj *object)
{
  // #ifdef DEBUG_LOG_GC
  //   printf("%p free ", object);
  //   printValue(OBJ_VAL(object));
  //   printf(" (%d)", object->type);
  //   printf("\n");
  // #endif

  switch (object->type)
  {
  case OBJ_BOUND_METHOD:
    FREE(ObjBoundMethod, object);
    break;

  case OBJ_CLASS:
  {
    ObjClass *klass = (ObjClass *)object;
    freeTable(&klass->methods);
    freeTable(&klass->fields);
    freeTable(&klass->staticFields);
    FREE(ObjClass, object);
    break;
  }

  case OBJ_NAMESPACE:
  {
    ObjNamespace *namespace = (ObjNamespace *)object;
    freeTable(&namespace->methods);
    freeTable(&namespace->fields);
    FREE(ObjNamespace, object);
    break;
  }

  case OBJ_CLOSURE:
  {
    ObjClosure *closure = (ObjClosure *)object;
    FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
    FREE(ObjClosure, object);
    break;
  }

  case OBJ_FUNCTION:
  {
    ObjFunction *function = (ObjFunction *)object;
    freeChunk(&function->chunk);
    FREE(ObjFunction, object);
    break;
  }

  case OBJ_INSTANCE:
  {
    ObjInstance *instance = (ObjInstance *)object;
    freeTable(&instance->fields);
    FREE(ObjInstance, object);
    break;
  }

  case OBJ_NATIVE:
    FREE(ObjNative, object);
    break;

  case OBJ_STRING:
  {
    ObjString *string = (ObjString *)object;
    FREE_ARRAY(char, string->chars, string->length + 1);
    FREE(ObjString, object);
    break;
  }

  case OBJ_LIST:
  {
    ObjList *list = (ObjList *)object;
    freeValueArray(&list->values);
    FREE(ObjList, list);
    break;
  }

  case OBJ_NATIVE_FUNC:
  {
    ObjNativeFunc *func = (ObjNativeFunc *)object;
    freeValueArray(&func->params);
    freeNativeFunc(func);
    FREE(ObjNativeFunc, func);
    break;
  }

  case OBJ_NATIVE_LIB:
  {
    ObjNativeLib *lib = (ObjNativeLib *)object;
    freeNativeLib(lib);
    FREE(ObjNativeLib, lib);
    break;
  }

  case OBJ_DICT:
  {
    ObjDict *dict = (ObjDict *)object;
    freeDict(dict);
    break;
  }

  case OBJ_FILE:
  {
    ObjFile *file = (ObjFile *)object;
    freeFile(file);
    FREE(ObjFile, object);
    break;
  }

  case OBJ_BYTES:
  {
    ObjBytes *bytes = (ObjBytes *)object;
    FREE_ARRAY(char, bytes->bytes, bytes->length);
    FREE(ObjBytes, object);
    break;
  }

  case OBJ_UPVALUE:
    FREE(ObjUpvalue, object);
    break;
  }
}

static void sweep()
{
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

void collectGarbage()
{
  if (!vm.gc)
    return;
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytesAllocated;
#endif

  markRoots();
  traceReferences();
  tableRemoveWhite(&vm.strings);
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %ld bytes (from %ld to %ld) next at %ld\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
#endif
}

void freeObjects()
{
  Obj *object = vm.objects;
  while (object != NULL)
  {
    Obj *next = object->next;
    freeObject(object);
    object = next;
  }

  free(vm.grayStack);
}

void freeList(Obj *object)
{
  if (object->type == OBJ_LIST)
  {
    ObjList *list = (ObjList *)object;
    freeValueArray(&list->values);
    FREE(ObjList, list);
  }
  else
  {
    ObjDict *dict = (ObjDict *)object;
    freeDict(dict);
  }
}

void freeLists()
{
  Obj *object = vm.listObjects;
  while (object != NULL)
  {
    Obj *next = object->next;
    freeList(object);
    object = next;
  }
}

void freeDictValue(dictItem *dictItem)
{
  free(dictItem->key);
  free(dictItem);
}

void freeDict(ObjDict *dict)
{
  for (int i = 0; i < dict->capacity; i++)
  {
    dictItem *item = dict->items[i];
    if (item != NULL)
    {
      freeDictValue(item);
    }
  }
  free(dict->items);
  free(dict);
}

void freeFile(ObjFile *file)
{
  if (file->isOpen)
  {
    fclose(file->file);
    file->isOpen = false;
  }
}

void freeNativeFunc(ObjNativeFunc *func)
{
  func->lib->functions--;
}

void freeNativeLib(ObjNativeLib *lib)
{
  closeNativeLib(lib);
}
