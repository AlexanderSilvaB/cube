#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "mempool.h"
#include "object.h"
#include "value.h"
#include "vm.h"

void initValueArray(ValueArray *array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(array->values, Value, oldCapacity, array->capacity);
        for (int i = oldCapacity; i < array->capacity; i++)
            array->values[i] = NULL_VAL;
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

static uint32_t hash(char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

void insertDict(ObjDict *dict, char *key, Value value)
{
    if (dict->count * 100 / dict->capacity >= 60)
        resizeDict(dict, true);

    uint32_t hashValue = hash(key);
    int index = hashValue % dict->capacity;
    char *key_m = ALLOCATE(char, strlen(key) + 1);

    if (!key_m)
    {
        printf("ERROR!");
        return;
    }

    strcpy(key_m, key);

    dictItem *item = ALLOCATE(dictItem, sizeof(dictItem));

    if (!item)
    {
        printf("ERROR!");
        return;
    }

    item->key = key_m;
    item->item = value;
    item->deleted = false;
    item->hash = hashValue;

    while (dict->items[index] && !dict->items[index]->deleted && strcmp(dict->items[index]->key, key) != 0)
    {
        index++;
        if (index == dict->capacity)
            index = 0;
    }

    if (dict->items[index])
    {
        mp_free(dict->items[index]->key);
        mp_free(dict->items[index]);
        dict->count--;
    }

    dict->items[index] = item;
    dict->count++;
}

void resizeDict(ObjDict *dict, bool grow)
{
    int newSize;

    if (grow)
        newSize = dict->capacity << 1; // Grow by a factor of 2
    else
        newSize = dict->capacity >> 1; // Shrink by a factor of 2

    dictItem **items = mp_calloc(newSize, sizeof(*dict->items));

    for (int j = 0; j < dict->capacity; ++j)
    {
        if (!dict->items[j])
            continue;
        if (dict->items[j]->deleted)
            continue;

        int index = dict->items[j]->hash % newSize;

        while (items[index])
            index = (index + 1) % newSize;

        items[index] = dict->items[j];
    }

    // Free deleted values
    for (int j = 0; j < dict->capacity; ++j)
    {
        if (!dict->items[j])
            continue;
        if (dict->items[j]->deleted)
            freeDictValue(dict->items[j]);
    }

    mp_free(dict->items);

    dict->capacity = newSize;
    dict->items = items;
}

Value searchDict(ObjDict *dict, char *key)
{
    int index = hash(key) % dict->capacity;

    if (!dict->items[index])
        return NULL_VAL;

    while (index < dict->capacity && dict->items[index] && !dict->items[index]->deleted &&
           strcmp(dict->items[index]->key, key) != 0)
    {
        index++;
        if (index == dict->capacity)
        {
            index = 0;
        }
    }

    if (dict->items[index] && !dict->items[index]->deleted)
    {
        return dict->items[index]->item;
    }

    return NULL_VAL;
}

char *searchDictKey(ObjDict *dict, int index)
{
    int _index = 0;
    for (int i = 0; i < dict->capacity; ++i)
    {
        dictItem *item = dict->items[i];

        if (!item || item->deleted)
        {
            continue;
        }

        if (index == _index)
            return item->key;
        _index++;
    }
    return NULL;
}

// Calling function needs to free memory
char *valueToString(Value value, bool literal)
{
    if (IS_BOOL(value))
    {
        char *str = AS_BOOL(value) ? "true" : "false";
        char *boolString = mp_malloc(sizeof(char) * (strlen(str) + 1));
        snprintf(boolString, strlen(str) + 1, "%s", str);
        return boolString;
    }
    else if (IS_NULL(value))
    {
        char *nilString = mp_malloc(sizeof(char) * 5);
        snprintf(nilString, 5, "%s", "null");
        return nilString;
    }
    else if (IS_NUMBER(value))
    {
        double number = AS_NUMBER(value);
        int numberStringLength = snprintf(NULL, 0, "%.15g", number) + 1;
        char *numberString = mp_malloc(sizeof(char) * numberStringLength);
        snprintf(numberString, numberStringLength, "%.15g", number);
        int l = strlen(numberString);
        for (int i = 0; i < l; i++)
        {
            if (numberString[i] == ',')
                numberString[i] = '.';
        }
        return numberString;
    }
    else if (IS_OBJ(value))
    {
        return objectToString(value, literal);
    }

    char *unknown = mp_malloc(sizeof(char) * 8);
    snprintf(unknown, 8, "%s", "unknown");
    return unknown;
}

Value toBytes(Value value)
{
    ObjBytes *bytes = NULL;
    char c = 0xFF;
#ifdef NAN_TAGGING
    if (IS_NULL(value))
        bytes = copyBytes(&c, 1);
    else if (IS_BOOL(value))
    {
        bool v = AS_BOOL(value);
        bytes = copyBytes(&v, sizeof(v));
    }
    else if (IS_NUMBER(value))
    {
        double v = AS_NUMBER(value);
        uint32_t i = (uint32_t)v;
        if ((double)i == v)
            bytes = copyBytes(&i, sizeof(i));
        else
            bytes = copyBytes(&v, sizeof(v));
    }
    else if (IS_OBJ(value))
    {
        bytes = objectToBytes(value);
    }
#else
    if (IS_NULL(value))
        bytes = copyBytes(&c, 1);
    else if (IS_BOOL(value))
        bytes = copyBytes(&value.as.boolean, sizeof(value.as.boolean));
    else if (IS_NUMBER(value))
    {
        double v = AS_NUMBER(value);
        uint32_t i = (uint32_t)v;
        if ((double)i == v)
            bytes = copyBytes(&i, sizeof(i));
        else
            bytes = copyBytes(&v, sizeof(v));
    }
    else if (IS_OBJ(value))
    {
        bytes = objectToBytes(value);
    }
#endif

    if (bytes == NULL)
        bytes = initBytes();
    return OBJ_VAL(bytes);
}

char *valueType(Value value)
{
    if (IS_BOOL(value))
    {
        char *str = mp_malloc(sizeof(char) * 6);
        snprintf(str, 5, "bool");
        return str;
    }
    else if (IS_NULL(value))
    {
        char *str = mp_malloc(sizeof(char) * 6);
        snprintf(str, 5, "null");
        return str;
    }
    else if (IS_NUMBER(value))
    {
        char *str = mp_malloc(sizeof(char) * 5);
        snprintf(str, 4, "num");
        return str;
    }
    else if (IS_OBJ(value))
    {
        return objectType(value);
    }

    char *unknown = mp_malloc(sizeof(char) * 8);
    snprintf(unknown, 8, "unknown");
    return unknown;
}

void printValue(Value value)
{
    char *output = valueToString(value, true);
    printf("%s", output);
    mp_free(output);
}

bool valuesEqual(Value a, Value b)
{
#ifdef NAN_TAGGING
    if (IS_OBJ(a) && IS_OBJ(b))
    {
        return objectComparison(a, b);
    }

    if (IS_ENUM_VALUE(a) || IS_ENUM_VALUE(b))
    {
        if (IS_NUMBER(a) && IS_ENUM_VALUE(b) && IS_NUMBER(AS_ENUM_VALUE(b)->value))
        {
            return AS_NUMBER(a) == AS_NUMBER(AS_ENUM_VALUE(b)->value);
        }
        else if (IS_NUMBER(b) && IS_ENUM_VALUE(a) && IS_NUMBER(AS_ENUM_VALUE(a)->value))
        {
            return AS_NUMBER(b) == AS_NUMBER(AS_ENUM_VALUE(a)->value);
        }
        return false;
    }

    return a == b;
#else
    if (a.type != b.type)
    {
        if (IS_NUMBER(a) && IS_ENUM_VALUE(b) && IS_NUMBER(AS_ENUM_VALUE(b)->value))
        {
            return AS_NUMBER(a) == AS_NUMBER(AS_ENUM_VALUE(b)->value);
        }
        else if (IS_NUMBER(b) && IS_ENUM_VALUE(a) && IS_NUMBER(AS_ENUM_VALUE(a)->value))
        {
            return AS_NUMBER(b) == AS_NUMBER(AS_ENUM_VALUE(a)->value);
        }
        return false;
    }

    switch (a.type)
    {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            return objectComparison(a, b);
    }

    return false;
#endif
}

Value toBool(Value value)
{
    if (IS_NULL(value))
        return BOOL_VAL(false);
    else if (IS_BOOL(value))
        return BOOL_VAL(AS_BOOL(value));
    else if (IS_NUMBER(value))
    {
        return BOOL_VAL(AS_NUMBER(value) > 0);
    }
    else if (IS_STRING(value))
    {
        char *str = AS_CSTRING(value);
        if (strlen(str) == 0 || strcmp(str, "0") == 0 || strcmp(str, "false") == 0)
            return BOOL_VAL(false);
        else
            return BOOL_VAL(false);
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *bytes = AS_BYTES(value);
        if (bytes->length == 0)
            return FALSE_VAL;
        return bytes->bytes[0] == 0x0 ? FALSE_VAL : TRUE_VAL;
    }
    return BOOL_VAL(true);
}

Value toNumber(Value value)
{
    if (IS_NULL(value))
        return NUMBER_VAL(0);
    else if (IS_BOOL(value))
        return NUMBER_VAL(AS_BOOL(value) ? 1 : 0);
    else if (IS_NUMBER(value))
    {
        return NUMBER_VAL(AS_NUMBER(value));
    }
    else if (IS_STRING(value))
    {
        char *str = AS_CSTRING(value);
        double value = strtod(str, NULL);
        return NUMBER_VAL(value);
    }
    else if (IS_BYTES(value))
    {
        ObjBytes *bytes = AS_BYTES(value);
        int len = bytes->length > sizeof(uint32_t) || bytes->length < 0 ? sizeof(uint32_t) : bytes->length;
        uint32_t value = 0;
        for (int i = 0; i < len; i++)
            value |= bytes->bytes[i] << (i * 8);

        return NUMBER_VAL(value);
    }
    else if (IS_ENUM_VALUE(value) && IS_NUMBER(AS_ENUM_VALUE(value)->value))
    {
        return NUMBER_VAL(AS_NUMBER(AS_ENUM_VALUE(value)->value));
    }
    else if (IS_ENUM_VALUE(value) && IS_BYTES(AS_ENUM_VALUE(value)->value))
    {
        return toNumber(AS_ENUM_VALUE(value)->value);
    }
    return NUMBER_VAL(1);
}

Value toString(Value value)
{
    char *str = valueToString(value, false);
    Value v = STRING_VAL(str);
    mp_free(str);
    return v;
}

Value copyValue(Value value)
{
    if (IS_STRING(value))
    {
        ObjString *oldStr = AS_STRING(value);
        char *str = ALLOCATE(char, oldStr->length + 1);
        strcpy(str, oldStr->chars);
        Value ret = STRING_VAL(str);
        FREE(char, str);
        return ret;
    }
    else if (IS_OBJ(value))
    {
        return copyObject(value);
    }
    return value;
}