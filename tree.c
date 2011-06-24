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
	unsigned int nPos;
	int *pos;
	struct node *children[4];
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




int findStringInTree(struct node *tree,char* query){
	unsigned int depth,charId;
	int pos, tmp;
	int gaps;
	int gapCutoff=5;
	unsigned int n=strlen(query);
	struct node *currentNode=tree;
	//store pointers to nodes to check out
	struct node *jumpBack=malloc(sizeof(struct node)*n);
	unsigned int jumpBackn=0;
	pos=-1;
	gaps=0;
	depth=0;
	for(unsigned int i=0;i<n;i++){
		charId=convertCharToIndex(query[i]);
		//printf("i:%d ",i);
		if(currentNode->children[charId]!=(struct node*)0 && currentNode->children[charId]->pos[currentNode->children[charId]->nPos-1]>pos){
			//printf("Match\n");
			currentNode=currentNode->children[charId];
			tmp=findMinPos(currentNode,pos);
			if(depth>10&&tmp>pos+1)printf("Jumping ahead at depth:%d pos: %d-%d\n",depth,tmp,pos);//only 5 in 1M
			pos=tmp;
			if(pos<0){errorMessage("Invalid position");exit(-2);}
			depth++;
		}else{
			//printf("Break\n");
			//break in string
			gaps++;
			i--;
			currentNode=tree;
			depth=0;
		}
		//printf("Iter:%d Pos:%d-%d Depth:%d Gaps:%d\n",i,pos,currentNode->pos[currentNode->nPos-1],depth,gaps);
		if(gaps>=gapCutoff)break;
	}
	if(gaps==gapCutoff)gaps=999;
	free(jumpBack);
	return(gaps);
}


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

/*
 * k mismatches, l gaps means that divide n-base query into n/(k+l+1) at least one must be perfect match
 */
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

void treeAlign(int *answer,char **ref,char **queries, int *nQueries){
	struct node *tree;
	printf("Building tree\n");
	tree=buildTree(ref[0]);
	printf("%d node tree ready\n",countNodes(tree));
	printf("Scanning %d queries\n",nQueries[0]);
	for(int i=0;i<nQueries[0];i++){
		//printf("Checking %d\n",i);
		answer[i]=findStringInTree(tree,queries[i]);
		//printf("Answer[%d] (of %d): %d\n",i,nQueries[0],answer[i]);
	}
	printf("Destroying tree\n");
	destroyTree(tree);
}



