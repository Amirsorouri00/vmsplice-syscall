#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>    // time()

#define SPLICE_SIZE  1024
#define K_MULTIPLY   2  

static int align_mask = 65535;
static int splice_flags;

static inline int error(const char *n)
{
	perror(n);
	return -1;
}

static int __check_pipe(int pfd)
{
	struct stat sb;

	if (fstat(pfd, &sb) < 0)
		return error("stat");
	if (!S_ISFIFO(sb.st_mode))
		return 1;

	return 0;
}

static inline int check_input_pipe(void)
{
	if (!__check_pipe(STDIN_FILENO))
		return 0;

	fprintf(stderr, "stdin must be a pipe\n");
	return 1;
}

static inline int check_output_pipe(void)
{
	if (!__check_pipe(STDOUT_FILENO))
		return 0;

	fprintf(stderr, "stdout must be a pipe\n");
	return 1;
}

void random_char_selector(char* ch)
{
    time_t t;
    srand((unsigned) time(&t));

    int chars [58]= {};
    // char ch[58] = {};

    for (int i = 0;i<58;i++){
        chars[i] = (rand() % 58) + 65;
        ch[i] = chars[i];
        // printf("[%d, %c], ", chars[i], ch[i]);
    }
    // printf("\n");
    
    return;
}

void k_generator(char* chars){
    char ch[58] = {};
    random_char_selector(ch);

    for(int i = 0; i< 1024; i++){
        chars[i] = ch[(rand() % 58)];
    }
    return;
}

char** empty_allocator(){
    char** buf = malloc(K_MULTIPLY);

    for(int i = 0;i<K_MULTIPLY ;i++)
        buf[i] = malloc(SPLICE_SIZE);
    
    return buf;
}

void free_allocator(char** mem){
    for(int i = 0; i< K_MULTIPLY;i++){
        mem[i];
    }
    free(mem);
    return;
}

void test_string_askii()
{
    int a = 'a';
    int A = 'A';
    int z = 'z';
    int Z = 'Z';
    printf("a = %d, A = %d, z = %d, Z = %d\n", a,A,z,Z);
    return;
}

void fake_data_generator(char** container){
    char ch[1024] = {};
    for(int i = 0; i < K_MULTIPLY; i++){
        k_generator(ch);
        container[i] = ch;
        // printf("container[%d]: %s\n", i, container[i]);
    }
}

static int usage(char *name)
{
	fprintf(stderr, "%s: [-u(nalign)] [-g(ift)]| ...\n", name);
	return 1;
}

static int parse_options(int argc, char *argv[])
{
	int c, index = 1;

	while ((c = getopt(argc, argv, "ug")) != -1) {
		switch (c) {
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
		written = vmsplice(fd, &iov[idx], 1, splice_flags);
		printf("here[%d]: written=%d\n", page_counter, written);

		if (written <= 0)
			return error("vmsplice");

		page_counter--;
		if ((size_t) written >= iov[idx].iov_len) {
			int extra = written - iov[idx].iov_len;
            nread+=written;
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

	if (check_output_pipe())
		return usage(argv[0]);
    
    char** buf = empty_allocator();
    fake_data_generator(buf);

    printf("%s\n",buf[1]);    
    ssize_t nread = do_vmsplice(STDOUT_FILENO, buf);
    if (-1 == nread) {
        printf("errno = %d\n", errno);
        perror("vmsplice");
        return 1;
    }
    printf("%ld\n", nread);

    free_allocator(buf);
    
    return 0;
}
