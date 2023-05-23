#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../inc/fifo.h"

void createFIFO(char* pathFIFO) {
	mkfifo(pathFIFO, 0666);
}

void writeFIFO(char* pathFIFO, void* buf, size_t size) {
	int fd = open(pathFIFO, O_WRONLY);
	if(write(fd, buf, size) == -1) {
		errExit2("write() failed");
	}
	close(fd);
}

void readFIFO(char* pathFIFO, const void* buf, size_t size) {
	int fd = open(pathFIFO, O_RDONLY);
	if(read(fd, buf, size) == -1) {
		errExit2("write() failed");
	}
	close(fd);
}

void errExit2(const char *msg) {
    perror(msg);
    exit(1);
}