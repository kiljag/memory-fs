#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"

static disk* mounted_disk_ptr = NULL;

/*utility functions*/ 

/*set bit at kth position in int array A*/
void setBit(uint32_t *A, uint32_t k) {
    uint32_t i = k / 32;
    uint32_t pos = k % 32;
    A[i] = A[i] | (1 << pos);
}

/*clear bit at kth position in int array A*/
void clearBit(uint32_t *A, uint32_t k) {
    uint32_t i = k / 32;
    uint32_t pos = k % 32;
    A[i] = A[i] & ~(1 << pos);
}

/*test if bit at kth position is set or not*/
uint32_t testBit(uint32_t *A, uint32_t k) {
    uint32_t i = k / 32;
    uint32_t pos = k % 32;

    if (A[i] & (1 << pos)) {
        return 1;
    } else {
        return 0;
    }
}

/* get the first available unset bit position
 * size_in_bits also denotes number of inodes */
int getFirstAvailableBit(uint32_t *A, uint32_t size_in_bits) {
    int i, pos;
    // printf("size in bits : %u\n", size_in_bits);

    //check in $(size_in_bits/32) integer blocks
    for (i = 0; i < (size_in_bits / 32); i++) {
        
        if (A[i] ^ 0xFFFFFFFF) {
            for(pos = 0; pos < 32; pos++) {
                int k = (32 * i) + pos;
                if(!testBit(A, (uint32_t)k)){
                    // printf("size_in_bits : %d i: %d, k: %d\n", size_in_bits, i, k);
                    return k;
                };
            }
        }
    }

    //check in remaining bits of last integer block
    // for (pos = 0; pos < (size_in_bits % 32); pos++) {
    //     int k = 32 * i + pos;
    //     if(!testBit(A, k)) {
    //         printf("i: %d, k: %d\n", i, k);
    //         return k;
    //     }
    // }

    return -1;
}

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int quick_sort_partition(int *arr, int low, int high) {
    int pivot = arr[high];
    int i = (low - 1);

    int j;
    for (j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i+1], &arr[high]);
    return (i+1);
}

/*sort an array of uint32_t*/
void quick_sort(uint32_t *arr, int low, int high) {
    if (low < high) {
        int pi = quick_sort_partition(arr, low, high);
        
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

/*an utility function to print block data in hex format*/
void print_block_data(char* block_data) {
    for (int i=0; i<1024; i++) {
        printf("%x:", ((uint32_t *)block_data)[i]);
    }
    printf("\n");
}

int format(disk *diskptr) {

    int total_blocks = diskptr->blocks;
    int inode_and_data_blocks =  total_blocks - 1;

    int inode_blocks = (int)(0.1 * inode_and_data_blocks);
    inode_blocks = (inode_blocks > 0) ? inode_blocks : 1; 

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
    super_block *super_block_struct = (super_block *)super_block_data;

    super_block_struct->magic_number = MAGIC;
    super_block_struct->blocks = inode_and_data_blocks;

    super_block_struct->inode_blocks = inode_blocks;
    super_block_struct->inodes = num_inodes;
    super_block_struct->inode_bitmap_block_idx = 1;
    super_block_struct->inode_block_idx = 1 + inode_bitmap_blocks + data_bitmap_blocks;

    super_block_struct->data_block_bitmap_idx = 1 + inode_bitmap_blocks;
    super_block_struct->data_block_idx = 1 + inode_bitmap_blocks + data_bitmap_blocks + inode_blocks;
    super_block_struct->data_blocks = data_blocks;

    // write super block
    write_block(diskptr, 0, super_block_data);

    return 0;
}

int mount(disk *diskptr) {
    
    /*read super block and store it in super_block_struct*/
    super_block super_block_struct;
    char *super_block_data = (char *)malloc(4096 * sizeof(char));
    read_block(diskptr, 0, super_block_data);
    memcpy((void *)&super_block_struct, (void *)super_block_data, sizeof(super_block));
    free(super_block_data);

    /*print super block info*/
    printf("==== SUPER BLOCK ====\n");
    printf("magic number : %u\n", super_block_struct.magic_number);
    printf("blocks : %u\n", super_block_struct.blocks);

    printf("inode_blocks : %u\n", super_block_struct.inode_blocks);
    printf("inodes : %u\n", super_block_struct.inodes);
    printf("inode bitmap block idx : %u\n", super_block_struct.inode_bitmap_block_idx);
    printf("inode block idx: %u\n", super_block_struct.inode_block_idx);

    printf("data block bitmap idx : %u\n", super_block_struct.data_block_bitmap_idx);
    printf("data block idx : %u\n", super_block_struct.data_block_idx);
    printf("data blocks : %u\n", super_block_struct.data_blocks);
    printf("==== *********** =====\n\n");
    
    /*check the magic number*/
    if (super_block_struct.magic_number == MAGIC) {
        printf("sfs is mounted\n\n");
        mounted_disk_ptr = diskptr;
        return 0;
    } else {
        printf("invalid magic number. aborting..\n\n");
        return -1;
    }
}

int create_file() {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));
    
    /*read super block and store it in super_block_struct*/
    super_block super_block_struct;
    read_block(diskptr, 0, block_data);
    memcpy((void *)&super_block_struct, (void *)block_data, sizeof(super_block));
    

    uint32_t inode_bitmap_block_idx = super_block_struct.inode_bitmap_block_idx;
    uint32_t num_inodes = super_block_struct.inodes;
   
    int free_inode_index = -1;
    int k;

    
    /* check for first available inode in all the inode bitmap blocks */
    int i;
    for (i = 0; i < num_inodes / (8 * 4096); i++) {
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);
        k = getFirstAvailableBit((uint32_t *)block_data, 8 * 4096);
        if (k >= 0) {
            free_inode_index = (8 * 4096 * i) + k;
            break;
        }
    }

    /* check in last partially utilized bitmap block */
    if (free_inode_index < 0 && num_inodes % (8 * 4096) != 0) {
        
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);


        k = getFirstAvailableBit((uint32_t *)block_data, num_inodes % (8 * 4096));
        if (k >= 0) {
            free_inode_index = (8 * 4096 * i) + k;
        }
    }
    // printf("free inode position : %d\n", free_inode_index);
    
    /*return -1 if no inodes are left to assign */
    if (free_inode_index < 0) {
        free(block_data);
        return -1;
    }
    
    /*set inode bitmap at the assigned inode position and write back to disk */
    setBit((uint32_t *)block_data, k);
    // for (int i=0; i<32*36; i++) {
    //     printf("%x:", ((uint32_t *)block_data)[i]);
    // }
    // printf("\n");

    // printf("writing to block %u\n\n", inode_bitmap_block_idx + i);
    write_block(diskptr, inode_bitmap_block_idx + i, block_data);

    /*read the block which has free inode data and initialize it */
    int free_inode_block = free_inode_index / 128;
    int free_inode_offset = (free_inode_index % 128) * sizeof(inode);
    read_block(diskptr, free_inode_block, block_data);

    inode* free_inode_ptr = (inode*)(block_data + free_inode_offset);
    free_inode_ptr->valid = 1;
    free_inode_ptr->size = 0;

    write_block(diskptr, free_inode_block, block_data);

    free(block_data);
    return free_inode_index;
}

