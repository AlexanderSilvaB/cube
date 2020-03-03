#include "enums.h"
#include "memory.h"
#include "vm.h"
#include "mempool.h"

static bool getNameEnum(int argCount) {
	if (argCount != 2) {
		runtimeError("name(s) takes 2 arguments (%d given)", argCount);
		return false;
	}

	Value value = pop();

	ObjEnum *enume = AS_ENUM(pop());

	int i = 0;
    Entry entry;
    Table *table = &enume->members;

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;
		
		if(valuesEqual(AS_ENUM_VALUE(entry.value)->value, value))
		{
			push(copyValue(OBJ_VAL(entry.key)));
			return true;
		}
    }

	push(NONE_VAL);
	return true;
}

static bool getValueEnum(int argCount) {
	if (argCount != 2) {
		runtimeError("value(s) takes 2 arguments (%d given)", argCount);
		return false;
	}

	Value value = pop();
	if(!IS_STRING(value))
	{
		runtimeError("value(s) argument must be a string.");
		return false;
	}

	ObjString *name = AS_STRING(value);

	ObjEnum *enume = AS_ENUM(pop());

	int i = 0;
    Entry entry;
    Table *table = &enume->members;

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;
		
		if(strcmp(entry.key->chars, name->chars) == 0)
		{
			push(AS_ENUM_VALUE(entry.value)->value);
			return true;
		}
    }

	push(NONE_VAL);
	return true;
}

static bool getEnum(int argCount) {
	if (argCount != 2) {
		runtimeError("get(s) takes 2 arguments (%d given)", argCount);
		return false;
	}

	Value value = pop();
	if(!IS_STRING(value))
	{
		runtimeError("get(s) argument must be a string.");
		return false;
	}

	ObjString *name = AS_STRING(value);

	ObjEnum *enume = AS_ENUM(pop());

	int i = 0;
    Entry entry;
    Table *table = &enume->members;

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;
		
		if(strcmp(entry.key->chars, name->chars) == 0)
		{
			push(entry.value);
			return true;
		}
    }

	push(NONE_VAL);
	return true;
}

static bool keysEnum(int argCount) {
	if (argCount != 1) {
		runtimeError("keys() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjEnum *enume = AS_ENUM(pop());

	ObjList *list = initList();

	int i = 0;
    Entry entry;
    Table *table = &enume->members;

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;

		writeValueArray(&list->values, copyValue(OBJ_VAL(entry.key)));
    }

	push(OBJ_VAL(list));
	return true;
}

static bool valuesEnum(int argCount) {
	if (argCount != 1) {
		runtimeError("values() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjEnum *enume = AS_ENUM(pop());

	ObjList *list = initList();

	int i = 0;
    Entry entry;
    Table *table = &enume->members;

    while (iterateTable(table, &entry, &i))
    {
        if (entry.key == NULL)
            continue;

		writeValueArray(&list->values, copyValue(AS_ENUM_VALUE(entry.value)->value));
    }

	push(OBJ_VAL(list));
	return true;
}

static bool getNameEnumValue(int argCount) {
	if (argCount != 1) {
		runtimeError("name() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjEnumValue *enumValue = AS_ENUM_VALUE(pop());
	push(OBJ_VAL(enumValue->name));
	return true;
}

static bool getValueEnumValue(int argCount) {
	if (argCount != 1) {
		runtimeError("value() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjEnumValue *enumValue = AS_ENUM_VALUE(pop());
	push(enumValue->value);
	return true;
}

static bool getEnumValue(int argCount) {
	if (argCount != 1) {
		runtimeError("getEnum() takes 1 argument (%d given)", argCount);
		return false;
	}

	ObjEnumValue *enumValue = AS_ENUM_VALUE(pop());
	push(OBJ_VAL(enumValue->enume));
	return true;
}

bool enumMethods(char *method, int argCount) {
	if (strcmp(method, "name") == 0)
		return getNameEnum(argCount);
	else if (strcmp(method, "value") == 0)
		return getValueEnum(argCount);
	else if (strcmp(method, "get") == 0)
		return getEnum(argCount);
	else if (strcmp(method, "keys") == 0)
		return keysEnum(argCount);
	else if (strcmp(method, "values") == 0)
		return valuesEnum(argCount);

	runtimeError("Enum has no method %s()", method);
	return false;
}

bool enumValueMethods(char *method, int argCount) {
	if (strcmp(method, "name") == 0)
		return getNameEnumValue(argCount);
	else if (strcmp(method, "value") == 0)
		return getValueEnumValue(argCount);
	else if (strcmp(method, "getEnum") == 0)
		return getEnumValue(argCount);

	runtimeError("EnumValue has no method %s()", method);
	return false;
}
