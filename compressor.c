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

    if (write_header(fd_r, fd_w, filename, dict_size) < 0) {
        close(fd_r);
        bitio_close(fd_w);
        htable_destroy(dictionary);
        exit(1);
    }

    while((ret = read(fd_r, &c, sizeof(char))) > 0) {
        if (htable_insert(dictionary, c, father, &new_father) == 1) {
            bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
        }
        father = new_father;
    }
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    father = 0;
    bitio_write(fd_w, (uint64_t *)&father, htable_index_bits(dictionary));
    close(fd_r);
    bitio_close(fd_w);
    return 0;
end:
    htable_destroy(dictionary);
    close(fd_r);
    bitio_close(fd_w);
    exit(1);
}
