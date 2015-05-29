#ifndef CHECKSUM_H_INCLUDED
#define CHECKSUM_H_INCLUDED

typedef struct checksum_enviroment CHECKENV;

CHECKENV *checksum_init();
void checksum_update(CHECKENV *checksum, char *buf, int size);
unsigned int checksum_final(CHECKENV *checksum);
void checksum_destroy(CHECKENV *checksum);


#endif // CHECKSUM_H_INCLUDED
