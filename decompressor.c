#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include "htable.h"
#include "bitio.h"
#include <errno.h>
#include <string.h>
#include <math.h>
#define DICT_SIZE 2

typedef struct
{
	unsigned int father;
	unsigned char value;
}DENTITY;

struct darray
{
	DENTITY* dictionary;
 	unsigned int nmemb;
 	unsigned int dim;
};

struct darray *
array_new(unsigned int size)
{
	struct darray* tmp;
	tmp=calloc(1,sizeof(struct darray));
	if (tmp == NULL){
		return NULL;
	}
	tmp->dictionary = calloc(DICT_SIZE,sizeof(DENTITY));
	if (tmp->dictionary == NULL){
		return NULL;
	}
	memset(tmp->dictionary,0,DICT_SIZE * sizeof(DENTITY));
	//tmp->nmemb = -1;
	tmp->nmemb = 0;
	tmp->dim = size;
	return tmp;		
}
void
array_reset (struct darray *da){
	memset(da->dictionary, 0, sizeof(DENTITY)*da->dim);
	da->nmemb = 0;
}
/*
 * Returns the number of entries currently in the dictionary
 */
int
insert (struct darray *da, unsigned int father, unsigned char value)
{
	if(father > 0){		
		
		da->dictionary[da->nmemb].father = father;
		da->dictionary[da->nmemb].value = value;
		da->nmemb++;
		if (da->nmemb >= da->dim){
			array_reset(da);
			printf("Dizionario azzerato\n");
			fflush(0);
		}		
	}
	//da->nmemb++;
	return da->nmemb;
}

unsigned int
get_size (struct darray* da)
{
	return da->nmemb;
}
//explore the tree from bottom to top and rfill the buffer with the read characters, returns the number of characters in the buffer
//in value there is the missing character of the previous call
unsigned int
explore_darray (struct darray *da, unsigned int index, char *buf, unsigned char *value)
{
	unsigned int offset; //counter from the bottom of the buffer (same size as the dictionary)
	if ( (da == NULL) || (buf == NULL) ){
		errno = EINVAL;
		return -1;
	}
	offset = da->dim-1; 
	if ( index < 256 ){ //we are already at the first level of the tree
		*value = (unsigned char)index;
		buf[offset] = (unsigned char)index;
		return 1;
	}
	
	while (da->dictionary[index-256].father >= 256){ //explore the tree
		buf[offset] = da->dictionary[index-256].value;
		offset--;
		index = da->dictionary[index-256].father;
	}
	buf[offset] = da->dictionary[index-256].value;
	buf[--offset] = (unsigned char)da->dictionary[index-256].father;
	*value = (unsigned char)da->dictionary[index-256].father;
	return da->dim - offset;
}

/*
 * @param da pointer to decompressor's dictionary.
 * @param father The father of the new node.
 * @param index The value that must be decoded.
 * @param old_value last value written at the previous iteration.
 * @param buf The destination buffer of the decoded characters
 * @return the number of bytes returned in buf
 */
int
explore_and_insert(struct darray* da, unsigned int father, unsigned int index, unsigned char* old_value, unsigned char* buf)
{
	unsigned int buf_len;
	
	if (father == 0) { //only first label received. Value to be added is unknow, so we don't perform an insert.
		
		//exploring
		buf_len = explore_darray(da, index, buf, old_value);
		return buf_len;
	}
	
	if (index == get_size(da) + 256) { //The 'recursion' case
		
		//adding
		da->dictionary[da->nmemb].father = father;
		da->dictionary[da->nmemb].value = (unsigned int)*old_value;
		da->nmemb++;
		
		//exploring
		buf_len = explore_darray(da, index, buf, old_value);
		
		//there may be the need for a dictionary reset
		if (da->nmemb >= da->dim) {
			array_reset(da);
			printf("Dizionario azzerato\n");
			fflush(0);
		}
		
		return buf_len;
	}
	
	//exploring
	buf_len = explore_darray(da, index, buf, old_value);
	
	//adding
	da->dictionary[da->nmemb].father = father;
	da->dictionary[da->nmemb].value = (unsigned int)*old_value;
	da->nmemb++;
	
	//there may be the need for a dictionary reset
	if (da->nmemb >= da->dim) {
		array_reset(da);
		printf("Dizionario azzerato\n");
		fflush(0);
	}
	
	return buf_len;		
}

int main() {
	int fd_w;
	int ret;
	uint64_t tmp;
	unsigned char old_value;
	struct bitio *fd_r;
	struct darray *da;
	unsigned char *buf;
	unsigned int father = 0, buf_len;
	fd_r = bitio_open ("compressed", 'r');
	fd_w = open ("B_NEW", (O_CREAT | O_TRUNC | O_WRONLY) , 0666);
	if( (da = array_new(DICT_SIZE)) == NULL){
		perror("error on array_new");
	}
	buf = calloc(DICT_SIZE, sizeof(unsigned char));
	printf("START\n");
	fflush(0);
	while( (ret = bitio_read (fd_r, &tmp, (int)(log2(da->nmemb + 257)+1)) > 0 )){
	//while( (ret = bitio_read (fd_r, &tmp, 9) > 0 )){
		//printf("letti %d bits\n", 9);
		printf("Letto %u\n", (unsigned int)tmp);
		fflush(0);
		if((unsigned int)tmp == 0){
			break;
		}
		
		//Here happens the magic...
		buf_len = explore_and_insert(da, father, (unsigned int)tmp, &old_value, buf);
		
		//printf("letti %d bits\n", ret);
		//printf("father: %d\n",father);
		//printf("value: %c\n\n",old_value);
	//	old_father = father;
		father = (unsigned int)tmp;
		//printf ("new_father: %d\n",father);
		
		write(fd_w, &buf[da->dim - buf_len], buf_len);
		//write(0, &buf[da->dim - buf_len], buf_len);
		//printf("\n");
	}
//	write(fd_w, &oldvalue, 1);
	bitio_close(fd_r);
	close(fd_w);
	return 0;	 
}

