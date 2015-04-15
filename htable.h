#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED
#include <stdint.h>

typedef struct htable TABLE;

TABLE *htable_new(int);
void htable_clear(TABLE *);
int htable_nmemb(TABLE *);
int htable_insert(TABLE *, unsigned char, unsigned int, unsigned int *);

#endif // HASH_H_INCLUDED
