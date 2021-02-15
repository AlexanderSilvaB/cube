#include "files.h"
#include "memory.h"
#include "mempool.h"
#include "vm.h"

static bool extendClass(int argCount)
{
    if (argCount < 2)
    {
        runtimeError("extend() takes at least 1 argument (%d given)", argCount);
        return false;
    }

    int nCopy = 0;

    ObjClass *klass = AS_CLASS(peek(argCount - 1));
    Value tmp;
    Value v;

    for (int i = 1; i < argCount; i++)
    {
        v = peek(i - 1);
        if (IS_DICT(v))
        {
            ObjDict *other = AS_DICT(v);
            for (int i = 0; i < other->capacity; ++i)
            {
                dictItem *item = other->items[i];

                if (!item || item->deleted)
                {
                    continue;
                }

                if (tableGet(&klass->methods, AS_STRING(STRING_VAL(item->key)), &tmp) || strcmp(item->key, "init") == 0)
                {
                    continue;
                }

                if (IS_FUNCTION(item->item) || IS_CLOSURE(item->item))
                    tableSet(&klass->methods, copyString(item->key, strlen(item->key)), item->item);
                else
                    tableSet(&klass->fields, copyString(item->key, strlen(item->key)), item->item);
                nCopy++;
            }
        }
        else if (IS_CLASS(v))
        {
            ObjClass *other = AS_CLASS(v);
            for (int i = 0; i <= other->methods.capacityMask; i++)
            {
                Entry *entry = &other->methods.entries[i];
                if (entry->key != NULL && strcmp(entry->key->chars, "init") != 0)
                {
                    if (!tableGet(&klass->methods, entry->key, &tmp))
                    {
                        tableSet(&klass->methods, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }

            for (int i = 0; i <= other->staticFields.capacityMask; i++)
            {
                Entry *entry = &other->staticFields.entries[i];
                if (entry->key != NULL)
                {
                    if (!tableGet(&klass->staticFields, entry->key, &tmp))
                    {
                        tableSet(&klass->staticFields, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }

            for (int i = 0; i <= other->fields.capacityMask; i++)
            {
                Entry *entry = &other->fields.entries[i];
                if (entry->key != NULL)
                {
                    if (!tableGet(&klass->fields, entry->key, &tmp))
                    {
                        tableSet(&klass->fields, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }
        }
        else if (IS_INSTANCE(v))
        {
            ObjInstance *other = AS_INSTANCE(v);
            for (int i = 0; i <= other->klass->methods.capacityMask; i++)
            {
                Entry *entry = &other->klass->methods.entries[i];
                if (entry->key != NULL && strcmp(entry->key->chars, "init") != 0)
                {
                    if (!tableGet(&klass->methods, entry->key, &tmp))
                    {
                        tableSet(&klass->methods, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }

            for (int i = 0; i <= other->klass->staticFields.capacityMask; i++)
            {
                Entry *entry = &other->klass->staticFields.entries[i];
                if (entry->key != NULL)
                {
                    if (!tableGet(&klass->staticFields, entry->key, &tmp))
                    {
                        tableSet(&klass->staticFields, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }

            for (int i = 0; i <= other->fields.capacityMask; i++)
            {
                Entry *entry = &other->fields.entries[i];
                if (entry->key != NULL)
                {
                    if (!tableGet(&klass->fields, entry->key, &tmp))
                    {
                        tableSet(&klass->fields, entry->key, entry->value);
                        nCopy++;
                    }
                }
            }
        }
        else
        {
            runtimeError("Cannot extend from '%s'", valueType(v));
            return false;
        }
    }

    for (int i = 0; i < argCount; i++)
        pop();

    push(NUMBER_VAL(nCopy));
    return true;
}

bool classMethods(char *method, int argCount)
{
    if (strcmp(method, "extend") == 0)
        return extendClass(argCount);

    runtimeError("class has no method %s()", method);
    return false;
}