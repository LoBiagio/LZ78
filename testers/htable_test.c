#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "htable.h"
#include "bitio.h"
#define DICT_SIZE 2000

int main() {
    COLLISIONS = 0;
    struct bitio *fd;
    TABLE *dictionary;
    int ret, size = 1, len_uint64;
    uint64_t *buf;
    unsigned int i;

    fd = bitio_open("B",'r');
    if (fd < 0) {
        printf("Errore bitio_open() read mode\n");
        exit(1);
    }

    dictionary = htable_new(DICT_SIZE);
    if (dictionary == NULL) {
        exit(1);
    }

    do {
        len_uint64 = size % 64 != 0 ? size / 64 + 1 : size / 64;
        buf = (uint64_t *)calloc(len_uint64, sizeof(uint64_t));
        ret = bitio_read(fd, buf, size);
        if (ret < 0) {
            printf("Error bitio_read()\n");
            exit(1);
        }
        if (ret == 0) {
            break;
        }
        DETECT_COLLISION = 1;
        if (htable_insert(dictionary, buf, ret) < 0) {
            printf("Error htable_insert()\n");
        }
        DETECT_COLLISION = 0;
        if (htable_search(dictionary, buf, ret, &i) == 0) {
            printf("Error htable_search()\n");
        }

        //* Optional *//
        if (htable_remove(dictionary, buf, ret) < 0) {
            printf("Error htable_remove()\n");
        }
        //************//

        size++;
        free(buf);
    } while (1);

    // How many collision in htable_insert()
    ret = COLLISIONS;
    htable_clear(dictionary);
    bitio_close(fd);
    return 0;
}
