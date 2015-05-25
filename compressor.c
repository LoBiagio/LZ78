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
#define DICT_SIZE 2

int write_header(int fd_r, struct bitio *fd_w, char *filename, int dict_size) {
    int ret, i, size;
    uint64_t file_size, last_mod;
    struct stat file_info;
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
    // File name
    for (i = 0; i < size; i++) {
        ret = bitio_write(fd_w, (uint64_t *)&filename[i], 8);
        if (ret != 8) {
            printf("Error writing file name\n");
            return -1;
        }
    }
    // File size
    file_size = htole64((uint64_t)file_info.st_size);
    ret = bitio_write(fd_w, &file_size, 64);
    if (ret != 64) {
        printf("Error writing file size\n");
        return -1;
    }
    // Time of last modification
    last_mod = htole64((uint64_t)file_info.st_mtim.tv_sec);
    ret = bitio_write(fd_w, &last_mod, 64);
    if (ret != 64) {
        printf("Error writing file's last modification\n");
        return -1;
    }
    // File checksum
    // TODO

    // Dictionary length
    dict_size = htole32(dict_size);
    ret = bitio_write(fd_w, (uint64_t *)&dict_size, 32);
    if (ret != 32) {
        printf("Error writing dictionary length\n");
        return -1;
    }
    return 0;
}

int main() {
    int fd_r, dict_size = DICT_SIZE, i, r;
    struct bitio *fd_w;
    int ret;
    char *filename = "B";
    unsigned char c;
    unsigned int father = 0, new_father;
    TABLE *dictionary;

    if ((fd_r = open("B", O_RDONLY)) < 0) {
        perror("Error opening file in read mode: ");
        exit(1);
    }
    if ((fd_w = bitio_open("compressed", 'w')) == NULL) {
        perror("Error opening file in write mode: ");
        close(fd_r);
        exit(1);
    }
    dictionary = htable_new(dict_size);

/*
    if (write_header(fd_r, fd_w, filename, dict_size) < 0) {
        close(fd_r);
        bitio_close(fd_w);
        htable_destroy(dictionary);
        exit(1);
    }
*/
    while((ret = read(fd_r, &c, sizeof(char))) > 0) {
        if (htable_insert(dictionary, c, father, &new_father) == 1) {
            i = htable_index_bits(dictionary);
            r = bitio_write(fd_w, (uint64_t *)&father, i);
            if (r != i) {
                printf("Error writing the compressed file\n");
                goto end;
            }
        }
        father = new_father;
    }
    if (ret < 0) {
        printf("Error reading file to compress\n");
        goto end;
    }
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    father = 0;
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));

    htable_destroy(dictionary);
    close(fd_r);
    bitio_close(fd_w);
    return 0;
end:
    htable_destroy(dictionary);
    close(fd_r);
    bitio_close(fd_w);
    exit(1);
}
