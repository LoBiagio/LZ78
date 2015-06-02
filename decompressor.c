#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include "htable.h"
#include "bitio.h"
#include "checksum.h"
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
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
	tmp->dictionary = calloc(size, sizeof(DENTITY));
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
    while (index >= 257) {
        index = da->dictionary[index-257].father;
    }
    return (unsigned char)(index - 1);
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
	while (index >= 257) {
        offset--;
        buf[offset] = da->dictionary[index-257].value;
        index = da->dictionary[index-257].father;
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
        if (father >= 257) {
            father = (unsigned int)da->dictionary[father].value + 1;
        }
        array_reset(da);
        return;
    }
    da->dictionary[da->nmemb].father = father;
    da->nmemb++;
}

/**
 * @brief Read all the information in the file header.
 *
 * The header is composed by the information fields listed below:
 * - original file name lenght (8 bits)
 * - original file name (variable lenght)
 * - original file size (64 bits)
 * - last modification (64 bits)
 * - dictionary lenght (32 bits)
 * - header checksum (32 bits)
 *
 * While reading the header, this function also compute and validate the header
 * checksum.
 *
 * @param fd the input file.
 * @param dict_size The content of dictionary lenght read in this file's header.
 * @return 0 if no errors occured, -1 otherwise
 */
int read_header(struct bitio *fd, unsigned int *dict_size, int v)
{
    int ret, i;
    char *string;
    uint64_t buf, tmp;
    CHECKENV *cs = checksum_init();

    // File name length
    ret = bitio_read(fd, &buf, 8);
    if (ret != 8) {
        fprintf(stderr, "Error reading file name length\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 1);
    string = (char *)malloc(buf + 1);
    if (string == NULL) {
        fprintf(stderr, "Error allocating memory for file name\n");
        return -1;
    }
    // File name
    string[buf] = '\0';
    for (i = 0; i < buf; i++) {
        ret = bitio_read(fd, &tmp, 8);
        if (ret != 8) {
            fprintf(stderr, "Error reading file name\n");
            free(string);
            return -1;
        }
        string[i] = (char)tmp;
        checksum_update(cs, (char *)&tmp, 1);
    }
    if (v == 1) {
        printf("Original file name: %s\n", string);
    }
    free(string);
    // File size
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
        fprintf(stderr, "Error reading file size\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 8);
    buf = le64toh(buf);
    if (v == 1) {
        printf("Original file size: %lu bytes\n", (long unsigned int)buf);
    }
    // Last modification
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
        fprintf(stderr, "Error reading file's last modification\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 8);
    buf = le64toh(buf);
    if (v == 1) {
        printf("Last modification: %s\n", ctime((time_t *)&buf));
    }

    // Dictionary length
    ret = bitio_read(fd, &buf, 32);
    if (ret != 32) {
        fprintf(stderr, "Error reading dictionary length\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 4);
    buf = le32toh(buf);
    *dict_size = buf;
    // Header checksum
    ret = bitio_read(fd, &buf, 32);
    if (ret != 32) {
        fprintf(stderr, "Error reading header checksum\n");
        return -1;
    }
    buf = le32toh(buf);
    if ((unsigned int)buf != checksum_final(cs)) {
        fprintf(stderr, "Error: header checksum mismatch\n");
        return -1;
    }
    if (v == 1) {
        printf("Header checksum matches\n");
    }
    return 0;
}

int decompress(int fd_w, struct bitio *fd_r, int dict_size) {
	struct darray *da;
	unsigned char *buf;
	int buf_len, buf_size, ret;
	uint64_t code;

	if ((da = array_new(dict_size)) == NULL){
		perror("Error on array_new");
		goto end;
	}
	buf_size = dict_size + 1;
	buf = calloc(buf_size, sizeof(unsigned char));
    if (buf == NULL) {
        printf("Error allocating buffer for string reversing\n");
        goto end;
    }

	// Read first code from compressor which is also the first char of the text
    if (bitio_read(fd_r, &code, 9) < 0) {
        printf("Error reading the compressed file\n");
        goto end;
    }
    buf_len = 1;
    // A node is inserted in the dictionary whose value will be known with the next code read
    // and whose father is the code just read
    insert_node(da, (unsigned int)code);
    code--;
    ret = write(fd_w, &code, sizeof(unsigned char));
    if (ret != sizeof(unsigned char)) {
        printf("Error writing the decompressed file\n");
        goto end;
    }

	while ((ret = bitio_read(fd_r, &code, (int)ceil(log2(da->nmemb + 258))) > 0)){
        fflush(NULL);
        if (code == 0) {
            break;
        }
		// With the new read code it is possible to see which is the value of the node previously added
		buf[buf_size-1] = look_for_mismatched_value(da, (unsigned int)code);
		// Update the dictionary with the found value only if it has not been cleared
		if (da->nmemb) {
            da->dictionary[da->nmemb-1].value = buf[buf_size-1];
		}
        ret = write(fd_w, buf + buf_size - buf_len, buf_len);
        if (ret != buf_len) {
            printf("Error writing the decompressed file\n");
            goto end;
        }
        // buf is filled with the values of the branch that has code as a leaf
        // (from code to the second level of the tree)
        // buf is filled from its bottom to its begining exept for its last value which will be insert
        // at the begining of the loop, in the next loop
        buf_len = explore_darray(da, (unsigned int)code, buf, buf_size);
        insert_node(da, (unsigned int)code);
	}
	if (ret < 0) {
        printf("Error reading the compressed file\n");
        goto end;
	}
	// Write values which are still in buf
	// Note that last value of buf is 0 and it is not written
	write(fd_w, buf + buf_size - buf_len, buf_len - 1);
	bitio_close(fd_r);
	close(fd_w);
	return 0;
end:
    bitio_close(fd_r);
	close(fd_w);
    exit(1);
}
