# CS 241 SPRING 2013
# The University of Illinois

CC = gcc
INC = -I.
FLAGS = -W -Wall -g
LINKOPTS = -lpthread

all: msort gen doc/html testfile seg_gen

doc/html:
	doxygen doc/Doxyfile

msort: msort.c
	$(CC) $(INC) $(FLAGS) $(LINKOPTS) msort.c -o msort

gen: gen.c
	$(CC) $(INC) $(FLAGS) gen.c -o gen

testfile: testfile.c
	$(CC) $(INC) $(FLAGS) testfile.c -o testfile

seg_gen: seg_gen.c
	 $(CC) $(INC) $(FLAGS) seg_gen.c -o seg_gen

clean:
	rm -rf msort gen doc/html testfile
