#ifndef CHECKSUM_H_INCLUDED
#define CHECKSUM_H_INCLUDED
#include <stdint.h>
typedef struct checksum_environment CHECKENV; 
CHECKENV*
checksum_init();
void
checksum_update(CHECKENV*,uint32_t,unsigned int);
uint32_t
checksum_final(CHECKENV*);
int
checksum_is_valid(CHECKENV*,uint32_t);
#endif 
