CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

all: parallel_min_max process_memory parallel_sum

parallel_min_max: parallel_min_max.c find_min_max.c utils.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -o $@ parallel_min_max.c find_min_max.c utils.c $(LDFLAGS)

process_memory: process_memory.c
	$(CC) $(CFLAGS) -o $@ process_memory.c $(LDFLAGS)

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) $(CFLAGS) -c -o $@ sum_lib.c

libsum.a: sum_lib.o
	ar rcs $@ $^

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c -o $@ utils.c

parallel_sum: parallel_sum.c libsum.a utils.o sum_lib.h utils.h
	$(CC) $(CFLAGS) -o $@ parallel_sum.c utils.o -L. -lsum $(LDFLAGS)

clean:
	rm -f parallel_min_max process_memory parallel_sum
	rm -f sum_lib.o utils.o libsum.a