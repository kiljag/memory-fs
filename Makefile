main: main.o disk.o sfs.o util.o
	gcc -o main main.o disk.o sfs.o util.o
main.o: main.c disk.h sfs.h
	gcc -c -g main.c
sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c
disk.o: disk.c disk.h
	gcc -c -g disk.c
util.o: util.c util.h
	gcc -c -g util.c

clean:
	rm outfile main *.o
