#ifndef CUBE_util_h
#define CUBE_util_h

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#define MakeDir(path, mode) _mkdir(path)
#define RmDir _rmdir
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define GetCurrentDir getcwd
#define MakeDir(path, mode) mkdir(path, mode)
#define RmDir rmdir
#endif

int readFd(int fd, int size, char *buff);
int writeFd(int fd, int size, char *buff);
int readFileRaw(FILE *fd, int size, char *buff);
int writeFileRaw(FILE *fd, int size, char *buff);
char *readFile(const char *path, bool verbose);
char *getFolder(const char *path);
int countBytes(const void *raw, int maxSize);
void replaceString(char *str, const char *find, const char *replace);
void replaceStringN(char *str, const char *find, const char *replace, int n);
char *findFile(const char *name);
bool existsFile(const char *name);
bool isValidType(const char *name);
char *getFileName(char *path);
char *getFileDisplayName(char *path);
uint64_t cube_clock();
void cube_wait(uint64_t t);
char *getEnv(const char *name);
char *getHome();
char *fixPath(const char *path);
bool findAndReadFile(const char *fileName, char **path, char **source);

#endif