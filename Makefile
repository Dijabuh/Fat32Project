CC = gcc
STND = -std=c99

F32: main.c str_func.o
	$(CC) $^ -g -o F32.out $(STND)

str_func.o: str_func.c
	$(CC) $^ -g -c -o str_func.o $(STND)

clean:
	rm -f *.o *.out
