CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
AR      = ar
ARFLAGS = rcs

all: client server

# --- библиотека ---
common.o: common.c common.h
	$(CC) $(CFLAGS) -c -o $@ $<

libcommon.a: common.o
	$(AR) $(ARFLAGS) $@ $^

# --- приложения ---
client: client.c common.h libcommon.a
	$(CC) $(CFLAGS) -o $@ client.c libcommon.a

server: server.c common.h libcommon.a
	$(CC) $(CFLAGS) -o $@ server.c libcommon.a

clean:
	rm -f client server common.o libcommon.a

