all: treefind
.PHONY: all

treefind: tree.c
	gcc -o treefind tree.c -lz -lpthread

