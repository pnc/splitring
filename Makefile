CC=gcc
CFLAGS=-std=c99 -framework Security -framework CoreFoundation

PROGRAM = splitring
VPATH = splitring/

splitring: src/main.c
	$(CC) $(CFLAGS) -o $(PROGRAM) src/main.c

clean:
	rm $(PROGRAM)

all: splitring
