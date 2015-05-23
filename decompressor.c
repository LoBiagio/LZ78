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
#define DICT_SIZE 10000
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
	tmp->nmemb = 0;
	tmp->dim = size;
	return tmp;
}

void
array_reset (struct darray *da){
	memset(da->dictionary, 0, sizeof(DENTITY)*da->dim);
	da->nmemb = 0;
}

unsigned char
look_for_mismatched_value(struct darray *da, unsigned int index) {
    while (index >= 256) {
        index = da->dictionary[index-256].father;
    }
    return (unsigned char)index;
}

unsigned int
get_size (struct darray* da)
{
	return da->nmemb;
}
//explore the tree from bottom to top and rfill the buffer with the read characters, returns the number of characters in the buffer
//in value there is the missing character of the previous call
unsigned int
explore_darray (struct darray *da, unsigned int index, unsigned char *buf, unsigned int buf_size)
{
	unsigned int offset; //counter from the bottom of the buffer (same size as the dictionary)
	if ( (da == NULL) || (buf == NULL) ){
		errno = EINVAL;
		return -1;
	}
	offset = buf_size - 1;
	while (index >= 256) {
        offset--;
        buf[offset] = da->dictionary[index-256].value;
        index = da->dictionary[index-256].father;
	}
	return buf_size - offset;
}

void
insert_node(struct darray *da, unsigned int father) {
    // Dictionary is full
    if (da->nmemb == da->dim){
        // The node is not inserted because it is already present in the first level of the tree.
        // Once the dictionary is cleared, next node will have as father the father that it would
        // have had if dictionary wasn't full but it is moved in the first level of the tree.
        if (father >= 256) {
            father = (unsigned int)da->dictionary[father].value;
        }
        array_reset(da);
        return;
    }
    da->dictionary[da->nmemb].father = father;
    da->nmemb++;
}

int main() {
	int fd_w;
	struct bitio *fd_r;
	struct darray *da;
	unsigned char *buf;
	int buf_len, buf_size, ret;
	uint64_t code;

	fd_r = bitio_open ("compressed", 'r');
	fd_w = open ("B_NEW", (O_CREAT | O_TRUNC | O_WRONLY) , 0666);
	if( (da = array_new(DICT_SIZE)) == NULL){
		perror("error on array_new");
	}
	buf_size = (int)ceil(sqrt(DICT_SIZE + 1) * 2);
	buf = calloc(buf_size, sizeof(unsigned char));
	// Read first code from compressor which is also the first char of the text
    if (bitio_read(fd_r, &code, 9) < 0) {
        exit(1);
    }
    buf_len = 1;
    // A node is inserted in the dictionary whose value will be known with the next code read
    // and whose father is the code just read
    insert_node(da, (unsigned int)code);
    write(fd_w, &code, sizeof(unsigned char));


	while ((ret = bitio_read(fd_r, &code, (int)ceil(log2(da->nmemb + 257))) > 0)){
		if ((unsigned int)code == 0){
			break;
		}
		// With the new code read is possible to see which is the value of the node previously added
		buf[buf_size-1] = look_for_mismatched_value(da, (unsigned int)code);
		// Update the dictionary with the found value only if it has not been cleared
		if (da->nmemb) {
            da->dictionary[da->nmemb-1].value = buf[buf_size-1];
		}
        write(fd_w, buf + buf_size - buf_len, buf_len);

        // buf is filled with the values of the branch that has code as a leaf
        // (from code to the second level of the tree)
        // buf is filled from its botton to its begining exept for its last value which will be insert
        // at the begining of the loop, in the next loop
        buf_len = explore_darray(da, (unsigned int)code, buf, buf_size);
        insert_node(da, (unsigned int)code);
	}
	// Write values which are still in buf
	// Note that last value of buf is 0 and it is not written
	write(fd_w, buf + buf_size - buf_len, buf_len - 1);
	bitio_close(fd_r);
	close(fd_w);
	return 0;
}
