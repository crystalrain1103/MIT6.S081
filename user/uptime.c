#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main() 
{
    int ticks = uptime();
    int seconds, minutes, hours, days;
    
    seconds = ticks / 10;
    minutes = seconds / 60;
    hours = minutes / 60;
    days = hours / 24;
    printf("%dd(s)%dh(s)%dm(s)%ds(s)\n", days, hours, minutes, seconds);
    
    exit(0);
}