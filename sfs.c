/*
Advances in Operating System Design
Assignment 2

Jagadeesh Killi (16CS30013)
Pruthvi Sampath Chabathula (16CS30028)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"
#include "util.h"

static disk* mounted_disk_ptr = NULL;
static super_block super_block_struct;
static char block_data[4096]; /*used to read block data*/

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


int get_set_first_available_inode_index() {
    
    disk* diskptr = mounted_disk_ptr;

    int inode_bitmap_block_idx = (int)super_block_struct.inode_bitmap_block_idx;
    int num_inodes = (int)super_block_struct.inodes;
   
    int free_inode_index = -1;
    int k;

    /* check for first available inode in all the inode bitmap blocks */
    int i; //to iterate the inode bitmap blocks
    int block_size_in_bits = 8 * 4096;

    for (i = 0; i < num_inodes / block_size_in_bits; i++) {
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);
        k = getFirstAvailableBit((int *)block_data, block_size_in_bits);
        if (k >= 0) {
            free_inode_index = (block_size_in_bits * i) + k;
            break;
        }
    }

    /* check in last partially utilized bitmap block if there is one*/
    if (free_inode_index < 0 && (num_inodes % block_size_in_bits) != 0) {
        read_block(diskptr, inode_bitmap_block_idx + i, block_data);
        int remaing_size_in_bits =  num_inodes % block_size_in_bits;
        k = getFirstAvailableBit((int *)block_data, remaing_size_in_bits);
        if (k >= 0) {
            free_inode_index = (block_size_in_bits * i) + k;
        }
    }

    /*if free data block is found*/
    if (free_inode_index >= 0) {
        setBit((int *)block_data, k);
        write_block(diskptr, inode_bitmap_block_idx + i, block_data);
    } else {
        printf("no inodes are left to assign\n");
    }

    return free_inode_index;
}


int get_set_first_available_data_block_index() {

    disk* diskptr = mounted_disk_ptr;

    int data_block_bitmap_idx = (int)super_block_struct.data_block_bitmap_idx;
    int num_data_blocks = (int)super_block_struct.data_blocks;

    int free_data_block_index = -1;
    int k;

    int i;
    int block_size_in_bits = 8 * 4096;

    for(i = 0; i < num_data_blocks / block_size_in_bits; i++) {
        read_block(diskptr, data_block_bitmap_idx + i, block_data);
        k = getFirstAvailableBit((int *)block_data, block_size_in_bits);
        if (k >= 0) {
            free_data_block_index = (block_size_in_bits * i) + k;
            break;
        }
    }

    if (free_data_block_index < 0 && (num_data_blocks % block_size_in_bits) != 0) {
        read_block(diskptr, data_block_bitmap_idx + i, block_data);
        int remaing_size_in_bits = num_data_blocks % block_size_in_bits;
        k = getFirstAvailableBit((int *)block_data, remaing_size_in_bits);
        if (k >= 0) {
            free_data_block_index = (block_size_in_bits * i) + k;
        }
    }

    /*if free data block is found*/
    if (free_data_block_index >= 0) {
        setBit((int *)block_data, k);
        write_block(diskptr, data_block_bitmap_idx + i, block_data);
    } else {
        printf("disk is full, no data blocks left\n");
    }

    return free_data_block_index;
}

void read_inode_struct(int inumber, inode* inode_struct_ptr) {

    disk* diskptr = mounted_disk_ptr;
    /*read inode from disk and copy it at inode_struct_ptr*/
    int inode_block_idx = (int)super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_idx, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)inode_struct_ptr, (void *)inode_ptr, sizeof(inode));
    return;
}

void write_inode_struct(int inumber, inode* inode_struct_ptr) {

    disk* diskptr = mounted_disk_ptr;
    /*write inode to disk*/
    int inode_block_idx = (int)super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_idx, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)inode_ptr, (void *)inode_struct_ptr, sizeof(inode));
    write_block(diskptr, inode_block_idx, block_data);
}


