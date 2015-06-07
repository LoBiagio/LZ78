#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include "htable.h"
#include "bitio.h"
#include "checksum.h"

/**
 * Write header function
 * It writes informations about file to compress:
 *  - Number of characters for file name (8 bit)
 *  - Original file name (variable length)
 *  - Original file size (64 bit)
 *  - Time of last modification (64 bit)
 *  - Dictionary length (32 bit)
 *  - Header chechsum (32 bit)
 * @param fd_r The file-to-compress descriptor
 * @param fd_w The bitio-file-to-write descriptor
 * @param filename The string passed as input to the compressor containing the
 * path of the file to compress
 * @param dict_size The dictionary dimension
 * @return 0 on succes, -1 on fail
 */
int
write_header(int fd_r, struct bitio *fd_w, char *filename, unsigned int dict_size) {
    int ret, i, size;
    uint64_t file_size, last_mod;
    unsigned int checksum;
    struct stat file_info;
    CHECKENV *cs = checksum_init();
    // Get file-to-compress info
    if (fstat(fd_r, &file_info) < 0) {
        perror("Error retriving file info: ");
        return -1;
    }
    // File name length
    filename = basename(filename);
    size = strlen(filename);
    if (size > 255) {
        size = 255;
    }
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

/**
 * Compress function
 * LZ78 algorithm to compress a text file. It also computes the file-to-compress
 * checksum as if it reads its contents.
 * @param fd_r The file-to-compress descriptor
 * @param fd_w The bitio-file-to-write descriptor
 * @param dict_size The dictionary dimension
 * @param v Verbose property
 * @return 0 on succes, -1 on fail
 */
int
compress(int fd_r, struct bitio *fd_w, unsigned int dict_size, int v)
{
    unsigned char c;
    unsigned int father = 0, new_father;
    int ret, i, r;
    TABLE *dictionary;
    unsigned int checksum;
    CHECKENV *cs = checksum_init();

    dictionary = htable_new(dict_size);
    while((ret = read(fd_r, &c, sizeof(char))) > 0) {
        // Update checksum value of the file-to-compress
        checksum_update(cs, (char *)&c, 1);
        // The couple <c,father> is inserted in dictionary if it is not already
        // present. If an insertion is performed, htable_insert returns 1, 0
        // otherwise. The function also updates the new_father: in case of a
        // match in the tree, new_father = index of the match. Otherwise
        // new_father = index of the couple <character which did not match,0>+1
        if (htable_insert(dictionary, c, father, &new_father) == 1) {
            i = 14;//htable_index_bits(dictionary);
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
    // Last value not written in the cycle
    i = 14;//htable_index_bits(dictionary);
    r = bitio_write(fd_w, (uint64_t *)&father, i);
    if (r != i) {
        printf("Error writing the compressed file\n");
        return -1;
    }
    // 0 is written to indicate the end of the compressed data
    father = 0;
    i = 14;//htable_index_bits(dictionary);
    r = bitio_write(fd_w, (uint64_t *)&father, i);
    if (r != i) {
        printf("Error writing the compressed file\n");
        return -1;
    }
    // File checksum
    checksum = (unsigned int)htole32(checksum_final(cs));
    r = bitio_write(fd_w, (uint64_t *)&checksum, 32);
    if (r != 32) {
        printf("Error writing the compressed file\n");
        return -1;
    }

    checksum_destroy(cs);
    htable_destroy(dictionary);
    close(fd_r);
    bitio_close(fd_w);
    return 0;
}
