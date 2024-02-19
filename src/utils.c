
#include "utils.h"

void exec(const char* bin)
{
    if(fork() == 0)
    {
        setsid();
        execlp(bin, "");
    }
}
