#
CC=gcc
CCarm=arm-linux-gnueabi-gcc
CFLAGS=-O2 -g
CFLAGS=-O2 
LDFLAGS=-lm

runcached: runcached.c
	$(CC) $(CFLAGS) -O runcached.c -o $@ $(LDFLAGS)

arm: runcached.c
	$(CCarm) $(CFLAGS) -O runcached.c -o $@ $(LDFLAGS)

clean: 
	rm -f runcached core
