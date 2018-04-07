CC:=gcc
CFLAGS:= -o
LIBS:=-lm

all: server client

server:	refserver.c CS_TCP.c
	$(CC) $(CFLAGS) server refserver.c CS_TCP.c 

client: refclient.c CS_TCP.c
	$(CC) $(CFLAGS) client refclient.c CS_TCP.c 

clean:
	rm server client
