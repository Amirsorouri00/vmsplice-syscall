#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "splicer-benchmarking.h"


static int align_mask = 65535;
static int splice_flags;
clock_t start, end;


static int parse_options(int argc, char *argv[])
{
	int opt, index = 1;

	while ((opt = getopt(argc, argv, ":gu")) != -1) {
		switch (opt) {
			case 'u':
				splice_flags = SPLICE_F_MOVE;
				index++;
				break;
			case 'g':
				splice_flags = SPLICE_F_GIFT;
				index++;
				break;
			default:
				return -1;
		}
	}

	// optind is for the extra arguments 
    // which are not parsed 
	// optind--;
    for(; optind <= argc; optind++){      
        printf("extra arguments: %s\n", argv[optind-1]);  
    } 

	return index;
}

int do_vmsplice(int fd, char **data)
{
    int page_counter = K_MULTIPLY - 1;
	struct iovec iov[] = {
		{
			.iov_base = data[page_counter],
			.iov_len = SPLICE_SIZE,
		},
	};
	int written, idx = 0;
    int nread = 0;
	while (page_counter >= 0) {
		/*
		 * in a real app you'd be more clever with poll of course,
		 * here we are basically just blocking on output room and
		 * not using the free time for anything interesting.
		 */
		// if (poll(&pfd, 1, -1) < 0)
		// 	return error("poll");
		written = svmsplice(fd, &iov[idx], 1, splice_flags);
		
		// if(page_counter%20 == 0)
		// 	printf("here[%d]: nread=%d\n", page_counter, nread);


		if (written <= 0)
			return error("vmsplice");

		if ((size_t) written >= iov[idx].iov_len) {
			int extra = written - iov[idx].iov_len;
            nread+=written;
			page_counter--;
			iov[idx].iov_len = SPLICE_SIZE;
			iov[idx].iov_base = data[page_counter];
		} else {
			// printf("here");
            nread+=written;
			iov[idx].iov_len -= written;
			iov[idx].iov_base += written;
		}
	}

    if(nread < 0){
        return -1;
    }
	return nread;
}

int main(int argc, char *argv[]) 
{   
    if (parse_options(argc, argv) < 0)
		return usage(argv[0]);
    
	ssize_t nread;
	char* name;
	int pip[2], fd[2];
	if (pipe(pip) < 0) 
        exit(1); 
	if (pipe(fd) < 0) 
		exit(1); 

	pid_t   childpid;
	if((childpid = fork()) == -1)
	{
		perror("fork");
		exit(1);
	}
	if(childpid == 0)
	{
		/* Child process closes up input side of pipe */
		name = "child";
		close(pip[0]);
		close(fd[0]);

		char** data = initializer();

		/* Send "string" through the output side of pipe */
		start = clocker(0, name);   
		size_printer(name);
		nread = do_vmsplice(pip[1], data);
		end = clocker(1, name);
    	// printf("in child: number of written into the pipe = %ld\n", nread);

		double result = time_calc(end, start, name);
		write(fd[1], &result, sizeof(start));
	    
		free_allocator(data);
		exit(0);
	}
	else
	{
		/* Parent process closes up output side of pipe */
		close(fd[1]);
		close(pip[1]);
		name = "parent";

		char** data1 = empty_allocator();

		start = clocker(0, name);   
		size_printer(name);
		nread = do_vmsplice(pip[0], data1);
		end = clocker(1, name);
	    printf("in parent: number of reads from the pipe = %ld\n", nread);

		double res;
		double result = time_calc(end, start, name);
		read(fd[0], &res, sizeof(start));
		printf("---------------------------------------------\n");
		printf("time to write and read into the pipe by vmsplice = %f\n", res + result);

	    free_allocator(data1);
		exit(0);
	}
    
    return 0;
}
