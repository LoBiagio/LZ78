#include "bitio.h"
#include <fcntl.h>
#include <stdio.h>

#define DICT_SIZE 1000
int compress(int,struct bitio*,unsigned int);
int decompress(int,struct bitio*,unsigned int);
int main() {
    int fd;
    unsigned int dict_size = DICT_SIZE;
    struct bitio *fd_bitio;
    if ((fd = open("B", O_RDONLY)) < 0) {
        perror("Error opening file in read mode: ");
        exit(1);
    }
    if ((fd_bitio = bitio_open("compressed", 'w')) == NULL) {
        perror("Error opening file in write mode: ");
        close(fd);
        exit(1);
    }
    compress(fd,fd_bitio,dict_size);
    if ((fd = open ("MYFILE", (O_CREAT | O_TRUNC | O_WRONLY) , 0666)) < 0) {
        perror("Error opening file in write mode: ");
        exit(1);
        }
    if ((fd_bitio = bitio_open ("compressed", 'r')) == NULL){
      	perror("Error opening file in read mode: ");
        close(fd);
        exit(1);
      }
    decompress(fd,fd_bitio,dict_size);
   
    
/*
    if (write_header(fd_r, fd_w, filename, dict_size) < 0) {
        close(fd_r);
        bitio_close(fd_w);
        htable_destroy(dictionary);
        exit(1);
    }
*/   
    return 0;
//end:
//    close(fd_r);
//    bitio_close(fd_w);
//    exit(1);
}
