#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

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
	len = strlen(parent);
	if (parent[len - 1] != c)
	{
		parent[len] = c;
		parent[len + 1] = '\0';
	}
	return parent;
}