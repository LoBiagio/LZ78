#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include "htable.h"
#include "bitio.h"
#define DICT_SIZE 2000
typedef struct
{
	unsigned int father;
	unsigned char value;
}DENTITY;

struct darray
{
	DENTITY* dictionary;
 	unsigned int nmemb;
 	unsigned int dim;
}

struct darray *
array_new(unsigned int size)
{
	struct darray* tmp;
	tmp=calloc(1,sizeof(struct darray));
	if (tmp == NULL){
		return NULL;
	}
	tmp->dictionary = calloc(DICT_SIZE,sizeof(DENTITY));
	if (tmp->dictionary == NULL){
		return NULL;
	}
	memset(tmp->dictionary,0,DICT_SIZE * sizeof(DENTITY));
	tmp->nmemb = 0;
	tmp->dim = size;
	return tmp;		
}

int
insert (struct darray *da, unsigned int father, unsigned char value)
{
	da->dictionary[da->nmemb].father = father;
	da->dictionary[da->nmemb].value = value;
}

unsigned int
get_size (struct darray* da)
{
	return da->nmemb;
}


int main() {


}
