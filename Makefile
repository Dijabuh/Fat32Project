CC = gcc
STND = -std=c99

F32: main.c
	$(CC) $^ -g -o F32.out $(STND)

clean:
	rm -f *.o *.out
