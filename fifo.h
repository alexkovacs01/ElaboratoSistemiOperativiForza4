#ifndef FIFO_H_ 
#define FIFO_H_

#include<unistd.h>
#include<sys/types.h>

struct bufFIFO{
	pid_t pidSender;						// pid_t of the sender
	unsigned short messageType;				// Type of message (1: Rquest to join as plaer, 2: Add new move, ...)
	int int1;										// Multiple usage
	int int2;										// Multiple usage
	char str1[50];							// Multiple usage
};

/// @param pathFIFO path for the FIFO
void createFIFO(char* pathFIFO);

/// @param pathFIFO path of the FIFO
/// @param buf Buffer of the data
/// @param size Size of the buffer
void writeFIFO(char* pathFIFO, void* buf, size_t size);

void readFIFO(char* pathFIFO, void* buf, size_t size,int *end_game);

void errExit2(const char *);

#endif