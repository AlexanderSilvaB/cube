#include "zip.h"
#include "../../mempool.h"
#include "../../util.h"

int zip_add_data(zip *zip, const char *name, const void *data, size_t size)
{
    if (!zip)
        return -1;

    if (zip_entry_open(zip, name) < 0)
        return -1;

    if (zip_entry_write(zip, data, size) < 0)
    {
        zip_entry_close(zip);
        return -1;
    }

    int len = zip_entry_size(zip);
    zip_entry_close(zip);

    return len;
}

int zip_add_file(zip *zip, const char *name, const char *file_name)
{
    if (!zip)
        return -1;

    if (zip_entry_open(zip, name) < 0)
        return -1;

    if (zip_entry_fwrite(zip, file_name) < 0)
    {
        zip_entry_close(zip);
        return -1;
    }

    int len = zip_entry_size(zip);
    zip_entry_close(zip);

    return len;
}

int zip_read(zip *zip, const char *name, void **buffer)
{
    if (!zip)
        return -1;

    if (zip_entry_open(zip, name) < 0)
        return -1;

    size_t len;
    if (zip_entry_read(zip, buffer, &len) < 0)
    {
        zip_entry_close(zip);
        return -1;
    }

    zip_entry_close(zip);

    return len;
}

int zip_extract_file(zip *zip, const char *name, const char *file_name)
{
    if (!zip)
        return -1;

    if (zip_entry_open(zip, name) < 0)
        return -1;

    if (zip_entry_fread(zip, file_name) < 0)
    {
        zip_entry_close(zip);
        return -1;
    }

    int len = zip_entry_size(zip);
    zip_entry_close(zip);

    return len;
}

int zip_count(zip *zip)
{
    if (!zip)
        return -1;
    return zip_total_entries(zip);
}

int zip_open_index(zip *zip, int index)
{
    if (!zip)
        return -1;
    return zip_entry_openbyindex(zip, index);
}

void zip_close_index(zip *zip)
{
    if (!zip)
        return;
    zip_entry_close(zip);
}

const char *zip_name_index(zip *zip)
{
    if (!zip)
        return NULL;

    return zip_entry_name(zip);
}

int zip_size_index(zip *zip)
{
    if (!zip)
        return -1;

    return zip_entry_size(zip);
}

int zip_isdir_index(zip *zip)
{
    if (!zip)
        return -1;

    return zip_entry_isdir(zip);
}

int zip_read_index(zip *zip, void **buffer)
{
    if (!zip)
        return -1;

    size_t len;
    if (zip_entry_read(zip, buffer, &len) < 0)
    {
        return -1;
    }

    return len;
}

bool zip_dir_entry(zip *zip, const char *path, const char *root)
{
    int len;
    char **files = listFiles(path, &len);
    char *name = mp_calloc(1024, sizeof(char));
    char *p = mp_calloc(1024, sizeof(char));

    for (int i = 0; i < len; i++)
    {
        if (strcmp(files[i], ".") == 0 || strcmp(files[i], "..") == 0)
            continue;

        strcpy(p, path);
        strcat(p, "/");
        strcat(p, files[i]);

        strcpy(name, root);
        strcat(name, files[i]);

        if (isDir(p))
        {
            strcat(name, "/");
            if (!zip_dir_entry(zip, p, name))
                return false;
        }
        else
        {
            if (zip_add_file(zip, name, p) < 0)
                return false;
        }
    }

    mp_free(p);
    mp_free(name);

    return true;
}

int zip_dir(const char *path, const char *zipName)
{
    zip *zip = zip_open(zipName, ZIP_DEFAULT_LEVEL, 'w');
    if (!zip)
        return -1;

    if (!zip_dir_entry(zip, path, ""))
    {
        zip_close(zip);
        return -1;
    }

    zip_close(zip);
    return 0;
}