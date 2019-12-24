#ifndef _CUBE_EXT_H_
#define _CUBE_EXT_H_

#include <stdbool.h>
#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
#ifdef WIN_EXPORT
// Exporting...
#ifdef __GNUC__
#define EXPORTED __attribute__((dllexport))
#else
#define EXPORTED __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define EXPORTED __attribute__((dllimport))
#else
#define EXPORTED __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define NOT_EXPORTED
#else
#if __GNUC__ >= 4
#define EXPORTED __attribute__((visibility("default")))
#define NOT_EXPORTED __attribute__((visibility("hidden")))
#else
#define EXPORTED
#define NOT_EXPORTED
#endif
#endif

typedef struct
{
    unsigned int length;
    unsigned char *bytes;
} cube_native_bytes;

typedef union {
    bool _bool;
    double _number;
    char *_string;
    cube_native_bytes _bytes;
} cube_native_value;

#endif
