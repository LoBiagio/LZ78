#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED
#include <stdint.h>

typedef struct htable TABLE;

int htable_init(TABLE *, int);
void htable_clear(TABLE *);
int htable_nmemb(TABLE *);
int htable_insert(TABLE *, uint64_t *, int);
int htable_remove(TABLE *, uint64_t *, int);

#endif // HASH_H_INCLUDED
