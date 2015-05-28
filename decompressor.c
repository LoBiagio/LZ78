#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "checksum.h"
#include "bitio.h"

/**
 * @brief This struct represent a decompressor dictionary entry.
 */
typedef struct
{
    unsigned int father; /** The value for this node father. */
    unsigned char value; /** The value (character) of the current character. */
}DENTITY;

/**
 * @brief This struct represent the decompressor dictionary.
 *
 * On the decompressor side we can use a simple array of dictionary nodes.
 * We perform each insertion in the first unused slot. We obtain this
 * adding each new entity in d->dictionary[d->nmemb].
 */
struct darray
{
    DENTITY* dictionary; /** The array of dictionary's entries */
     unsigned int nmemb; /** Number of entities currently stored */
     unsigned int dim; /** Maximum number of elements that can be stored */
};

/**
 * @brief Create a new struct darray of the given size.
 *
 * @param size The number of entry the newly created struct darray can store.
 * @return A pointer to the newly created struct darray. If some kind of 
 * error occours, this function returns NULL.
 */
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
    //TODO memset should not be necessary
    memset(tmp->dictionary,0,size * sizeof(DENTITY));
    tmp->nmemb = 0;
    tmp->dim = size;
    return tmp;        
}

/**
 * @brief Reset the dictionary without deallocating it
 */
void
array_reset (struct darray *da){
    memset(da->dictionary, 0, sizeof(DENTITY)*da->dim);
    da->nmemb = 0;
}

/**
 * @brief Returns how many nodes are currently stored in the dictionary.
 * 
 * @return The number of elements stored in the dictionary
 */
unsigned int
get_size (struct darray* da)
{
    return da->nmemb;
}

/**
 * @brief This function explore the dictionary and stores the decoded string.
 *
 * This function explores the dictionary starting from one of its leafs to the
 * root. It stores the decoded string and passes such string to the caller.
 * The first character of the decoded string is also stored in the character 
 * pointed by <value>. explore_darray() does so since such first character is
 * the one that has to be stored as <value> for the node we should have added 
 * at the previous decoding. We delayed the insertion since we ignored the 
 * value of the mismatching character at that moment.
 *
 * @param *da pointer to struct darray.
 * @param index This is the index we receive from the compressor. It is our 
 * starting point for the dictionary exploration.
 * @param *buf pointer to the buffer for the output decoding string
 * @param *value The first character of the decoded string
 * @return The number of characters inserted into the decoding buffer buf. If
 * either <da> or <buf> are NULL, explore_darray returns -1
 */
unsigned int
explore_darray(struct darray *da, unsigned int index, char *buf, unsigned char *value)
{
    unsigned int offset;
    if ( (da == NULL) || (buf == NULL) ){
        errno = EINVAL;
        return -1;
    }
    offset = da->dim;    //TODO check buf size 
    if ( index < 256 ){ //we are already at the first level of the tree
        *value = (unsigned char)index;
        buf[offset] = (unsigned char)index;
        return 1;
    }

    /* Notice how we deal with the first 256 extended ASCII character which 
     * are not effectively stored into the dictionary.
     */
    while (da->dictionary[index-256].father >= 256){ //explore the tree
        buf[offset] = da->dictionary[index-256].value;
        offset--;
        index = da->dictionary[index-256].father;
    }
    buf[offset] = da->dictionary[index-256].value;
    buf[--offset] = (unsigned char)da->dictionary[index-256].father;
    *value = (unsigned char)da->dictionary[index-256].father;
    return da->dim + 1 - offset;
}

