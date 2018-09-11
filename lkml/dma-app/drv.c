#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>


void main()
{

char rdBuf[7];
int fd = open("/dev/som_mmap", O_RDONLY, 0);

char *mem = mmap(NULL, 1024 * 4096, PROT_READ, MAP_SHARED, fd, 0);

memcpy(rdBuf, mem, 7);
printf("Data Read:%s\n", rdBuf);
munmap(mem, 1024 * 4096);
close (fd);
}


