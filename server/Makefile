

.PHONY: all
all: aesdsocket

writer: writer.o
	$(CC) $(LDFLAGS) -o aesdsocket aesdsocket.o

writer.o: writer.c
	$(CC) $(CCFLAGS) -o aesdsocket.o -c aesdsocket.c
	
.PHONY: clean
clean:
	rm -f aesdsocket.o aesdsocket


