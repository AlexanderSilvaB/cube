/**
 * @Author: Alexander Silva Barbosa
 * @Date:   1969-12-31 21:00:00
 * @Last Modified by:   Alexander Silva Barbosa
 * @Last Modified time: 2021-09-24 22:16:57
 */
#include <cube/cubeext.h>
#include <sqlite3.h>
#include <stdio.h>
#include <vector>

typedef struct
{
    sqlite3 *db;
    char *message;
    bool closed;
    cube_native_var *data;
} DB;

std::vector<DB> dbs;

void free_cube_native_var(cube_native_var *var, bool skipFirst, bool skipInterns)
{
    if (var == NULL)
        return;

    if (!skipInterns)
    {
        if (var->type == TYPE_STRING)
        {
            free(var->value._string);
        }
        else if (var->type == TYPE_BYTES)
        {
            free(var->value._bytes.bytes);
        }
    }
    if (var->is_list && var->list != NULL)
    {
        for (int i = 0; i < var->size; i++)
        {
            free_cube_native_var(var->list[i], false, skipInterns);
        }
        free(var->list);
        var->list = NULL;
        var->size = 0;
        var->capacity = 0;
    }
    if (var->is_dict && var->dict != NULL)
    {
        for (int i = 0; i < var->size; i++)
        {
            free_cube_native_var(var->dict[i], false, skipInterns);
        }
        free(var->dict);
        var->dict = NULL;
        var->size = 0;
        var->capacity = 0;
    }
    if (!skipFirst)
        free(var);
}

void free_all_cube_native_var(cube_native_var *var)
{
    free_cube_native_var(var, false, false);
}

bool getDB(int id, DB **db)
{
    if (id < 0 || id >= dbs.size())
        return false;
    *db = &dbs[id];
    return !dbs[id].closed;
}

void prepare_db(DB *db)
{
    if (db->message)
    {
        // sqlite3_free(db->message);
        db->message = NULL;
    }

    if (db->data != NULL)
    {
        free_all_cube_native_var(db->data);
        db->data = NULL;
    }

    db->data = NULL;
}

extern "C"
{
    EXPORTED int open(char *path, bool create)
    {
        DB db;
        db.closed = false;
        db.message = NULL;
        db.data = NULL;
        // int rc = sqlite3_open(path, &db.db);
        int flags = SQLITE_OPEN_READWRITE;
        if (create)
            flags |= SQLITE_OPEN_CREATE;

        int rc = sqlite3_open_v2(path, &db.db, flags, NULL);
        if (rc != SQLITE_OK)
            return -1;

        dbs.push_back(db);
        return dbs.size() - 1;
    }

    EXPORTED bool close(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return false;

        prepare_db(db);

        sqlite3_close(db->db);

        db->closed = true;
        return true;
    }

    EXPORTED bool exec(int id, char *sql)
    {
        DB *db;
        if (!getDB(id, &db))
            return false;

        prepare_db(db);

        cube_native_var *list = NULL;

        sqlite3_stmt *stmt = NULL;
        int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            db->message = (char *)sqlite3_errmsg(db->db);
            return false;
        }

        int rowCount = 0;
        rc = sqlite3_step(stmt);
        while (rc != SQLITE_DONE && rc != SQLITE_OK)
        {
            rowCount++;
            int colCount = sqlite3_column_count(stmt);
            if (colCount == 0)
                break;

            if (db->data == NULL)
                db->data = NATIVE_NULL();

            if (list == NULL)
                list = db->data;

            if (!IS_NATIVE_LIST(list))
                TO_NATIVE_LIST(list);

            cube_native_var *item = NATIVE_DICT();

            for (int colIndex = 0; colIndex < colCount; colIndex++)
            {
                int type = sqlite3_column_type(stmt, colIndex);
                const char *columnName = sqlite3_column_name(stmt, colIndex);
                if (type == SQLITE_INTEGER)
                {
                    int valInt = sqlite3_column_int(stmt, colIndex);
                    ADD_NATIVE_DICT(item, COPY_STR(columnName), NATIVE_NUMBER(valInt));
                }
                else if (type == SQLITE_FLOAT)
                {
                    double valDouble = sqlite3_column_double(stmt, colIndex);
                    ADD_NATIVE_DICT(item, COPY_STR(columnName), NATIVE_NUMBER(valDouble));
                }
                else if (type == SQLITE_TEXT)
                {
                    const unsigned char *valChar = sqlite3_column_text(stmt, colIndex);
                    ADD_NATIVE_DICT(item, COPY_STR(columnName), NATIVE_STRING_COPY((const char *)valChar));
                }
                else if (type == SQLITE_BLOB)
                {
                    const void *data = sqlite3_column_blob(stmt, colIndex);
                    int len = sqlite3_column_bytes(stmt, colIndex);
                    ADD_NATIVE_DICT(item, COPY_STR(columnName), NATIVE_BYTES_COPY(len, (unsigned char *)data));
                }
                else if (type == SQLITE_NULL)
                {
                    ADD_NATIVE_DICT(item, COPY_STR(columnName), NATIVE_NULL());
                }
            }

            ADD_NATIVE_LIST(list, item);
            rc = sqlite3_step(stmt);
        }

        rc = sqlite3_finalize(stmt);
        return rc == SQLITE_OK;
    }

    EXPORTED cube_native_var *data(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return NATIVE_NULL();

        cube_native_var *result = db->data;
        if (result == NULL)
            result = NATIVE_NULL();
        db->data = NULL;

        return result;
    }

    EXPORTED char *error(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return NULL;

        return db->message;
    }

    EXPORTED int lastInserted(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return -1;

        return sqlite3_last_insert_rowid(db->db);
    }
}