#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <Windows.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif
#include "memory.h"
#include "mempool.h"
#include "util.h"
#include "vm.h"


int readFd(int fd, int size, char *buff)
{
#ifdef _MSC_VER
    return -1;
#else
    return read(fd, buff, size);
#endif
}

int writeFd(int fd, int size, char *buff)
{
#ifdef _MSC_VER
    return -1;
#else
    return write(fd, buff, size);
#endif
}

int readFileRaw(FILE *fd, int size, char *buff)
{
#ifdef _MSC_VER
    return -1;
#else
    return fread(buff, sizeof(char), size, fd);
#endif
}

int writeFileRaw(FILE *fd, int size, char *buff)
{
#ifdef _MSC_VER
    return -1;
#else
    return fwrite(buff, sizeof(char), size, fd);
#endif
}

char *readFile(const char *path, bool verbose)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        if (verbose)
            fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)mp_malloc(fileSize + 1);
    if (buffer == NULL)
    {
        if (verbose)
            fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        if (verbose)
            fprintf(stderr, "Could not read file \"%s\".\n", path);
        return NULL;
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

const char *cube_strchr(const char *s, int c1, int c2, int *c)
{
    if (s == NULL)
    {
        return NULL;
    }
    if ((c1 > 255) || (c1 < 0))
    {
        return NULL;
    }
    int s_len;
    int i;
    s_len = strlen(s);
    for (i = 0; i < s_len; i++)
    {
        if ((char)c1 == s[i])
        {
            *c = c1;
            return (const char *)&s[i];
        }
        if (c2 >= 0 && c2 <= 255)
        {
            if ((char)c2 == s[i])
            {
                *c = c2;
                return (const char *)&s[i];
            }
        }
    }
    return NULL;
}

const char *cube_strrchr(const char *s, int c1, int c2, int *c)
{
    if (s == NULL)
    {
        return NULL;
    }
    if ((c1 > 255) || (c1 < 0))
    {
        return NULL;
    }
    int s_len;
    int i;
    s_len = strlen(s);
    for (i = s_len - 1; i >= 0; i--)
    {
        if ((char)c1 == s[i])
        {
            *c = c1;
            return (const char *)&s[i];
        }
        if (c2 >= 0 && c2 <= 255)
        {
            if ((char)c2 == s[i])
            {
                *c = c2;
                return (const char *)&s[i];
            }
        }
    }
    return NULL;
}

char *getFolder(const char *path)
{

    int c;
    char *lastSlash = NULL;
    char *parent = NULL;
    lastSlash = cube_strrchr(path, '/', '\\', &c); // you need escape character
    if (lastSlash == NULL)
        return NULL;
    int len = strlen(path) - strlen(lastSlash) + 3;
    parent = mp_malloc(sizeof(char) * len);
    strncpy(parent, path, len - 2);
    parent[len - 2] = '\0';
    len = strlen(parent);
    if (parent[len - 1] != (char)c)
    {
        parent[len] = (char)c;
        parent[len + 1] = '\0';
    }
    return parent;
}

int countBytes(const void *raw, int maxSize)
{
    const unsigned char *bytes = (unsigned char *)raw;
    int sz = 0;
    int i = 0;
    for (i = 0; i < maxSize; i++)
    {
        if (bytes[i] != 0)
            sz = i + 1;
    }
    return sz;
}

void replaceString(char *str, const char *find, const char *replace)
{
    char *pt = strstr(str, find);
    int lenF = strlen(find);
    int lenR = strlen(replace);
    int lenS = strlen(str);
    char *tmp = ALLOCATE(char, strlen(str));
    while (pt != NULL)
    {
        strcpy(tmp, pt + lenF);
        memcpy(pt, replace, lenR);
        strcpy(pt + lenR, tmp);
        pt = strstr(str, find);
    }
    FREE_ARRAY(char, tmp, lenS);
}

void replaceStringN(char *str, const char *find, const char *replace, int N)
{
    char *pt = strstr(str, find);
    int lenF = strlen(find);
    int lenR = strlen(replace);
    int lenS = strlen(str);
    char *tmp = ALLOCATE(char, strlen(str));
    int n = 0;
    while (pt != NULL & n < N)
    {
        strcpy(tmp, pt + lenF);
        memcpy(pt, replace, lenR);
        strcpy(pt + lenR, tmp);
        pt = strstr(str, find);
        n++;
    }
    FREE_ARRAY(char, tmp, lenS);
}

char *findFile(const char *name)
{
    char *strPath = mp_malloc(sizeof(char) * (strlen(name) + 2));
    strcpy(strPath, name);

    FILE *file = fopen(strPath, "r");
    int index = 0;
    while (file == NULL && index < vm.paths->values.count)
    {
        char *folder = AS_CSTRING(vm.paths->values.values[index]);
        mp_free(strPath);
        strPath = mp_malloc(sizeof(char) * (strlen(folder) + strlen(name) + 2));
        strcpy(strPath, folder);
        strcat(strPath, name);
        file = fopen(strPath, "r");
        index++;
    }
    if (file == NULL)
    {
        mp_free(strPath);
        strPath = NULL;
    }
    else
        fclose(file);
    return strPath;
}

bool existsFile(const char *name)
{
    FILE *file = fopen(name, "r");
    if (file == NULL)
    {
        return false;
    }
    fclose(file);
    return true;
}

bool isValidType(const char *name)
{
    if (strcmp(name, "none") == 0 || strcmp(name, "bool") == 0 || strcmp(name, "num") == 0 ||
        strcmp(name, "class") == 0 || strcmp(name, "package") == 0 || strcmp(name, "method") == 0 ||
        strcmp(name, "func") == 0 || strcmp(name, "instance") == 0 || strcmp(name, "native") == 0 ||
        strcmp(name, "str") == 0 || strcmp(name, "file") == 0 || strcmp(name, "bytes") == 0 ||
        strcmp(name, "list") == 0 || strcmp(name, "dict") == 0 || strcmp(name, "nativefunc") == 0 ||
        strcmp(name, "nativelib") == 0)
        return true;
    return false;
}

char *getFileName(char *path)
{
    int start = 0;
    int len = strlen(path);
    for (int i = 0; i < len; i++)
    {
        if (path[i] == '\\' || path[i] == '/')
        {
            start = i + 1;
        }
    }
    return &path[start];
}

char *getFileDisplayName(char *path)
{
    char *fileName = getFileName(path);
    int end = strlen(fileName);
    for (int i = 0; i < end; i++)
    {
        if (fileName[i] == '.')
        {
            end = i;
            break;
        }
    }
    char *name = (char *)mp_malloc(sizeof(char) * (end + 1));
    memcpy(name, fileName, end);
    name[end] = '\0';
    return name;
}

uint64_t cube_clock()
{
#ifdef _MSC_VER
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    double nanoseconds_per_count = 1.0e9 / (double)freq.QuadPart;

    LARGE_INTEGER time1;
    QueryPerformanceCounter(&time1);

    uint64_t ns = time1.QuadPart * nanoseconds_per_count;
    return ns;
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    uint64_t ret = t.tv_sec * 1e9 + t.tv_nsec;
    return ret;
#endif
}

#ifdef _MSC_VER
void usleep(__int64 usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

void cube_wait(uint64_t t)
{
    usleep(t);
}

char *getEnv(const char *name)
{
    if (strcmp(name, "INDEPENDENT_HOME") == 0)
    {
        return getHome();
    }
    return getenv(name);
}

char *getHome()
{
#ifdef WIN32
    char *home = getEnv("UserProfile");
    if (home == NULL)
    {
        char *homeDrive = getEnv("HOMEDRIVE");
        char *homePath = getEnv("HOMEPATH");
        if (homeDrive != NULL && homePath != NULL)
        {
            home = (char *)mp_malloc(sizeof(char) * (strlen(homeDrive) + strlen(homePath) + 2));
            home[0] = '\0';
            strcpy(home, homeDrive);
            strcat(home, homePath);
        }
    }

#else
    char *home = getEnv("HOME");
#endif
    return home;
}

char *fixPath(const char *path)
{
    char *home = getHome();
    char *pathStr = (char *)mp_malloc(sizeof(char) * 256);
    strcpy(pathStr, path);
    if (home != NULL)
    {
        replaceString(pathStr, "~", home);
    }
    return pathStr;
}