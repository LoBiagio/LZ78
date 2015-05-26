#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checksum.h"

struct checksum_enviroment {
    char partial[4];
    int sum;
    int count;
};

CHECKENV *
checksum_init() {
    CHECKENV *checksum = (CHECKENV *)calloc(1, sizeof(CHECKENV));
    return checksum;
}

void
checksum_update(CHECKENV *checksum, char *buf, int size) {
    int i = 0, j;
    while (size > 0) {
        checksum->partial[checksum->count] = buf[i];
        i++;
        checksum->count++;
        if (checksum->count == 4) {
            checksum->sum += le32toh((int)*checksum->partial);
            for (j = 0; j < 4; j++) {
                checksum->partial[j] = 0;
            }
            checksum->count = 0;
        }
        size--;
    }
}

unsigned int
checksum_final(CHECKENV *checksum) {
    if (checksum->count != 0) {
        checksum->sum += le32toh((int)*checksum->partial);
    }
    return (unsigned int)htole32(0 - checksum->sum);
}

int
checksum_is_valid (CHECKENV *ce, unsigned int in)
{
    return (ce->partial + in == 0);
}

void
checksum_destroy(CHECKENV *checksum) {
    free(checksum);
}
