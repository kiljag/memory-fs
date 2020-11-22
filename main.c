#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"
#include "util.h"


void fill_random_char_data(char* data, int size) {
	for(int i=0; i<size; i++) {
		data[i] = 65 + (char)(random() % 26);
	}
}

void createRootDir()
{
	int a = create_file(); // Creates root directory and allots inode 0

}

void testCreateDir2()
{
	printf("\n");
	printf("----------------------------------------------------------------------\n");
	printf("\n");
	printf(" Testing create_directory() 2 ");
	printf("\n");

	int a,i;
	a =  stat(0);
	printf("\nSize of root dir before is 0\n\n");
	for(i=0;i<500;i++)
    {
        char* fname = (char*)calloc(10,1);
        char b[] = "dir";
        memcpy(fname,b,3);
        sprintf(fname+3, "%d", i); 
        a = create_dir(fname);
    }
    a = stat(0);
	printf("\nSize of root dir before is 64*500 (extra64 bytes because Dir1 is already present in root directory created in testCreateDir)\n\n");
}

void testCreateDir()
{	
	printf("\n");
	printf("----------------------------------------------------------------------\n");
	printf("\n");
	printf(" Testing create_directory() ");
	printf("\n");
	int a;

	

	a = create_dir("Dir1"); // Create directory Dir1 in root 
	a = stat(1);
	printf("\nSize of Dir1 before is 0\n");
	

	a = create_dir("Dir1/Dir2"); // Create directory Dir2 in Dir1 
	a = create_dir("Dir1/Dir3"); // Create directory Dir3 in Dir1
	a = create_dir("Dir1/Dir3/Dir4"); // Create directory Dir3 in Dir3
	a = create_dir("Dir1/Dir5"); // Create directory Dir4 in Dir1

	a = stat(1);
	printf("\nSize of Dir1 after creating Dir2,Dir3 and Dir5 is 192bytes (64 bytes for each directory entry)\n");
}

void testremoveDir()
{
	printf("\n");
	printf("----------------------------------------------------------------------\n");
	printf("\n");
	printf(" Testing remove_directory() ");
	printf("\n");
	int a;

	a = stat(1);
	printf("\nSize of Dir1 before is 192bytes \n");

	a = remove_dir("Dir1/Dir2");
	a = remove_dir("Dir1/Dir5");

	a = stat(1);
	printf("\nSize of Dir1 after removing Dir2 and Dir5 is 64bytes \n");

	a = remove_dir("Dir1/Dir2");
	printf("\nThe above message is because we are trying to delete Dir2 which is already deleted\n");


}

void testReadandWrite()
{
	printf("\n");
	printf("----------------------------------------------------------------------\n");
	printf("\n");
	printf("Testing read and write \n");
	printf("\n");
	int a;

	create_dir("Dir1/Dir3/Dir4/file1.txt");

	char* write_data = (char*)calloc(10000,1);
	fill_random_char_data(write_data, 10000);
	printf("Size of data written : 10000\n");

	a = write_file("Dir1/Dir3/Dir4/file1.txt", write_data, 10000, 0);

	char* read_data = (char*)calloc(10000,1);
    a = read_file("Dir1/Dir3/Dir4/file1.txt", read_data, 20, 0);
	printf("\nFirst 20 bytes of read data : %s\n",read_data);

	printf("/n/nThe file's inode is the following\n");
	stat(2);
}

int main(){

	disk* diskptr = create_disk(4096*1024 + 24); // Create a disk of size of 4MB
    int a = format(diskptr); // Format the disk
    a = mount(diskptr); // Mount Simple File System

	createRootDir(); // Creates root directory in SFS, all file address are with respect to root dorectory

	testCreateDir(); // Tests create_dir() function 
	testCreateDir2(); // Creates 100 Directory files in root directory

	testremoveDir(); // Removes Dir2 and Dir5 in Dir1

	testReadandWrite(); // Creates an file using create_dir(), writes data into file(), reads the data from the file

	// // int x = create_file();
	// create_dir("dir1");
	// fill_random_char_data(write_data, 10000);
	// create_dir("dir1/a.txt");
	// write_file("dir1/a.txt", write_data, 10000, 0);


	return 0;
}
