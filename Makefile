CC = gcc
STND = -std=c99

project3: main.c str_func.o bpb.o cd.o cluster.o create.o ls.o mkdir.o mv.o size.o rm.o cp.o open.o rw.o
	$(CC) $^ -g -o $@ $(STND)

str_func.o: str_func.c
	$(CC) $^ -g -c -o str_func.o $(STND)

bpb.o: bpb.c
	$(CC) $^ -g -c -o bpb.o $(STND)
	
cd.o: cd.c
	$(CC) $^ -g -c -o cd.o $(STND)

cluster.o: cluster.c
	$(CC) $^ -g -c -o cluster.o $(STND)

create.o: create.c
	$(CC) $^ -g -c -o create.o $(STND)

ls.o: ls.c
	$(CC) $^ -g -c -o ls.o $(STND)

mkdir.o: mkdir.c
	$(CC) $^ -g -c -o mkdir.o $(STND)

mv.o: mv.c
	$(CC) $^ -g -c -o mv.o $(STND)

size.o: size.c
	$(CC) $^ -g -c -o size.o $(STND)

rm.o: rm.c
	$(CC) $^ -g -c -o rm.o $(STND)

cp.o: cp.c
	$(CC) $^ -g -c -o cp.o $(STND)

open.o: open.c
	$(CC) $^ -g -c -o open.o $(STND)

rw.o: rw.c
	$(CC) $^ -g -c -o rw.o $(STND)

clean:
	rm -f *.o *.out

release:
	tar -cvf project3_Anderson_Brown.tar *.c *.h commit_log.txt Makefile README.md
