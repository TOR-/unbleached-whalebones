CC:=clang-5.0

LIBS:=

CFLAGS:= -Wall -Wextra -g $(LIBS)

COMMON_OBJS:= CS_TCP.o application.o

DEPS:= CS_TCP.h application.h

.PHONY: all clean

all: server client 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server:	server.o $(COMMON_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o $(COMMON_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f server server.o
	rm -f client client.o
	rm -f $(COMMON_OBJS)


