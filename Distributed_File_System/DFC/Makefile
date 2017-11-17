INCLUDE_DIRS =
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lcrypto -lssl

HFILES=
CFILES= dfc.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: dfc

clean:
	-rm -f *.o *.d
	-rm -f client dfc

dfc: dfc.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)


depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
