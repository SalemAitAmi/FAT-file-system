CC=gcc
CFLAGS=-c -Wall -g -DDEBUG

all: statFATfs lsFATfs catFATfs storFATfs

statFATfs: statFATfs.o
	$(CC) statFATfs.o -o statFATfs

statFATfs.o: statFATfs.c disk.h
	$(CC) $(CFLAGS) statFATfs.c

lsFATfs: lsFATfs.o
	$(CC) lsFATfs.o -o lsFATfs

lsFATfs.o: lsFATfs.c disk.h
	$(CC) $(CFLAGS) lsFATfs.c

catFATfs: catFATfs.o
	$(CC) catFATfs.o -o catFATfs

catFATfs.o: catFATfs.c disk.h
	$(CC) $(CFLAGS) catFATfs.c

storFATfs: storFATfs.o
	$(CC) storFATfs.o -o storFATfs

storFATfs.o: storFATfs.c disk.h
	$(CC) $(CFLAGS) storFATfs.c

clean:
	rm -rf *.o statFATfs lsFATfs catFATfs storFATfs
