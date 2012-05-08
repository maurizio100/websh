CC=gcc
CFLAGS = -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g -c
PROG = websh

all: $(PROG)

execute: $(PROG)
	./$(PROG)

websh: $(PROG).o
	$(CC) -o $(PROG) $(PROG).o

$(PROG).o: $(PROG).c
	$(CC) $(CFLAGS) $(PROG).c

clean:
	rm -f $(PROG).o $(PROG)

test: $(PROG)
	./$(PROG) -s websh:h1

.PHONY: all clean
