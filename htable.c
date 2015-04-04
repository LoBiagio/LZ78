#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint64_t *value;
    int len;
} ENTRY;

typedef struct
{
    ENTRY *entries;
    int nmemb;
    int dim;
} TABLE;

int init_htable(TABLE * table, int size) {
    table->nmemb = 0;
    table->entries = (ENTRY *)calloc(size + (size / 2), sizeof(ENTRY));
    if (table->entries == NULL) {
        return -1;
    }
    table->dim = size + (size / 2);
    return 1;
}

void clear_htable(TABLE *table){
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

int get_entries_num(TABLE *table) {
    return table->nmemb;
}

uint64_t get_hash(uint64_t *str, int len) {
    uint64_t hash = 5381lu, c;
    int i;

    for (i = 0; i < len; i++) {
        c = str[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/* Insert in table an entry value of len bits */
int insert(TABLE *table, uint64_t *value, int len) {
    uint64_t hash;

    // Table is full
    if (table->nmemb == table->dim) {
        return -1;
    }

    hash = get_hash(value, len);
    hash %= table->dim;
    while (table->entries[hash].len) {
        hash = (hash + 1) % table->dim;
    }

    table->entries[hash].value = (uint64_t *)calloc((len % 64 != 0 ? len / 64 + 1 : len / 64), sizeof(uint64_t));
    // Allocation fails
    if (table->entries[hash].value == NULL) {
        return -1;
    }
    if(memcpy(table->entries[hash].value, value, len) == NULL) {
        free(table->entries[hash].value);
        return -1;
    }
    table->entries[hash].len = len;
    table->nmemb++;
    return 1;
}

/* Remove from table an entry value of len bits */
int remove(TABLE *table, uint64_t *value, int len) {
    uint64_t hash;
    int search = 0;

    // Empty table
    if (table->nmemb == 0) {
        return -1;
    }

    hash = get_hash(value, len);
    hash %= table->dim;
    while (!memcmp(table->entries[hash].value, value, (len % 64 != 0 ? len / 64 + 1 : len / 64)) && search != table->dim) {
        hash = (hash + 1) % table->dim;
        search++;
    }
    // Element not found
    if (search == table->dim) {
        return -1;
    }
    free(table->entries[hash].value);
    table->entries[hash].len = 0;
    table->nmemb--;
    return 0;
}
