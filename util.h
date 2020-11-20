#include <stdio.h>
#include <stdint.h>


/* bit operations */

void setBit(int *A, int k);
void clearBit(int *A, int k);
int testBit(int *A, int k);
int getFirstAvailableBit(int *A, int size_int_bits);


/* sorting */
void quick_sort(int *arr, int low, int high);

void print_block_data(char *block_data);
