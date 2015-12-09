//R CMD SHLIB -lz tree.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <pthread.h>


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
#define MAXLINELENGTH 100000
#define QUEUESIZE 100

struct node{
	//int minPos,maxPos;
	unsigned int nPos; //number of positions
	int *pos; //array of positions sorted from least to greatest
	struct node *children[4]; //pointer to 4 children
};
//return null pointer if bad
//assume single seqs/qual per line and no comments
char** getSeqFromFastq(gzFile *in,char **buffers){
	size_t length;
	
	if(gzgets(in,buffers[0],MAXLINELENGTH)==(char*)0)return((char**)0);
	//peak ahead
	//ungetc(tmpChar,in);
	//name,seq,name again,qual
	for(int i=1;i<4;i++){
		if(gzgets(in,buffers[i],MAXLINELENGTH)==(char*)0){
			errorMessage("Incomplete fastq record");
			exit(-99);
		}
		//remove new lines
		//length=strlen(buffers[i]);
		//if(buffers[i][length-1]=='\n')buffers[i][length-1]='\0';
	}
	return(buffers);
}


/*
struct buffer{
	char *line[4];
};

struct bufferQueue{
	struct buffer **buffers;	
	unsigned int start,end,n,maxN;
};

struct bufferQueue *createQueue(int n){
	int i,j;
	struct bufferQueue *queue=(struct bufferQueue *)malloc(sizeof(struct bufferQueue));
	queue->buffers=(struct buffer **)malloc(sizeof(struct buffer *)*n);
	for(i=0;i<QUEUESIZE;i++){
		for(j=0;j<4;j++){
			queue->buffers[i]->line[j]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	}
	queue->n=0;
	queue->start=0;
	queue->end=n-1;
	queue->maxN=n;
}

int addToQueueFromGzFile(struct bufferQueue *queue, gzFile *in){
	if(queue->n >= queue->maxN)return(0);
	int nextId=(queue->end+1)%queue->maxN;
	if(getSeqFromFastq(in,queue->buffers[nextId]) == (char**)0)return(-1);
	queue->end=nextId;
	queue->n++;
}
*/

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

char complementBase(char base){
	return base=='A'?'T':
	base=='C'?'G':
	base=='G'?'C':
	base=='T'?'A':'N';
}

