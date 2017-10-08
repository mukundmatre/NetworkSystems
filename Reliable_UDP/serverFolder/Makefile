INCLUDE_DIRS =
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

HFILES=
CFILES= server.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: server

clean:
	-rm -f *.o *.d
	-rm -f client server

server: server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o


depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
