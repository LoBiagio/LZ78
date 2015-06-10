#include "htable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "log2.h"

/* This is the initial number of bit to be written by the compressor.
 * There are 258 symbols in the compressor dictionary when the first index is 
 * sent.*/
#define INITIAL_BITS_NUMBER 9

/* With how_many_bits set to INITIAL_BITS_NUMBER we have at most 512 different 
 * symbols */
#define MAX_INITIAL_NUMBER_OF_ELEMENTS 512

typedef struct hentry
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
    
    /** how many bit we need to write at each iteration. This is based on the 
     * number of entries in the dictionary
     */
    unsigned int how_many_bits;
    
    /** Used to decide when to increment the value of how_many_bits. */
    unsigned int threshold;
};

/**
 * New table
 * It creates a new hash table with a dimension specified by size * 1.5
 * @param size The max number of elements which can be stored in the table
 * @return The pointer to the table
 */
TABLE *
htable_new(int size) {
    TABLE *table = (TABLE *)calloc(1, sizeof(TABLE));
    if (table == NULL) {
        printf("Error: table allocation failed.\n");
        return NULL;
    }
    table->nmemb = 0;
    table->entries = (ENTRY *)calloc(size + (size / 2), sizeof(ENTRY));
    if (table->entries == NULL) {
        printf("Error: dictionary allocation failed.\n");
        free(table);
        return NULL;
    }
    
    table->how_many_bits = INITIAL_BITS_NUMBER;
    table->threshold = MAX_INITIAL_NUMBER_OF_ELEMENTS;
    table->dim = size;
    return table;
}

/**
 * Clear table
 * Set all values to 0
 * @param table The table to clear
 */
void
htable_clear(TABLE *table) {
    memset(table->entries, 0, (table->dim + (table->dim / 2))* sizeof(ENTRY));
    table->nmemb = 0;
    table->how_many_bits = INITIAL_BITS_NUMBER;
    table->threshold = MAX_INITIAL_NUMBER_OF_ELEMENTS;
}

/**
 * Hash function
 * Generate a hash value from the couple <char,father>. Use sp to better spread
 * the result
 * @param value The first component used to compute hash
 * @param index The second compenent used to compute hash
 * @param sp It increases dispersion
 * @return The computed hash value
 */
unsigned int
get_hash(char value, int index, unsigned int sp) {
    unsigned int hash = 5381u;
    return ((hash << 5) + hash) * value + index * (sp - value);
}
/**
 * This function writes in the param index the position in the table where the
 * couple <value,father> has been found or, if it is not present, the position
 * where the couple can be stored
 * @param table The hash table where <value,father> is searched
 * @param value The first field of the couple to search/insert in the table
 * @param father The second field of the couple to search/insert in the table
 * @param index This value will contain the position in the hash table where the
 * couple <value,father> has been found/has to be stored.
 * @return 1 if an insertion is performed, 0 if the couple <value,father> is
 * already in the hash table
 */
/* index is the position where the value has been found or can be stored */
int
htable_getPosition(TABLE *table, unsigned char value, int father, unsigned int *index) {
    int search = 0;
    int tabdim = table->dim + table->dim / 2;
    // <value, father> conresponds to a prestored element
    if (father == 0) {
        return 1;
    }
    *index = get_hash(value, father, tabdim);
    *index %= tabdim;
    while (search != tabdim) {
        // Element does not exist, index is pointing on the first free array
        // cell found. If father == 0, the entry is empty
        if (table->entries[*index].father == 0) {
            break;
        }
        // Element found
        if (table->entries[*index].father == father && table->entries[*index].value == value) {
            return 1;
        }
        // COLLISION:
        // In the actual position in the hash table there is a couple that
        // differs from <value,father>
        *index = (*index + 1) % tabdim;
        search++;
    }
    return 0;
}

/**
 * Insert in table the couple <value,father> i.e. a char and its father
 * @param table The hash table where the couple has to be stored
 * @param value The first field of the couple to insert in the hash table
 * @param father The second field of the couple to insert in the hash table
 * @param new_father This field stores the index of the couple <value,father>
 * which is already in the hash table (i.e. a match in the tree occoured).
 * Otherwise it stores the index of the couple <value,0> (i.e. a chain of
 * characters did not match any tree branch)
 * @return 1 if insertion succeeds, 0 if <value,father> is already in table
 */
int
htable_insert(TABLE *table, unsigned char value, unsigned int father, unsigned int *new_father) {
    unsigned int position;
    if (htable_getPosition(table, value, father, &position)) {
        // Couple <value,father> is already in table: the new_father takes the
        // index of the matched couple in the table
        if (father == 0) {
            *new_father = (unsigned int)value + 1;
        }
        else {
            *new_father = table->entries[position].index;
        }
        return 0;
    }

    // Table is full
    if (table->nmemb == table->dim) {
        htable_clear(table);
        // new_father conresponds to the value of the node that has just been
        // insterted even if it is not a real insertion: the node with father 0
        // is already in the dictionary
        *new_father = (unsigned int)value + 1;
        // An insertion would have been performed if table wasn't full, so 1 is returned
        // and the father is passed to decompressor once this function ends
        return 1;
    }

    // Insert value
    table->entries[position].value = value;
    table->entries[position].father = father;
    table->entries[position].index = table->nmemb + 257;
    table->nmemb++;
    *new_father = (unsigned int)(value + 1);
    
    /* checking if we must increase the number of to be written bit.
     * At every instant we have exactly table->nmemb symbols in our dictionary 
     * + 257 non explicitly inserted symbols (root + first layer of root 
     * offsprings)*/
    if (table->nmemb + 257 > table->threshold) {
        table->how_many_bits++;
        table->threshold <<= 1;
    }
    return 1;
}

/**
 * This function returns the minimum number of bits on which the index of the
 * elements in the table should be rapresented
 */
unsigned int
htable_index_bits(TABLE *table) {
    // Once the dictionary is cleared the index of the father is sent to the 
    // decompressor.
    // That index is on the same bit length of table->dim
    return table->nmemb == 0 ? htable_log2(table->dim + 257) : htable_log2(table->nmemb + 257);
}

/**
 * This function resets the hash table, deallocates it and its entries
 */
void
htable_destroy(TABLE *table) {
    htable_clear(table);
    free(table->entries);
    table->dim = 0;
    table->threshold = 0;
    table->how_many_bits = 0;
    free(table);
}
