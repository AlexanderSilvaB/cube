/**
 * @Author: Alexander Silva Barbosa
 * @Date:   1969-12-31 21:00:00
 * @Last Modified by:   Alexander Silva Barbosa
 * @Last Modified time: 2021-09-25 00:05:46
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table)
{
    table->count = 0;
    table->capacityMask = -1;
    table->entries = NULL;
}

void freeTable(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);
    initTable(table);
}

static Entry *findEntry(Entry *entries, int capacityMask, ObjString *key)
{
    uint32_t index = key->hash & capacityMask;
    // printf("index: %d, key: %s, hash: %d, capacity: %d\n", index, key->chars, key->hash, capacityMask);
    Entry *tombstone = NULL;

    for (;;)
    {
        Entry *entry = &entries[index];
        // if (entry->key != NULL)
        // {
        //     printf("%s -> %s\n", entry->key->chars, key->chars);
        // }
        // printf("index: %d, key: %s, hash: %d, capacity: %d\n", index, key->chars, key->hash, capacityMask);
        if (entry->key == NULL)
        {
            if (IS_NULL(entry->value))
            {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                // We found a tombstone.
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        else if (entry->key == key)
        {
            // We found the key.
            return entry;
        }
        index = (index + 1) & capacityMask;
    }
}

bool iterateTable(Table *table, Entry *entry, int *iterator)
{
    int i = *iterator;

    if (table->count == 0)
        return false;

    if (i > table->capacityMask)
        return false;

    Entry *entries = table->entries;

    *entry = entries[i];
    i++;
    *iterator = i;
    return true;
}

bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacityMask, key);
    if (entry->key == NULL)
        return false;

    *value = entry->value;
    return true;
}

static void adjustCapacity(Table *table, int capacityMask)
{
    Entry *entries = ALLOCATE(Entry, capacityMask + 1);
    for (int i = 0; i <= capacityMask; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NULL_VAL;
    }
    table->count = 0;
    for (int i = 0; i <= table->capacityMask; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL)
            continue;
        Entry *dest = findEntry(entries, capacityMask, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacityMask + 1);
    table->entries = entries;

    table->capacityMask = capacityMask;
}

bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > (table->capacityMask + 1) * TABLE_MAX_LOAD)
    {
        int capacityMask = GROW_CAPACITY(table->capacityMask + 1) - 1;
        adjustCapacity(table, capacityMask);
    }

    Entry *entry = findEntry(table->entries, table->capacityMask, key);
    bool isNewKey = entry->key == NULL;

    if (isNewKey && IS_NULL(entry->value))
        table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0)
        return false;

    Entry *entry = findEntry(table->entries, table->capacityMask, key);
    if (entry->key == NULL)
        return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

void tableAddAll(Table *from, Table *to)
{
    for (int i = 0; i <= from->capacityMask; i++)
    {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL)
        {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash & table->capacityMask;

    for (;;)
    {
        Entry *entry = &table->entries[index];

        if (entry->key == NULL)
        {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NULL(entry->value))
                return NULL;
        }
        else if (entry->key->length == length && entry->key->hash == hash &&
                 memcmp(entry->key->chars, chars, length) == 0)
        {
            // We found it.
            return entry->key;
        }

        index = (index + 1) & table->capacityMask;
    }
}

void tableRemoveWhite(Table *table)
{
    for (int i = 0; i <= table->capacityMask; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked)
        {
#if defined(DEBUG_LOG_GC) && defined(DEBUG_LOG_GC_DETAILS)
            printf("table delete: ");
            printValue(OBJ_VAL(entry->key));
            printf("\n");
#endif
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table *table)
{
    for (int i = 0; i <= table->capacityMask; i++)
    {
        Entry *entry = &table->entries[i];
        mark_object((Obj *)entry->key);
        mark_value(entry->value);
    }
}
