
CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS=-lpthread

all : proj2

proj2 : proj2.o
	$(CC) $(CFLAGS) -o proj2 proj2.o $(LFLAGS)

proj2.o : proj2.c
	$(CC) $(CFLAGS) -c proj2.c

clean :
	rm proj2 *.o
