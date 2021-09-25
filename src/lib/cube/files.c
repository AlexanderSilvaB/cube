#include "files.h"
#include "memory.h"
#include "mempool.h"
#include "vm.h"

ObjFile *openFile(char *fileName, char *mode)
{
    ObjFile *file = initFile();
    file->file = fopen(fileName, mode);
    file->path = fileName;
    file->mode = 0;
    if (strchr(mode, 'r') != NULL)
        file->mode |= FILE_MODE_READ;
    if (strchr(mode, 'w') != NULL)
        file->mode |= FILE_MODE_WRITE;
    if (strchr(mode, 'b') != NULL)
        file->mode |= FILE_MODE_BINARY;

    if (file->file == NULL)
    {
        return NULL;
    }

    file->isOpen = true;
    return file;
}

static bool writeFile(int argCount, bool newLine)
{
    if (argCount != 2)
    {
        runtimeError("write() takes 2 arguments (%d given)", argCount);
        return false;
    }

    Value data = pop();
    ObjFile *file = AS_FILE(pop());

    if (!FILE_CAN_WRITE(file))
    {
        runtimeError("File is not writable!");
        return false;
    }

    int wrote = 0;
    if (!FILE_IS_BINARY(file))
    {
        ObjString *string = AS_STRING(toString(data));
        wrote = fprintf(file->file, "%s", string->chars);
        if (newLine)
            fprintf(file->file, "\n");
    }
    else
    {
        ObjBytes *bytes = AS_BYTES(toBytes(data));
        if (bytes->length < 0)
        {
            runtimeError("Unsafe bytes are not writable!");
            return false;
        }
        wrote = fwrite(bytes->bytes, sizeof(char), bytes->length, file->file);
        if (newLine)
        {
            char c = '\n';
            fwrite(&c, sizeof(char), 1, file->file);
        }
    }

    fflush(file->file);

    push(NUMBER_VAL(wrote));

    return true;
}

static bool readFile(int argCount)
{
    if (argCount < 1 || argCount > 2)
    {
        runtimeError("read(|n|) takes 1 or 2 argument (%d given)", argCount);
        return false;
    }

    int sz = -1;
    size_t fileSize = 0;
    if (argCount > 1)
    {
        sz = AS_NUMBER(pop());
        fileSize = sz;
    }

    ObjFile *file = AS_FILE(pop());

    if (!FILE_CAN_READ(file))
    {
        runtimeError("File is not readable!");
        return false;
    }

    if (sz < 0)
    {
        size_t currentPosition = ftell(file->file);
        // Calculate file size
        fseek(file->file, 0L, SEEK_END);
        fileSize = ftell(file->file);
        rewind(file->file);

        // Reset cursor position
        if (currentPosition < fileSize)
            fileSize -= currentPosition;
        fseek(file->file, currentPosition, SEEK_SET);
    }

    char *buffer = (char *)mp_malloc(fileSize + 1);
    if (buffer == NULL)
    {
        runtimeError("Not enough memory to read \"%s\".\n", file->path);
        return false;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file->file);
    if (ferror(file->file))
    {
        runtimeError("Could not read file \"%s\".\n", file->path);
        return false;
    }

    buffer[bytesRead] = '\0';

    if (!FILE_IS_BINARY(file))
        push(OBJ_VAL(copyString(buffer, strlen(buffer))));
    else
        push(BYTES_VAL(buffer, bytesRead));
    mp_free(buffer);
    return true;
}

static bool readFileBytes(int argCount)
{
    if (argCount < 1 || argCount > 2)
    {
        runtimeError("readBytes(|n|) takes 0 or 1 argument (%d given)", argCount);
        return false;
    }

    int size = -1;
    size_t fileSize = 0;
    if (argCount > 1)
    {
        size = AS_NUMBER(pop());
        fileSize = size;
    }

    ObjFile *file = AS_FILE(pop());

    if (!FILE_CAN_READ(file))
    {
        runtimeError("File is not readable!");
        return false;
    }

    if (size < 0)
    {
        size_t currentPosition = ftell(file->file);
        // Calculate file size
        fseek(file->file, 0L, SEEK_END);
        fileSize = ftell(file->file);
        rewind(file->file);

        // Reset cursor position
        if (currentPosition < fileSize)
            fileSize -= currentPosition;
        fseek(file->file, currentPosition, SEEK_SET);
        size = fileSize;
    }

    char *buffer = (char *)mp_malloc(size + 1);
    if (buffer == NULL)
    {
        runtimeError("Not enough memory to read %d bytes from \"%s\".\n", size, file->path);
        return false;
    }

    size_t bytesRead = fread(buffer, sizeof(char), size, file->file);
    if (ferror(file->file))
    {
        runtimeError("Could not read %d bytes from file \"%s\".\n", size, file->path);
        return false;
    }

    push(BYTES_VAL(buffer, bytesRead));

    mp_free(buffer);
    return true;
}

