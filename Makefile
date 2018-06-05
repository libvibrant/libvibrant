CFLAGS=-g -Wall
CC=gcc

LDFLAGS=$(shell pkg-config --cflags libdrm)

# Required libs are libdrm, x11, and xrandr. The math library is used for
# generating some example gamma LUTs.
LDLIBS = $(shell pkg-config --libs libdrm x11 xrandr) -lm

# All sources
SOURCES=demo.c
# All executables to be cleaned
EXECUTABLES=cmdemo

demo: prebuild $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) $(LDLIBS) -o $(EXECUTABLES)

.PHONY: prebuild clean
prebuild:
	$(shell xxd -i < help.txt > help.xxd && echo ', 0' >> help.xxd)

clean:
	rm -f $(EXECUTABLES)
