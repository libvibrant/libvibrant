CFLAGS=-g -Wall
CC=gcc

LDFLAGS=$(shell pkg-config --cflags --libs libdrm)
LDLIBS = -lm

SOURCES=$(wildcard *.c)
EXECUTABLES=$(patsubst %.c,%,$(SOURCES))

all: $(EXECUTABLES)

clean:
	rm -f $(EXECUTABLES)
