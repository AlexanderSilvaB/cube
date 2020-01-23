#ifndef _CUBE_LINENOISE_H_
#define _CUBE_LINENOISE_H_

#include <cube/cubeext.h>

#define LINENOISE_MAX_LINE 4096

void linenoise_init();
bool linenoise_load_history(const char *path);
bool linenoise_save_history(const char *path);
bool linenoise_add_history(const char *line);
bool linenoise_set_history_max_len(size_t len);
void linenoise_set_multiline(bool multiLineMode);
bool linenoise_read_line(const char *prompt, char *line);
void linenoise_add_keyword(const char *keyword);
void linenoise_add_keyword_2(const char *keyword, const char *property);

#endif
