//R CMD SHLIB tree.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define forR 0
#if forR==1
	#include <R.h>
	#define errorMessage(A) error(A)
	#define warningMessage(A) warning(A)
#else
	#define errorMessage(A) printf(A)
	#define warningMessage(A) printf(A)
#endif

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))

struct node{
	//int minPos,maxPos;
	unsigned int nPos; //number of positions
	int *pos; //array of positions sorted from least to greatest
	struct node *children[4]; //pointer to 4 children
};

/*
 * Don't need this now that we're using smarter sort
int compareInt(const void *x,const void *y){
	int *ix=(int*)x;
	int *iy=(int*)y;
	return(*ix-*iy);   //*ix>*iy?1:*ix<*iy?-1:0
}
*/

int findMinPos(struct node *node, int pos){
	for(unsigned int i=0;i<node->nPos;i++){
		if(node->pos[i]>pos)return(node->pos[i]); //careful: always >?
	}
	warningMessage("Couldn't find valid position");
	return(-99);
}

void addNodePos(struct node *node,int pos){
	int *tmp=malloc(sizeof(int)*node->nPos+1);
	unsigned int foundInsert=0;
	for(unsigned int i=0;i<node->nPos;i++){
		if(!foundInsert&&pos<node->pos[i]){
			tmp[i]=pos;
			foundInsert=1;
		}
		tmp[i+foundInsert]=node->pos[i];
	}
	//must be the biggest
	if(!foundInsert)tmp[node->nPos]=pos;
	//qsort(tmp,node->nPos+1,sizeof(int),compareInt); //could do this smarter since we actually know we're getting a sorted array
	free(node->pos);
	node->pos=tmp;
	node->nPos++;
}

struct node* createNode(unsigned int pos){
	struct node* thisNode;
	thisNode=(struct node *)malloc(sizeof(struct node));
	if(thisNode==(struct node*)0){
		errorMessage("No memory");
		exit(-99);
	}
	//store all positions
	//thisNode->minPos=pos;
	//thisNode->maxPos=pos;
	thisNode->pos=malloc(sizeof(int));
	thisNode->nPos=1;
	thisNode->pos[0]=pos;
	for(int i=0;i<4;i++)thisNode->children[i]=NULL;
	return thisNode;
}

unsigned int convertCharToIndex(char base){//somesort of inline here?
	return base=='A'?0:
	base=='C'?1:
	base=='G'?2:
	base=='T'?3:99;
}
char convertIndexToChar(int index){//somesort of inline here?
	return index==0?'A':
	index==1?'C':
	index==2?'G':
	index==3?'T':'N';
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
				addNodePos(currentNode->children[charId],j);
				//currentNode->children[charId]->minPos=MIN(j,currentNode->children[charId]->pos[0]);
				//currentNode->children[charId]->maxPos=MAX(j,currentNode->children[charId]->maxPos);
			}
			currentNode=currentNode->children[charId];
			//printf("char:%c\n",thisChar);
		}
	}
	return(head);
}

unsigned int countNodes(struct node *tree){
	unsigned int count=0;
	for(unsigned int i=0;i<4;i++){
		if(tree->children[i]!=(struct node*)0)count+=countNodes(tree->children[i]);
	}
	count++;
	return(count);
}

int destroyTree(struct node *tree){
	for(unsigned int i=0;i<4;i++){
		if(tree->children[i]!=(struct node*)0)destroyTree(tree->children[i]);
	}
	free(tree->pos);
	free(tree);
	return(0);
}

char* readFastq(char *fileName){
		FILE *in = fopen(fileName,"rt");
		char name[100000],seq[100000];
		size_t length;
		while(fgets(name,100000,in) != (char*)0){
			length=strlen(name);
			if(name[length-1]=='\n')name[length-1]='\0';
			if(fgets(seq,100000,in)==(char*)0){
				printf("Missing sequence");
			}
			length=strlen(seq);
			if(seq[length-1]=='\n')seq[length-1]='\0';
		}
		fclose(in);

}




