#include <stdio.h>

#include "disk.h"
#include "sfs.h"


int main(){
	
	disk* diskptr = create_disk(40984);

	/* testing disk */
	printf("size : %d\n", diskptr->size);
	printf("blocks : %d\n", diskptr->blocks);
	printf("diskptr : %p\n", diskptr);
	printf("block_arr : %p\n", diskptr->block_arr);
	
	char sample_write_block[4096] = {'A', 'B', 'C', '\0'};
	char sample_read_block[4096];

	write_block(diskptr, 0, sample_write_block);
	read_block(diskptr, 0, sample_read_block);

	printf("%s\n", sample_read_block);

	/* testing file system */
	format(diskptr);
	mount(diskptr);
	




	return 0;
}