#include <cube/cubeext.h>
#include "kuba-zip.h"


typedef struct zip_file_t
{
    const char *name;
    struct zip_t *zip;
    int id;
    struct zip_file_t *next;
    int arg;
} zip_file;

zip_file *first = NULL;

zip_file *alloc_zip_file()
{
    zip_file *zip = (zip_file*)malloc(sizeof(zip_file));
    zip->id = 0;
    zip->name = NULL;
    zip->zip = NULL;
    zip->next = NULL;
    zip->arg = 0;
    return zip;
}

void free_zip_file(zip_file *zip)
{
    free(zip);
}

int add_zip_file(zip_file *zip)
{
    zip_file *current = first;
    zip->id = *(int*)zip;
    if(current == NULL)
    {
        first = zip;
        return zip->id;
    }
    while(current->next != NULL)
    {
        current = current->next;
    }
    current->next = zip;
    return zip->id;
}

zip_file *get_zip_file(int id)
{
    zip_file *current = first;
    while(current != NULL)
    {
        if(current->id == id)
            return current;
        current = current->next;
    }
    return NULL;
}

bool remove_zip_file(int id)
{
    zip_file *current = first;
    zip_file *parent = NULL;
    while(current != NULL)
    {
        if(current->id == id)
        {
            if(parent != NULL)
            {
                parent->next = current->next;
            }
            else
            {
                first = current->next;
            }
            zip_close(current->zip);
            free_zip_file(current);
            return true;
        }
        parent = current;
        current = current->next;
    }
    return false;
}

int on_extract_entry(const char *filename, void *arg) 
{
    zip_file *zip = (zip_file*)arg;
    zip->arg++;
    return 0;
}

EXPORTED cube_native_var* cube_zip_open(cube_native_var *name, cube_native_var *mode)
{
    
    struct zip_t *z = zip_open(AS_NATIVE_STRING(name), ZIP_DEFAULT_COMPRESSION_LEVEL, AS_NATIVE_STRING(mode)[0]);
    if(z == NULL)
        return NATIVE_NONE();
    
    zip_file *zip = alloc_zip_file();
    zip->name = COPY_STR(AS_NATIVE_STRING(name));
    zip->zip = z;
    int id = add_zip_file(zip);
    cube_native_var *result = NATIVE_NUMBER(id);
    return result;
}


EXPORTED cube_native_var* cube_zip_close(cube_native_var *id)
{
    bool rc = remove_zip_file(AS_NATIVE_NUMBER(id));
    cube_native_var *result = NATIVE_BOOL(rc);
    return result;
}

EXPORTED cube_native_var* cube_zip_add(cube_native_var *id, cube_native_var *name, cube_native_var *data)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    int len = 0;
    zip_entry_open(zip->zip, AS_NATIVE_STRING(name));
    {
        cube_native_bytes bytes = AS_NATIVE_BYTES(data);
        zip_entry_write(zip->zip, bytes.bytes, bytes.length);
        len = zip_entry_size(zip->zip);
    }
    zip_entry_close(zip->zip);
    
    return NATIVE_NUMBER(len);
}

EXPORTED cube_native_var* cube_zip_add_file(cube_native_var *id, cube_native_var *name, cube_native_var *fileName)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    int len = 0;
    zip_entry_open(zip->zip, AS_NATIVE_STRING(name));
    {
        zip_entry_fwrite(zip->zip, AS_NATIVE_STRING(fileName));
        len = zip_entry_size(zip->zip);
    }
    zip_entry_close(zip->zip);
    
    return NATIVE_NUMBER(len);
}

EXPORTED cube_native_var* cube_zip_extract(cube_native_var *id, cube_native_var *path)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    zip->arg = 0;
    int rc = zip_extract(zip->name, AS_NATIVE_STRING(path), on_extract_entry, (void*)zip);
    if(rc != 0)
        return NATIVE_NONE();
    return NATIVE_NUMBER(zip->arg);
}

EXPORTED cube_native_var* cube_zip_read(cube_native_var *id, cube_native_var *name)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    if(zip_entry_open(zip->zip, AS_NATIVE_STRING(name)) != 0)
        return NATIVE_NONE();
    
    void *buf;
    size_t len;

    if(zip_entry_read(zip->zip, &buf, &len) < 0)
        return NATIVE_NONE();

    zip_entry_close(zip->zip);

    return NATIVE_BYTES_ARG(len, buf);
}

EXPORTED cube_native_var* cube_zip_extract_file(cube_native_var *id, cube_native_var *name, cube_native_var *fileName)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    if(zip_entry_open(zip->zip, AS_NATIVE_STRING(name)) != 0)
        return NATIVE_BOOL(false);

    if(zip_entry_fread(zip->zip, AS_NATIVE_STRING(fileName)) < 0)
        return NATIVE_BOOL(false);

    zip_entry_close(zip->zip);

    return NATIVE_BOOL(true);
}

EXPORTED cube_native_var* cube_zip_list(cube_native_var *id)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    cube_native_var *list = NATIVE_LIST();
    int i, n = zip_total_entries(zip->zip);
    for (i = 0; i < n; ++i) 
    {
        int rc = zip_entry_openbyindex(zip->zip, i);
        {
            const char *name = zip_entry_name(zip->zip);
            if(name == NULL)
            {
                zip_entry_close(zip->zip);
                continue;
            }

            cube_native_var *ev = NATIVE_STRING_COPY(name);
            ADD_NATIVE_LIST(list, ev);
        }
        zip_entry_close(zip->zip);
    }

    return list;
}

EXPORTED cube_native_var* cube_zip_details(cube_native_var *id)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    cube_native_var *dict = NATIVE_DICT();
    int i, n = zip_total_entries(zip->zip);
    for (i = 0; i < n; ++i) 
    {
        int rc = zip_entry_openbyindex(zip->zip, i);
        {
            const char *name = zip_entry_name(zip->zip);
            if(name == NULL)
            {
                zip_entry_close(zip->zip);
                continue;
            }

            cube_native_var *val = NATIVE_DICT();
            ADD_NATIVE_DICT(val, COPY_STR("dir"), NATIVE_BOOL(zip_entry_isdir(zip->zip)));
            ADD_NATIVE_DICT(val, COPY_STR("size"), NATIVE_NUMBER(zip_entry_size(zip->zip)));
            ADD_NATIVE_DICT(val, COPY_STR("crc32"), NATIVE_NUMBER(zip_entry_crc32(zip->zip)));
            
            ADD_NATIVE_DICT(dict, COPY_STR(name), val);
        }
        zip_entry_close(zip->zip);
    }

    return dict;
}

EXPORTED cube_native_var* cube_zip_read_all(cube_native_var *id)
{
    zip_file *zip = get_zip_file(AS_NATIVE_NUMBER(id));
    if(zip == NULL)
        return NATIVE_NONE();

    cube_native_var *dict = NATIVE_DICT();
    int i, n = zip_total_entries(zip->zip);
    for (i = 0; i < n; ++i) 
    {
        int rc = zip_entry_openbyindex(zip->zip, i);
        {
            const char *name = zip_entry_name(zip->zip);
            if(name == NULL || zip_entry_isdir(zip->zip))
            {
                zip_entry_close(zip->zip);
                continue;
            }

            void *buf;
            size_t len;

            cube_native_var *val;
            if(zip_entry_read(zip->zip, &buf, &len) < 0)
                val = NATIVE_NONE();
            else
                val = NATIVE_BYTES_ARG(len, buf);
            
            ADD_NATIVE_DICT(dict, COPY_STR(name), val);
        }
        zip_entry_close(zip->zip);
    }

    return dict;
}