//make recursive with minPos and maxMisses (and currentNode)? then call on each substring (and mismatch)?
//return -matches or 1 if we made it
int findStringInTree(struct node *tree,char *query,struct node *currentNode, int pos, unsigned int maxMismatch){ // unsigned int maxGap, unsigned int maxMismatch, unsigned int maxInsert){
	int match;
	//iterators
	unsigned int i,k;
	int j;

	unsigned int depth,charId;
	int tmp;
	//int *match=malloc(sizeof(int)*4); //matches, completed, mismatches, splices (should probably have insertions, deletions too)
	//for(i=0;i<5;i++)match[i]=0;
	unsigned int n=strlen(query);
	//printf("Pointer size: %lu\n",sizeof(int*));
	//store binary for nodes to check out if we need to check splicing (don't need to check if our min pos didn't change)
	int *pathPos=malloc(sizeof(int)*(n+1));//if we start storing multiples, e.g. down multiple paths, then we'll need more
	//unsigned int nJumpBack=0;
	//store pointers to nodes to check mismatch
	struct node **path=malloc(sizeof(struct node*)*(n+1));//if we start storing multiples, e.g. down multiple paths, then we'll need more
	unsigned int nPath=0;
	depth=0;
	int bestMatch;
	char base, tmpChar;

	//printf("Running on char %d string with %d mismatch.\n",n,maxMismatch);
	
	pathPos[0]=pos; 
	path[0]=currentNode; 
	bestMatch=1;//Really shouldn't default to success
	for(i=0;i<n;i++){
		charId=convertCharToIndex(query[i]);
		//printf("i:%d ",i);
		//appropriate child is not null & we're not passed the position of that child
		if(currentNode->children[charId]!=(struct node*)0 && currentNode->children[charId]->pos[currentNode->children[charId]->nPos-1]>pos){
			//printf("Match\n");
			currentNode=currentNode->children[charId];
			tmp=findMinPos(currentNode,pos);
			//if(tmp>pos+1){
				//if(depth>0){
				//}
				//printf("Jumping ahead at depth:%d pos: %d-->%d. %d nodes to jump back to\n",depth,pos,tmp,nJumpBack);
			//}
			pos=tmp;
			if(pos<0){errorMessage("Invalid position");exit(-2);}
			depth++;
			path[depth]=currentNode;
			pathPos[depth]=pos;
		}else{
			//printf("Break at depth %d",depth,i);
			bestMatch=-depth; //kind of dangerous probably should do another way
			//printf("Break\n");
			//break in string
			if(maxMismatch>0){
				for(j=depth;j>=0;j--){
					//printf("j:%d ",j);
					//splicing
					if(pathPos[j]-pathPos[j-1]>1&&j>0||j==depth){
						//jump to root and try to match the rest, respecting the position of this match
						tmp=findStringInTree(tree,&query[j],tree,pathPos[j],maxMismatch-1);
						//printf("Splice %d %s<----\n",tmp,&query[j]);
						if(tmp>0){
							bestMatch=1;
							break; //depth loop
						}
						tmp=tmp-j;
						if(tmp<bestMatch)bestMatch=tmp;
					}
					//mismatch
					tmpChar=query[j];
					for(k=0;k<4;k++){
						base=convertIndexToChar(k);
						if(tmpChar==base)continue;
						//substitute char in string here
						query[j]=base;
						//printf("%s",&query[j]);
						tmp=findStringInTree(tree,&query[j],path[j],pathPos[j],maxMismatch-1); //find best match for remainder of the string with base substituted at the next position
						if(tmp>0){
							bestMatch=1;
							break; //ACTG loop
						}
						tmp=tmp-j;
						//printf("Mismatch %c->%c %d  Best:%d",tmpChar,base,tmp,bestMatch);
						if(tmp<bestMatch)bestMatch=tmp;
					}
					query[j]=tmpChar;
					//printf("\n");
					if(bestMatch>0)break;//depth loop
				}
			}
			break; //could do an insertion here
		}
	}
	free(path);
	free(pathPos);
	return(bestMatch);
}


/* Not too useful for now don't feel like keeping up to date
int checkPossibleMatch(struct node *tree,char* query, int nSegments){
	int result, n, r,end;
	char tmp;
	n=strlen(query);
	r=n/nSegments;
	//printf("n: %d r: %d",n,r);
	if(n<nSegments)return(1);
	for(unsigned int j=0;j<nSegments;j++){
		if(j==nSegments-1)end=n;
		else end=(j+1)*r-1;
		tmp=query[end+1];
		query[end+1]='\0';
		result=findStringInTree(tree,&query[j*r]);
		//printf("i: %d j: %d start: %d end: %d string: %s result:%d\n",i,j,j*r,end,&queries[i][j*r],result);
		query[end+1]=tmp;
		if(result==0)return(1);
	}
	return(0);
}
*/

/*
 * k mismatches, l gaps means that divide n-base query into n/(k+l+1) at least one must be perfect match
 */
/* Not too useful for now. Not mantaining.
void anyChanceOfMatch(int *answers, char **ref, char **queries,int *nQueries, int *nGapMismatch){
	int start,end,result,n,r,nSegments,tmpResult;
	nSegments=nGapMismatch[0]+1;
	char tmp;

	struct node *tree;
	printf("Building tree\n");
	tree=buildTree(ref[0]);
	printf("%d node tree ready\n",countNodes(tree));
	printf("Scanning %d queries\n",nQueries[0]);
	for(int i=0;i<nQueries[0];i++){
		answers[i]=checkPossibleMatch(tree,queries[i],nSegments);
	}
	destroyTree(tree);
}
*/

void treeAlign(int *answer,char **ref,char **queries, int *nQueries,int *maxMismatch){
	struct node *tree;
	printf("Building tree\n");
	tree=buildTree(ref[0]);
	printf("%d node tree ready\n",countNodes(tree));
	printf("Scanning %d queries (max mismatch %d)\n",nQueries[0],maxMismatch[0]);
	for(int i=0;i<nQueries[0];i++){
		//printf("Checking %d\n",i);
		answer[i]=findStringInTree(tree,queries[i],tree,-1,maxMismatch[0]);
		//DO SOME CRAP WITH MATCH ARRAY
		//printf("Answer[%d] (of %d): %d\n",i,nQueries[0],answer[i]);
	}
	printf("Destroying tree\n");
	destroyTree(tree);
}



