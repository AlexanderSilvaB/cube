#include "collections.h"
#include "memory.h"
#include "mempool.h"
#include "vm.h"

// This is needed for list deepCopy
ObjDict *copyDict(ObjDict *oldDict, bool shallow);

void addList(ObjList *list, Value value)
{
    writeValueArray(&list->values, value);
}

void rmList(ObjList *list, Value search)
{
    int skip = 0;

    for (int i = 0; i < list->values.capacity; ++i)
    {
#ifndef NAN_TAGGING
        if (valuesEqual(list->values.values[i], search))
        {
            skip++;
        }
#else
        if (!list->values.values[i])
            continue;

        if (list->values.values[i] == search)
        {
            skip++;
        }
#endif
        if (skip > 0)
        {
            if (i + skip >= list->values.capacity)
                break;

            list->values.values[i] = list->values.values[i + skip];
        }
    }

    list->values.count -= skip;
    if (list->values.count < 0)
        list->values.count = 0;
}

void stretchList(ObjList *list1, ObjList *list2)
{
    for (int i = 0; i < list2->values.count; ++i)
    {
        Value val = list2->values.values[i];

        if (IS_DICT(val))
            val = OBJ_VAL(copyDict(AS_DICT(val), false));
        else if (IS_LIST(val))
            val = OBJ_VAL(copyList(AS_LIST(val), false));

        writeValueArray(&list1->values, val);
    }
}

void shrinkList(ObjList *list, int num)
{
    list->values.count -= num;
    if (list->values.count < 0)
        list->values.count = 0;
}

static bool pushListItem(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("push() takes 2 arguments (%d given)", argCount);
        return false;
    }

    Value listItem = pop();

    ObjList *list = AS_LIST(pop());
    writeValueArray(&list->values, listItem);
    push(NULL_VAL);

    return true;
}

