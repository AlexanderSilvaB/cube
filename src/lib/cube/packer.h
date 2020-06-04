#ifndef CLOX_packer_h
#define CLOX_packer_h

typedef struct
{
    char title[256];
    char binary[256];
    char description[256];
} cube_binary_options;

extern cube_binary_options cube_bin_options;

bool pack(const char *source, const char *path, const char *output);

#endif