int complementString(char *read,char *out){
	unsigned int counter=0;
	while(read[counter]!='\0'&&read[counter]!='\n'){
		out[counter]=complementBase(read[counter]);
		counter++;
	}
	out[counter]='\0';
	return(1);
}
struct node* buildTree(char *s1) {
	struct node *head, *currentNode;
	char thisChar;
	unsigned int charId;
	head=createNode(-1);
	size_t n=strlen(s1);
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
	if(tree==(struct node*)0)return(count);//catch empty tree
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
	size_t n=strlen(query);
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
		if(query[i]=='\n')break;//Assuming new line means end of string
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


int writeSeqToFastq(gzFile *out, char **buffers){
	size_t length;
	for(int i=0;i<4;i++){
		if(gzputs(out,buffers[i])==-1)return(0);
	}
	return(1);
}

int onlyACTG(char *read){
	unsigned int counter=0;
	while(read[counter]!='\0'&&read[counter]!='\n'){
		if(convertCharToIndex(read[counter])>3){
			//printf("Bad: %c -> %u\n",read[counter],convertCharToIndex(read[counter]));
			return(0);
		}
		counter++;
	}
	return(1);
}

char *revString(const char *str,char *str2){
	size_t n=strlen(str);
	//char *buffer=(char *)malloc(sizeof(char)*(n+1));
	if(str[n-1]=='\n')n--;
	str2[n]='\0';
	for(int i=n-1;i>=0;i--){
		str2[n-1-i]=str[i];
	}
	return(str2);
}

int strCat(char *str1, const char *str2){
	int counter=0;
	while(str1[counter]!='\0'&&str1[counter]!='\n')counter++;//assuming new line is end of string
	int firstPos=counter;
	counter=0;
	while(str2[counter]!='\0'){
		str1[firstPos+counter]=str2[counter];
		counter++;
	}
	str1[firstPos+counter]='\0';
	return(1);
}

/*
void findPrimersInFastq(char **primers, char **fileName, char **outNames, int *nPrimers){
	gzFile *in; //file pointer for fastqs
	gzFile **outs=(gzFile **)malloc(sizeof(gzFile *)*nPrimers);
	char* buffers[4];
	for(i=0;i<4;i++)buffers[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	int i;
	in=gzopen(in,"rt");
	for(i=0;i<nPrimers+1;i++){
		outs[i]=gzopen(outNames[i],"w");
	}
	while(getSeqFromFastq(in,buffers) != (char**)0){
	}
	free(outs);	
}*/

int switchBuffers(char **buffer1, char **buffer2){
	char *tmp; //for switching buffers
	for(int i=0;i<4;i++){
		tmp=buffer1[i];
		buffer1[i]=buffer2[i];
		buffer2[i]=tmp;
	}
	return(1);
}

/*
void *eightWayFindStringInTreePar(void *fsitArgs){
		struct fsitArgs *args=(struct fsitArgs *)fsitArgs;
		struct buffer seqs;

		strcpy(seqs[0],buffers[1]);
		revString(seqs[0],seqs[1]);
		complementString(seqs[0],seqs[2]);
		revString(seqs[2],seqs[3]);
		for(i=0;i<2;i++){
			for(j=0;j<4;j++){
				if(i==0)index=j;
				else index=4+(j/2*2)+1-(j%2); //using integer division to floor. and switching 4-5 and 6-7 to make later comparisons easy 0-4,1-5,...
				args[i*4+j]->tree=tree[i];
				args[i*4+j]->query=seqs[j];
				args[i*4+j]->currentNode=tree[i];
				args[i*4+j]->pos=-1;
				args[i*4+j]->maxMismatch=maxMismatch;
				args[i*4+j]->out=&ans[index];
				args[i*4+j]->id=i*4+j;
				if(pthread_create(&threads[i*4+j],NULL,findStringInTreePar,args[i*4+j])){errorMessage("Couldn't create thread");exit(-98);}
				//ans[index]=findStringInTree(tree[i],seqs[j],tree[i],-1,maxMismatch);
			}
		}
	struct fsitArgs *args=(struct fsitArgs *)fsitArgs;
	//printf("HERE %d ",args->id);
	*args->out=findStringInTree(args->tree,args->query,args->currentNode,args->pos,args->maxMismatch);
	//printf("HERE2 %d ",args->id);
}
*/
void *findStringInTreePar(void *fsitArgs){
	struct fsitArgs *args=(struct fsitArgs *)fsitArgs;
	//printf("HERE %d ",args->id);
	*args->out=findStringInTree(args->tree,args->query,args->currentNode,args->pos,args->maxMismatch);
	//printf("HERE2 %d ",args->id);
}
void findReadsInFastq(char** ref, char **fileName, int *parameters,char **outNames){
	int i,j; //iterators
	int index; //for filling in answers appropriately
	int maxMismatch=parameters[0]; //at most x mismatch and splices
	int minPartial=parameters[1]; //require x length to call partial match, if left and right partial then call a match 
	int sumMatch=parameters[2]; //if left length + right length > x, call a match
	struct node *tree[2]; //big suffix tree for aligning
	gzFile *in,*outMatch,*outPartial; //file pointer for fastqs
	//line buffers
	char* buffers[4];
	for(i=0;i<4;i++)buffers[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	char* buffers2[4];
	for(i=0;i<4;i++)buffers2[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	//seq conversions
	char* seqs[4];
	for(i=0;i<4;i++)seqs[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	
	char tmpStr[1000];//to append to things

	long int counter=0, matchCounter=0, partialCounter=0;
	struct fsitArgs *args[8];
	for(i=0;i<8;i++)args[i]=(struct fsitArgs *)malloc(sizeof(struct fsitArgs));
	int ans[8];//answer from findStringInTree
	int isMatch,isPartial;
	char **fastqCheck;//keep track of whether our fastq is empty

	//threads
	pthread_t threads[8];

	printf("Building tree\n");
	tree[0]=buildTree(ref[0]);
	printf("Building reverse tree\n");
	revString(ref[0],seqs[0]);
	tree[1]=buildTree(seqs[0]);

	printf("%d node trees ready\n",countNodes(tree[0]));


	printf("Opening file %s\n",fileName[0]);
	//I think this should work with uncompressed files too
	in=gzopen(fileName[0],"rt");
	printf("Opening outFile %s\n",outNames[0]);
	outMatch=gzopen(outNames[0],"w");
	printf("Opening outFile %s\n",outNames[1]);
	outPartial=gzopen(outNames[1],"w");

	printf("Scanning file (max mismatch %d, min partial %d, sum match %d)\n",maxMismatch,minPartial,sumMatch);
	fastqCheck=getSeqFromFastq(in,buffers);
	while(fastqCheck != (char**)0){
		counter++;
		if(counter%10000==0)printf("Working on read %ld\n",counter);
		if(!onlyACTG(buffers[1])){
			//write somewhere else?
			//printf("Bad read %ld: %s\n",counter,buffers[1]);
			fastqCheck=getSeqFromFastq(in,buffers);
			continue;
		}
		strcpy(seqs[0],buffers[1]);
		revString(seqs[0],seqs[1]);
		complementString(seqs[0],seqs[2]);
		revString(seqs[2],seqs[3]);
		for(i=0;i<2;i++){
			for(j=0;j<4;j++){
				if(i==0)index=j;
				else index=4+(j/2*2)+1-(j%2); //using integer division to floor. and switching 4-5 and 6-7 to make later comparisons easy 0&4,1&5,...
				args[i*4+j]->tree=tree[i];
				args[i*4+j]->query=seqs[j];
				args[i*4+j]->currentNode=tree[i];
				args[i*4+j]->pos=-1;
				args[i*4+j]->maxMismatch=maxMismatch;
				args[i*4+j]->out=&ans[index];
				args[i*4+j]->id=i*4+j;
				if(pthread_create(&threads[i*4+j],NULL,findStringInTreePar,args[i*4+j])){errorMessage("Couldn't create thread");exit(-98);}
				//ans[index]=findStringInTree(tree[i],seqs[j],tree[i],-1,maxMismatch);
			}
		}

		//read from disk while we're waiting
		fastqCheck=getSeqFromFastq(in,buffers2);
		//printf("%p",fastqCheck);
		//stop waiting here? and throw it all into some sort of giant queue?
		for(i=0;i<2;i++){
			for(j=0;j<4;j++){
				if(pthread_join(threads[i*4+j],NULL)){errorMessage("Couldn't join thread");exit(-97);}
			}
		}
		isMatch=0;
		isPartial=0;
		for(i=0;i<4;i++){
			if(ans[i]>0||ans[i+4]>0||(ans[i]<-minPartial&&ans[i+4]<-minPartial)||(ans[i]+ans[i+4])<-sumMatch)isMatch=1; //found a match, or both side have partial, or sum of sides enough
			if(ans[i]<-minPartial||ans[i+4]<-minPartial)isPartial=1;
			sprintf(tmpStr,":%d|%d",-ans[i],-ans[i+4]);
			strCat(buffers[0],tmpStr);
		}
		strCat(buffers[0],"\n");

		if(isMatch){ //also ans+ans2>X
			writeSeqToFastq(outMatch,buffers);
			matchCounter++;
		}else if(isPartial){
			writeSeqToFastq(outPartial,buffers);
			partialCounter++;
		}
		switchBuffers(buffers,buffers2);
		//ignore bad matches for now
	}
	printf("All done. Found %ld matches, %ld partials from %ld reads\n",matchCounter,partialCounter,counter);
	for(i=0;i<4;i++)free(buffers[i]);
	for(i=0;i<4;i++)free(buffers2[i]);
	for(i=0;i<4;i++)free(seqs[i]);
	for(i=0;i<8;i++)free(args[i]);
	printf("Closing file %s\n",fileName[0]);
	gzclose(in);
	printf("Closing outFiles\n");
	gzclose(outMatch);
	gzclose(outPartial);
	printf("Destroying trees\n");
	for(i=0;i<2;i++) destroyTree(tree[i]);
	return;
}

void findReadsInFastqNoPar(char** ref, char **fileName, int *parameters,char **outNames){
	int i,j; //iterators
	int index; //for filling in answers appropriately
	int maxMismatch=parameters[0]; //at most x mismatch and splices
	int minPartial=parameters[1]; //require x length to call partial match, if left and right partial then call a match 
	int sumMatch=parameters[2]; //if left length + right length > x, call a match
	struct node *tree[2]; //big suffix tree for aligning
	gzFile *in,*outMatch,*outPartial; //file pointer for fastqs
	//line buffers
	char* buffers[4];
	for(i=0;i<4;i++)buffers[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	//seq conversions
	char* seqs[4];
	for(i=0;i<4;i++)seqs[i]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
	
	char tmpStr[1000];//to append to things

	long int counter=0, matchCounter=0, partialCounter=0;
	//answer from findStringInTree
	int ans[8];
	int isMatch,isPartial;

	printf("Building tree\n");
	tree[0]=buildTree(ref[0]);
	printf("Building reverse tree\n");
	revString(ref[0],seqs[0]);
	tree[1]=buildTree(seqs[0]);

	printf("%d node trees ready\n",countNodes(tree[0]));


	printf("Opening file %s\n",fileName[0]);
	//I think this should work with uncompressed files too
	in=gzopen(fileName[0],"rt");
	printf("Opening outFile %s\n",outNames[0]);
	outMatch=gzopen(outNames[0],"w");
	printf("Opening outFile %s\n",outNames[1]);
	outPartial=gzopen(outNames[1],"w");

	printf("Scanning file (max mismatch %d, min partial %d, sum match %d)\n",maxMismatch,minPartial,sumMatch);
	while(getSeqFromFastq(in,buffers) != (char**)0){
		counter++;
		if(counter%1000000==0)printf("Working on read %ld\n",counter);
		if(!onlyACTG(buffers[1])){
			//write somewhere else?
			//printf("Bad read %ld: %s\n",counter,buffers[1]);
			continue;
		}
		strcpy(seqs[0],buffers[1]);
		revString(seqs[0],seqs[1]);
		complementString(seqs[0],seqs[2]);
		revString(seqs[2],seqs[3]);
		for(i=0;i<2;i++){
			for(j=0;j<4;j++){
				if(i==0)index=j;
				else index=4+(j/2*2)+1-(j%2); //using integer division to floor. and switching 4-5 and 6-7 to make later comparisons easy 0-4,1-5,...
				ans[index]=findStringInTree(tree[i],seqs[j],tree[i],-1,maxMismatch);
			}
		}
		isMatch=0;
		isPartial=0;
		for(i=0;i<4;i++){
			if(ans[i]>0||ans[i+4]>0||(ans[i]<-minPartial&&ans[i+4]<-minPartial)||(ans[i]+ans[i+4])<-sumMatch)isMatch=1; //found a match, or both side have partial, or sum of sides enough
			if(ans[i]<-minPartial||ans[i+4]<-minPartial)isPartial=1;
			sprintf(tmpStr,":%d|%d",-ans[i],-ans[i+4]);
			strCat(buffers[0],tmpStr);
		}
		strCat(buffers[0],"\n");

		if(isMatch){ //also ans+ans2>X
			writeSeqToFastq(outMatch,buffers);
			matchCounter++;
		}else if(isPartial){
			writeSeqToFastq(outPartial,buffers);
			partialCounter++;
		}
		//ignore bad matches for now
	}
	printf("All done. Found %ld matches, %ld partials from %ld reads\n",matchCounter,partialCounter,counter);
	for(i=0;i<4;i++)free(buffers[i]);
	for(i=0;i<4;i++)free(seqs[i]);
	printf("Closing file %s\n",fileName[0]);
	gzclose(in);
	printf("Closing outFiles\n");
	gzclose(outMatch);
	gzclose(outPartial);
	printf("Destroying trees\n");
	for(i=0;i<2;i++) destroyTree(tree[i]);
	return;
}

char* readFastq(char *fileName){
	FILE *in = fopen(fileName,"rt");
	char name[MAXLINELENGTH],seq[MAXLINELENGTH];
	size_t length;
	while(fgets(name,MAXLINELENGTH,in) != (char*)0){
		length=strlen(name);
		if(name[length-1]=='\n')name[length-1]='\0';
		if(fgets(seq,MAXLINELENGTH,in)==(char*)0){
			printf("Missing sequence");
		}
		length=strlen(seq);
		if(seq[length-1]=='\n')seq[length-1]='\0';
	}
	fclose(in);
}


void treeAlign(int *answer,char **ref,char **queries, int *nQueries,int *maxMismatch){
	struct node *tree;
	printf("Building tree\n");
	tree=buildTree(ref[0]);
	printf("%d node tree ready\n",countNodes(tree));
	printf("Scanning %d queries (max mismatch %d)\n",nQueries[0],maxMismatch[0]);
	for(int i=0;i<nQueries[0];i++){
		//printf("Checking %d\n",i);
		answer[i]=findStringInTree(tree,queries[i],tree,-1,maxMismatch[0]);
		//printf("Answer[%d] (of %d): %d\n",i,nQueries[0],answer[i]);
	}
	printf("Destroying tree\n");
	destroyTree(tree);
}






