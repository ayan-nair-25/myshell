CC = gcc
CFLAGS = -Wall 

mysh.o: mysh.c
	$(CC) $(CFLAGS) mysh.c ArrayList.c -o mysh
