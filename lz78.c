#include "bitio.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DICT_SIZE 1000
int compress(int,struct bitio*,unsigned int,int);
int decompress(int,struct bitio*,unsigned int,int);
int write_header(int, struct bitio*,char *,unsigned int);
int read_header(struct bitio*,unsigned int *, int);

/**
 * @brief In the main we can choose these options without any argument:
 * c for compress, d for decompress, h for help, v for verbose;
 *	and these options with the relative argument:
 *	i with the name of the input file, o with the name of the output file and
 *	s with the size of the dictionary.
 */
int main(int argc, char *argv []) {
    int fd, s = 0, v = 0, compr = 0;
    //compr is set to 1 if we want to compress, set to 2 if we want to decompress
    char *source, *dest;
    unsigned int dict_size = DICT_SIZE, d_dict_size;
    struct bitio *fd_bitio;
    int opt;

    while ((opt = getopt(argc,argv,"cdhvi:o:s:")) != -1){
		switch (opt){
			case 'c':
			compr = (compr==2?compr:1);
			break;
			case 'd':
			compr = (compr==1?compr:2);
			break;
			case 'i':
				source = optarg; //take the input name from optarg
			break;
			case 'o':
				dest = optarg; //take the output name from optarg
			break;
			case 's':
			dict_size = atoi(optarg);
			s = 1;
			break;
			case 'h':
				printf("Usage: c for compress, d for decompress, i <input file>, o <output file>, d <dictionary_size>\n");
				break;
			case 'v':
				v = 1;
			break;
			case '?':
			if(optopt == 'i'){
				printf("An input file is required\n");
				exit(1);
			} else if(optopt == 'o'){
				printf("No name specified for destination file\n");
				exit(1);
			} else if(optopt == 's'){
				printf("No dimension specified for dictionary size\n");
				exit(1);
			} else{
				printf("Try -h for help\n");
				exit(1);
			}
			break;
			default:
				printf("Try -h for help\n");
				exit(1);
	 	}	
	}
	if(compr == 1){	//compressing
		if ((fd = open(source, O_RDONLY)) < 0) {
			perror("Error opening file in read mode: ");
			exit(1);
		}
		if ((fd_bitio = bitio_open(dest, 'w')) == NULL) {
			perror("Error opening file in write mode: ");
			close(fd);
			exit(1);
		}
		write_header(fd,fd_bitio,source,dict_size);
		printf("Compressing...\n");
		compress(fd,fd_bitio,dict_size,v);
		printf("Compress completed\n");
	}
	if (compr == 2){ //decompressing
		if (s == 1){ //if s is set on the decompressor we return an error
			printf("Error on  specifying dictionary size\n");
			exit(1); 
		}
		if ((fd = open (dest, (O_CREAT | O_TRUNC | O_WRONLY) , 0666)) < 0) {
			perror("Error opening file in write mode: ");
			exit(1);
		}
		if ((fd_bitio = bitio_open (source, 'r')) == NULL){
			perror("Error opening file in read mode: ");
			close(fd);
			exit(1);
		}
		read_header(fd_bitio,&d_dict_size, v);
		if (v == 1) {
		    printf("Decompressing...\n");
		}
		decompress(fd,fd_bitio,d_dict_size,v);
		if (v == 1) {
		    printf("Decompress completed\n");
		}
	}
	return 0;

}
