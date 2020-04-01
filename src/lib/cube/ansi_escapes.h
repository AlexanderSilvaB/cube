#ifndef _CUBE_ANSI_ESCAPES_H_
#define _CUBE_ANSI_ESCAPES_H_

enum Colors
{
    RESET_COLOR,
    BLACK_TXT = 30,
    RED_TXT,
    GREEN_TXT,
    YELLOW_TXT,
    BLUE_TXT,
    MAGENTA_TXT,
    CYAN_TXT,
    WHITE_TXT,

    BLACK_BKG = 40,
    RED_BKG,
    GREEN_BKG,
    YELLOW_BKG,
    BLUE_BKG,
    MAGENTA_BKG,
    CYAN_BKG,
    WHITE_BKG
};

void setupConsole(void);
void restoreConsole(void);
void clearScreen(void);
void clearScreenToBottom(void);
void clearScreenToTop(void);
void clearLine(void);
void clearLineToRight(void);
void clearLineToLeft(void);

#endif