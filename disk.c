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

static char* block_arr_ptr_static;

disk* create_disk(int nbytes) {


    char* disk_array = (char *)malloc(nbytes*sizeof(char));
    if(disk_array == NULL) {
        return NULL;
    }

    disk *diskptr = (struct disk *)disk_array;
    block_arr_ptr_static = (disk_array + sizeof(disk));

    diskptr->size = nbytes;
    diskptr->blocks = (nbytes - 24) / BLOCKSIZE;
    diskptr->reads = 0;
    diskptr->writes = 0;
    diskptr->block_arr = &block_arr_ptr_static;

    return diskptr;
}

int read_block(disk *diskptr, int blocknr, void *block_data) {
    
    if (blocknr < 0 || blocknr >= diskptr->blocks) {
        printf("block %d is out of range\n", blocknr);
        return -1;
    }

    // printf("reading disk block : %d\n", blocknr);

    char *block_arr_ptr = *(diskptr->block_arr);
    char *block_ptr = block_arr_ptr + blocknr*BLOCKSIZE;
    // memcpy(block_data, (void *)block_ptr, BLOCKSIZE);
    int *dest = (int *)block_data;
    int *src = (int *)block_ptr;
    for (int i = 0; i < 1024; i++) {
        *dest = *src;
        dest++;
        src++;
    }

    diskptr->reads++;
    return 0;
}

int write_block(disk *diskptr, int blocknr, void *block_data) {
    
    if (blocknr < 0 || blocknr >= diskptr->blocks) {
        printf("block %d is out of range\n", blocknr);
        return -1;
    }

    // printf("writing disk block : %d\n", blocknr);

    char *block_arr_ptr = *(diskptr->block_arr);
    char *block_ptr = block_arr_ptr + blocknr*BLOCKSIZE;
    // memcpy((void *)block_ptr, block_data, BLOCKSIZE);
    int *dest = (int *)block_ptr;
    int *src = (int *)block_data;
    for (int i = 0; i < 1024; i++) {
        *dest = *src;
        dest++;
        src++;
    }
    diskptr->writes++;
    return 0;
}

int free_disk(disk *diskptr) {
    /*convert diskptr to char*/
    char *disk_array = (char *)diskptr;
    free(disk_array);
    return 0;
}

/*utility script*/
void print_block(disk *diskptr, int blocknr) {

    if (blocknr >= diskptr->blocks) {
        printf("Invalid block number");
    }

    char *block_arr_ptr = *(diskptr->block_arr);
    char *block_ptr = block_arr_ptr + blocknr*BLOCKSIZE;
    uint32_t *block_int_ptr = (uint32_t *)block_ptr;
    
    for (int i=0; i<1024; i++) {
        printf("%x:", block_int_ptr[i]);
    }
    printf("\n");
}
