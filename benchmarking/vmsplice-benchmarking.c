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
    unsigned long nr_segs = 1024/2;

    local.iov_base = buf;
    local.iov_len = nr_segs;

    ssize_t nread = vmsplice(STDIN_FILENO, &local, 4, SPLICE_F_MOVE);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    printf("%s\n", buf);
    
    return 0;
}



// #define _GNU_SOURCE
// #include <errno.h>
// #include <stdio.h>
// #include <fcntl.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/uio.h>

// #define ALIGN(buf)	(char *) (((unsigned long) (buf) + align_mask) & ~align_mask)
// #define SPLICE_SIZE     1024

// static int align_mask = 65535;

// int main(int argc, char *argv[]) {
//     struct iovec local;
//     // unsigned char *buf[128];
//     // char *buf = malloc(SPLICE_SIZE);
//     char *buf = ALIGN(malloc(SPLICE_SIZE));

//     local.iov_base = buf;
//     local.iov_len = 512;

//     ssize_t nread = vmsplice(STDIN_FILENO, &local, 4, SPLICE_F_MOVE);
//     if (-1 == nread) {
//         printf("errno = %d\n", errno);
//         perror("vmsplice");
//         return 1;
//     }
//     return 0;
// }