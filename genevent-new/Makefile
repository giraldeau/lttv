SHELL = /bin/sh

CC = gcc

CFLAGS = -Wall -g

all: genevent

install: genevent
	cp -f genevent /usr/local/bin

genevent: genevent.o parser.o 
	$(CC) $(CFLAGS) -o $@ $^

genevent.o: genevent.c genevent.h parser.h
	$(CC) $(CFLAGS) -c -o $@ $<

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -rf *.o *~ *# genevent


