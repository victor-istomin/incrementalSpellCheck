#if defined(_WIN32) 
#define __STDC__ 1
#include <conio.h>
int getch() { return _getch(); }
#else
#include <termios.h>
#include <cstdio>

int getch()
{
    termios oldSettings = {0}, newSettings = {0};

    tcgetattr(0, &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= ~ICANON;             // disable buffered i/o
    newSettings.c_lflag &= ~ECHO;               // set no echo mode
    tcsetattr(0, TCSANOW, &newSettings);

    int ch = getchar();

    tcsetattr(0, TCSANOW, &oldSettings);

    return ch;
}

#endif
