# vim: ts=4 sw=4 noexpandtab :

CROSS=m68k-atari-mintelf-
CC=$(CROSS)gcc
CFLAGS=-Os
OBJS=main.o

all: toshead.ttp

toshead.ttp: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	-rm *.o
