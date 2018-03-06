CFLAGS=-g -Wall
CC=gcc

LDFLAGS=$(shell pkg-config --cflags libdrm)
LDLIBS = $(shell pkg-config --libs libdrm) -lm

# All sources
SOURCES=demo.c
# The build directory
BUILD=build
# All executables to be cleaned
EXECUTABLES=$(BUILD)/demo

$(BUILD)/demo: demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -f $(EXECUTABLES)
