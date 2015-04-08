#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED
#include <stdint.h>

typedef struct htable TABLE;

//* Delete this block *//
int DETECT_COLLISION, COLLISIONS;
//*********************//

TABLE *htable_new(int);
void htable_clear(TABLE *);
int htable_nmemb(TABLE *);
int htable_insert(TABLE *, uint64_t *, int);
int htable_remove(TABLE *, uint64_t *, int);
int htable_search(TABLE *, uint64_t *, int, unsigned int *);

#endif // HASH_H_INCLUDED
