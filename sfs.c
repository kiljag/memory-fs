#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"
#include "util.h"

static disk* mounted_disk_ptr = NULL;
static super_block super_block_struct;


int format(disk *diskptr) {

    uint32_t total_blocks = diskptr->blocks;
    uint32_t inode_and_data_blocks =  total_blocks - 1;

    uint32_t inode_blocks = (int)(0.1 * inode_and_data_blocks);
    inode_blocks = (inode_blocks > 0) ? inode_blocks : 1; 

    uint32_t num_inodes = inode_blocks * 128;
    uint32_t inode_bitmap_blocks = (num_inodes) / (8 * 4096);
    if (num_inodes % (8 * 4096) != 0) {
        inode_bitmap_blocks += 1;
    }

    uint32_t remaining_blocks = inode_and_data_blocks - (inode_blocks + inode_bitmap_blocks);
    uint32_t data_bitmap_blocks = (remaining_blocks) / (8 * 4096);
    if (remaining_blocks % (8 * 4096) != 0) {
        data_bitmap_blocks +=  1;
    }
    
    uint32_t data_blocks = remaining_blocks - data_bitmap_blocks;

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

    // write initialized super block back to disk
    write_block(diskptr, 0, super_block_data);

    return 0;
}


int mount(disk *diskptr) {
    
    /*read super block and store it in static super_block_struct*/
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
    
    int inode_bitmap_block_idx = (int)super_block_struct.inode_bitmap_block_idx;
    int num_inodes = (int)super_block_struct.inodes;
   
    int free_inode_index = -1;
    int k;

    /* check for first available inode in all the inode bitmap blocks */
    int i; //to iterate the inode bitmap blocks
    for (i = 0; i < num_inodes / (8 * 4096); i++) {
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);
        k = getFirstAvailableBit((int *)block_data, 8 * 4096);
        if (k >= 0) {
            free_inode_index = (8 * 4096 * i) + k;
            break;
        }
    }

    /* check in last partially utilized bitmap block if there is any*/
    if (free_inode_index < 0 && num_inodes % (8 * 4096) != 0) {
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);
        int remaing_size =  num_inodes % (8 * 4096);
        k = getFirstAvailableBit((int *)block_data, remaing_size);
        if (k >= 0) {
            free_inode_index = (8 * 4096 * i) + k;
        }
    }
    
    /*return -1 if no inodes are left to assign */
    if (free_inode_index < 0) {
        printf("free inodes are unavaible\n");
        free(block_data);
        return -1;
    }
    
    /*set inode bitmap at the assigned inode position and write back to disk */
    setBit((int *)block_data, k);
    write_block(diskptr, inode_bitmap_block_idx + i, block_data);

    /*read the block which has free inode data and initialize it */
    int free_inode_block = free_inode_index / 128;
    int free_inode_offset = (free_inode_index % 128) * 32;
    read_block(diskptr, free_inode_block, block_data);
    // print_block_data(block_data);
    inode* free_inode_ptr = (inode*)(block_data + free_inode_offset);
    free_inode_ptr->valid = 1;
    free_inode_ptr->size = 0;
    for (int i = 0; i < 5; i++) {
        free_inode_ptr->direct[i] = 0;
    }
    free_inode_ptr->indirect = 0;

    /*write the initialized inode back to disk*/
    write_block(diskptr, free_inode_block, block_data);

    free(block_data);
    // print_block_data(block_data);
    return free_inode_index;
}

void intialize_inode(int inumber) {
    return;
}

/* utility functions */
void read_inode_struct(int inumber, inode* inode_ptr) {
    return;
}

