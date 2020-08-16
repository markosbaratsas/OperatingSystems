#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

void doWrite (int fd, const char *buff, int len) {
	size_t idx = 0;
	ssize_t wcnt;
	// len = strlen(buff); its passed as a variable from previous function
	do {
		wcnt = write(fd,buff + idx, len - idx);
		if (wcnt == -1){ /* error */
			perror("In writing file");
			exit(1);
		}
		idx += wcnt;
	} while (idx < len);

}


void write_file(int fd, const char *infile) {
	// perform read 	
	int readFile;
	readFile = open(infile, O_RDONLY);
	if (readFile == -1){
		perror(infile);
		exit(1);
	}
	// perform read(...)
	char buff[1024];
	ssize_t rcnt;
	for (;;){
		rcnt = read(readFile,buff,sizeof(buff)-1);
		if (rcnt == 0) /* end‐of‐file */
			break;
		if (rcnt == -1){ /* error */
			perror(infile);
			exit(1);
		}
		buff[rcnt] = '\0';
		doWrite (fd, buff, strlen(buff));
	}
	//end read
	close(readFile);
}


int main(int argc, char **argv) {
	if(argc != 3 && argc !=4) {
		printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
		return 1;
	}
	int fd, oflags, mode;
	if(argc == 4) { 
		oflags = O_CREAT | O_WRONLY | O_TRUNC;
		mode = S_IRUSR | S_IWUSR;
		fd = open(argv[3], oflags, mode);
	}
	else {
		oflags = O_CREAT | O_WRONLY | O_TRUNC;
                mode = S_IRUSR | S_IWUSR;
                fd = open("fconc.out", oflags, mode);
	}
	write_file(fd, argv[1]);
	write_file(fd,argv[2]);
	
	return 0;	

}


















