all: treefind
.PHONY: all

treefind: tree.c main.c tests.bash testtree.c makefile
	gcc -Wall -c tree.c -lz -lpthread -coverage
	gcc -Wall -o treefind main.c tree.o -lz -lpthread -coverage
	gcc -Wall -o testtree testtree.c tree.o -lz -lpthread -coverage
	./testtree
	bash tests.bash
	gcov main.c tree.h tree.c
	gcc -o treefind main.c -lz -lpthread


