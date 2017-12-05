INCLUDE_DIRS =
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lssl -lcrypto

HFILES=
CFILES= webproxy.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: webproxy

clean:
	-rm -f *.o *.d
	-rm -f webproxy

webproxy: webproxy.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)


depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
