#Makefile
CC=gcc
CFLAGS=-g -c
LFLAGS=-lm

.PHONY: clean

all: lz78

lz78: lz78.o compressor.o decompressor.o htable.o bitio.o
	$(CC) $^ -o $@ $(LFLAGS)

lz78.o: lz78.c
	$(CC) $(CFLAGS) -o $@

compressor: compressor.o htable.o bitio.o
	$(CC) $^ -o $@ $(LFLAGS)

decompressor: decompressor.o htable.o bitio.o
	$(CC) $^ -o $@ $(LFLAGS)

compressor.o: compressor.c htable.h bitio.h
	$(CC) $(CFLAGS) $< -o $@

decompressor.o: decompressor.c htable.h bitio.h
	$(CC) $(CFLAGS) $< -o $@

bitio.o: bitio.c
	$(CC) $(CFLAGS) $? -o $@

htable.o: htable.c
	$(CC) $(CFLAGS) $? -o $@

clean:
	rm -fr *.o; \
       if [ -e B_NEW ] ; then rm B_NEW ; fi ; \
       if [ -e compressed ] ; then rm compressed ; fi

