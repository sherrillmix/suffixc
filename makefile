all: suffic
.PHONY: all

suffic: tree.c main.c makefile
	gcc -c tree.c -lz -lpthread
	gcc -o suffic main.c tree.o -lz -lpthread


test: tree.c main.c tests.bash testtree.c makefile
	gcc -Wall -c tree.c -lz -lpthread -coverage
	gcc -Wall -o suffic main.c tree.o -lz -lpthread -coverage
	gcc -Wall -o testtree tree.o testtree.c -lz -lpthread -coverage
	./testtree
	bash tests.bash
	gcov main.c tree.h tree.c
	rm suffic tree.o
	make suffic
