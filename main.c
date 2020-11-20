#include <stdio.h>

#include "disk.h"
#include "sfs.h"


void test_disk_read_write(disk* diskptr) {
	char sample_write_block[4096] = {'A', 'B', 'C', '\0'};
	char sample_read_block[4096];

	write_block(diskptr, 0, sample_write_block);
	read_block(diskptr, 0, sample_read_block);

	printf("%s\n\n", sample_read_block);
}

int main(){
	
	// disk* diskptr = create_disk(40960+24);
	disk* diskptr = create_disk(409624);
	

	/* testing disk */
	printf("size : %d\n", diskptr->size);
	printf("blocks : %d\n\n", diskptr->blocks);
	
	// test_disk_read_write(diskptr);
	
	
	/* testing file system */
	format(diskptr);
	mount(diskptr);

	// int inode_index;
	// inode_index = create_file();
	// inode_index = create_file();
	// remove_file(inode_index);
	// inode_index = create_file();

	print_block(diskptr, 1);
	int inode_index = create_file();
	print_block(diskptr, 1);
	stat(inode_index);
	remove_file(inode_index);
	stat(inode_index);

	// 
	
	// for (int i=0; i< 200; i++) {
	// 	printf("i : %d\n", i);
	// 	int inode_index = create_file();
	// 	// printf("i : %d inode_index : %d\n", i, inode_index);
	// }
	



	return 0;
}