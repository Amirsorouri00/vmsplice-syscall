#define _GNU_SOURCE
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    struct iovec local; 
    int fd2 = open("../../random-files/20m-files/file.txt", O_RDONLY);
    char buf[1000]; 
    unsigned long nr_segs = 500;

    local.iov_base = buf;//(void *)0x10000; 
    local.iov_len = 500;

    ssize_t nread = vmsplice(fd2, &local, nr_segs, SPLICE_F_MORE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    return 0;
}

