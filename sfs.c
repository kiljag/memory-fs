#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"

static disk* mounted_disk_ptr = NULL;

/*utility functions*/ 

/*set bit at kth position in int array A*/
void setBit(unsigned int *A, unsigned int k) {
    int i = k / 32;
    int pos = k % 32;
    A[i] = A[i] | (1 << pos);
}

/*clear bit at kth position in int array A*/
void clearBit(unsigned int *A, unsigned int k) {
    int i = k / 32;
    int pos = k % 32;
    A[i] = A[i] & ~(1 << pos);
}

/*test if bit at kth position is set or not*/
int testBit(unsigned int *A, unsigned int k) {
    int i = k / 32;
    int pos = k % 32;

    if (A[i] & (1 << pos)) {
        return 1;
    } else {
        return 0;
    }
}

/*get the first available unset bit position*/
int getFirstAvailableBit(unsigned int *A, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if(A[i] != 0xFFFFFFFF) {
            printf("block found: %d\n", i);
            int pos;
            for(pos = 0; pos < 32; pos++) {
                unsigned int k = 32 * i + pos;
                if(!testBit(A, k)){
                    return k;
                };
            }
        }
    }

    return -1;
}

int format(disk *diskptr) {

    int total_blocks = diskptr->blocks;
    int inode_and_data_blocks =  total_blocks - 1;

    int inode_blocks = (int)(0.1 * inode_and_data_blocks);
    int num_inodes = inode_blocks * 128;
    int inode_bitmap_blocks = (num_inodes) / (8 * 4096);
    if (num_inodes % (8 * 4096) != 0) {
        inode_bitmap_blocks += 1;
    }

    int remaining_blocks = inode_and_data_blocks - (inode_blocks + inode_bitmap_blocks);
    int data_bitmap_blocks = (remaining_blocks) / (8 * 4096);
    if (remaining_blocks % (8 * 4096) != 0) {
        data_bitmap_blocks +=  1;
    }
    int data_blocks = remaining_blocks - data_bitmap_blocks;

    char *super_block_data = (char *)malloc(4096 * sizeof(char));
    super_block *super_block_ptr = (super_block *)super_block_data;

    super_block_ptr->magic_number = MAGIC;
    super_block_ptr->blocks = inode_and_data_blocks;

    super_block_ptr->inode_blocks = inode_blocks;
    super_block_ptr->inodes = num_inodes;
    super_block_ptr->inode_bitmap_block_idx = 1;
    super_block_ptr->inode_block_idx = 1 + inode_bitmap_blocks + data_bitmap_blocks;

    super_block_ptr->data_block_bitmap_idx = 1 + inode_bitmap_blocks;
    super_block_ptr->data_block_idx = 1 + inode_bitmap_blocks + data_bitmap_blocks + inode_blocks;
    super_block_ptr->data_blocks = data_blocks;

    // write super block
    write_block(diskptr, 0, super_block_data);

    return 0;
}

int mount(disk *diskptr) {
    
    super_block super_block_ptr;
    char *super_block_data = (char *)malloc(4096 * sizeof(char));
    read_block(diskptr, 0, super_block_data);
    memcpy((void *)&super_block_ptr, (void *)super_block_data, sizeof(super_block));
    free(super_block_data);
    
    if (super_block_ptr.magic_number == MAGIC) {
        printf("sfs is mounted\n");
        mounted_disk_ptr = diskptr;
        return 0;
    } else {
        return -1;
    }
}

int create_file() {
    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char *super_block_data = (char *)malloc(4096 * sizeof(char));
    read_block(diskptr, 0, super_block_data);
    super_block *super_block_ptr = (super_block *)super_block_data;

    
    
    return 0;
}

int remove_file(int inumber) {
    return 0;
}

int stat(int inumber) {
    return 0;
}

int read_i(int inumber, char *data, int length, int offset) {
    return 0;
}

int write_i(int inumber, char *data, int length, int offset) {
    return 0;
}


int read_file(char *filepath, char *data, int length, int offset) {
    return 0;
}
int write_file(char *filepath, char *data, int length, int offset) {
    return 0;
}
int create_dir(char *dirpath) {
    return 0;
}
int remove_dir(char *dirpath) {
    return 0;
}