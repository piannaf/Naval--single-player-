CC = gcc
CFLAGS = -Wall -ansi -pedantic
OBJECTS = naval.o

all: naval

naval: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

debug: $(OBJECTS)
	$(CC) -g $(CFLAGS) -o $@ $^
