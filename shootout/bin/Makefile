CYCC=../../bin/cyclone1 -g
CC=gcc
RM=rm -f
all: hist munge load csv

munge: munge.c
	$(CC) munge.c -o munge
hist: cychist.cyc
	$(CYCC)  cychist.cyc -o hist -lm

load:	load.c
	$(CC) -Wall -g load.c -o load -lpthread

bencher:	bencher.c
	$(CC) -Wall -g bencher.c -o bench 

csv:	csv.c
	$(CC) -Wall -g csv.c -o csv

clean:
	$(RM) munge hist load csv
