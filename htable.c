#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

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
    int collision;
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

void htable_clear(TABLE *table) {
    memset(table->entries, 0, table->dim * sizeof(ENTRY));
    table->nmemb = 0;
}

int htable_nmemb(TABLE *table) {
    return table->nmemb;
}

unsigned int get_hash(char value, int index, unsigned int dim) {
    unsigned int hash = 5381u;
    return ((hash << 5) + hash) * value + index * (dim - value);
}

/* index is the position where the value has been found or can be stored */
int htable_getPosition(TABLE *table, unsigned char value, int father, unsigned int *index) {
    int search = 0;
    // <value, father> conresponds to a prestored element
    if (father == 0) {
        return 1;
    }
    *index = get_hash(value, father, table->dim);
    *index %= table->dim;
    while (search != table->dim) {
        // Element does not exist, index is pointing on the first free array cell found
        // If father == 0, the entry is empty
        if (table->entries[*index].father == 0) {
            break;
        }
        // Element found
        if (table->entries[*index].father == father && table->entries[*index].value == value) {
            return 1;
        }
        table->collision++;
        *index = (*index + 1) % table->dim;
        search++;
    }
    return 0;
}

/*  Insert in table a char and its father.
    <new_father> stores the index of <value> s.t. <value, father> is already in the table.
    Otherwise it stores the index of <value, 0> which has the same value of <value>
    It returns <1> if insertion succeeds
               <0> if <value, father> is already in table
*/
int htable_insert(TABLE *table, unsigned char value, unsigned int father, unsigned int *new_father,unsigned int *reset) {
    unsigned int position;
        // Element already in table
    if (htable_getPosition(table, value, father, &position)) {
        if (father == 0) {
            *new_father = (unsigned int)value;
        }
        else {
            *new_father = table->entries[position].index;
        }
        return 0;
    }
    // Table is full
    if (table->nmemb == table->dim-1) {
        htable_clear(table);
       // printf("DIZIONARIO AZZERATO\n");
        *reset = 1;
    }
    // Insert value
    table->entries[position].value = value;
    table->entries[position].father = father;
    table->entries[position].index = table->nmemb + 256;
    table->nmemb++;
    *new_father = (unsigned int)value;
    return 1;
}

int htable_index_bits(TABLE *table) {
    return (int)log2(table->nmemb + 255) + 1;
}

void htable_destroy(TABLE *table) {
    htable_clear(table);
    free(table->entries);
    table->dim = 0;
    free(table);
}

int htable_collision(TABLE *table) {
    return table->collision;
}
