CC:=gcc
LIBS:=
CFLAGS:= -Wall -Wextra -g $(LIBS)
DEPS:= CS_TCP.h application.h
OBJS:= CS_TCP.o application.o

all: server client


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server:	server.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f server server.o
	rm -f client client.o
	rm -f $(OBJS)

