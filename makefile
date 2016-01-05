all: suffixc README.md
.PHONY: all

suffixc: tree.c main.c makefile
	$(CC) -c tree.c -lz -lpthread
	$(CC) -o suffixc main.c tree.o -lz -lpthread


TEMP := $(shell mktemp)
README.md: suffixc README.template
	./suffixc -h>$(TEMP)
	sed -e "/##USAGEHERE##/{r $(TEMP)" -e "d;}" <README.template >README.md

test: tree.c main.c tests.bash testtree.c makefile
	$(CC) -Wall -c tree.c -lz -lpthread -coverage
	$(CC) -Wall -o suffixc main.c tree.o -lz -lpthread -coverage
	$(CC) -Wall -o testtree tree.o testtree.c -lz -lpthread -coverage
	./testtree
	bash tests.bash
	gcov main.c tree.h tree.c
	rm suffixc tree.o
	make suffixc
