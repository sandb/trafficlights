CC=gcc
CFLAGS=-O2 -lcurl -ljson -ggdb

all: trafficlights

trafficlights: trafficlights.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o trafficlights
