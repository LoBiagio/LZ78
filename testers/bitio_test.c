#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include "bitio.h"
#define STRIDE  140

int main()
{
    struct bitio *fd1;
    struct bitio *fd2;
    int ret;
    uint64_t prova[STRIDE % 64 != 0 ? STRIDE / 64 + STRIDE % 64 : STRIDE / 64];

    fd1 = bitio_open("B",'r');
    if (fd1 < 0) {
        printf("Errore bitio_open() read mode\n");
        exit(1);
    }

    fd2 = bitio_open("C", 'w');
    if (fd2 < 0) {
        printf("Errore open() write mode\n");
        exit(1);
    }
    do {
        ret = bitio_read(fd1, prova, STRIDE);
        if (ret < 0) {
            perror("Errore bitio_read()");
            exit(1);
        }
        if (ret == 0) {
            break;
        }
        ret = bitio_write(fd2, prova, ret);
        if (ret < 0) {
            printf("Errore bitio_write()\n");
            exit(1);
        }
    } while (ret >= 0);

    bitio_close(fd1);
    bitio_close(fd2);
    return 0;
}
