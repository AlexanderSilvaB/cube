#ifndef _CUBE_ZIP_H_
#define _CUBE_ZIP_H_

#include "kuba-zip.h"

typedef struct zip_t zip;

#define ZIP_DEFAULT_LEVEL ZIP_DEFAULT_COMPRESSION_LEVEL

extern zip *zip_open(const char *zipname, int level, char mode);
extern void zip_close(zip *zip);
int zip_add_data(zip *zip, const char *name, const void *data, size_t bufsize);
int zip_add_file(zip *zip, const char *name, const char *file_name);
extern int zip_extract(const char *zipname, const char *dir, int (*on_extract_entry)(const char *filename, void *arg),
                       void *arg);
int zip_read(zip *zip, const char *name, void **buffer);
int zip_extract_file(zip *zip, const char *name, const char *file_name);
int zip_count(zip *zip);
int zip_open_index(zip *zip, int index);
void zip_close_index(zip *zip);
const char *zip_name_index(zip *zip);
int zip_size_index(zip *zip);
int zip_isdir_index(zip *zip);
int zip_read_index(zip *zip, void **buffer);

int zip_dir(const char *path, const char *zipName);

#endif
