#ifndef _DCUBE_DEBUG_H_
#define _DCUBE_DEBUG_H_

void init_debugger(int argc, const char *argv[]);
void finish_debugger();
bool hasBreakPoint(int line, const char *path);
void addBreakpoint(char *line);
void removeBreakpoint(char *line);

#endif
