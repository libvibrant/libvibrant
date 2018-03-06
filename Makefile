CFLAGS=-g -Wall
CC=gcc

LDFLAGS=$(shell pkg-config --cflags libdrm)
LDLIBS = $(shell pkg-config --libs libdrm) -lm

SOURCES=$(wildcard *.c)
EXECUTABLES=$(patsubst %.c,%,$(SOURCES))

all: $(EXECUTABLES)

clean:
	rm -f $(EXECUTABLES)
