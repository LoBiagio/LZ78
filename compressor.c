#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include "htable.h"
#include "bitio.h"
#include <math.h>
#define DICT_SIZE 10000

int main() {
    int fd_r;
    struct bitio *fd_w;
    int ret;
    unsigned char c;
    unsigned int father = 0, new_father,reset = 0;
    TABLE *dictionary;

    fd_r = open("B", O_RDONLY);
    fd_w = bitio_open("compressed", 'w');
    dictionary = htable_new(DICT_SIZE);

    while((ret = read(fd_r, &c, sizeof(char)))) {
        if (htable_insert(dictionary, c, father, &new_father,&reset) == 1) {
        	if (reset == 1){
            	bitio_write(fd_w, (uint64_t *)&father, (int)log2(DICT_SIZE -1 + 255) + 1);
            	reset = 0;
            }
            else {
            	bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
            }
		    //printf("value:%c\n", c);
		  	//printf("father:%u\n",father);
		 	//printf("elements%u\n",htable_nmemb(dictionary)+255);
			//printf("bit:%d\n",htable_index_bits(dictionary));
        }
        father = new_father;
        
    }
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    printf("%u\n",father);
    father= 0;
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    printf("%d,%d\n", htable_nmemb(dictionary), htable_collision(dictionary));
    close(fd_r);
    bitio_close(fd_w);
    return 0;
}
