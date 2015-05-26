#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
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
decompress(int fd_w, struct bitio* fd_r, unsigned int dictionary_size)
{
	int ret;
	uint64_t tmp;
	unsigned char old_value;
	struct darray *da;
	unsigned char *buf;
	unsigned int father = 0, buf_len;
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

int read_header(struct bitio *fd, int *dict_size) {
    int ret, i;
    char *string;
    uint64_t buf, tmp;
    // File name length
    ret = bitio_read(fd, &buf, 8);
    if (ret != 8) {
        printf("Error reading file name length\n");
        return -1;
    }

    string = (char *)malloc(buf + 1);
    if (string == NULL) {
        printf("Error allocating memory for file name\n");
        return -1;
    }
    // File name
    string[buf] = '\0';
    for (i = 0; i < buf; i++) {
        ret = bitio_read(fd, &tmp, 8);
        if (ret != 8) {
            printf("Error reading file name\n");
            free(string);
            return -1;
        }
        string[i] = (char)tmp;
    }
    printf("Original file name: %s\n", string);
    free(string);
    // File size
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
            printf("Error reading file size\n");
            return -1;
        }
    buf = le64toh(buf);
    printf("Original file size: %lu bytes\n", (long unsigned int)buf);
    // Last modification
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
            printf("Error reading file's last modification\n");
            return -1;
        }
    buf = le64toh(buf);
    printf("Last modification: %d\n", ctime((time_t *)&buf));
    // File checksum
    // TODO

    // Dictionary length
    ret = bitio_read(fd, &buf, 32);
    if (ret != 32) {
        printf("Error reading dictionary length\n");
        return -1;
    }
    buf = le32toh(buf);
    *dict_size = buf;
    return 0;
}
