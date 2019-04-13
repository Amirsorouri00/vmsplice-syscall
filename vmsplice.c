#define _GNU_SOURCE
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>




int main(int argc, char *argv[]) {
    struct iovec local[1]; 
    char buf1[1000] = "amirhosse"; 
    int buf;
    ssize_t nread; 
    unsigned long nr_segs = 200;

    local[0].iov_base = &buf;//(void *)0x10000; 
    local[0].iov_len = 20;
    // int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // if (fd == -1) {
    //     perror("open");
    //     exit(EXIT_FAILURE);
    // }
    nread = vmsplice(STDIN_FILENO, local, nr_segs, SPLICE_F_MORE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    return 0;
}