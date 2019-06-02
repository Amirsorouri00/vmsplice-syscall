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
		printf("here[%d]: written=%d\n", page_counter, written);

		if (written <= 0)
			return error("vmsplice");

		if ((size_t) written >= iov[idx].iov_len) {
			int extra = written - iov[idx].iov_len;
            nread+=written;
			page_counter--;
			iov[idx].iov_len = SPLICE_SIZE;
			iov[idx].iov_base = data[page_counter];
		} else {
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

	// if (check_output_pipe())
	// 	return usage(argv[0]);
    
	char** data = initializer();
	char** data1 = empty_allocator();


	int pip[2];
	if (pipe(pip) < 0) 
        exit(1); 

    printf("%s\n",data[0]);    
    ssize_t nread = do_vmsplice(pip[1], data);
    ssize_t nread2 = do_vmsplice(pip[0], data1);
    printf("%s\n",data1[0]);    


    printf("%ld\n", nread);
    printf("%ld\n", nread2);

    free_allocator(data1);
    
    return 0;
}
