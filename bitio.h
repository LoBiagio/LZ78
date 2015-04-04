#ifndef BITIO_H_INCLUDED
#define BITIO_H_INCLUDED
#include <stdint.h>

struct bitio;

struct bitio *bitio_open(const char*, char);
int bitio_write(struct bitio*, uint64_t *, int);
int bitio_read(struct bitio*, uint64_t *, int);
int bitio_flush(struct bitio*);
int bitio_close(struct bitio*);

#endif // BITIO_H_INCLUDED
