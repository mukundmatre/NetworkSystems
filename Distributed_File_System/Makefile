INCLUDE_DIRS =
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lssl

HFILES=
CFILES= dfs.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: dfs

clean:
	-rm -f *.o *.d
	-rm -f client dfs

dfs: dfs.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)


depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
