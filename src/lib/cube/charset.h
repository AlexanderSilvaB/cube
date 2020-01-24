#ifndef _CUBE_CHARSET_H_
#define _CUBE_CHARSET_H_

#include "common.h"

#ifdef UNICODE

#define CHAR wchar_t
#define PRINTF wprintf
#define PUTS(s) fputws(s, stdout)
#define PUTCHAR putwchar
#define PUTC(c) putwc(c, stdout)


#else

#define CHAR char
#define PRINTF printf
#define PUTS puts
#define PUTCHAR putchar
#define PUTC putc

#endif

#endif
