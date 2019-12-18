#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "memory.h"

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
	parent[len-2] = '\0';
	len = strlen(parent);
	if (parent[len - 1] != c)
	{
		parent[len] = c;
		parent[len + 1] = '\0';
	}
	return parent;
}

int countBytes(const void* raw, int maxSize)
{
	const unsigned char* bytes = (unsigned char*)raw;
	int sz = 0;
	int i = 0;
	for(i = 0; i < maxSize; i++)
	{
		if(bytes[i] != 0)
			sz = i + 1;
	}
	return sz;
}

void replaceString(char *str, const char* find, const char* replace)
{
	char *pt = strstr(str, find);
	int lenF = strlen(find);
	int lenR = strlen(replace);
	int lenS = strlen(str);
	char *tmp = ALLOCATE(char, strlen(str));
	while(pt != NULL)
	{
		strcpy(tmp, pt + lenF);
		memcpy(pt, replace, lenR);
		strcpy(pt + lenR, tmp);
		pt = strstr(str, find);
	}
	FREE_ARRAY(char, tmp, lenS);
}