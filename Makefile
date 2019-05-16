CC=gcc
CFLAGS=-I.

discover: discover.o
	$(CC) -o discover discover.o
