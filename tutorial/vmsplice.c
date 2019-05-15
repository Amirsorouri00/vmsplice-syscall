#define _GNU_SOURCE
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define ALIGN(buf)	(void *) (((unsigned long) (buf) + align_mask) & ~align_mask)
#define SPLICE_SIZE     256

static int align_mask = 65535;

int main(int argc, char *argv[]) {
    struct iovec local[1]; 
    unsigned char *buf1;
    buf1 = ALIGN(malloc(SPLICE_SIZE));
    ssize_t nread; 
    unsigned long nr_segs = 250;

    local[0].iov_base = buf1;
    local[0].iov_len = nr_segs;
    
    nread = vmsplice(STDIN_FILENO, local, nr_segs, SPLICE_F_MOVE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    return 0;
}