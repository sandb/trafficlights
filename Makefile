CC=gcc
#CFLAGS=-O2 -lcurl -ljson -ggdb
CFLAGS=-O2 -lcurl -ljson

all: trafficlights

trafficlights: trafficlights.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o trafficlights
