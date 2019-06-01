#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>


int main(int argc, char *argv[]) {
    struct iovec local;
    char buf[100] = "";
    unsigned long nr_segs = 100;

    local.iov_base = buf;
    local.iov_len = nr_segs;

    ssize_t nread = vmsplice(STDIN_FILENO, &local, 1, SPLICE_F_MOVE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    printf("%s\n", buf);
    
    return 0;
}




