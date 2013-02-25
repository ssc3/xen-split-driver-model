/* Cache example*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define BLOCKSIZE 512
char image[] =
{
	'P', '5', ' ', '2', '4', ' ', '7', ' ', '1', '5', '\n',
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 3, 3, 3, 0, 0, 7, 7, 7, 7, 0, 0,11,11,11,11, 0, 0,15,15,15,15, 0,
	0, 3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0,15, 0, 0,15, 0,
	0, 3, 3, 3, 0, 0, 0, 7, 7, 7, 0, 0, 0,11,11,11, 0, 0, 0,15,15,15,15, 0,
	0, 3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0,11, 0, 0, 0, 0, 0,15, 0, 0, 0, 0,
	0, 3, 0, 0, 0, 0, 0, 7, 7, 7, 7, 0, 0,11,11,11,11, 0, 0,15, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
};
int main(int argc, char** argv)
{
	//unsigned char *buffer;
        unsigned char* buffer2;

	int sector_num=atoi(argv[1]);
	char* filename = argv[2];

	posix_memalign((void*) &buffer2, BLOCKSIZE, 4096);

	int f = open(filename, O_DIRECT|O_SYNC);
	//write(f, buffer, BLOCKSIZE);
	int j = 0;
	lseek (f, sector_num*512, SEEK_SET);
	read(f, buffer2, 512);
	//for(j=0; j<BLOCKSIZE; j++)
	printf("%x ", buffer2[BLOCKSIZE - 1]);
	printf("\n");
	free(buffer2);
	return 0;
}
