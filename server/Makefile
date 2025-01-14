

CC ?= $(CROSS_COMPILE)gcc
CCFLAGS ?= -g -Wall -Werror


.PHONY: all
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CCFLAGS) -o aesdsocket aesdsocket.o

aesdsocket.o: aesdsocket.c queue.h
	$(CC) $(CCFLAGS) -o aesdsocket.o -c aesdsocket.c
	
.PHONY: clean
clean:
	rm -f aesdsocket.o aesdsocket


