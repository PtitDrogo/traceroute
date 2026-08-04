#include "../includes/traceroute.h"


struct termios orig_termios;

void disable_echoctl(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig_termios);
    t = orig_termios;
    t.c_lflag &= ~ECHOCTL;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void restore_termios(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}