int create_file() {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    
    /*get free inode*/
    int free_inode_index = get_set_first_available_inode_index();

    if (free_inode_index < 0) { /*return -1 if no inodes are left to assign */
        return -1;
    }
    
    /*read the block which has free inode data and initialize it */
    int free_inode_block_idx = (int)super_block_struct.inode_block_idx + (free_inode_index / 128);
    int free_inode_block_offset = (free_inode_index % 128) * 32;
    read_block(diskptr, free_inode_block_idx, block_data);


    inode* free_inode_ptr = (inode *)(block_data + free_inode_block_offset);
    free_inode_ptr->valid = 1;
    free_inode_ptr->size = 0;
    for (int i = 0; i < 5; i++) {
        free_inode_ptr->direct[i] = 0;
    }
    free_inode_ptr->indirect = 0;

    /*write the initialized inode back to disk*/
    write_block(diskptr, free_inode_block_idx, block_data);

    return free_inode_index;
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
        printf("Inode number %d is out of range\n", inumber);
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /* read inode data and store it in super_block_struct
     * set inode->valid to zero and write the block back to disk */
    int inode_block_idx = (int)super_block_struct.inode_block_idx + (inumber / 128);
    int inode_block_offset = (inumber % 128) * 32;
    read_block(diskptr, inode_block_idx, block_data);
    inode* inode_ptr = (inode *)(block_data + inode_block_offset);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));
    inode_ptr->valid = 0;
    write_block(diskptr, inode_block_idx, block_data);

    /* update inode bitmap */
    int inode_bitmap_block_index = (int)super_block_struct.inode_bitmap_block_idx + (inumber / (8 * 4096));
    int inode_bitmap_block_offset =  inumber % ( 8 * 4096);
    read_block(diskptr, inode_bitmap_block_index, block_data);
    clearBit((int *)block_data, inode_bitmap_block_offset);
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
        read_block(diskptr, (int)inode_struct.indirect, block_data);
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

    int block_size_in_bits = 4096 * 8;
    int num_data_bitmap_blocks = (int)super_block_struct.data_blocks / block_size_in_bits;
    num_data_bitmap_blocks += (super_block_struct.data_blocks % block_size_in_bits != 0) ? 1 : 0;
    
    /*clear the data bitmap blocks, block wise */
    int k = 0;
    for (int j = 0; j < num_data_bitmap_blocks; j++) {
        int bitmap_block_index = (int)super_block_struct.data_block_bitmap_idx + j;
        read_block(diskptr, bitmap_block_index, block_data);
        while(k < num_blocks_used && block_idx_arr[k] < (block_size_in_bits * (j+1))) {
            clearBit((int *)block_data, block_idx_arr[k] % block_size_in_bits);
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

    // printf("function stat..\n");
    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));

    /*check if inumber is a valid inode or not*/
    if (inumber >= (int)super_block_struct.inodes) {
        printf("Inode number %d is out of range\n", inumber);
        free(block_data);
        return -1;
    }

    inode inode_struct;

    /*read inode and copy it to inode_struct*/
    int inode_block_idx = (int)super_block_struct.inode_block_idx + (inumber / 128);
    read_block(diskptr, inode_block_idx, block_data);
    inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));

    /*check if inode is valid or not*/
    if ((int)inode_struct.valid == 0) {
        printf("Inode with number %d is not a vaild inode\n", inumber);
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

    /*read data block indices into an array*/
    int num_data_blocks_used = (int)inode_struct.size / 4096;
    num_data_blocks_used += ((int)inode_struct.size % 4096 != 0) ? 1 : 0;

    int *blocks_idx_arr = (int *)malloc(num_data_blocks_used * sizeof(int));
    if (num_data_blocks_used > 5) {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, 5 * sizeof(int));
        read_block(diskptr, (int)super_block_struct.data_block_idx + (int)inode_struct.indirect, block_data);
        memcpy((void *)(blocks_idx_arr + 5), (void *)block_data, (num_data_blocks_used - 5) * sizeof(int));

    } else {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, num_data_blocks_used * sizeof(int));
    }

    printf("\n==== inode %d statistics ====\n", inumber);
    printf("logical size : %d\n", (int)inode_struct.size);
    printf("# direct pointers   : %d\n", direct_pointers);
    printf("# indirect pointers : %d\n", indirect_pointers);
    if(indirect_pointers > 0) {
        printf("# indirect pointer block : %d\n", (int)inode_struct.indirect);
    }
    printf("# data blocks : ");
    for (int i = 0; i < num_data_blocks_used; i++) {
        printf("%d, ", blocks_idx_arr[i]);
    }
    printf("\n");
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

    /*read inode and copy it to inode_struct*/
    inode inode_struct;
    read_inode_struct(inumber, &inode_struct);

    
    // int inode_block_idx = (int)super_block_struct.inode_block_idx + (inumber / 128);
    // read_block(diskptr, inode_block_idx, block_data);
    // inode* inode_ptr = (inode *)(block_data + (inumber % 128) * 32);
    // memcpy((void *)&inode_struct, (void *)inode_ptr, sizeof(inode));

    /*check if offset is valid*/
    if (offset >=  inode_struct.size) {
        free(block_data);
        return -1;
    }

    if (length <= 0) {
        free(block_data);
        return 0;
    }

    if (offset + length > (int)inode_struct.size) {
        length = (int)inode_struct.size - offset;
    }

    /*read bytes in range [offset, offset + length) */
    length = (length < inode_struct.size - offset) ? length : (inode_struct.size - offset);

    /*read data block indices into an array*/
    int num_data_blocks_used = (int)inode_struct.size / 4096;
    num_data_blocks_used += ((int)inode_struct.size % 4096 != 0) ? 1 : 0;

    int *blocks_idx_arr = (int *)malloc(num_data_blocks_used * sizeof(int));
    if (num_data_blocks_used > 5) {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, 5 * sizeof(int));
        read_block(diskptr, (int)super_block_struct.data_block_idx + (int)inode_struct.indirect, block_data);
        memcpy((void *)(blocks_idx_arr + 5), (void *)block_data, (num_data_blocks_used - 5) * sizeof(int));

    } else {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, num_data_blocks_used * sizeof(int));
    }

    int block_begin = offset / 4096;
    int block_end = (offset + length - 1) / 4096;
    int num_read_blocks = block_end - block_begin + 1;
    char* blocks_storage = (char *)malloc(num_read_blocks * 4096 * sizeof(char));
    int i, j;
    for (i = block_begin, j = 0; i <= block_end; i++, j++) {
        read_block(diskptr, (int)super_block_struct.data_block_idx + blocks_idx_arr[i], blocks_storage + 4096*j);
    }

    int byte_offset = offset % 4096;
    memcpy(data, blocks_storage + byte_offset, length);

    free(blocks_storage);
    free(blocks_idx_arr);
    return length;
}


