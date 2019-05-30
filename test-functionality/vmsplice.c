#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#define ALIGN(buf)	(char *) (((unsigned long) (buf) + align_mask) & ~align_mask)
static int align_mask = 65535;
#define SPLICE_SIZE     1024


int main(int argc, char *argv[]) {
    struct iovec local[1];
    unsigned char *buf1;
    unsigned char buf2[40] = "";
    printf("%p\n", buf1);
    // printf("%p\n", buf2);


    buf1 = ALIGN(malloc(SPLICE_SIZE));
    printf("%p\n", buf1);
    // buf1 = calloc(0, SPLICE_SIZE);
    // buf1 = "";
    ssize_t nread; 
    unsigned long nr_segs = 1024;

    local[0].iov_base = buf1;
    local[0].iov_len = 1024;

    printf("%p\n", local[0].iov_base);
    
    nread = vmsplice(STDIN_FILENO, local, 4, SPLICE_F_MOVE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }

    printf("%s\n", buf1);
    free(buf1);
    return 0;
}




