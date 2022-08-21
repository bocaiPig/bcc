CFLAGS=-std=c11 -g -fno-common

CC=clang

bcc: main.o
	$(CC) -o bcc $(CFLAGS) main.o

test: bcc
	./test.sh

clean:
	rm -f bcc *.o *.s tmp* a.out
