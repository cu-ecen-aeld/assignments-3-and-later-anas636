
CC=$(CROSS_COMPILE)gcc


.PHONY: all
all: writer

writer: writer.o
	$(CC) -o writer writer.o

writer.o: writer.c
	$(CC) -o writer.o -c writer.c -g -Wall

	
.PHONY: clean
clean:
	rm -f *.o writer


