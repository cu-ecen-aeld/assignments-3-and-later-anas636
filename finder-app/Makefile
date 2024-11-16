

.PHONY: all
all: writer

writer: writer.o
	$(CC) $(LDFLAGS) -o writer writer.o

writer.o: writer.c
	$(CC) $(CCFLAGS) -o writer.o -c writer.c
	
.PHONY: clean
clean:
	rm -f writer.o writer


