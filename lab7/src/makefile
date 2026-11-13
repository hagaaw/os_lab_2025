CC = gcc
CFLAGS = -Wall -Wextra -O2

all: tcp_client tcp_server udp_client udp_server

tcp_client:
	$(CC) $(CFLAGS) tcpclient.c -o tcp_client

tcp_server:
	$(CC) $(CFLAGS) tcpserver.c -o tcp_server

udp_client:
	$(CC) $(CFLAGS) udpclient.c -o udp_client

udp_server:
	$(CC) $(CFLAGS) udpserver.c -o udp_server

clean:
	rm -f tcp_client tcp_server udp_client udp_server

