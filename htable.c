#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct hentry
{
    char *value;
    int len;
} ENTRY;

struct htable
{
    ENTRY *entries;
    int nmemb;
    int dim;
};

TABLE *htable_new(int size) {
    TABLE *table = (TABLE *)calloc(1, sizeof(TABLE));
    if (table == NULL) {
        printf("Error: table allocation failed.\n");
        return NULL;
    }
    table->nmemb = 0;
    table->entries = (ENTRY *)calloc(size, sizeof(ENTRY));
    if (table->entries == NULL) {
        printf("Error: dictionary allocation failed.\n");
        table->dim = 0;
        free(table);
        return NULL;
    }
    table->dim = size;
    return table;
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

unsigned int get_hash(char *value, int len) {
    unsigned int hash = 5381u;
    int i;

    for (i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + value[i];
    }

    return hash;
}

/* index is the position where the value has been found or can be stored */
int htable_search(TABLE *table, uint64_t *value, int len, unsigned int *index) {
    int len_bytes = len % 8 != 0 ? len / 8 + 1 : len / 8;
    int search = 0, found = 0;
    unsigned int hash;
    hash = get_hash((char *)value, len_bytes);
    hash %= table->dim;
    while (search != table->dim) {
        // Element does not exist, hash is on the first free array cell found
        if (table->entries[hash].len == 0) {
            break;
        }
        // Element found
        if (len == table->entries[hash].len && !memcmp(value, table->entries[hash].value, len_bytes)) {
            found = 1;
            break;
        }
        
        //* Delete this block *//
        if (DETECT_COLLISION) {
            DETECT_COLLISION = 0;
            COLLISIONS++;
            printf("Collision\n");
        }
        //*********************//
        
        hash = (hash + 1) % table->dim;
        search++;
    }
    if (index != NULL) {
        *index = hash;
    }
    return found;
}

/* Insert in table a value of len bits */
int htable_insert(TABLE *table, uint64_t *value, int len) {
    unsigned int index;
    int len_bytes;

    // Table is full
    if (table->nmemb == table->dim) {
        printf("Error: the dictionary is full.\n");
        return -1;
    }

    // Element already exists
    if (htable_search(table, value, len, &index)) {
        printf("Error: the value is already in the dictionary.\n");
        return -1;
    }

    len_bytes = len % 8 != 0 ? len / 8 + 1 : len / 8;
    table->entries[index].value = (char *)calloc(len_bytes, sizeof(char));
    // Allocation fails
    if (table->entries[index].value == NULL) {
        printf("Error: allocation for new entry in dictionary failed.\n");
        return -1;
    }
    if(memcpy(table->entries[index].value, value, len_bytes) == NULL) {
        printf("Error: copying new entry in dictionary failed.\n");
        free(table->entries[index].value);
        return -1;
    }
    table->entries[index].len = len;
    table->nmemb++;
    return 1;
}

/* Remove from table an entry value of len bits */
int htable_remove(TABLE *table, uint64_t *value, int len) {
    unsigned int index;

    // Empty table
    if (table->nmemb == 0) {
        printf("Error: the dictionary is empty, nothing to remove.\n");
        return -1;
    }

    // Element not found
    if (!htable_search(table, value, len, &index)) {
        printf("Error: the element to remove is not in the dictionary.\n");
        return -1;
    }
    free(table->entries[index].value);
    table->entries[index].len = 0;
    table->nmemb--;
    return 0;
}
