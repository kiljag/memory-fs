#include <stdio.h>
#include <stdlib.h>

#include "disk.h"
#include "sfs.h"
#include "util.h"



void test_disk_read_write(disk* diskptr) {
	
	return;
}

void test_read_write_inode() {
	char write_block[4096] = {'A', 'B', 'C', '\0'};
	char read_block[4096];

	int inode_index = create_file();
	stat(inode_index);

	printf("\n\n");
	for (int i = 0; i < 10; i++) {
		for(int j=0; j< 3000; j++) {
			write_block[j] = (char )(random() % 128);
			// printf("%x:", write_block[j]);
		}
		

		int write_len = write_i(inode_index, write_block, 3000, 3000*i);
		// printf("write_len : %d\n", write_len);
		// stat(inode_index);
	}
	printf("\n");

	int read_len = -1;
	int read_offset = 0;
	printf("\n\n");
	while((read_len = read_i(inode_index, read_block, 1000, read_offset)) > 0) {
		
		for (int j =0; j < read_len; j++) {
			// printf("%x:", read_block[j]);
		}
		read_offset += read_len;
		// printf("read_len : %d\n", read_len);
		// printf("read_offset : %d\n", read_offset);
		// printf("\n");
	}
	printf("\n");
	

	
}

void fill_random_char_data(char* data, int size) {
	for(int i=0; i<size; i++) {
		data[i] = 65 + (char)random() % 60;
	}
}

int main(){
	
	// disk* diskptr = create_disk(40960+24);
	// disk* diskptr = create_disk(409624);
	disk* diskptr = create_disk(4096*1000 + 24);
	
	printf("size : %d\n", diskptr->size);
	printf("blocks : %d\n\n", diskptr->blocks);
	
	
	/* testing file system */
	format(diskptr);
	mount(diskptr);
	
	
	/**/
	char *write_data = (char *)malloc(10000 * sizeof(char));
	char *read_data = (char *)malloc(10000 * sizeof(char));
	// fill_random_char_data(write_data);

	create_dir("dir1");
	fill_random_char_data(write_data, 10000);
	write_file("dir1/a.txt", write_data, 10000, 0);


	return 0;
}