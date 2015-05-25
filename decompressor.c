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
	tmp->dictionary = calloc(size,sizeof(DENTITY));
	if (tmp->dictionary == NULL){
		return NULL;
	}
	memset(tmp->dictionary,0,size * sizeof(DENTITY));
	tmp->nmemb = 0;
	tmp->dim = size;
	return tmp;		
}
void
array_reset (struct darray *da){
	memset(da->dictionary, 0, sizeof(DENTITY)*da->dim);
	da->nmemb = 0;
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
	offset = da->dim;	//TODO check buf size 
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
	return da->dim + 1- offset;	//TODO check buf size
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
explore_and_insert(struct darray* da, unsigned int father, unsigned int *index, unsigned char* old_value, unsigned char* buf)
{
	unsigned int buf_len;
	
	if (father == 0) { //only first label received. Value to be added is unknow, so we don't perform an insert.
		
		//exploring
		buf_len = explore_darray(da, *index, buf, old_value);
		return buf_len;
	}
	
	if (*index == get_size(da) + 256) { //The 'recursion' case
		
		//adding
		da->dictionary[da->nmemb].father = father;
		da->dictionary[da->nmemb].value = (unsigned int)*old_value;
		da->nmemb++;
		
		//exploring
		buf_len = explore_darray(da, *index, buf, old_value);
		
		//there may be the need for a dictionary reset
		if (da->nmemb >= da->dim) {
			array_reset(da);	//after dictionary reset, father must be reset!
			*index = (unsigned int)0;
		}
		
		return buf_len;
	}
	
	//exploring
	buf_len = explore_darray(da, *index, buf, old_value);
	
	//adding
	da->dictionary[da->nmemb].father = father;
	da->dictionary[da->nmemb].value = (unsigned int)*old_value;
	da->nmemb++;
	
	//there may be the need for a dictionary reset
	if (da->nmemb >= da->dim) {
		array_reset(da);	//after dictionary reset, father must be reset!
		*index = (unsigned int)0;
	}
	
	return buf_len;		
}

int
decompress(const char *input_file_name, const char *output_file_name, unsigned int dictionary_size)
{
	int fd_w;
	int ret;
	uint64_t tmp;
	unsigned char old_value;
	struct bitio *fd_r;
	struct darray *da;
	unsigned char *buf;
	unsigned int father = 0, buf_len;
	fd_r = bitio_open (input_file_name, 'r');
	fd_w = open (output_file_name, (O_CREAT | O_TRUNC | O_WRONLY) , 0666);
	if( (da = array_new(dictionary_size)) == NULL){
		perror("error on array_new");
	}
	buf = calloc(dictionary_size + 1, sizeof(unsigned char));	//TODO check buf size
	while( (ret = bitio_read (fd_r, &tmp, (int)(log2(da->nmemb + 257)+1)) > 0 )){
		if((unsigned int)tmp == 0){
			break;
		}
		
		//Here happens the magic...
		buf_len = explore_and_insert(da, father, (unsigned int *)&tmp, &old_value, buf);
		
		father = (unsigned int)tmp;
		
		write(fd_w, &buf[da->dim + 1 - buf_len], buf_len);	//TODO check buf size

	}
	bitio_close(fd_r);
	close(fd_w);
	return 1;
}

int main() {
	int ret = decompress("compressed", "B_NEW", DICT_SIZE);
	if (ret) {
		return 0;
	} else {
		return 1;
	}
}
