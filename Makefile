#Makefile

CC=gcc
CFLAGS=-g -c
LDFLAGS=-lm

.PHONY: clean


all: lz78
	
lz78: lz78.o compressor.o decompressor.o bitio.o htable.o
	$(CC) $^ -o $@ $(LDFLAGS)

lz78.o: lz78.c
	$(CC) $(CFLAGS) $< -o $@

decompressor.o: decompressor.c bitio.h
	$(CC) $(CFLAGS) $< -o $@

compressor.o: compressor.c bitio.h
	$(CC) $(CFLAGS) $< -o $@

bitio.o: bitio.c
	$(CC) $(CFLAGS) $^ -o $@

htable.o: htable.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -fr *.o
