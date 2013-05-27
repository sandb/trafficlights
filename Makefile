CC=gcc

#note: on some platforms apparently, you'll need to specify "json-c" iso "json".
LIB_JSON=json
LIB_CURL=libcurl

CFLAGS=-O2
CFLAGS+=$(shell pkg-config --cflags ${LIB_JSON})
CFLAGS+=$(shell pkg-config --cflags ${LIB_CURL})

LDFLAGS+=$(shell pkg-config --libs ${LIB_JSON})
LDFLAGS+=$(shell pkg-config --libs ${LIB_CURL})

all: trafficlights

trafficlights: trafficlights.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o trafficlights

.PHONY: all clean
