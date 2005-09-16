SHELL = /bin/sh

CC = gcc

CFLAGS = -g -o

all: genevent

install: genevent
	cp genevent /usr/local/bin

genevent:genevent.o parser.o 
	$(CC) $(CFLAGS) $@ genevent.o parser.o

genevent.o: genevent.c genevent.h parser.h
	$(CC) -g -c genevent.c

parser.o: parser.c parser.h
	$(CC) -g -c parser.c

.PHONY: clean

clean:
	rm -rf *.o *~ *#


