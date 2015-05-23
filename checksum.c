#include "bitio.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "checksum.h"
struct checksum_environment
{
	unsigned int count;
	uint32_t partial, buf;
};

CHECKENV*
checksum_init ()
{
	CHECKENV *tmp;
	if((tmp = calloc(1,sizeof(CHECKENV)))== NULL){
		return NULL;
	}
	tmp->count = 0;
	tmp->partial = 0;
	tmp->buf = 0;
	return tmp;
}

void
checksum_update(CHECKENV* ce, uint32_t in, unsigned int num_byte)
{
	int j = 0, i;
	in = htole32(in);
	unsigned char *tmp = (unsigned char*)&in;
	unsigned char *out = (unsigned char*)&ce->buf;
	for (i = ce->count; i < 4 && j < num_byte; i++) {
		out[i] = tmp[j];
		j++;
	}
	
	if (i > 3) {
		ce->partial += ce->buf;
		i = 0;
		ce->buf = 0;
	}
	
	while(j < 4 && i < 4){
		out[i] = tmp[j];
		j++;
		i++;
	}
	ce->count = (ce->count + num_byte)%4;
}

uint32_t
checksum_final(CHECKENV *ce)
{
	int i;
	unsigned char *tmp = (unsigned char*)&ce->buf;
	for(i = ce->count; i < 4; i++){
		tmp[i] = 0;
	}
	ce->partial += ce->buf;
	return (uint32_t)0 - ce->partial;
}

int
checksum_is_valid (CHECKENV *ce, uint32_t in)
{
	return (ce->partial + in == 0); 
}

