//R CMD SHLIB tree.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))

struct node{
	int minPos,maxPos;
	struct node *children[4];
};

struct node* createNode(unsigned int pos){
	struct node* thisNode;
	thisNode=(struct node *)malloc(sizeof(struct node));
	if(thisNode==(struct node*)0){
		printf("No memory");
		exit(-99);
	}
	thisNode->minPos=pos;
	thisNode->maxPos=pos;
	for(int i=0;i<4;i++)thisNode->children[i]=NULL;
	return thisNode;
}

unsigned int convertCharToIndex(char base){//somesort of inline here?
	return base=='A'?0:
	base=='C'?1:
	base=='G'?2:
	base=='T'?3:99;
}


struct node* buildTree(char *s1) {
	struct node *head, *currentNode;
	char thisChar;
	unsigned int charId;
	head=createNode(-1);
	unsigned int n=strlen(s1);
	//head->hasChildren[1]=99;
	//printf("%d",head->hasChildren[1]);

	//could do this in linear instead of n^2 if necessary
	//also could be more efficient in memory
	for(int i=n-1;i>=0;i--){
		//printf("i:%d\n",i);
		currentNode=head;
		for(int j=i;j<n;j++){
			//printf("j:%d\n",j);
			thisChar=s1[j];
			charId=convertCharToIndex(thisChar);
			//check if currentNode's child is filled
			if(currentNode->children[charId]==(struct node*)0){
				//Add node
				//printf("Created node");
				currentNode->children[charId]=createNode(j);
			}else{
				//Update node
				//printf("Updated node");
				currentNode->children[charId]->minPos=MIN(j,currentNode->children[charId]->minPos);
				currentNode->children[charId]->maxPos=MAX(j,currentNode->children[charId]->maxPos);
			}
			currentNode=currentNode->children[charId];
			//printf("char:%c\n",thisChar);
		}
	}
	return(head);
}

int destroyTree(struct node *tree){
	for(unsigned int i=0;i<4;i++){
		if(tree->children[i]!=(struct node*)0)destroyTree(tree->children[i]);
	}
	free(tree);
	return(0);
}

int findStringInTree(struct node *tree,char* query){
	int gapCutoff=5;
	unsigned int n=strlen(query);
	unsigned int depth,pos,charId;
	int gaps;
	struct node *currentNode=tree;
	pos=0;
	gaps=0;
	for(unsigned int i=0;i<n;i++){
		charId=convertCharToIndex(query[i]);
		//printf("i:%d ",i);

		if(currentNode->children[charId]!=(struct node*)0 && currentNode->children[charId]->maxPos>=pos){
			//match
			//printf("Match\n");
			currentNode=currentNode->children[charId];
			pos=currentNode->minPos;
		}else{
			//printf("Break\n");
			//break in string
			gaps++;
			i--;
			currentNode=tree;
		}
		if(gaps>=gapCutoff)break;
	}
	if(gaps==gapCutoff)gaps=999;
	return(gaps);
}


void treeAlign(int *answer,char **ref,char **queries, int *nQueries){
	struct node *tree;
	printf("Building tree\n");
	tree=buildTree(ref[0]);
	printf("Tree ready\n");
	printf("Scanning %d queries\n",nQueries[0]);
	for(int i=0;i<nQueries[0];i++){
		//printf("Checking %d\n",i);
		answer[i]=findStringInTree(tree,queries[i]);
		//printf("Answer[%d] (of %d): %d\n",i,nQueries[0],answer[i]);
	}
	printf("Destroying tree\n");
	destroyTree(tree);
}



