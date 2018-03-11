#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main (void) 
{
    pid_t pid = 1231;
    int address = 0x1234;

    syscall(333, pid, address);
    return 0;
}
