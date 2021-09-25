/**
 * @Author: Alexander Silva Barbosa
 * @Date:   1969-12-31 21:00:00
 * @Last Modified by:   Alexander Silva Barbosa
 * @Last Modified time: 2021-09-25 01:35:42
 */
#include <cube/cubeext.h>
#include <libpq-fe.h>
#include <postgres.h>
#include <sstream>
#include <string>
#include <vector>

typedef struct
{
    PGconn *db;
    char *message;
    bool closed;
    int lastId;
    cube_native_var *data;
} DB;

#include <catalog/pg_type.h>

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
    EXPORTED int opendb(char *host, char *user, char *passwd, char *name, bool create)
    {
        std::stringstream ss;
        ss << "host=" << host << " hostaddr=" << host;
        if (user != NULL)
            ss << " user=" << user;
        if (passwd != NULL)
            ss << " password=" << passwd;
        if (name != NULL)
            ss << " dbname=" << name;

        DB db;
        db.closed = false;
        db.message = NULL;
        db.data = NULL;
        db.db = PQconnectdb(ss.str().c_str());
        if (PQstatus(db.db) == CONNECTION_BAD)
        {
            if (!create)
            {
                fprintf(stderr, "%s\n", PQerrorMessage(db.db));
                PQfinish(db.db);
                return -1;
            }
            else
            {
                ss = std::stringstream();
                ss << "host=" << host << " hostaddr=" << host;
                if (user != NULL)
                    ss << " user=" << user;
                if (passwd != NULL)
                    ss << " password=" << passwd;

                db.db = PQconnectdb(ss.str().c_str());
                if (PQstatus(db.db) == CONNECTION_BAD)
                {
                    fprintf(stderr, "%s\n", PQerrorMessage(db.db));
                    PQfinish(db.db);
                    return -1;
                }

                std::stringstream ss;
                ss << "CREATE DATABASE " << name;
                PGresult *res = PQexec(db.db, ss.str().c_str());
                if (PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    fprintf(stderr, "%s\n", PQerrorMessage(db.db));
                    PQclear(res);
                    PQfinish(db.db);
                    return -1;
                }

                PQclear(res);
            }
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

        PQfinish(db->db);

        db->closed = true;
        return true;
    }

    EXPORTED bool exec(int id, char *sql)
    {
        DB *db;
        if (!getDB(id, &db))
            return false;

        prepare_db(db);

        PGresult *res = PQexec(db->db, sql);
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            db->message = (char *)PQerrorMessage(db->db);
            PQclear(res);
            return false;
        }

        db->lastId = PQoidValue(res);

        cube_native_var *list = NULL;

        if (db->data == NULL)
            db->data = NATIVE_NULL();

        if (list == NULL)
            list = db->data;

        if (!IS_NATIVE_LIST(list))
            TO_NATIVE_LIST(list);

        int rows = PQntuples(res);
        int ncols = PQnfields(res);

        char *field;
        char *value;
        int length;

        for (int i = 0; i < rows; i++)
        {

            cube_native_var *item = NATIVE_DICT();

            for (int j = 0; j < ncols; j++)
            {
                field = PQfname(res, j);
                value = PQgetvalue(res, i, j);
                length = PQgetlength(res, i, j);

                if (PQgetisnull(res, i, j))
                {
                    ADD_NATIVE_DICT(item, COPY_STR(field), NATIVE_NULL());
                }
                // else if (IS_NUM(field->type))
                // {
                //     double val = atof(row[i]);
                //     ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_NUMBER(val));
                // }
                // else if (PQbinaryTuples(res))
                // {
                //     ADD_NATIVE_DICT(item, COPY_STR(field->name), NATIVE_BYTES_COPY(len[i], (unsigned char *)row[i]));
                // }
                else
                {
                    ADD_NATIVE_DICT(item, COPY_STR(field), NATIVE_STRING_COPY(value));
                }
            }

            ADD_NATIVE_LIST(list, item);
        }

        PQclear(res);

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

    EXPORTED int lastInserted(int id)
    {
        DB *db;
        if (!getDB(id, &db))
            return -1;

        return db->lastId;
    }
}