#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED
#include <stdint.h>

typedef struct htable TABLE;

TABLE *htable_new(int);
void htable_destroy(TABLE *);
int htable_insert(TABLE *, unsigned char, unsigned int, unsigned int *);
unsigned int htable_index_bits(TABLE *);
#endif // HASH_H_INCLUDED
