#include "bitio.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
typedef struct
{
	unsigned int count;
	uint32_t partial, buf;
}CHECKENV;

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

int main()
{
	CHECKENV *ce;
	char c;
	int fd;
	uint32_t tmp, val;
	if((ce = checksum_init()) == NULL){
		printf("error on checksum init\n");
		return 1;
	}
/*	checksum_update(ce,tmp,1);
//	printf("%d\n",(int)checksum_final(ce));
	val = checksum_final(ce);
	printf("%d\n",checksum_is_valid(ce,val));
*/
	fd = open("B", O_RDONLY);
	while( read(fd,	&c, 1)){
		tmp = (uint32_t)c;
		checksum_update(ce, tmp, 1);
	}
	val = checksum_final(ce);
	printf("%d\n",val);
	printf("%d\n",checksum_is_valid(ce,val));
	return 0;
	
}
