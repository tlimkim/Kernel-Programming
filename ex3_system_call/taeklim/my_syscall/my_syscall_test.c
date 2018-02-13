#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main (void) 
{
    syscall(333);
    return 0;
}
