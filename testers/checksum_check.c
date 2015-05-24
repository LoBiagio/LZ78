#include "../checksum.h"
#include <stdio.h>
#include <fcntl.h>
int main()
{
	CHECKENV *ce;
	int ret;
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
	while( ret = read(fd,	&tmp, 1)){
		checksum_update(ce, tmp, ret);
	}
	val = checksum_final(ce);
	printf("%d\n",val);
	printf("%d\n",checksum_is_valid(ce,val));
	return 0;
	
}