/**
 * @brief This function performs all the actions for inserting a new node 
 * inside the dictionary.
 *
 * This function is called every time the decompressor receives a new node 
 * index. In particular, it decodes the index and adds a new node with the same 
 * characted value as the first character of the decode string.
 * The exploration starts from the value passed in <index> and continues 
 * until the function reaches the first layer of root offsprings.
 * The first offsprings are stored such that each node contains a character 
 * value equal to its own index (i.e. the string 'A' is encoded as 65).
 * With such design, explore_and_insert() can check when the first root 
 * offsprings are reached and so it can decode the first characted for the
 * output string.
 *
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
            array_reset(da);    //after dictionary reset, father must be reset!
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
        array_reset(da);    //after dictionary reset, father must be reset!
        *index = (unsigned int)0;
    }
    
    return buf_len;        
}

int
decompress(int fd_w, struct bitio* fd_r, unsigned int dictionary_size, int v)
{
    int ret;
    uint64_t tmp;
    unsigned char old_value;
    struct darray *da;
    unsigned char *buf;
    unsigned int father = 0, buf_len;
    CHECKENV *cs = checksum_init();
    if( (da = array_new(dictionary_size)) == NULL){
        perror("error on array_new");
    }
    if(v == 1){
    printf("A new array initialized\n");
    printf("Buffer allocation\n");
    }
    buf = calloc(dictionary_size + 1, sizeof(unsigned char));    //TODO check buf size
    while( (ret = bitio_read(fd_r, &tmp, (int)(log2(da->nmemb + 257)+1)) > 0 )){ //TODO controlla ret!
        if((unsigned int)tmp == 0){
            break;
        }
        if (v == 1){
            printf("Number of bits read:%d\n",(int)log2(da->nmemb + 257)+1);
        }
        //Here happens the magic...
        buf_len = explore_and_insert(da, father, (unsigned int *)&tmp, &old_value, buf);
        if (v == 1){
            printf("Explore and insert completed. Father:%u, value read:%d, new value to add:%c\n",father,(int)tmp,old_value);
        }
        father = (unsigned int)tmp;
        write(fd_w, &buf[da->dim + 1 - buf_len], buf_len);    //TODO check buf size
        if (v == 1){
            write(0, &buf[da->dim + 1 - buf_len], buf_len);
            printf("\n");
        }
        checksum_update(cs, (char *)&buf[da->dim + 1 - buf_len], buf_len);

    }
    tmp = 0;
    ret = bitio_read(fd_r, &tmp, 32); //TODO controlla ret
    ret = le32toh((int)tmp);
    if (ret != checksum_final(cs)) {
        printf("Errore checksum\n");
        return 1;
    }
    printf("File checksum matches\n");
    checksum_destroy(cs);
    bitio_close(fd_r);
    close(fd_w);
    return 1;
}

int read_header(struct bitio *fd, unsigned int *dict_size) 
{
    int ret, i;
    char *string;
    uint64_t buf, tmp;
    CHECKENV *cs = checksum_init();
    
    // File name length
    ret = bitio_read(fd, &buf, 8);
    if (ret != 8) {
        printf("Error reading file name length\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 1);
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
        checksum_update(cs, (char *)&tmp, 1);
    }
    printf("Original file name: %s\n", string);
    free(string);
    // File size
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
        printf("Error reading file size\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 8);
    buf = le64toh(buf);
    printf("Original file size: %lu bytes\n", (long unsigned int)buf);
    // Last modification
    ret = bitio_read(fd, &buf, 64);
    if (ret != 64) {
        printf("Error reading file's last modification\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 8);
    buf = le64toh(buf);
    printf("Last modification: %s\n", ctime((time_t *)&buf));

    // Dictionary length
    ret = bitio_read(fd, &buf, 32);
    if (ret != 32) {
        printf("Error reading dictionary length\n");
        return -1;
    }
    checksum_update(cs, (char *)&buf, 4);
    buf = le32toh(buf);
    *dict_size = buf;
    // Header checksum
    ret = bitio_read(fd, &buf, 32);
    if (ret != 32) {
        printf("Error reading header checksum\n");
        return -1;
    }
    buf = le32toh(buf);
    if ((unsigned int)buf != checksum_final(cs)) {
        printf("Error: header checksum mismatch\n");
        return -1;
    }
    printf("Header checksum matches\n");
    return 0;
}
