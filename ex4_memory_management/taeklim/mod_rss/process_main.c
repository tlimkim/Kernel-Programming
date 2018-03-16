#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h> // system call

int main (int argc, char **argv)
{
    int page_num, page_size;
    int dummy = 0;
    int *a;
    int x = 0; int i, j;
    struct rusage before_usage, end_usage;

    pid_t pid;

    page_num = atoi(argv[1]);
    pid = getpid();
    page_size = getpagesize();

    printf("pid: %d, page_size: %d \n", pid, page_size);

    a = aligned_alloc(page_size, page_size * page_num);
    if (a == NULL) {
	fprintf(stderr, "Failed allocating memory \n");
	exit(EXIT_FAILURE);
    }

    printf("Address of array alloced: %lx \n", a);

    // step 1 ---> generating N faults(MIN)
    getrusage(RUSAGE_SELF, &before_usage);
    for (i = 0; i < page_size * page_num / sizeof(int); i += (page_size/sizeof(int))) {
	a[i] = 0;
    }
    getrusage(RUSAGE_SELF, &end_usage);

    while (1) {
	sleep(10000);
    }
/*
    for (i = 0; i < page_size * page_num / sizeof(int); i += (page_size/sizeof(int))) {
	printf("vm address: %lx \n", &a[i]);
    }
*/
    return 0;
}
