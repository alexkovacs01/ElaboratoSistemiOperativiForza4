
CFLAGS=-Wall -std=gnu99 -g
INCLUDES=-I./inc

SERVER_SRCS=src/server.c src/my_own_library_max_dim.c src/fifo.c
CLIENT_SRCS=src/client.c src/my_own_library_max_dim.c src/fifo.c

SERVER_OBJS=$(SERVER_SRCS:.c=.o)
CLIENT_OBJS=$(CLIENT_SRCS:.c=.o)

all: F4server F4client

F4server: $(SERVER_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

F4client: $(CLIENT_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

.c.o:
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	@rm -f src/*.o F4client F4server
	@echo "Removed object files and executables..."
