CC = gcc
CFLAGS = -pthread -Wall 

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c -lncurses

client: client.c
	$(CC) $(CFLAGS) -o client client.c -lncurses

clean:
	rm -f server client