int write_i(int inumber, char *data, int length, int offset) {

    if (mounted_disk_ptr == NULL) {
        printf("sfs is not mounted..\n");
        return -1;
    }

    disk* diskptr = mounted_disk_ptr;
    char* block_data = (char *)malloc(4096 * sizeof(char));

    /*check if inumber is valid or not*/
    if (inumber >= (int)super_block_struct.inodes) {
        free(block_data);
        return -1;
    }

    /*get inode struct*/
    inode inode_struct;
    read_inode_struct(inumber, &inode_struct);


    /*offset can be at max inode size, usually appending*/
    if(offset < 0 && offset > (int)inode_struct.size) {
        free(block_data);
        return -1;
    }

    /*compute the extra blocks needed for the write operation*/
    int data_blocks_used = (int)inode_struct.size / 4096;
    data_blocks_used += ((int)inode_struct.size % 4096 != 0) ? 1 : 0;
    
    int total_blocks_needed = (offset + length) / 4096;
    total_blocks_needed += ((offset + length) % 4096 != 0) ? 1 : 0;

    int extra_blocks_needed = total_blocks_needed - data_blocks_used;
    int *blocks_idx_arr = (int *)malloc(total_blocks_needed * sizeof(int));

    if (data_blocks_used > 5) {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, 5 * sizeof(int));
        read_block(diskptr, (int)super_block_struct.data_block_idx + (int)inode_struct.indirect, block_data);
        memcpy((void *)(blocks_idx_arr + 5), (void *)block_data, (data_blocks_used - 5) * sizeof(int));

    } else {
        memcpy((void *)blocks_idx_arr, (void *)inode_struct.direct, data_blocks_used * sizeof(int));
    }

    int idx_it = data_blocks_used;
    // printf("idx_it : %d\n",idx_it);

    if (total_blocks_needed > 5) {

        if (data_blocks_used > 5) { /* inode already has an indirect pointer*/
            // printf("case 1\n");
            /*get extra indirect blocks*/
            int data_block_idx = (int)super_block_struct.data_block_idx;
            int* indirect_pointers = (int *)malloc(1024 * sizeof(int));
            read_block(diskptr, data_block_idx + (int)inode_struct.indirect, (char *)indirect_pointers);
            // print_block_data((char *)indirect_pointers);
            int* idp_offset = indirect_pointers + (data_blocks_used - 5);

            for (int i = 0; i < extra_blocks_needed; i++) {
                int free_data_block_index = get_set_first_available_data_block_index();
                blocks_idx_arr[idx_it++] = free_data_block_index;
                *idp_offset = free_data_block_index;
                idp_offset++;
            }

            /*update indirect block*/
            write_block(diskptr, data_block_idx + (int)inode_struct.indirect, (char *)indirect_pointers);
            free(indirect_pointers);
    
        } else { /*inode doesn't have an indirect pointer*/
            // printf("case 2\n");
            /*get extra direct blocks first*/
            for (int i = data_blocks_used; i < 5; i++) {
                int free_data_block_index = get_set_first_available_data_block_index();
                blocks_idx_arr[idx_it++] = free_data_block_index;
                inode_struct.direct[i] = free_data_block_index;
            }
            

            /*get extra indirect blocks */
            int free_indirect_block_index = get_set_first_available_data_block_index();
            inode_struct.indirect = (uint32_t)free_indirect_block_index;

            int data_block_idx = (int)super_block_struct.data_block_idx;
            int* indirect_pointers = (int *)malloc(1024 * sizeof(int));
            read_block(diskptr, data_block_idx + (int)inode_struct.indirect, (char *)indirect_pointers);
            
            int* idp_offset = indirect_pointers;


            for (int i = 0; i < (total_blocks_needed - 5); i++) {
                int free_data_block_index = get_set_first_available_data_block_index();
                blocks_idx_arr[idx_it++] = free_data_block_index;
                *idp_offset = free_data_block_index;
                idp_offset++;
            }

            /*update indirect block*/
            // print_block_data((char *)indirect_pointers);
            write_block(diskptr, data_block_idx + (int)inode_struct.indirect, (char *)indirect_pointers);
            free(indirect_pointers);
        }

    } else { /*write can be fulfilled by direct pointers*/
        // printf("case 3\n");
        for (int i = data_blocks_used; i < total_blocks_needed; i++) {
            int free_data_block_index = get_set_first_available_data_block_index();
            blocks_idx_arr[idx_it++] = free_data_block_index;
            inode_struct.direct[i] = free_data_block_index;
        }
    }

    // printf("blocks idx arr : ");
    // for ( int i=0; i < total_blocks_needed; i++) {
    //     printf("%d, ", blocks_idx_arr[i]);
    // }
    // printf("\n");

    int block_begin = offset / 4096;
    int block_end = (offset + length - 1) / 4096;
    
    int data_block_idx = (int)super_block_struct.data_block_idx;
    int num_write_blocks = block_end - block_begin + 1;
    char* blocks_storage = (char *)malloc(num_write_blocks * 4096 * sizeof(char));
    int i, j;
    for (i = block_begin, j = 0; i <= block_end; i++, j++) {
        read_block(diskptr, data_block_idx + blocks_idx_arr[i], blocks_storage + 4096*j);
    }

    int byte_offset = offset % 4096;
    memcpy(blocks_storage + byte_offset, data, length);

    for (i = block_begin, j = 0; i <= block_end; i++, j++) {
        write_block(diskptr, data_block_idx + blocks_idx_arr[i], blocks_storage + 4096*j);
    }

    /*update inode size*/
    inode_struct.size = (uint32_t)(offset + length);
    write_inode_struct(inumber, &inode_struct);

    free(blocks_storage);
    free(blocks_idx_arr);

    return length;
}