static bool insertListItem(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("insert() takes 3 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_NUMBER(peek(0)))
    {
        runtimeError("insert() third argument must be a number");
        return false;
    }

    int index = AS_NUMBER(pop());
    Value insertValue = pop();
    ObjList *list = AS_LIST(pop());

    if (index < 0 || index > list->values.count)
    {
        runtimeError("Index passed to insert() is out of bounds for the list given");
        return false;
    }

    if (list->values.capacity < list->values.count + 1)
    {
        int oldCapacity = list->values.capacity;
        list->values.capacity = GROW_CAPACITY(oldCapacity);
        list->values.values = GROW_ARRAY(list->values.values, Value, oldCapacity, list->values.capacity);
    }

    list->values.count++;

    for (int i = list->values.count - 1; i > index; --i)
        list->values.values[i] = list->values.values[i - 1];

    list->values.values[index] = insertValue;
    push(NULL_VAL);

    return true;
}

static bool popListItem(int argCount)
{
    if (argCount < 1 || argCount > 2)
    {
        runtimeError("pop() takes either 1 or 2 arguments (%d  given)", argCount);
        return false;
    }

    ObjList *list;
    Value last;

    if (argCount == 1)
    {
        if (!IS_LIST(peek(0)))
        {
            runtimeError("pop() only takes a list as an argument");
            return false;
        }

        list = AS_LIST(pop());

        if (list->values.count == 0)
        {
            runtimeError("pop() called on an empty list");
            return false;
        }

        last = list->values.values[list->values.count - 1];
    }
    else
    {
        if (!IS_LIST(peek(1)))
        {
            runtimeError("pop() only takes a list as an argument");
            return false;
        }

        if (!IS_NUMBER(peek(0)))
        {
            runtimeError("pop() index argument must be a number");
            return false;
        }

        int index = AS_NUMBER(pop());
        list = AS_LIST(pop());

        if (list->values.count == 0)
        {
            runtimeError("pop() called on an empty list");
            return false;
        }

        if (index < 0 || index > list->values.count)
        {
            runtimeError("Index passed to pop() is out of bounds for the list given");
            return false;
        }

        last = list->values.values[index];

        for (int i = index; i < list->values.count - 1; ++i)
            list->values.values[i] = list->values.values[i + 1];
    }
    list->values.count--;
    push(last);

    return true;
}

static bool removeListItem(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("remove() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    Value search = pop();
    ObjList *list = AS_LIST(pop());

    int skip = 0;

    for (int i = 0; i < list->values.capacity; ++i)
    {
#ifndef NAN_TAGGING
        if (valuesEqual(list->values.values[i], search))
        {
            skip++;
        }
#else
        if (!list->values.values[i])
            continue;

        if (list->values.values[i] == search)
        {
            skip++;
        }
#endif
        if (skip > 0)
        {
            if (i + skip >= list->values.capacity)
                break;

            list->values.values[i] = list->values.values[i + skip];
        }
    }

    list->values.count -= skip;
    if (list->values.count < 0)
        list->values.count = 0;

    push((skip > 0 ? TRUE_VAL : FALSE_VAL));
    return true;
}

static bool containsListItem(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("contains() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    Value search = pop();
    ObjList *list = AS_LIST(pop());

    for (int i = 0; i < list->values.capacity; ++i)
    {
#ifndef NAN_TAGGING
        if (valuesEqual(list->values.values[i], search))
        {
            push(TRUE_VAL);
            return true;
        }
#else
        if (!list->values.values[i])
            continue;

        if (list->values.values[i] == search)
        {
            push(TRUE_VAL);
            return true;
        }
#endif
    }

    push(FALSE_VAL);
    return true;
}

static bool indexList(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("contains() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    Value search = pop();
    ObjList *list = AS_LIST(pop());

    int i = 0;
    for (i = 0; i < list->values.capacity; ++i)
    {
#ifndef NAN_TAGGING
        if (valuesEqual(list->values.values[i], search))
        {
            break;
        }
#else
        if (!list->values.values[i])
            continue;

        if (list->values.values[i] == search)
        {
            break;
        }
#endif
    }

    if (i >= list->values.capacity)
        i = -1;

    push(NUMBER_VAL(i));
    return true;
}

bool listContains(Value listV, Value search, Value *result)
{

    ObjList *list = AS_LIST(listV);

    for (int i = 0; i < list->values.capacity; ++i)
    {
#ifndef NAN_TAGGING
        if (valuesEqual(list->values.values[i], search))
        {
            *result = TRUE_VAL;
            return true;
        }
#else
        if (!list->values.values[i])
            continue;

        if (list->values.values[i] == search)
        {
            *result = TRUE_VAL;
            return true;
        }
#endif
    }

    *result = FALSE_VAL;
    return true;
}

static bool swapListItems(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("swap() takes 3 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))
    {
        runtimeError("swap() arguments must be numbers");
        return false;
    }

    int i = AS_NUMBER(pop());
    int j = AS_NUMBER(pop());

    ObjList *list = AS_LIST(pop());

    if (i < 0 || i > list->values.count || j < 0 || j > list->values.count)
    {
        runtimeError("Index passed to swap() is out of bounds for the list given");
        return false;
    }

    Value tmp = list->values.values[i];
    list->values.values[i] = list->values.values[j];
    list->values.values[j] = tmp;
    push(OBJ_VAL(list));
    return true;
}

ObjList *copyList(ObjList *oldList, bool shallow)
{
    DISABLE_GC;
    ObjList *newList = initList();

    for (int i = 0; i < oldList->values.count; ++i)
    {
        Value val = oldList->values.values[i];

        if (!shallow)
        {
            if (IS_DICT(val))
                val = OBJ_VAL(copyDict(AS_DICT(val), false));
            else if (IS_LIST(val))
                val = OBJ_VAL(copyList(AS_LIST(val), false));
        }

        writeValueArray(&newList->values, val);
    }
    RESTORE_GC;
    return newList;
}

static bool swapAllListItems(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("swapAll() takes 3 arguments (%d given)", argCount);
        return false;
    }

    if (!IS_LIST(peek(0)))
    {
        runtimeError("swapAll() arguments must be another list");
        return false;
    }

    ObjList *list1 = AS_LIST(pop());
    ObjList *list2 = AS_LIST(pop());

    ObjList *copy1 = copyList(list1, false);
    ObjList *copy2 = copyList(list2, false);

    list1->values.count = 0;
    list2->values.count = 0;

    int n = 0;
    for (int i = 0; i < copy1->values.count; i++, n++)
        writeValueArray(&list2->values, copy1->values.values[i]);

    for (int i = 0; i < copy2->values.count; i++, n--)
        writeValueArray(&list1->values, copy2->values.values[i]);

    push(NUMBER_VAL(n));
    return true;
}

static bool copyListShallow(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("copy() takes 1 argument (%d  given)", argCount);
        return false;
    }

    ObjList *oldList = AS_LIST(pop());
    push(OBJ_VAL(copyList(oldList, true)));

    return true;
}

static bool copyListDeep(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("deepCopy() takes 1 argument (%d  given)", argCount);
        return false;
    }

    ObjList *oldList = AS_LIST(pop());
    push(OBJ_VAL(copyList(oldList, false)));

    return true;
}

static bool joinList(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("join(str) takes 2 arguments (%d  given)", argCount);
        return false;
    }

    ObjString *str = AS_STRING(toString(pop()));
    ObjList *list = AS_LIST(pop());

    int size = 50;
    char *listString = mp_malloc(sizeof(char) * size);
    listString[0] = '\0';

    int listStringSize;

    int delLen = str->length;

    for (int i = 0; i < list->values.count; ++i)
    {
        char *element = valueToString(list->values.values[i], false);

        int elementSize = strlen(element);
        listStringSize = strlen(listString);

        if ((elementSize + 2 + delLen) >= (size - listStringSize - 1))
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
        }

        strncat(listString, element, size - listStringSize - 1);

        mp_free(element);

        if (i != list->values.count - 1)
            strncat(listString, str->chars, size - listStringSize - 1);
    }

    listStringSize = strlen(listString);
    push(STRING_VAL(listString));
    mp_free(listString);

    return true;
}

