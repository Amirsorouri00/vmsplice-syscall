#define _GNU_SOURCE
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/poll.h>

#define SPLICE_SIZE     1024
#define ALIGN(buf)	(void *) (((unsigned long) (buf) + align_mask) & ~align_mask)

static int align_mask = 65535;
// int filelen = 2048;


ssize_t vmsplice_transfer(int pipe_out, char* data, size_t length);
ssize_t splice_transfer(int fd_in, loff_t *off_in, int fd_out,
                      loff_t *off_out, size_t len, unsigned int flags);
ssize_t tee_cpy(int fd_in, int fd_out, int len, int flag);



int main (int argc, int *argv[]){
    int pip[2];
    int childpid[2] = {};
    unsigned char *b1, *b2;
    b1 = ALIGN(malloc(SPLICE_SIZE + align_mask));
    // b2 = ALIGN(malloc(SPLICE_SIZE + align_mask));

    if (pipe(pip) < 0)
        exit(1);

    if((childpid[0] = fork()) == -1)
    {
        perror("fork-1");
        exit(1);
    }

    if(childpid[0] == 0)
    {
        /**
         *  #Child-1
         *  Child process closes up input side of pipe 
         *  Splice() and Tee() workout together in this section
         */
        close(pip[0]);

        /* Send "string" through the output side of pipe */
        write(pip[1], 'string', (strlen('string')+1));
        exit(0);
    }
    else
    {
        /* Parent process closes up output side of pipe */
        if((childpid[1] = fork()) == -1)
        {
            perror("fork-2");
            exit(1);
        }
        if(childpid[1] == 0)
        {
            /**
             *  #Child-2
             *  Child process closes up input side of pipe 
             *  Vmsplice() works in this section
             */
            
            close(pip[1]);

            /* Send "string" through the output side of pipe */
            write(pip[0], 'string', (strlen('string')+1));
            exit(0);
        }
        else{
            close(pip[1]);
            close(pip[0]);

            /* Read in a string from the pipe */
            // int nbytes = read(pip[0], 'readbuffer', sizeof('readbuffer'));
            // printf("Received string: %s", 'readbuffer');
            exit(0);
        }
        
    }


    return 0;
}

/**
 * data is aligned to page boundaries,
 * and length is a multiple of the page size
 */ 
ssize_t vmsplice_transfer(int pipe_out, char* b1, size_t len)
{
    struct pollfd pfd = { .fd = pipe_out, .events = POLLOUT, };
	struct iovec iov[] = {
		{
			.iov_base = b1,
			.iov_len = len / 2, // #Notice
		}
	};
	int written, idx = 0;

	while (len) {
		/*
		 * In a real app you'd be more clever with poll of course,
		 * here we are basically just blocking on output room and
		 * not using the free time for anything interesting.
		 */
		if (poll(&pfd, 1, -1) < 0)
			return error("poll");

		written = vmsplice(pipe_out, &iov[idx], 2 - idx, SPLICE_F_GIFT);

		if (written <= 0)
			return error("vmsplice");

		len -= written;
		if ((size_t) written >= iov[idx].iov_len) {
			int extra = written - iov[idx].iov_len;

			idx++;
			iov[idx].iov_len -= extra;
			iov[idx].iov_base += extra;
		} else {
			iov[idx].iov_len -= written;
			iov[idx].iov_base += written;
		}
	}

	return written;
}

ssize_t splice_transfer(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags)
{
    size_t nread;
    while(len > 0){
        nread = splice(fd_in, off_in, fd_out, off_out, len , SPLICE_F_MOVE);
        if (nread < 0){
            perror("splice");
            break;
        }
        len -= nread;
    }
    return (ssize_t)nread;
    // int nread = splice(fd_in, NULL, fd_out,
    //         NULL, len, SPLICE_F_MOVE);

    // if (-1 == nread) {
    //     printf("errno = %d\n", errno);
    //     perror("splice");
    //     exit(EXIT_FAILURE);
    // }
    // return 1;
}
                      


long tee_cpy(int fd_in, int fd_out, int filelen, int flag)
{
    long cnt = 0;

    do{
        // tee stdin to stdout 
        int len = tee(fd_in, fd_out, INT_MAX, SPLICE_F_NONBLOCK);
        if (len < 0){
            if(errno == EAGAIN)
                continue;
            perror("tee");
            exit(EXIT_FAILURE);
        }
        else{
            if(len == 0)
                break;
            // Consume stdin by splicing it to a file. 
            cnt += splice_transfer(fd_in, NULL, fd_out, NULL, len, SPLICE_F_MOVE);
        }
        
    } while (1);
    
    return cnt;



     // do{
        // tee stdin to stdout
    // int len = tee(STDIN_FILENO, STDOUT_FILENO, filelen, SPLICE_F_NONBLOCK);
    // filelen -= len;
    // if (len < 0){
    //     // printf("%d", len);
    //     // if(errno == EAGAIN)
    //     //     continue;
    //     perror("tee");
    //     exit(EXIT_FAILURE);
    // }
    // else{
    //     if(filelen == 0 || len == 0)
    //         return len;
    //         // break; 
    //     cnt += len;
    //     printf("\n%ld\n", cnt);
    // }
    // // } while (1);
}