// int read_file(char *filepath, char *data, int length, int offset) {
//     return 0;
// }
// int write_file(char *filepath, char *data, int length, int offset) {
//     return 0;
// }
// int create_dir(char *dirpath) {
//     return 0;
// }
// int remove_dir(char *dirpath) {
//     return 0;
// }


int find_file(char *filename,int dirnode)// finds the given file/directory in directory given by dirnode
{
    if(strlen(filename) >=48) 
    {
        printf("File not found\n");
        return -1;
    }

    char* block_data = (char *)malloc(4096 * sizeof(char)); // stores the block containing inode
    inode cur_inode;
    int cur_inode_block = super_block_struct.inode_block_idx + (dirnode)/128; 
    int ret = read_block(mounted_disk_ptr,cur_inode_block,block_data); 

    int offset = (dirnode)%128;
    memcpy(&cur_inode,block_data+ offset*sizeof(inode),sizeof(inode)); //load inode

    int fsize = cur_inode.size;

    char* dirlist = (char*)malloc(fsize); 
    ret = read_i(dirnode,dirlist,fsize,0); // load directory file

    char* searchfile = (char*)malloc(48);
    memset(searchfile,'\0',48); //memory to store an entry of directory list

    int i = 0;
    for(i=0;i<fsize/64;i++)
    {
        memcpy(searchfile,dirlist+(i*64)+8,48); // load an entry from dirlist
        if(strcmp(searchfile,filename) == 0) // search for file
        {
            int inum;
            memcpy(&inum,(void *)dirlist+(i*64)+60,4);
            free(block_data);
            free(dirlist);
            free(searchfile);

            return inum; //return i node number of file
        }
    }
    free(block_data);
    free(dirlist);
    free(searchfile);

    return -1;
}

