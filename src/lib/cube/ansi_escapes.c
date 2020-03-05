#ifdef _WIN32
#include <windows.h>
#endif

#include "ansi_escapes.h"
#include <stdio.h>

#ifdef _WIN32
// Some old MinGW/CYGWIN distributions don't define this:
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE stdoutHandle;
static DWORD outModeInit;

void setupConsole(void)
{
    DWORD outMode = 0;
    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdoutHandle == INVALID_HANDLE_VALUE)
    {
        exit(GetLastError());
    }

    if (!GetConsoleMode(stdoutHandle, &outMode))
    {
        exit(GetLastError());
    }

    outModeInit = outMode;

    // Enable ANSI escape codes
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(stdoutHandle, outMode))
    {
        exit(GetLastError());
    }
}

void restoreConsole(void)
{
    // Reset colors
    printf("\x1b[0m");

    // Reset console mode
    if (!SetConsoleMode(stdoutHandle, outModeInit))
    {
        exit(GetLastError());
    }
}
#else
void setupConsole(void)
{
}

void restoreConsole(void)
{
    // Reset colors
    printf("\x1b[0m");
}
#endif

enum ClearCodes
{
    CLEAR_FROM_CURSOR_TO_END,
    CLEAR_FROM_CURSOR_TO_BEGIN,
    CLEAR_ALL
};

void clearScreen(void)
{
    printf("\x1b[%dJ", CLEAR_ALL);
}

void clearScreenToBottom(void)
{
    printf("\x1b[%dJ", CLEAR_FROM_CURSOR_TO_END);
}

void clearScreenToTop(void)
{
    printf("\x1b[%dJ", CLEAR_FROM_CURSOR_TO_BEGIN);
}

void clearLine(void)
{
    printf("\x1b[%dK", CLEAR_ALL);
}

void clearLineToRight(void)
{
    printf("\x1b[%dK", CLEAR_FROM_CURSOR_TO_END);
}

void clearLineToLeft(void)
{
    printf("\x1b[%dK", CLEAR_FROM_CURSOR_TO_BEGIN);
}

void moveUp(int positions)
{
    printf("\x1b[%dA", positions);
}

void moveDown(int positions)
{
    printf("\x1b[%dB", positions);
}

void moveRight(int positions)
{
    printf("\x1b[%dC", positions);
}

void moveLeft(int positions)
{
    printf("\x1b[%dD", positions);
}

void moveTo(int row, int col)
{
    printf("\x1b[%d;%df", row, col);
}