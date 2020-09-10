#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "packer.h"
#include "util.h"

#ifdef _WIN32
#include <windows.h>
#endif

cube_binary_options cube_bin_options;

bool pack(const char *source, const char *path, const char *output)
{
    memset(&cube_bin_options, '\0', sizeof(cube_bin_options));

    printf("Compiling to byte code: ...\n");
    bool forceInclude = vm.forceInclude;
    vm.forceInclude = true;
    ObjFunction *fn = compile(source, path);
    vm.forceInclude = forceInclude;

    if (fn == NULL)
    {
        printf("Could not compile the code!\n");
        return false;
    }

    FILE *file = tmpfile();
    if (file == NULL)
    {
        printf("Could not write the binary!\n");
        return false;
    }

    if (!initByteCode(file))
    {
        fclose(file);
        printf("Could not write the binary!\n");
        return false;
    }

    if (!writeByteCode(file, OBJ_VAL(fn)))
    {
        fclose(file);
        printf("Could not write the binary!\n");
        return false;
    }

    if (!finishByteCode(file))
    {
        fclose(file);
        printf("Could not write the binary!\n");
        return false;
    }

    fseek(file, 0L, SEEK_END);
    size_t sz = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *buffer = (char *)malloc(sz);
    if (buffer == NULL)
    {
        fclose(file);
        printf("Not enough memory to compile binary!\n");
        return false;
    }

    size_t bytesRead = fread(buffer, sizeof(char), sz, file);
    if (bytesRead < sz)
    {
        fclose(file);
        printf("Not enough memory to compile binary!\n");
        return false;
    }

    fclose(file);

    size_t codeSize = (sz * 6) + 1024;
    char *code = malloc(codeSize);
    memset(code, '\0', codeSize);

    printf("Generating binary source: ...\n");

    strcpy(code, "#include <stdio.h>\n");
#ifdef _WIN32
    strcpy(code, "#include <windows.h>\n");
#endif
    strcat(code, "extern \"C\"\n{\n");
    strcat(code, "\tint runCode(const char *source, const char *path, int argc, const char *argv[]);\n");
    strcat(code, "\tvoid startCube(int argc, const char *argv[]);\n");
    strcat(code, "\tvoid stopCube();\n");
    strcat(code, "}\n");
    strcat(code, "\nint main(int argc, const char *argv[])\n{\n");
    if (cube_bin_options.title[0] != '\0')
    {
#ifdef _WIN32
        sprintf(code + strlen(code), "\tSetConsoleTitle(TEXT(\"%s\"));\n", cube_bin_options.title);
#else
        sprintf(code + strlen(code), "\tprintf(\"%%c]0;%%s%%c\", '\033', \"%s\", '\007');\n", cube_bin_options.title);
#endif
    }
    strcat(code, "\tunsigned char src[] = {");
    int i = 0;
    for (i = 0; i < sz; i++)
    {
        sprintf(code + strlen(code), " 0x%x,", (unsigned char)buffer[i]);
    }
    strcat(code, " 0x0 };\n");
    strcat(code, "\tstartCube(argc, argv);\n");
    strcat(code, "\tint rc = runCode((char*)src, \"temp.cube\", argc, argv);\n");
    strcat(code, "\tstopCube();\n\treturn rc;\n}");

    free(buffer);

    codeSize = strlen(code);

#ifdef _WIN32
    TCHAR szWideString[MAX_PATH];
    if (GetTempPath(MAX_PATH, szWideString) == 0)
    {
        szWideString[0] = '\0';
    }
    char codeName[MAX_PATH + 16];
    if (szWideString[0] != '\0')
    {
        size_t nNumCharConverted;
        wcstombs_s(&nNumCharConverted, codeName, MAX_PATH, szWideString, MAX_PATH);
        strcat(codeName, "/cube_code.c");
    }
    else
        strcpy(codeName, ".tmp.c");
#else
    char *codeName = "/tmp/cube_code.c";
#endif

    if (!writeFile(codeName, code, codeSize, true))
    {
        free(code);
        printf("Could not write the binary!\n");
        return false;
    }
    free(code);

#ifdef _WIN32
    char *ext = ".exe";
#else
    char *ext = "";
#endif

    char *out = NULL;
    if (output == NULL)
    {
        out = malloc(strlen(path) * 2);
        if (cube_bin_options.binary[0] == '\0')
        {
            strcpy(out, path);
            if (strstr(out, ".cube") != NULL)
                replaceStringN(out, ".cube", ext, 1);
            else
                strcat(out + strlen(out), ext);
        }
        else
        {
            strcpy(out, cube_bin_options.binary);
        }
    }
    else
    {
        out = malloc(strlen(output) + 1);
        strcpy(out, output);
    }

    printf("Compiling to native code: ...\n");

    char *cmd = malloc(1024);
#ifdef _WIN32
    sprintf(cmd, "g++ %s -o %s -l:libcube.a -lpthread -l:libcube_linenoise.a -l:libcubeffi.a", codeName, out);
#else
    sprintf(cmd, "g++ %s -o %s -l:libcube.a -lm -ldl -lpthread -l:libcube_linenoise.a -l:libcubeffi.a -l:libcjson.a",
            codeName, out);
#endif
    // printf("%s\n", cmd);
    system(cmd);
    free(cmd);

    printf("%s compiled to %s\n", path, out);

    free(out);

    return true;
}