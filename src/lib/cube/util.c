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

bool writeFile(const char *path, const char *buffer, size_t bufferSize, bool verbose)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL)
    {
        if (verbose)
            fprintf(stderr, "Could not open file \"%s\".\n", path);
        return false;
    }

    size_t bytesWriten = fwrite(buffer, sizeof(char), bufferSize, file);
    if (bytesWriten < bufferSize)
    {
        if (verbose)
            fprintf(stderr, "Could not write file \"%s\".\n", path);
        return false;
    }

    fclose(file);
    return true;
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

char *cube_strrstr(const char *s, const char *m)
{
    char *last = NULL;
    size_t n = strlen(m);

    while ((s = strchr(s, *m)) != NULL)
    {
        if (!strncmp(s, m, n))
            last = (char *)s;
        if (*s++ == '\0')
            break;
    }
    return last;
}

char *getFolder(const char *path)
{

    int c;
    const char *lastSlash = NULL;
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

char *findFile(const char *nameOrig)
{
    char *name = fixPath(nameOrig);
    if (name == NULL)
        return NULL;

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
    mp_free(name);
    return strPath;
}

char *findLibrary(const char *nameOrig)
{
    char *path = findFile(nameOrig);
    if (path == NULL)
    {
        char *name = (char *)mp_malloc(strlen(nameOrig) + 16);
        strcpy(name, "lib");
        strcat(name, nameOrig);
        path = findFile(name);
        mp_free(name);
    }
    return path;
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

bool isDir(const char *path)
{
    char *p = fixPath(path);

    struct stat statbuf;
    if (stat(p, &statbuf) != 0)
    {
        mp_free(p);
        return false;
    }
    mp_free(p);

    int rc = S_ISDIR(statbuf.st_mode);
    return (rc != 0);
}

bool isValidType(const char *name)
{
    if (strcmp(name, "null") == 0 || strcmp(name, "bool") == 0 || strcmp(name, "num") == 0 ||
        strcmp(name, "class") == 0 || strcmp(name, "module") == 0 || strcmp(name, "method") == 0 ||
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

bool findAndReadFile(const char *fileNameS, char **path, char **source, char **folderPath)
{
    char *fileName = fixPath(fileNameS);
    if (fileName == NULL)
        return false;

    char *s = readFile(fileName, false);
    int len = strlen(fileName);

    char *strPath = (char *)mp_malloc(sizeof(char) * (len + 1));
    strcpy(strPath, fileName);

    ObjString *folder = NULL;

    int index = 0;
    while (s == NULL && index < vm.paths->values.count)
    {
        folder = AS_STRING(vm.paths->values.values[index]);
        mp_free(strPath);
        strPath = mp_malloc(sizeof(char) * (folder->length + len + 2));
        strcpy(strPath, folder->chars);
        strcat(strPath, fileName);
        s = readFile(strPath, false);
        index++;
    }

    if (folderPath != NULL && folder != NULL)
    {
        *folderPath = mp_malloc(sizeof(char) * (folder->length + 1));
        strncpy(*folderPath, folder->chars, folder->length);
        (*folderPath)[folder->length] = '\0';
    }

    mp_free(fileName);
    *source = s;
    *path = strPath;
    return s != NULL;
}

bool findAndReadFileIncluding(const char *initialPath, const char *fileNameS, char **path, char **source,
                              char **folderPath)
{
    if (initialPath != NULL)
    {
        char *fileName = fixPath(fileNameS);
        if (fileName == NULL)
            return false;

        int len = strlen(fileName);
        char *strPath = mp_malloc(sizeof(char) * (strlen(initialPath) + len + 2));
        strcpy(strPath, initialPath);
        strcat(strPath, fileName);
        char *s = readFile(strPath, false);

        if (s != NULL)
        {

            if (folderPath != NULL)
            {
                *folderPath = mp_malloc(sizeof(char) * (strlen(initialPath) + 1));
                strncpy(*folderPath, initialPath, strlen(initialPath));
                (*folderPath)[strlen(initialPath)] = '\0';
            }

            mp_free(fileName);
            *source = s;
            *path = strPath;
            return true;
        }
        else
        {
            mp_free(fileName);
            mp_free(strPath);
        }
    }

    return findAndReadFile(fileNameS, path, source, folderPath);
}

char **listFiles(const char *pathRaw, int *size)
{
    char *path = fixPath(pathRaw);
    char **list = NULL;
    int N = 0;
#ifdef _MSC_VER
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char sPath[2048];
    sprintf(sPath, "%s\\*.*", path);
    if ((hFind = FindFirstFile(sPath, &fdFile)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            list = mp_realloc(list, sizeof(char *) * (N + 1));
            list[N] = mp_malloc(sizeof(char) * (strlen(fdFile.cFileName) + 1));
            strcpy(list[N], fdFile.cFileName);
            N++;
        } while (FindNextFile(hFind, &fdFile)); // Find the next file.

        FindClose(hFind); // Always, Always, clean things up!
    }

#else
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            list = mp_realloc(list, sizeof(char *) * (N + 1));
            list[N] = mp_malloc(sizeof(char) * (strlen(dir->d_name) + 1));
            strcpy(list[N], dir->d_name);
            N++;
        }
        closedir(d);
    }
#endif
    mp_free(path);
    *size = N;
    return list;
}

uint8_t *cube_bytebyte(const uint8_t *b1, const uint8_t *b2, size_t n1, size_t n2)
{
    uint8_t *ptr = NULL;
    for (size_t i = 0, j = 0; i < n1; i++)
    {
        if (b1[i] == b2[j])
        {
            if (j == 0)
                ptr = (uint8_t *)&b1[i];
            j++;
            if (j == n2)
                break;
        }
        else
        {
            ptr = NULL;
            j = 0;
        }
    }
    return ptr;
}

uint32_t hash(char *str)
{
    uint32_t hash = 2166136261u;
    size_t length = strlen(str);
    int c;

    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }

    return hash;
}