bool listMethods(char *method, int argCount)
{
    if (strcmp(method, "push") == 0 || strcmp(method, "add") == 0)
        return pushListItem(argCount);
    else if (strcmp(method, "remove") == 0)
        return removeListItem(argCount);
    else if (strcmp(method, "insert") == 0)
        return insertListItem(argCount);
    else if (strcmp(method, "pop") == 0)
        return popListItem(argCount);
    else if (strcmp(method, "contains") == 0)
        return containsListItem(argCount);
    else if (strcmp(method, "index") == 0)
        return indexList(argCount);
    else if (strcmp(method, "copy") == 0)
        return copyListShallow(argCount);
    else if (strcmp(method, "deepCopy") == 0)
        return copyListDeep(argCount);
    else if (strcmp(method, "join") == 0)
        return joinList(argCount);
    else if (strcmp(method, "swap") == 0)
        return swapListItems(argCount);
    else if (strcmp(method, "swapAll") == 0)
        return swapAllListItems(argCount);

    runtimeError("List has no method %s()", method);
    return false;
}

static bool getDictItem(int argCount)
{
    if (argCount != 3)
    {
        runtimeError("get() takes 3 arguments (%d  given)", argCount);
        return false;
    }

    Value defaultValue = pop();

    if (!IS_STRING(peek(0)))
    {
        runtimeError("Key passed to get() must be a string");
        return false;
    }

    Value key = pop();
    ObjDict *dict = AS_DICT(pop());

    Value ret = searchDict(dict, AS_CSTRING(key));

#ifndef NAN_TAGGING
    if (valuesEqual(ret, NULL_VAL))
#else
    if (ret == NULL_VAL)
#endif
        push(defaultValue);
    else
        push(ret);

    return true;
}

static bool dictKeys(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("get() takes 1 arguments (%d  given)", argCount);
        return false;
    }

    ObjDict *dict = AS_DICT(pop());
    ObjList *list = initList();

    int len = 0;

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
    }

    push(OBJ_VAL(list));

    return true;
}

static bool dictValues(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("get() takes 1 arguments (%d  given)", argCount);
        return false;
    }

    ObjDict *dict = AS_DICT(pop());
    ObjList *list = initList();

    for (int i = 0; i < dict->capacity; ++i)
    {
        dictItem *item = dict->items[i];

        if (!item || item->deleted)
        {
            continue;
        }

        writeValueArray(&list->values, copyValue(item->item));
    }

    push(OBJ_VAL(list));

    return true;
}