int remove_file(int inumber) {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }
    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));

    /*read super block and store it in super_block_struct*/
    super_block super_block_struct;
    read_block(diskptr, 0, block_data);
    memcpy((void *)&super_block_struct, (void *)block_data, sizeof(super_block));
    
    /*check inumber a valid inode or not*/
    if (inumber >= super_block_struct.inodes) {
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /* read inode data and store it in super_bloc_struct
     * set inode->valid to zero and write the block back to disk */
    uint32_t inode_relative_pos = inumber % 128;
    uint32_t inode_block_index = super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_index, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));
    inode_ptr->valid = 0;
    write_block(diskptr, inode_block_index, block_data);

    /* update inode bitmap */
    uint32_t inode_bitmap_block_index = super_block_struct.inode_bitmap_block_idx;
    inode_bitmap_block_index += (inumber / (8 * 4096));
    uint32_t inode_bitmap_bit_pos =  inumber % ( 8 * 4096);
    read_block(diskptr, inode_bitmap_block_index, block_data);
    print_block_data(block_data);
    clearBit((uint32_t *)block_data, inode_bitmap_bit_pos);
    print_block_data(block_data);
    write_block(diskptr, inode_bitmap_block_index, block_data);
    read_block(diskptr, inode_bitmap_block_index, block_data);
    print_block_data(block_data);

    /*update the data block bitmap */
    uint32_t num_data_blocks_used = inode_struct.size / 4096;
    num_data_blocks_used += (inode_struct.size % 4096 != 0) ? 1 : 0;

    uint32_t num_blocks_used =  num_data_blocks_used;
    num_blocks_used += (num_data_blocks_used > 5) ? 1 : 0; //block used to store indirect pointers
    uint32_t *data_block_idx_arr = (uint32_t *)malloc(num_blocks_used * sizeof(uint32_t));
    int i;
    for (i = 0; i < 5; i++) {
        data_block_idx_arr[i] = inode_struct.direct[i];
    }
    if (num_data_blocks_used = 5) {
        /* read the indirect pointer block */
        read_block(diskptr, inode_struct.indirect, block_data);
        uint32_t *indirect_block_idx_arr = (uint32_t *)block_data;
        for (; i < (num_data_blocks_used - 5); i++) {
            data_block_idx_arr[i] = indirect_block_idx_arr[i-5];
        }

        data_block_idx_arr[i] = inode_struct.indirect;
    }

    /*sort the block used to clear the data bitmap blocks block wise*/
    quick_sort((int *)data_block_idx_arr, 0, (int)(num_blocks_used - 1));

    uint32_t num_bitmap_blocks = super_block_struct.data_blocks / (4096 * 8);
    num_bitmap_blocks += (super_block_struct.data_blocks % (4096 * 8) != 0) ? 1 : 0;

    int j, k;
    
    /*clear the data bitmap blocks, bitmap block wise */
    for (j = 0; j < num_bitmap_blocks; j++) {
        int bitmap_block_index = super_block_struct.data_block_bitmap_idx + j;
        read_block(diskptr, bitmap_block_index, block_data);
        while(k < num_blocks_used && data_block_idx_arr[k] < (4096*8*(j+1))) {
            clearBit((uint32_t *)block_data, data_block_idx_arr[k]);
        }
        write_block(diskptr, bitmap_block_index, block_data);
        if (k >= num_blocks_used) {
            break;
        }
    }

    free(block_data);
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