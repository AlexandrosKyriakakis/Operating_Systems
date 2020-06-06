#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int doWrite(int fd, const char *buff, int len){
	size_t idx = 0;
	ssize_t wcnt = 0;
	do {
		wcnt = write(fd,buff + idx, len - idx);
		if (wcnt == -1) {
			perror("write");
			return 1;
		}
		idx += wcnt;
	} while (idx < len);
}

int write_file(int fd, const char *infile){
	char buff[1024];
	ssize_t rcnt, fd_open; 
	fd_open = open(infile, O_RDONLY);
	if (fd_open == -1 ){
		perror("open");
		exit(1);
	}
	for (;;){
		rcnt = read(fd_open,buff,sizeof(buff)-1);
		if (rcnt == 0) 
			return 0;
		if (rcnt == -1){
			perror("read");
			return 1;
		}
		int len = strlen(buff);
		doWrite(fd,buff,len);
	}
	close(fd_open);

}
int main(int argc, char** argv) {
	if (argc < 3 || argc > 4){
		printf("Usage: ./fconc infile1 infile2 [outfile (default:fonc.out)]\n");
	}
	else{
		int fd_out, oflags, mode;
		oflags = O_CREAT | O_WRONLY | O_TRUNC;
		mode = S_IRUSR | S_IWUSR;
		if (argc == 3) {
			fd_out = open("fconc.out", oflags, mode);
		}
		else {
			fd_out = open(argv[3],oflags,mode);
		}
		if (fd_out == -1){
			perror("open");
			exit(1);
		}
		write_file(fd_out, argv[1]);
		write_file(fd_out, argv[2]);
		close(fd_out);
	}

	return 0;
}

