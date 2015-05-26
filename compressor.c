#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "htable.h"
#include "bitio.h"
#include "checksum.h"
#define DICT_SIZE 10000

int write_header(int fd_r, struct bitio *fd_w, char *filename, unsigned int dict_size) {
    int ret, i, size;
    uint64_t file_size, last_mod;
    unsigned int checksum;
    struct stat file_info;
    CHECKENV *cs = checksum_init();

    if (fstat(fd_r, &file_info) < 0) {
        perror("Error retriving file info: ");
        return -1;
    }
    // File name length
    size = strlen(filename);
    ret = bitio_write(fd_w, (uint64_t *)&size, 8);
    if (ret != 8) {
        printf("Error writing file name length\n");
        return -1;
    }
    checksum_update(cs, (char *)&size, 1);
    // File name
    for (i = 0; i < size; i++) {
        ret = bitio_write(fd_w, (uint64_t *)&filename[i], 8);
        if (ret != 8) {
            printf("Error writing file name\n");
            return -1;
        }
        checksum_update(cs, &filename[i], 1);
    }
    // File size
    file_size = htole64((uint64_t)file_info.st_size);
    ret = bitio_write(fd_w, &file_size, 64);
    if (ret != 64) {
        printf("Error writing file size\n");
        return -1;
    }
    checksum_update(cs, (char *)&file_size, 8);
    // Time of last modification
    last_mod = htole64((uint64_t)file_info.st_mtim.tv_sec);
    ret = bitio_write(fd_w, &last_mod, 64);
    if (ret != 64) {
        printf("Error writing file's last modification\n");
        return -1;
    }
    checksum_update(cs, (char *)&last_mod, 8);
    // Dictionary length
    dict_size = htole32(dict_size);
    ret = bitio_write(fd_w, (uint64_t *)&dict_size, 32);
    if (ret != 32) {
        printf("Error writing dictionary length\n");
        return -1;
    }
    checksum_update(cs, (char *)&dict_size, 4);
    // Header checksum
    checksum = htole32(checksum_final(cs));
    ret = bitio_write(fd_w, (uint64_t *)&checksum, 32);
    if (ret != 32) {
        printf("Error writing dictionary length\n");
        return -1;
    }
    return 0;
}

int 
compress(int fd_r, struct bitio *fd_w, unsigned int dict_size,int v)
{
	unsigned char c;
	unsigned int father = 0, new_father;
	int ret, i, r;
	TABLE *dictionary;
	unsigned int checksum;
	CHECKENV *cs = checksum_init();
	dictionary = htable_new(dict_size);
	while((ret = read(fd_r, &c, sizeof(char))) > 0) {
	checksum_update(cs, (char *)&c, 1);
        if (htable_insert(dictionary, c, father, &new_father) == 1) {
        	if(v == 1){
        	printf("Insert in dictionary value:%c at father:%u\n",c,father);
        	}
            i = htable_index_bits(dictionary);
            if (v == 1){
            printf("Number of bits read:%d\n",i);
            printf("Writing the node %u, in file compressed\n",father);
            }
            r = bitio_write(fd_w, (uint64_t *)&father, i);
            
            if (r != i) {
                printf("Error writing the compressed file\n");
                return -1;
            }
        }
        father = new_father;
    }
    if (ret < 0) {
        printf("Error reading file to compress\n");
        return -1;
    }
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    father = 0;
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    
    // File checksum
    checksum = (unsigned int)htole32(checksum_final(cs));
    bitio_write(fd_w, (uint64_t *)&checksum, 32);

    checksum_destroy(cs);
    htable_destroy(dictionary);
    close(fd_r);
    bitio_close(fd_w);
    return 1;
}
