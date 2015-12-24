#include "tree.h"

int getRefFromFasta(const char *in,char *out){
  char buffer[MAXLINELENGTH];
  gzFile inFile;
  out[0]='\0';
  inFile=gzopen(in,"r");
  //check if working and trash since we assume only 1 sequence and won't keep the name
  if(gzgets(inFile,buffer,MAXLINELENGTH)==(char*)0)return(0);
  while(strlen(buffer)<MAXLINELENGTH && gzgets(inFile,buffer,MAXLINELENGTH-strlen(buffer))!=(char*)0){
	  trimSeq(buffer);
	  strCat(out,buffer);
  }
  gzclose(inFile);
  return(1);
}

int trimSeq(char *seq){
	int ii;
	for(ii=strlen(seq);ii>0;ii--){
		//fprintf(stderr,"%d:'%c ?:%d-%d'\n",ii,seq[ii],isspace(seq[ii]),iscntrl(seq[ii]));
		//low asciis are control characters
		if((int)seq[ii]<33)seq[ii]='\0';
		else break;
	}
	return(1);	
}

//return null pointer if bad
//assume single seqs/qual per line and no comments
int getSeqFromFastq(gzFile *in,char **buffers){
  int ii;

  if(gzgets(*in,buffers[0],MAXLINELENGTH)==(char*)0)return(0);
  //peak ahead
  //ungetc(tmpChar,in);
  //name,seq,name again,qual
  for(ii=1;ii<4;ii++){
    if(gzgets(*in,buffers[ii],MAXLINELENGTH)==(char*)0){
      errorMessage("Incomplete fastq record");
      exit(-99);
    }
  }
  return(1);
}

int findMinPos(struct node *node, int pos){
  unsigned int ii;
  for(ii=0;ii<node->nPos;ii++){
    if(node->pos[ii]>pos)return(node->pos[ii]); //careful: always >?
  }
  warningMessage("Couldn't find valid position");
  return(-99);
}

