#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <pthread.h>

#define errorMessage(A) fprintf(stderr,A)
#define warningMessage(A) fprintf(stderr,A)

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MAXLINELENGTH 100000
#define QUEUESIZE 100
#define MAXSTRINGIN 1000

struct node{
  //int minPos,maxPos;
  unsigned int nPos; //number of positions
  int *pos; //array of positions sorted from least to greatest
  struct node *children[4]; //pointer to 4 children
};


//arguments to pass into pthread_create
struct fsitArgs{
  struct node *tree;
  char *query;
  struct node *currentNode;
  int pos;
  unsigned int maxMismatch;
  int *out;
  int id;
  char *buffer;
};

char** getSeqFromFastq(gzFile *in,char **buffers);
int findMinPos(struct node *node, int pos);
void addNodePos(struct node *node,int pos);
struct node* createNode(unsigned int pos);
unsigned int convertCharToIndex(char base);
char convertIndexToChar(int index);
char complementBase(char base);
int complementString(char *read,char *out);
struct node* buildTree(char *s1);
unsigned int countNodes(struct node *tree);
int destroyTree(struct node *tree);
int findStringInTree(struct node *tree,char *query,struct node *currentNode, int pos, unsigned int maxMismatch);
int writeSeqToFastq(gzFile *out, char **buffers);
int onlyACTG(char *read);
char *revString(const char *str,char *str2);
int strCat(char *str1, const char *str2);
int switchBuffers(char **buffer1, char **buffer2);
void *findStringInTreePar(void *fsitArgs);
void findReadsInFastq(char** ref, char **fileName, int *parameters,char **outNames);
char* readFastq(char *fileName);

#endif /*TREE_H*/
