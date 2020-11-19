#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"

static char* block_arr_ptr;

disk* create_disk(int nbytes) {


    char* disk_array = (char *)malloc(nbytes*sizeof(char));
    if(disk_array == NULL) {
        return NULL;
    }

    disk *diskptr = (struct disk *)disk_array;
    block_arr_ptr = (disk_array + sizeof(disk));

    diskptr->size = nbytes;
    diskptr->blocks = (nbytes - 24) / BLOCKSIZE;
    diskptr->reads = 0;
    diskptr->writes = 0;
    diskptr->block_arr = &block_arr_ptr;

    return diskptr;
}

int read_block(disk *diskptr, int blocknr, void *block_data) {

    if (blocknr >= diskptr->blocks) {
        return -1;
    }

    char *block_arr_ptr = *(diskptr->block_arr);
    char *block_ptr = block_arr_ptr + blocknr*BLOCKSIZE;
    memcpy(block_data, (void *)block_ptr, BLOCKSIZE);

    diskptr->reads++;
    return 0;
}

int write_block(disk *diskptr, int blocknr, void *block_data) {
    if (blocknr >= diskptr->blocks) {
        return -1;
    }

    char *block_arr_ptr = *(diskptr->block_arr);
    char *block_ptr = block_arr_ptr + blocknr*BLOCKSIZE;
    memcpy((void *)block_ptr, block_data, BLOCKSIZE);
    diskptr->writes++;
    return 0;
}

int free_disk(disk *diskptr) {
    /*convert diskptr to char*/
    char* disk_array = (char*)diskptr;
    free(disk_array);
}