void addNodePos(struct node *node,int pos){
  int *tmp=malloc(sizeof(int)*node->nPos+1);
  unsigned int ii;
  unsigned int foundInsert=0;
  for(ii=0;ii<node->nPos;ii++){
    if(!foundInsert&&pos<node->pos[ii]){
      tmp[ii]=pos;
      foundInsert=1;
    }
    tmp[ii+foundInsert]=node->pos[ii];
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
  int ii;
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
  for(ii=0;ii<4;ii++)thisNode->children[ii]=NULL;
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
  int ii,jj;

  //could do this in linear instead of n^2 if necessary
  //also could be more efficient in memory
  for(ii=n-1;ii>=0;ii--){
    //fprintf(stderr,"ii:%d\n",ii);
    currentNode=head;
    for(jj=ii;jj<n;jj++){
      thisChar=s1[jj];
      charId=convertCharToIndex(thisChar);
      //fprintf(stderr,"jj:%d=%c:%d\n",jj,thisChar,charId);
      //check if currentNode's child is filled
      if(currentNode->children[charId]==(struct node*)0){
        //Add node
        //printf("Created node");
        currentNode->children[charId]=createNode(jj);
      }else{
        //Update node
        //printf("Updated node");
        addNodePos(currentNode->children[charId],jj);
        //currentNode->children[charId]->minPos=MIN(jj,currentNode->children[charId]->pos[0]);
        //currentNode->children[charId]->maxPos=MAX(jj,currentNode->children[charId]->maxPos);
      }
      currentNode=currentNode->children[charId];
      //printf("char:%c\n",thisChar);
    }
  }
  return(head);
}

unsigned int countNodes(struct node *tree){
  unsigned int count=0;
  unsigned int ii;
  if(tree==(struct node*)0)return(count);//catch empty tree
  for(ii=0;ii<4;ii++){
	 //fprintf(stderr,"Node: %d\n",ii);
    if(tree->children[ii]!=(struct node*)0)count+=countNodes(tree->children[ii]);
  }
  count++;
  //fprintf(stderr,"Leaving node. Count %d\n",count);
  return(count);
}

int destroyTree(struct node *tree){
  unsigned int ii;
  for(ii=0;ii<4;ii++){
    if(tree->children[ii]!=(struct node*)0)destroyTree(tree->children[ii]);
  }
  free(tree->pos);
  free(tree);
  return(1);
}


//make recursive with minPos and maxMisses (and currentNode)? then call on each substring (and mismatch)?
//return -matches or 1 if we made it
int findStringInTree(struct node *tree,char *query,struct node *currentNode, int pos, unsigned int maxMismatch){ // unsigned int maxGap, unsigned int maxMismatch, unsigned int maxInsert){
  //iterators
  unsigned int ii,k;
  int jj;

  unsigned int depth,charId;
  int tmp;
  size_t n=strlen(query);
  char *queryCopy;
  queryCopy=(char*)malloc(sizeof(char)*n+1);
  strcpy(queryCopy,query);
  //printf("Pointer size: %lu\n",sizeof(int*));
  //store binary for nodes to check out if we need to check splicing (don't need to check if our min pos didn't change)
  int *pathPos=malloc(sizeof(int)*(n+1));//if we start storing multiples, e.g. down multiple paths, then we'll need more
  //unsigned int nJumpBack=0;
  //store pointers to nodes to check mismatch
  struct node **path=malloc(sizeof(struct node*)*(n+1));//if we start storing multiples, e.g. down multiple paths, then we'll need more
  depth=0;
  int bestMatch;
  char base, tmpChar;

  //printf("Running on char %d string with %d mismatch.\n",n,maxMismatch);

  pathPos[0]=pos; 
  path[0]=currentNode; 
  bestMatch=1;//Really shouldn't default to success
  for(ii=0;ii<n;ii++){
    if(queryCopy[ii]=='\n')break;//Assuming new line means end of string
    charId=convertCharToIndex(queryCopy[ii]);
    //printf("ii:%d ",ii);
    //appropriate child is not null & we're not passed the position of that child
    if(currentNode->children[charId]!=(struct node*)0 && currentNode->children[charId]->pos[currentNode->children[charId]->nPos-1]>pos){
      //printf("Match\n");
      currentNode=currentNode->children[charId];
      tmp=findMinPos(currentNode,pos);
      pos=tmp;
      if(pos<0){errorMessage("Invalid position");exit(-2);}
      depth++;
      path[depth]=currentNode;
      pathPos[depth]=pos;
    }else{
      //printf("Break at depth %d",depth,ii);
      bestMatch=-depth; //kind of dangerous probably should do another way
      //printf("Break\n");
      //break in string
      if(maxMismatch>0){
        for(jj=depth;jj>=0;jj--){
          //printf("jj:%d ",jj);
          //splicing
          if((pathPos[jj]-pathPos[jj-1]>1&&jj>0)||jj==depth){
            //jump to root and try to match the rest, respecting the position of this match
            tmp=findStringInTree(tree,&queryCopy[jj],tree,pathPos[jj],maxMismatch-1);
            //printf("Splice %d %s<----\n",tmp,&queryCopy[jj]);
            if(tmp>0){
              bestMatch=1;
              break; //depth loop
            }
            tmp=tmp-jj;
            if(tmp<bestMatch)bestMatch=tmp;
          }
          //mismatch
          tmpChar=queryCopy[jj];
          for(k=0;k<4;k++){
            base=convertIndexToChar(k);
            if(tmpChar==base)continue;
            //substitute char in string here
            queryCopy[jj]=base;
            //printf("%s",&query[jj]);
            tmp=findStringInTree(tree,&queryCopy[jj],path[jj],pathPos[jj],maxMismatch-1); //find best match for remainder of the string with base substituted at the next position
            if(tmp>0){
              bestMatch=1;
              break; //ACTG loop
            }
            tmp=tmp-jj;
            //printf("Mismatch %c->%c %d  Best:%d",tmpChar,base,tmp,bestMatch);
            if(tmp<bestMatch)bestMatch=tmp;
          }
          queryCopy[jj]=tmpChar;
          //printf("\n");
          if(bestMatch>0)break;//depth loop
        }
      }
      break; //could do an insertion here
    }
  }
  free(path);
  free(pathPos);
  free(queryCopy);
  return(bestMatch);
}


int writeSeqToFastq(gzFile *out, char **buffers){
  int ii;
  for(ii=0;ii<4;ii++){
    if(gzputs(*out,buffers[ii])==-1)return(0);
  }
  return(1);
}

int onlyACTG(char *read){
  unsigned int counter=0;
  while(read[counter]!='\0'&&read[counter]!='\n'){
    if(convertCharToIndex(read[counter])>3){
      //fprintf(stderr,"Bad: %c -> %u\n",read[counter],convertCharToIndex(read[counter]));
      return(0);
    }
    counter++;
  }
  return(1);
}

int revString(const char *str,char *str2){
  size_t n=strlen(str);
  int ii;
  //char *buffer=(char *)malloc(sizeof(char)*(n+1));
  if(str[n-1]=='\n')n--;
  str2[n]='\0';
  for(ii=n-1;ii>=0;ii--){
    str2[n-1-ii]=str[ii];
  }
  return(0);
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
  return(0);
}

int switchBuffers(char **buffer1, char **buffer2){
  char *tmp; //for switching buffers
  int ii;
  for(ii=0;ii<4;ii++){
    tmp=buffer1[ii];
    buffer1[ii]=buffer2[ii];
    buffer2[ii]=tmp;
  }
  return(0);
}


void *findStringInTreePar(void *fsitArgs){
  struct fsitArgs *args=(struct fsitArgs *)fsitArgs;
  *args->out=findStringInTree(args->tree,args->query,args->currentNode,args->pos,args->maxMismatch);
  return(fsitArgs);
}

void findReadsInFastq(char* ref, char *fileName, int *parameters,char **outNames){
  int ii,jj; //iterators
  int index; //for filling in answers appropriately
  int maxMismatch=parameters[0]; //at most x mismatch and splices
  int minPartial=parameters[1]; //require x length to call partial match, if left and right partial then call a match 
  int sumMatch=parameters[2]; //if left length + right length > x, call a match
  struct node *tree[2]; //big suffix tree for aligning
  gzFile in,outMatch,outPartial; //file pointer for fastqs
  //line buffers
  char* buffers[4];
  for(ii=0;ii<4;ii++)buffers[ii]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
  char* buffers2[4];
  for(ii=0;ii<4;ii++)buffers2[ii]=(char *)malloc(sizeof(char)*MAXLINELENGTH);
  //seq conversions
  char* seqs[4];
  for(ii=0;ii<4;ii++)seqs[ii]=(char *)malloc(sizeof(char)*MAXLINELENGTH);

  char tmpStr[1000];//to append to things

  long int counter=0, matchCounter=0, partialCounter=0;
  struct fsitArgs *args[8];
  for(ii=0;ii<8;ii++)args[ii]=(struct fsitArgs *)malloc(sizeof(struct fsitArgs));
  int ans[8];//answer from findStringInTree
  int isMatch,isPartial;
  int fastqCheck;//keep track of whether our fastq is empty

  //threads
  pthread_t threads[8];

  fprintf(stderr,"Building tree\n");
  //fprintf(stderr,"Seq ..%s..\n",ref);

  tree[0]=buildTree(ref);
  fprintf(stderr,"Building reverse tree\n");
  revString(ref,seqs[0]);
  tree[1]=buildTree(seqs[0]);

  fprintf(stderr,"%u node trees ready\n",countNodes(tree[0]));

  fprintf(stderr,"Opening fastq file %s\n",fileName);
  //I think this should work with uncompressed files too
  in=gzopen(fileName,"rt");
  fprintf(stderr,"Opening outFile %s\n",outNames[0]);
  outMatch=gzopen(outNames[0],"w");
  fprintf(stderr,"Opening outFile %s\n",outNames[1]);
  outPartial=gzopen(outNames[1],"w");

  fprintf(stderr,"Scanning file (max mismatch %d, min partial %d, sum match %d)\n",maxMismatch,minPartial,sumMatch);
  fastqCheck=getSeqFromFastq(&in,buffers);
  while(fastqCheck){
    counter++;
    if(counter%10000==0)fprintf(stderr,"Working on read %ld\n",counter);
    if(!onlyACTG(buffers[1])){
      //write somewhere else?
      //printf("Bad read %ld: %s\n",counter,buffers[1]);
      fastqCheck=getSeqFromFastq(&in,buffers);
      continue;
    }
    strcpy(seqs[0],buffers[1]);
    revString(seqs[0],seqs[1]);
    complementString(seqs[0],seqs[2]);
    revString(seqs[2],seqs[3]);
    for(ii=0;ii<2;ii++){
      for(jj=0;jj<4;jj++){
        if(ii==0)index=jj;
        else index=4+(jj/2*2)+1-(jj%2); //using integer division to floor. and switching 4-5 and 6-7 to make later comparisons easy 0&4,1&5,...
        args[ii*4+jj]->tree=tree[ii];
        args[ii*4+jj]->query=seqs[jj];
        args[ii*4+jj]->currentNode=tree[ii];
        args[ii*4+jj]->pos=-1;
        args[ii*4+jj]->maxMismatch=maxMismatch;
        args[ii*4+jj]->out=&ans[index];
        args[ii*4+jj]->id=ii*4+jj;
        if(pthread_create(&threads[ii*4+jj],NULL,findStringInTreePar,args[ii*4+jj])){errorMessage("Couldn't create thread");exit(-98);}
        //ans[index]=findStringInTree(tree[ii],seqs[jj],tree[ii],-1,maxMismatch);
      }
    }

    //read from disk while we're waiting
    fastqCheck=getSeqFromFastq(&in,buffers2);
    //printf("%p",fastqCheck);
    //stop waiting here? and throw it all into some sort of giant queue?
    for(ii=0;ii<2;ii++){
      for(jj=0;jj<4;jj++){
        if(pthread_join(threads[ii*4+jj],NULL)){errorMessage("Couldn't join thread");exit(-97);}
      }
    }
    isMatch=0;
    isPartial=0;
    for(ii=0;ii<4;ii++){
      if(ans[ii]>0||ans[ii+4]>0||(ans[ii]<-minPartial&&ans[ii+4]<-minPartial)||(ans[ii]+ans[ii+4])<-sumMatch)isMatch=1; //found a match, or both side have partial, or sum of sides enough
      if(ans[ii]<-minPartial||ans[ii+4]<-minPartial)isPartial=1;
      sprintf(tmpStr,":%d|%d",-ans[ii],-ans[ii+4]);
      strCat(buffers[0],tmpStr);
    }
    strCat(buffers[0],"\n");

    if(isMatch){ //also ans+ans2>X
      writeSeqToFastq(&outMatch,buffers);
      matchCounter++;
    }else if(isPartial){
      writeSeqToFastq(&outPartial,buffers);
      partialCounter++;
    }
    switchBuffers(buffers,buffers2);
    //ignore bad matches for now
  }
  fprintf(stderr,"All done. Found %ld matches, %ld partials from %ld reads\n",matchCounter,partialCounter,counter);
  for(ii=0;ii<4;ii++)free(buffers[ii]);
  for(ii=0;ii<4;ii++)free(buffers2[ii]);
  for(ii=0;ii<4;ii++)free(seqs[ii]);
  for(ii=0;ii<8;ii++)free(args[ii]);
  fprintf(stderr,"Closing file %s\n",fileName);
  gzclose(in);
  fprintf(stderr,"Closing outFiles\n");
  gzclose(outMatch);
  gzclose(outPartial);
  fprintf(stderr,"Destroying trees\n");
  for(ii=0;ii<2;ii++) destroyTree(tree[ii]);
  return;
}

