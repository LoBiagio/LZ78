#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct hentry
{
    uint64_t *value;
    int len;
};


int htable_init(TABLE *table, int size) {
    table->nmemb = 0;
    table->entries = (ENTRY *)calloc(size + (size / 2), sizeof(ENTRY));
    if (table->entries == NULL) {
        return -1;
    }
    table->dim = size + (size / 2);
    return 1;
}

void htable_clear(TABLE *table){
    int i;
    for (i = 0; i < table->dim; i++) {
        if (table->entries[i].len) {
            free(table->entries[i].value);
            table->entries[i].len = 0;
        }
    }
    free(table->entries);
    table->nmemb = 0;
    table->dim = 0;
}

int htable_nmemb(TABLE *table) {
    return table->nmemb;
}

uint64_t get_hash(uint64_t *value, int len) {
    uint64_t hash = 5381lu, c;
    int i;

    for (i = 0; i < len; i++) {
        c = value[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int htable_search(TABLE *table, uint64_t *value, int len, uint64_t *index) {
    int uint64_len = len % 64 != 0 ? len / 64 + 1 : len / 64;
    int search = 0;
    *index = get_hash(value, uint64_len);
    *index %= table->dim;
    while (search != table->dim) {
        // Element does not exist, index is on the first free array cell found
        if (table->entries[*index].len == 0) {
            return 0;
        }
        // Element found
        if (len == table->entries[*index].len && memcmp(value, table->entries[*index].value, uint64_len)) {
                return 1;
        }
        ///////////////////////

        printf("COLLISION\n");

        ///////////////////////
        *index = (*index + 1) % table->dim;
        search++;
    }
    // Element not found
    return 0;
}
/* Insert in table an entry value of len bits */
int htable_insert(TABLE *table, uint64_t *value, int len) {
    uint64_t index;

    // Table is full
    if (table->nmemb == table->dim) {
        return -1;
    }

    // Element already exists
    if (!htable_search(table, value, len, &index)) {
        return -1;
    }

    table->entries[index].value = (uint64_t *)calloc((len % 64 != 0 ? len / 64 + 1 : len / 64), sizeof(uint64_t));
    // Allocation fails
    if (table->entries[index].value == NULL) {
        return -1;
    }
    if(memcpy(table->entries[index].value, value, len) == NULL) {
        free(table->entries[index].value);
        return -1;
    }
    table->entries[index].len = len;
    table->nmemb++;
    return 1;
}

/* Remove from table an entry value of len bits */
int htable_remove(TABLE *table, uint64_t *value, int len) {
    uint64_t index;

    // Empty table
    if (table->nmemb == 0) {
        return -1;
    }

    // Element not found
    if (!htable_search(table, value, len, &index)) {
        return -1;
    }
    free(table->entries[index].value);
    table->entries[index].len = 0;
    table->nmemb--;
    return 0;
}
