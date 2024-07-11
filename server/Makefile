CC=gcc
CFLAGS= -g -Wall -Werror

ifdef CROSS_COMPILE
	CC = ${CROSS_COMPILE}gcc	
endif

all: aesdsocket

writer: aesdsocket.o
		$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket

writer.o:writer.c
		$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o

clean:
	rm -f *.o aesdsocket *.elf *.map
