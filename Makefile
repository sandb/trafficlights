CC=gcc
CFLAGS=-O2 -lcurl -ljson-c

all: trafficlights

trafficlights: trafficlights.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o trafficlights

.PHONY: all clean
