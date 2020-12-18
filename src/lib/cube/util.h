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
bool writeFile(const char *path, const char *buffer, size_t bufferSize, bool verbose);
char *getFolder(const char *path);
int countBytes(const void *raw, int maxSize);
void replaceString(char *str, const char *find, const char *replace);
void replaceStringN(char *str, const char *find, const char *replace, int n);
char *findFile(const char *name);
char *findLibrary(const char *name);
bool existsFile(const char *name);
bool isDir(const char *path);
bool isValidType(const char *name);
char *getFileName(char *path);
char *getFileDisplayName(char *path);
uint64_t cube_clock();
void cube_wait(uint64_t t);
char *getEnv(const char *name);
char *getHome();
char *fixPath(const char *path);
bool findAndReadFile(const char *fileName, char **path, char **source);
char **listFiles(const char *pathRaw, int *size);
char *cube_strrstr(const char *s, const char *m);
uint8_t *cube_bytebyte(const uint8_t *b1, const uint8_t *b2, size_t n1, size_t n2);

#endif