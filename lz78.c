#include "bitio.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DICT_SIZE 1000
int compress(int,struct bitio*,unsigned int,int);
int decompress(int,struct bitio*,unsigned int,int);
int write_header(int, struct bitio*,char *,unsigned int);
int read_header(struct bitio*,unsigned int *);

int main(int argc, char *argv []) {
    int fd, index, d_v = 0, c_v = 0, compr = 1;
    char *c_source, *c_dest = "compressed", *d_source = "compressed", *d_dest = "NEWFILE";
    unsigned int dict_size = DICT_SIZE, d_dict_size;
    struct bitio *fd_bitio;
    int opt;
    while ((opt = getopt(argc,argv,"cdvi:o:s:ih")) != -1){
		switch (opt){
			case 'c':
			compr = 1;
			break;
			case 'd':
			compr = 2;
			break;
			case 'i':
			if (compr == 1){
				c_source = optarg;
			} 
			if(compr == 2){
				d_source = optarg;
			}
			break;
			case 'o': //FIXME così non prende il nome output se -d lo specifico da ultimo
			if(compr == 1){
				c_dest = optarg;
			}
			if(compr == 2){
				d_dest = optarg;
			}
			break;
			case 's':
			if(compr == 1){
			dict_size = atoi(optarg);
			} 
			else{
				fprintf(stderr,"size specified only with option c\n");
			}
			break;
				case 'h':
				printf("Usage: i <input file>, o <output file>, s <dictionary_size>\n");
				exit(1);
				break;
			case 'v':
			if (compr == 1){
				c_v = 1;
			}
			if (compr == 2){
				d_v = 1;
			}
			break;
			case '?':
			if(optopt == 'i'){
				printf("An input file is required\n");
				exit(1);
			}
			if(optopt == 'o'){
				printf("No name specified for destination file\n");
				exit(1);
			}
			if(optopt == 's'){
				printf("No dimension specified for dictionary size\n");
				exit(1);
			}
			/*	default:
				printf("try -h for help\n");
				exit(1);
	*/ 	}	
	}
	if(compr == 1){	
		if ((fd = open(c_source, O_RDONLY)) < 0) {
			perror("Error opening file in read mode: ");
			exit(1);
		}
		if ((fd_bitio = bitio_open(c_dest, 'w')) == NULL) {
			perror("Error opening file in write mode: ");
			close(fd);
			exit(1);
		}
		write_header(fd,fd_bitio,c_source,dict_size);
		printf("Compressing...\n");
		compress(fd,fd_bitio,dict_size,c_v);
		printf("Compress completed\n");
	}
	if (compr == 2){
		if ((fd = open (d_dest, (O_CREAT | O_TRUNC | O_WRONLY) , 0666)) < 0) {
			perror("Error opening file in write mode: ");
			exit(1);
		}
		if ((fd_bitio = bitio_open (d_source, 'r')) == NULL){
			perror("Error opening file in read mode: ");
			close(fd);
			exit(1);
		}
		read_header(fd_bitio,&d_dict_size);
		printf("Decompressing...\n");
		decompress(fd,fd_bitio,d_dict_size,d_v);
		printf("Decompress completed\n");
	}
    		
  /*  for (index = optind; index < argc; index++){
    	printf ("Non-option argument %s\n", argv[index]);   
    }*/

	return 0;
end:
 //   close(fd);
 //   bitio_close(fd_bitio);
    exit(1);

}
