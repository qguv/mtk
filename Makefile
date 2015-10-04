CC=gcc 
CFLAGS=-Wall --std=c11
main: main.o

clean:
	rm -f main main.o
