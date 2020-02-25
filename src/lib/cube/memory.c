#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "vm.h"
#include "native.h"
#include "gc.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void *reallocate(void *previous, size_t oldSize, size_t newSize)
{
  vm.bytesAllocated += newSize - oldSize;

  if (vm.autoGC && newSize > oldSize)
  {
    #ifdef DEBUG_STRESS_GC
    gc_collect();
    #else
    gc_maybe_collect();
    #endif
  }

  if (newSize == 0)
  {
    free(previous);
    return NULL;
  }

  return realloc(previous, newSize);
}

void freeObject(Obj *object)
{
  #ifdef DEBUG_LOG_GC
    printf("%p free ", object);
    printValue(OBJ_VAL(object));
    printf(" (%d)", object->type);
    printf("\n");
  #endif

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

  case OBJ_REQUEST:
  {
    ObjRequest *request = (ObjRequest *)object;
    FREE(ObjRequest, request);
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
    string->chars = NULL;
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

  case OBJ_TASK:
    FREE(ObjTask, object);
    break;

  case OBJ_UPVALUE:
    FREE(ObjUpvalue, object);
    break;
  }
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
  if(func->lib != NULL && func->lib->functions == 0)
  {
    FREE(ObjNativeLib, func->lib);
  }
}

void freeNativeLib(ObjNativeLib *lib)
{
  closeNativeLib(lib);
}
