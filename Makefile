main: main.o disk.o sfs.o util.o
	gcc -o main main.o disk.o sfs.o util.o -Wall
main.o: main.c disk.h sfs.h
	gcc -c -g main.c -Wall
sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c -Wall
disk.o: disk.c disk.h
	gcc -c -g disk.c -Wall
util.o: util.c util.h
	gcc -c -g util.c -Wall