int read_file(char *filepath, char *data, int length, int offset) { // A function to read a filepath(absolute address) file 


    int len = strlen(filepath);
    int inum;
    char *fname = (char*)malloc(len+1); //temp variable to store file address
    memset(fname,'\0',len+1);
    memcpy(fname,filepath,len);

    char* token = strtok(fname, "/"); 
    
    int dirnode = 0;
    while (token != NULL) {  // iteratively go down the directory structure
        // printf("%s\n", token); 

        inum = find_file(token,dirnode);  // inum contains the i node number of required file
        
        if(inum < 0)
        {
            printf("File Not Found\n");
            return -1;
        }
        dirnode = inum;
        token = strtok(NULL, "/"); 
    } 

    int ret = read_i(inum,data,length,offset); 
    free(fname);
  
    return ret;
}

int write_file(char *filepath, char *data, int length, int offset) {

    int inum;     // inum contains the i node number of required file
    int len = strlen(filepath);
    char *fname = (char*)malloc(len+1); // local copy of filepath
    memset(fname,'\0',len+1);
    memcpy(fname,filepath,len);

    char* token = strtok(fname, "/"); 
    
    int dirnode = 0;
    while (token != NULL) { // iteratively go down the directory structure
        // printf("%s\n", token); 

        inum = find_file(token,dirnode);
        
        if(inum < 0)
        {
            printf("File Not Found\n");
            return -1;
        }
        dirnode = inum;
        token = strtok(NULL, "/"); 
    } 

    int ret = write_i(inum,data,length,offset); // write data to file whose inode is inum
    return ret;
}

