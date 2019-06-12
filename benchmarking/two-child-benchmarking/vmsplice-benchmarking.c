#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/poll.h>



#include "../splicer-benchmarking.h"


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

int do_vmsplice(int fd, char **data, char* mode)
{
    // struct pollfd pfdout = { .fd = fd, .events = POLLOUT, };
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
		// if(mode == "write")
		// 	if (poll(&pfdout, 1, -1) < 0)
		// 		return error("poll");
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
	if (pipe(pip) < 0 || pipe(fd) < 0) 
        exit(1); 

	char** data1 = empty_allocator();
	char** data = initializer();

	pid_t   childpid;
	if((childpid = fork()) == -1)
	{
		perror("fork");
		exit(1);
	}
	if(childpid == 0)
	{
		/* Child process closes up input side of pipe */
		name = "First Child";
		close(pip[0]);
		close(fd[0]);

		/* Send "string" through the output side of pipe */
		start = clocker(0, name);   
		size_printer(name);
		nread = do_vmsplice(pip[1], data, "write");
		end = clocker(1, name);
		
		// double result = time_calc(end, start, name);
		printf("---------------------------------------------\n");
		write(fd[1], &start, sizeof(start));
		write(fd[1], &end, sizeof(start));
		// write(fd[1], &result, sizeof(start));
	    
		free_allocator(data);
		exit(0);
	}
	else
	{
		pid_t child2_pid;
        if((child2_pid = fork()) == -1)
        {
            error("fork");
            exit(1);
        }
		if(child2_pid == 0)
	    {
			/* Second Child process closes up output side of pipe */
            name = "Second Child";
			close(fd[1]);
			close(pip[1]);

			start = clocker(0, name);   
			// size_printer(name);
			nread = do_vmsplice(pip[0], data1, "read");
			end = clocker(1, name);

			printf("in %s: number of reads from the pipe = %ld\n", name, nread);
            printf("---------------------------------------------\n");

            clock_t first_start;
            clock_t first_end;
            read(fd[0], &first_start, sizeof(start));
            read(fd[0], &first_end, sizeof(start));
			double first_result = time_calc(first_end, first_start, "First Child");
            printf("---------------------------------------------\n");

			double result = time_calc(end, start, name);
            printf("---------------------------------------------\n");

            result = time_calc(end, first_start, "realtime_calculation");
		}
		else{
			/* Parent process closes up output side of pipe */
            int child1_status, child2_status;
            waitpid(childpid, &child1_status, 0);
            waitpid(child2_pid, &child2_status, 0);
            if (child1_status == 0 && child2_status == 0)  // Verify child process terminated without error.  
            {
                printf("The child processes terminated normally.\n");    
            }
            else{printf("The child process terminated with an error!.\n");}
			free_allocator(data);
            free_allocator(data1);
            exit(0);
		}
	}
    
    return 0;
}
