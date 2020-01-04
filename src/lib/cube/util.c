#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "memory.h"
#include "vm.h"

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

	char *buffer = (char *)malloc(fileSize + 1);
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

char *getFolder(const char *path)
{
	char c = '/';
	char *lastSlash = NULL;
	char *parent = NULL;
	lastSlash = strrchr(path, c); // you need escape character
	int len = strlen(path) - strlen(lastSlash) + 3;
	parent = malloc(sizeof(char) * len);
	strncpy(parent, path, len - 2);
	parent[len - 2] = '\0';
	len = strlen(parent);
	if (parent[len - 1] != c)
	{
		parent[len] = c;
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

char *findFile(const char *name)
{
	char *strPath = malloc(sizeof(char) * (strlen(name) + 2));
	strcpy(strPath, name);

	FILE *file = fopen(strPath, "r");
	int index = 0;
	while (file == NULL && index < vm.paths->values.count)
	{
		char *folder = AS_CSTRING(vm.paths->values.values[index]);
		free(strPath);
		strPath = malloc(sizeof(char) * (strlen(folder) + strlen(name) + 2));
		strcpy(strPath, folder);
		strcat(strPath, name);
		file = fopen(strPath, "r");
		index++;
	}
	if (file == NULL)
	{
		free(strPath);
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

bool isValidType(const char* name)
{
	if(	strcmp(name, "none") == 0 ||
		strcmp(name, "bool") == 0 ||
		strcmp(name, "num") == 0 ||
		strcmp(name, "class") == 0 ||
		strcmp(name, "package") == 0 ||
		strcmp(name, "method") == 0 ||
		strcmp(name, "func") == 0 ||
		strcmp(name, "instance") == 0 ||
		strcmp(name, "native") == 0 ||
		strcmp(name, "str") == 0 ||
		strcmp(name, "file") == 0 ||
		strcmp(name, "bytes") == 0 ||
		strcmp(name, "list") == 0 ||
		strcmp(name, "dict") == 0 ||
		strcmp(name, "nativefunc") == 0 ||
		strcmp(name, "nativelib") == 0)
		return true;
	return false;
}

char *getFileName(char *path)
{
	int start = 0;
	int len = strlen(path);
	for(int i = 0; i < len; i++)
	{
		if(path[i] == '\\' || path[i] == '/')
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
	for(int i = 0; i < end; i++)
	{
		if(fileName[i] == '.')
		{
			end = i;
			break;
		}
	}
	char *name = (char*)malloc(sizeof(char) * (end + 1));
	memcpy(name, fileName, end);
	name[end] = '\0';
	return name;
}