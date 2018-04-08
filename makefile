CC:=gcc
CFLAGS:= -o
LIBS:=-lm

all: server client

server:	server.c CS_TCP.c
	$(CC) $(CFLAGS) server server.c CS_TCP.c 

client: client.c CS_TCP.c
	$(CC) $(CFLAGS) client client.c CS_TCP.c 

clean:
	rm server client
