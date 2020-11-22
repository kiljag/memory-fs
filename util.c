/*
Advances in Operating System Design
Assignment 2

Jagadeesh Killi (16CS30013)
Pruthvi Sampath Chabathula (16CS30028)
*/

#include <stdio.h>
#include <stdint.h>

#include "util.h"

/*set bit at kth position in int array A*/
void setBit(int *A, int k) {
    int i = k / 32;
    int pos = k % 32;
    A[i] = A[i] | (1 << pos);
}

/*clear bit at kth position in int array A*/
void clearBit(int *A, int k) {
    int i = k / 32;
    int pos = k % 32;
    A[i] = A[i] & ~(1 << pos);
}


/*test if bit at kth position is set or not*/
int testBit(int *A, int k) {
    int i = k / 32;
    int pos = k % 32;

    if (A[i] & (1 << pos)) {
        return 1;
    } else {
        return 0;
    }
}


/* get the first available unset bit position
 * size_in_bits also denotes number of inodes */
int getFirstAvailableBit(int *A, int size_in_bits) {
    int i, pos;
    // printf("size in bits : %u\n", size_in_bits);

    //check in $(size_in_bits/32) integer blocks
    for (i = 0; i < (size_in_bits / 32); i++) {
        
        if (A[i] ^ 0xFFFFFFFF) {
            for(pos = 0; pos < 32; pos++) {
                int k = (32 * i) + pos;
                if(!testBit(A, k)){
                    // printf("size_in_bits : %d i: %d, k: %d\n", size_in_bits, i, k);
                    return k;
                };
            }
        }
    }

    // check in remaining bits of last integer block
    for (pos = 0; pos < (size_in_bits % 32); pos++) {
        int k = 32 * i + pos;
        if(!testBit(A, k)) {
            // printf("size_in_bits : %d i: %d, k: %d\n", size_in_bits, i, k);
            return k;
        }
    }

    return -1;
}


/*quick sort utilities*/

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
void quick_sort(int *arr, int low, int high) {
    if (low < high) {
        int pi = quick_sort_partition(arr, low, high);
        
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

/*an utility function to print block data in hex format*/
void print_block_data(char* block_data) {
    for (int i=0; i<1024; i++) {
        printf("%x:", ((int *)block_data)[i]);
    }
    printf("\n");
}



