#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct hentry //TODO//FIXME
{
    unsigned char value;
    unsigned int father; // All the elements with father = 0 are outside the dictionary
    unsigned int index;
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
        free(table);
        return NULL;
    }
    table->dim = size;
    return table;
}

void htable_clear(TABLE *table){
    int i;
    memset(table->entries, 0, table->dim * sizeof(ENTRY));
    table->nmemb = 0;
}

int htable_nmemb(TABLE *table) {
    return table->nmemb;
}

unsigned int get_hash(char value, int index) {
    unsigned int hash = 5381u;
    return ((hash << 5) + hash) + value + index;
}

/* index is the position where the value has been found or can be stored */
int htable_search(TABLE *table, unsigned char value, int father, int *index) {
    int search = 0, found = 0;
    unsigned int hash;
    hash = get_hash(value, father);
    hash %= table->dim;
    while (search != table->dim) {
        // Element does not exist, hash is on the first free array cell found
        // If father == 0, the entry is empty
        if (table->entries[hash].father == 0) {
            break;
        }
        // Element found
        if (table->entries[hash].father == father && table->entries[hash].value == value) {
            found = 1;
            break;
        }

        hash = (hash + 1) % table->dim;
        search++;
    }
    if (index != NULL) {
        *index = hash;
    }
    return found;
}

/* Insert in table a value of len bits */
int htable_insert(TABLE *table, unsigned char value, unsigned int father, unsigned int *index) {
    // Element already exists
    if (htable_search(table, value, father, index)) {
        printf("Error: the value is already in the dictionary.\n");
        return 0;
    }
    // Table is full
    if (table->nmemb == table->dim) {
        return -1;
    }

    table->entries[*index].value = value;
    table->entries[*index].father = father;
    table->nmemb++;
    return 1;
}

void htable_destroy(TABLE *table) {
    memset(table->entries, 0, table->dim * sizeof(ENTRY));
    free(table->entries);
    table->dim = 0;
    table->nmemb = 0;
    free(table);
}