typedef struct find_file2_out{

    int previnode;
    int nextbyte;
    int nwinode;

}find_file2_out;


find_file2_out find_file2(char *filename,int dirnode) // finds the given file/directory in directory given by dirnode
{
    find_file2_out out; // output structure that returns essential info
    out.nwinode = -1;
    out.previnode = dirnode;

    if(strlen(filename) >=48) // if file name is >= 48 bytes, file not found
    {
        printf("File not found\n");
        return out;
    }

    char* block_data = (char *)malloc(4096 * sizeof(char)); // temp memory to store block info
    inode cur_inode; // temp memory to store inode info of current block (dirnode)
    int cur_inode_block = super_block_struct.inode_block_idx + (dirnode)/128;
    int ret = read_block(mounted_disk_ptr,cur_inode_block,block_data);

    int offset = (dirnode)%128;
    memcpy(&cur_inode,block_data+ offset*sizeof(inode),sizeof(inode)); //copy the inode in block into inode structure

    int fsize = cur_inode.size; //size of inode

    char* dirlist = (char*)malloc(fsize); // temp structure storing dir file data of dirnode
    ret = read_i(dirnode,dirlist,fsize,0);

    char* searchfile = (char*)malloc(48); // an element of dirlist
    memset(searchfile,'\0',48);

    out.nextbyte = fsize; //first free byte number in the file

    int i = 0;
    for(i=0;i<fsize/64;i++) //extracts each child's entry in dir list and compares to see if filename is present or not 
    {
        memcpy(searchfile,dirlist+(i*64)+8,48);
        if(strcmp(searchfile,filename) == 0)
        {
            int inum;
            memcpy(&inum,(void *)dirlist+(i*64)+60,4);

            out.nwinode = inum;
        }
    }
    free(block_data);
    free(dirlist);
    free(searchfile);

    return out; // returns essential info about the search file
}

int create_dir(char *dirpath) { // creates a dirctory whose address is given by dirpath, 0 on success -1 on failure

    find_file2_out inum; //structure containing essential information returned from find_file2 
    int len = strlen(dirpath);
    char *fname = (char*)malloc(len+1);
    char *tempname = (char*)malloc(len+1);
    memset(fname,'\0',len+1);
    memset(tempname,'\0',len+1);
    memcpy(fname,dirpath,len); //temporarily storing dirpath in fname variable

    char* token = strtok(fname, "/"); 

    int n = 0;
    while (token != NULL) {  // tokenizing code for getting number of directories in the path
        n++;
        // printf("%s\n", token); 
        token = strtok(NULL, "/"); 

    }

    memcpy(fname,dirpath,len); 
    token = strtok(fname, "/");

    int dirnode = 0;             // initialize dirnode to zero which represents root inode
    int i = 0;
    while (token != NULL) { // tokenizing code 2 to go through the given path iteratively
        i++;
        // printf("%s\n", token); 

        inum = find_file2(token,dirnode); // dirnode represents current directory(initially 0, means root) 
                                         // Given filename (token) is checked in current directory

        if(inum.nwinode < 0 && i<n) // i < n => dir in intermediate path, inum.nwnode < 0 given file/dir is not found in current dir
        {
            printf("File Not Found\n");
            free(fname);
            free(tempname);
            return -1;
        }
        
        if(inum.nwinode >= 0 && i==n) // i ==  n => the file/dir we are supposed to create
        {
            printf("There is already a file with same name \n");
            free(fname);
            free(tempname);
            return -1;
        }

        dirnode = inum.nwinode;
        memcpy(tempname,token,strlen(token));
        token = strtok(NULL, "/"); 
    }

    if(strlen(tempname) >=48 )
    {
        printf("File name is too long(>47) : NOT SUPPORTED\n");
        return -1;
    }
    

    int inode_created = create_file();
    char* data = (char*)calloc(64,1);
    int temp = 1; // valid bit
    memcpy(data,&temp,4);



    temp = 0; // creation a directory
    memcpy(data+4,&temp,4);

    memcpy(data+8,tempname,48); // copying filename
    
    temp = 48;
    memcpy(data+56,&temp,4); //  filename size

    temp = inode_created;
    memcpy(data+60,&temp,4); // inode number
    // printf("node%s\n",data+8);
    int ret = write_i(inum.previnode,data,64,inum.nextbyte);

    free(fname);
    free(tempname);
    free(data);

    return 0;
}