int remove_file(int inumber) {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));
    
    /*check inumber a valid inode or not*/
    if (inumber >= (int)super_block_struct.inodes) {
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /* read inode data and store it in super_bloc_struct
     * set inode->valid to zero and write the block back to disk */
    int inode_block_index = (int)super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_index, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));
    inode_ptr->valid = 0;
    write_block(diskptr, inode_block_index, block_data);

    /* update inode bitmap */
    int inode_bitmap_block_index = (int)super_block_struct.inode_bitmap_block_idx;
    inode_bitmap_block_index += (inumber / (8 * 4096));
    int inode_bitmap_bit_pos =  inumber % ( 8 * 4096);
    read_block(diskptr, inode_bitmap_block_index, block_data);
    clearBit((int *)block_data, inode_bitmap_bit_pos);
    write_block(diskptr, inode_bitmap_block_index, block_data);
    

    /*collect all the data blocks used by inode  */
    int num_data_blocks_used = (int)inode_struct.size / 4096;
    num_data_blocks_used += (inode_struct.size % 4096 != 0) ? 1 : 0;
    int num_blocks_used;

    int *block_idx_arr = NULL;
    
    if (num_data_blocks_used > 5) { /*in case an indirect pointer is used*/
        num_blocks_used = num_data_blocks_used + 1;
        block_idx_arr = (int *)malloc(num_blocks_used * sizeof(int));

        /*copy direct pointers*/
        memcpy((void *)block_idx_arr, (void *)inode_struct.direct, 5 * sizeof(int));

        /*read indirect block and copy indirect pointers*/
        read_block(diskptr, inode_struct.indirect, block_data);
        memcpy((void *)(block_idx_arr + 5), (void *)block_data, (num_data_blocks_used - 5) * sizeof(int));

        /*copy the block index used to store indirect pointers*/
        block_idx_arr[num_blocks_used - 1] = (int)(inode_struct.indirect);
 
    } else { /* only direct pointers are used */
        num_blocks_used = num_data_blocks_used;
        block_idx_arr = (int *)malloc(num_blocks_used * sizeof(int));

        /*copy direct pointers*/
        memcpy((void *)block_idx_arr, (void *)inode_struct.direct, num_blocks_used * sizeof(int));
    }

    /*sort the block used to clear the data bitmap blocks block wise*/
    quick_sort((int *)block_idx_arr, 0, (int)(num_blocks_used - 1));

    int num_bitmap_blocks = (int)super_block_struct.data_blocks / (4096 * 8);
    num_bitmap_blocks += (super_block_struct.data_blocks % (4096 * 8) != 0) ? 1 : 0;
    
    
    /*clear the data bitmap blocks, block wise */
    int k = 0;
    for (int j = 0; j < num_bitmap_blocks; j++) {
        int bitmap_block_index = super_block_struct.data_block_bitmap_idx + j;
        read_block(diskptr, bitmap_block_index, block_data);
        while(k < num_blocks_used && block_idx_arr[k] < (4096*8*(j+1))) {
            clearBit((int *)block_data, block_idx_arr[k]);
            k++;
        }
        write_block(diskptr, bitmap_block_index, block_data);
        if (k >= num_blocks_used) {
            break;
        }
    }

    free(block_idx_arr);
    free(block_data);
    return 0;
}

int stat(int inumber) {
    printf("function stat..\n");
    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));

    /*check if inumber is a valid inode or not*/
    if (inumber >= (int)super_block_struct.inodes) {
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /*read inode and copy it to inode_struct*/
    uint32_t inode_block_index = super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_index, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));
    print_block_data(block_data);

    /*check if inode is valid or not*/
    if ((int)inode_struct.valid == 0) {
        printf("Invalid inode index %d\n", inumber);
        free(block_data);
        return -1;
    }

    int num_blocks_used = (int)inode_struct.size / (4096);
    num_blocks_used += (inode_struct.size % 4096 != 0) ? 1 : 0;

    int direct_pointers, indirect_pointers;
    if (num_blocks_used > 5) {
        direct_pointers = 5;
        indirect_pointers = num_blocks_used - 5;
    } else {
        direct_pointers = num_blocks_used;
        indirect_pointers = 0;
    }

    printf("\n==== inode %d statistics ====\n", inumber);
    printf("logical size : %d\n", (int)inode_struct.size);
    printf("# direct pointers   : %d\n", direct_pointers);
    printf("# indirect pointers : %d\n", indirect_pointers);
    printf("==== ******************** ====\n");

    free(block_data);
    return 0;
}

int read_i(int inumber, char *data, int length, int offset) {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));

    /*check if inumber is valid or not*/
    if (inumber >= super_block_struct.inodes) {
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /*read inode and copy it to inode_strruct*/
    uint32_t inode_relative_pos = inumber % 128;
    uint32_t inode_block_index = super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_index, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));

    /*check and offset are valid*/
    if (offset >=  inode_struct.size) {
        free(block_data);
        return -1;
    }

    if (offset + length > inode_struct.size) {
        length = inode_struct.size - offset;
    }

    /*read bytes in range [offset, offset + length) */
    length = (length < inode_struct.size - offset) ? length : (inode_struct.size - offset);

    /*read data block indices into an array*/
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