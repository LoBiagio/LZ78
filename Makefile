#lz78

# LZ78 is a compression algorithm by A. Lempel J. Ziv.
# The version of the algorithm implemented in this program is a bit different
# from the original. It starts with a pre-filled dictionary and all the non 
# matching characters encountered during encoding are sent as the first 
# characters of the next string.

# Most of the rules in this makefile are implicit. We just set the desired 
# values in the variables and let make do all the work for us.
# See https://www.gnu.org/prep/standards/html_node/index.html
# for more informations

CC=gcc
CPPFLAGS=-O2 -Wall -Werror
#LDLIBS=

.PHONY: clean test

all: lz78

debug: CPPFLAGS=-g -Wall -Werror
debug: lz78


lz78: lz78.o compressor.o decompressor.o htable.o bitio.o checksum.o log2.o

compressor.o: htable.h bitio.h checksum.h

decompressor.o: bitio.h checksum.h log2.h

htable.o: log2.h

clean:
	rm -fr *.o;

test: lz78
	testers/comp_decomp.sh
