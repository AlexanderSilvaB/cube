#include <stdlib.h>                                               

#include "common.h"                                               
#include "memory.h"
#include "vm.h"

void* reallocate(void* previous, size_t oldSize, size_t newSize) 
{
    if (newSize == 0) 
    {                                             
        free(previous);                                               
        return NULL;                                                  
    }

    return realloc(previous, newSize);                              
}

static void freeObject(Obj* object)
{
    switch (object->type)
    {
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue, closure->upvalues, closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }

        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }

        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;

        case OBJ_STRING:
        {
            ObjString* string = (ObjString*)object;
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

		case OBJ_DICT: {
			ObjDict *dict = (ObjDict *)object;
		    freeDict(dict);
			break;
		}

        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

void freeObjects()
{
    Obj* object = vm.objects;
    while (object != NULL)
    {
        Obj* next = object->next;
        freeObject(object);      
        object = next;
    }
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