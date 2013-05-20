CC=gcc
CFLAGS=-O2 -lcurl -ljson-c -ggdb

all: trafficlights

trafficlights: trafficlights.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o trafficlights

.PHONY: all clean
