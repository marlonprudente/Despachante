# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -I.
	
despachante: pingpong.o queue.o pingpong-dispatcher.o
	$(CC) -o despachante pingpong.c queue.c pingpong-dispatcher.c
	
clean:
	rm *.o despachante