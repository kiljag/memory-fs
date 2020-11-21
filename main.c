#include <stdio.h>
#include <stdlib.h>

#include "disk.h"
#include "sfs.h"
#include "util.h"



void test_disk_read_write(disk* diskptr) {
	char sample_write_block[4096] = {'A', 'B', 'C', '\0'};
	char sample_read_block[4096];

	write_block(diskptr, 0, sample_write_block);
	read_block(diskptr, 0, sample_read_block);

	// printf("%s\n\n", sample_read_block);

	for (int i = 0; i < 200; i++) {
		printf("i : %d\n", i);
		int* write_arr = (int *)sample_write_block;
		int* read_arr = (int *)sample_read_block;
		for (int j = 0; j < 1024; j++){
			write_arr[j] = (int)random();
		}

		print_block_data((char *)write_arr);
		write_block(diskptr, 2, (char *)write_arr);
		print_block(diskptr, 2);
		read_block(diskptr, 2, (char *)read_arr);
		print_block_data((char *)read_arr);
		printf("\n\n");
	}

}

void test_read_write_inode() {
	char data[10] = {'A', 'B', 'C'};
	int inode_index = create_file();
	
	int len = write_i(inode_index, data, sizeof(data), 0);
}

int main(){
	
	// disk* diskptr = create_disk(40960+24);
	disk* diskptr = create_disk(409624);
	
	printf("size : %d\n", diskptr->size);
	printf("blocks : %d\n\n", diskptr->blocks);
	
	
	/* testing file system */
	format(diskptr);
	mount(diskptr);
	
	
	// int inode_index = create_file();
	// stat(inode_index);
	// remove_file(inode_index);
	// stat(inode_index);
	
	test_read_write_inode();

	return 0;
}