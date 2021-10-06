CC=gcc  #compiler
TARGET=memory #target file name
 
all:	memory.o 
	$(CC) memory.c -o $(TARGET)
 
clean:
	rm *.o $(TARGET)
