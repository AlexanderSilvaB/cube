#include <cube/cubeext.h>
#include <my_global.h>
#include <mysql.h>
#include <vector>

typedef struct
{
    MYSQL *db;
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
    EXPORTED int opendb(char *host, char *user, char *passwd, char *name)
    {
        DB db;
        db.closed = false;
        db.message = NULL;
        db.data = NULL;
        db.db = mysql_init(NULL);
        if (db.db == NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(db.db));
            return -1;
        }

        if (mysql_real_connect(db.db, host, user, passwd, name, 0, NULL, 0) == NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(db.db));
            mysql_close(db.db);
            return -1;
        }

        dbs.push_back(db);
        return dbs.size() - 1;
    }

    EXPORTED bool close(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return false;

        prepare_db(db);

        mysql_close(db->db);

        db->closed = true;
        return true;
    }

    EXPORTED bool exec(int id, char *sql)
    {
        DB *db;
        if (!getDB(id, &db))
            return false;

        prepare_db(db);

        if (mysql_query(db->db, sql))
        {
            db->message = (char *)mysql_error(db->db);
            return false;
        }

        cube_native_var *list = NULL;

        MYSQL_RES *result = mysql_store_result(db->db);
        if (result == NULL)
            return true;

        int num_fields = mysql_num_fields(result);
        if (num_fields == 0)
        {
            mysql_free_result(result);
            return true;
        }

        if (db->data == NULL)
            db->data = NATIVE_NULL();

        if (list == NULL)
            list = db->data;

        if (!IS_NATIVE_LIST(list))
            TO_NATIVE_LIST(list);

        MYSQL_ROW row;
        MYSQL_FIELD *field;

        while ((row = mysql_fetch_row(result)))
        {
            mysql_field_seek(result, 0);
            unsigned long *len = mysql_fetch_lengths(result);

            cube_native_var *item = NATIVE_DICT();

            for (int i = 0; i < num_fields; i++)
            {
                field = mysql_fetch_field(result);
                if (row[i] == NULL || field->type == MYSQL_TYPE_NULL)
                {
                    ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_NULL());
                }
                else if (IS_NUM(field->type))
                {
                    double val = atof(row[i]);
                    ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_NUMBER(val));
                }
                else if (field->type == MYSQL_TYPE_BIT)
                {
                    ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_BYTES_COPY(len[i], (unsigned char *)row[i]));
                }
                else
                {
                    ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_STRING_COPY(row[i]));
                }
            }

            ADD_NATIVE_LIST(list, item);
        }
        mysql_free_result(result);

        return true;
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
}