typedef struct find_file3_out{

    int previnode;
    int nextbyte;
    int nwinode;
    int replacebyte;

}find_file3_out;

find_file3_out find_file3(char *filename,int dirnode)
{
    find_file3_out out;
    out.nwinode = -1;
    out.previnode = dirnode;

    if(strlen(filename) >=48) 
    {
        printf("File not found\n");
        return out;
    }

    char* block_data = (char *)malloc(4096 * sizeof(char));
    inode cur_inode;
    int cur_inode_block = super_block_struct.inode_block_idx + (dirnode)/128;
    int ret = read_block(mounted_disk_ptr,cur_inode_block,block_data);

    int offset = (dirnode)%128;
    memcpy(&cur_inode,block_data+ offset*sizeof(inode),sizeof(inode));

    int fsize = cur_inode.size;

    char* dirlist = (char*)malloc(fsize);
    ret = read_i(dirnode,dirlist,fsize,0);

    char* searchfile = (char*)malloc(48);
    memset(searchfile,'\0',48);

    out.nextbyte = fsize;

    int i = 0;
    for(i=0;i<fsize/64;i++)
    {
        memcpy(searchfile,dirlist+(i*64)+8,48);
        if(strcmp(searchfile,filename) == 0)
        {
            int inum;
            out.replacebyte = i*64;
            memcpy(&inum,(void *)dirlist+(i*64)+60,4);

            out.nwinode = inum;
        }
    }
    free(block_data);
    free(dirlist);
    free(searchfile);

    return out;
}

int remove_dir(char *dirpath) { //removes directory given by dirpath

    find_file3_out inum; //output structure of fine_file3 fn containing essential information
    int len = strlen(dirpath);
    char *fname = (char*)malloc(len+1); //maintains a copy of dirpath
    memset(fname,'\0',len+1);
    memcpy(fname,dirpath,len);

    // char *fname = (char*)malloc(len+1);
    // memset(fname,'\0',len+1);
    // memcpy(fname,dirpath,len);

    char* token = strtok(fname, "/");

    int dirnode = 0; // represents the root directory
    while (token != NULL) { // iteratively pass through directory dtructure


        inum = find_file3(token,dirnode);
        
        if(inum.nwinode < 0)
        {
            
            printf("File Not Found\n");
            free(fname);
            return -1;
        }

        dirnode = inum.nwinode;
        // strcpy(tempname,token,strlen(token));
        token = strtok(NULL, "/"); 
    }
    // printf("%d\n",inum.previnode);
    int ret = remove_file(inum.nwinode); // remove file at the address
    char* data = (char*)calloc(inum.nextbyte,1); //for resetting dir entry in parent dir

    // write_i(inum.previnode,data,0,inum.replacebyte);
    read_i(inum.previnode,data,inum.nextbyte,0);
    memcpy(data+inum.replacebyte,data+inum.nextbyte-64,64);
    // printf("yo %s\n",data+8);
    write_i(inum.previnode,data,inum.nextbyte-64,0); //replace the empty spot with last entry

    // memset(data,'\0',64);
    // write_i(inum.previnode,data,64,inum.nextbyte-64);

    free(data);
    free(fname);
    return 0;
}