static uint32_t hash(char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

static bool removeDictItem(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("remove() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    if (!IS_STRING(peek(0)))
    {
        runtimeError("Key passed to remove() must be a string");
        return false;
    }

    char *key = AS_CSTRING(pop());
    ObjDict *dict = AS_DICT(pop());

    int index = hash(key) % dict->capacity;

    while (dict->items[index] && strcmp(dict->items[index]->key, key) != 0)
    {
        index++;
        if (index == dict->capacity)
            index = 0;
    }

    if (dict->items[index])
    {
        dict->items[index]->deleted = true;
        dict->count--;
        push(NULL_VAL);

        if (dict->capacity != 8 && dict->count * 100 / dict->capacity <= 35)
            resizeDict(dict, false);

        return true;
    }

    runtimeError("Key '%s' passed to remove() does not exist", key);
    return false;
}

static bool dictItemExists(int argCount)
{
    if (argCount != 2)
    {
        runtimeError("exists() takes 2 arguments (%d  given)", argCount);
        return false;
    }

    if (!IS_STRING(peek(0)))
    {
        runtimeError("Key passed to exists() must be a string");
        return false;
    }

    char *key = AS_CSTRING(pop());
    ObjDict *dict = AS_DICT(pop());

    for (int i = 0; i < dict->capacity; ++i)
    {
        if (!dict->items[i])
            continue;

        if (strcmp(dict->items[i]->key, key) == 0)
        {
            push(TRUE_VAL);
            return true;
        }
    }

    push(FALSE_VAL);
    return true;
}

bool dictContains(Value dictV, Value keyV, Value *result)
{

    if (!IS_STRING(keyV))
    {
        runtimeError("Key passed to exists() must be a string");
        return false;
    }

    char *key = AS_CSTRING(keyV);
    ObjDict *dict = AS_DICT(dictV);

    for (int i = 0; i < dict->capacity; ++i)
    {
        if (!dict->items[i])
            continue;

        if (strcmp(dict->items[i]->key, key) == 0)
        {
            *result = TRUE_VAL;
            return true;
        }
    }

    *result = FALSE_VAL;
    return true;
}

ObjDict *copyDict(ObjDict *oldDict, bool shallow)
{
    DISABLE_GC;
    ObjDict *newDict = initDict();

    for (int i = 0; i < oldDict->capacity; ++i)
    {
        if (oldDict->items[i] == NULL)
            continue;

        Value val = oldDict->items[i]->item;

        if (!shallow)
        {
            if (IS_DICT(val))
                val = OBJ_VAL(copyDict(AS_DICT(val), false));
            else if (IS_LIST(val))
                val = OBJ_VAL(copyList(AS_LIST(val), false));
        }

        insertDict(newDict, oldDict->items[i]->key, val);
    }
    RESTORE_GC;
    return newDict;
}

static bool copyDictShallow(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("copy() takes 1 argument (%d  given)", argCount);
        return false;
    }

    ObjDict *oldDict = AS_DICT(pop());
    push(OBJ_VAL(copyDict(oldDict, true)));

    return true;
}

static bool copyDictDeep(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("deepCopy() takes 1 argument (%d  given)", argCount);
        return false;
    }

    ObjDict *oldDict = AS_DICT(pop());
    push(OBJ_VAL(copyDict(oldDict, false)));

    return true;
}

bool dictMethods(char *method, int argCount)
{
    if (strcmp(method, "get") == 0)
        return getDictItem(argCount);
    else if (strcmp(method, "keys") == 0)
        return dictKeys(argCount);
    else if (strcmp(method, "values") == 0)
        return dictValues(argCount);
    else if (strcmp(method, "remove") == 0)
        return removeDictItem(argCount);
    else if (strcmp(method, "exists") == 0)
        return dictItemExists(argCount);
    else if (strcmp(method, "copy") == 0)
        return copyDictShallow(argCount);
    else if (strcmp(method, "deepCopy") == 0)
        return copyDictDeep(argCount);

    runtimeError("Dict has no method %s()", method);
    return false;
}