static bool readLineFile(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("readLine() takes 1 argument (%d given)", argCount);
        return false;
    }

    char line[4096];

    ObjFile *file = AS_FILE(pop());

    if (!FILE_CAN_READ(file))
    {
        runtimeError("File is not readable!");
        return false;
    }

    if (fgets(line, 4096, file->file) != NULL)
        push(OBJ_VAL(copyString(line, strlen(line))));
    else
        push(OBJ_VAL(copyString("", 0)));

    return true;
}

static bool seekFile(int argCount)
{
    if (argCount < 2 || argCount > 3)
    {
        runtimeError("seek() takes 2 or 3 arguments (%d given)", argCount);
        return false;
    }

    int seekType = SEEK_SET;

    if (argCount == 3)
    {
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))
        {
            runtimeError("seek() arguments must be numbers");
            return false;
        }

        int seekTypeNum = AS_NUMBER(pop());

        switch (seekTypeNum)
        {
            case 0:
                seekType = SEEK_SET;
                break;
            case 1:
                seekType = SEEK_CUR;
                break;
            case 2:
                seekType = SEEK_END;
                break;
            default:
                seekType = SEEK_SET;
                break;
        }
    }

    if (!IS_NUMBER(peek(0)))
    {
        runtimeError("seek() argument must be a number");
        return false;
    }

    int offset = AS_NUMBER(pop());
    ObjFile *file = AS_FILE(pop());
    fseek(file->file, offset, seekType);

    push(NULL_VAL);
    return true;
}

static bool eofFile(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("eof() takes 1 argument (%d given)", argCount);
        return false;
    }

    ObjFile *file = AS_FILE(pop());

    bool eof = true;

    if (file->isOpen)
    {
        eof = feof(file->file);
    }

    push(BOOL_VAL(eof));
    return true;
}

static bool sizeFile(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("size() takes 1 argument (%d given)", argCount);
        return false;
    }

    ObjFile *file = AS_FILE(pop());

    int sz = -1;

    if (file->isOpen)
    {
        size_t currentPosition = ftell(file->file);

        fseek(file->file, 0L, SEEK_END);
        size_t fileSize = ftell(file->file);
        rewind(file->file);

        // Reset cursor position
        fseek(file->file, currentPosition, SEEK_SET);
        sz = fileSize;
    }

    push(NUMBER_VAL(sz));
    return true;
}

static bool posFile(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("pos() takes 1 argument (%d given)", argCount);
        return false;
    }

    ObjFile *file = AS_FILE(pop());

    int pos = -1;

    if (file->isOpen)
    {
        pos = ftell(file->file);
    }

    push(NUMBER_VAL(pos));
    return true;
}

static bool closeFile(int argCount)
{
    if (argCount != 1)
    {
        runtimeError("close() takes 1 argument (%d given)", argCount);
        return false;
    }

    ObjFile *file = AS_FILE(pop());

    if (file->isOpen)
    {
        fclose(file->file);
        file->isOpen = false;
    }

    push(NULL_VAL);
    return true;
}

bool fileMethods(char *method, int argCount)
{
    if (strcmp(method, "write") == 0)
        return writeFile(argCount, false);
    else if (strcmp(method, "writeLine") == 0)
        return writeFile(argCount, true);
    else if (strcmp(method, "read") == 0)
        return readFile(argCount);
    else if (strcmp(method, "readLine") == 0)
        return readLineFile(argCount);
    else if (strcmp(method, "readBytes") == 0)
        return readFileBytes(argCount);
    else if (strcmp(method, "seek") == 0)
        return seekFile(argCount);
    else if (strcmp(method, "pos") == 0)
        return posFile(argCount);
    else if (strcmp(method, "close") == 0)
        return closeFile(argCount);
    else if (strcmp(method, "eof") == 0)
        return eofFile(argCount);
    else if (strcmp(method, "size") == 0)
        return sizeFile(argCount);

    runtimeError("File has no method %s()", method);
    return false;
}