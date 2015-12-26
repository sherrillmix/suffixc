#include "tree.h"
#include <ctype.h>

int main (int argc, char *argv[]){
  int nMismatch=0,nThread=2;
  int c;
  int ii;
  char usage[500];
  char refFile[MAXSTRINGIN+1];
  char outPrefix[MAXSTRINGIN+1];
  strcpy(outPrefix,"suffixrOut");
  char fastqFile[MAXSTRINGIN+1];
  char ref[MAXLINELENGTH+1];
  char **outFiles;
  outFiles=(char**) malloc(sizeof(char*)*2);
  for(ii=0;ii<2;ii++)outFiles[ii]=(char*) malloc(sizeof(char)*MAXSTRINGIN+101);
  int params[3];
  sprintf(usage,"Usage: %s ref.fa reads.fastq [-m 2] [-t 4]\n  first argument: a reference sequence in a fasta file (if this is much more than 10kb then we could get memory problems)\n  second argument: a fastq file containing the reads to search\n  -o: (optional) the output prefix for outfiles (default: suffixrOut)\n  -t: (optional) specify how many threads to use (default: 2)\n  -m: (optional) specify how many mismatches to tolerate (default: 00)\n  -h: (optional) display this message and exit\n",argv[0]);
  while ((c = getopt (argc, argv, "hm:t:o:")) != -1){
    switch (c){
		case 'o':
			strcpy(outPrefix,optarg);
      case 'm':
        nMismatch = atoi(optarg);
        break;
      case 't':
        nThread = atoi(optarg);
        break;
      case 'h':
        fprintf(stderr,"%s",usage);
        return(3);
      case '?':
        if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return 1;
    }
  }

  if(optind!=argc-2){
    fprintf(stderr,"%s",usage);
    return(2);
  }

  for (ii = 0; ii < argc-optind; ii++){
    if(strlen(argv[optind+ii])>MAXSTRINGIN){
      fprintf(stderr,"Filenames must be less than %d characters long",MAXSTRINGIN);
      return(4);
    }
    switch(ii){
      case 0:
        strcpy(refFile,argv[optind+ii]);
        break;
      case 1:
        strcpy(fastqFile,argv[optind+ii]);
        break;
    }
  }
  fprintf (stderr,"nMismatch: %d\nnThread: %d\nrefFile: %s\nfastqFile: %s\noutPrefix: %s\n", nMismatch, nThread,refFile,fastqFile,outPrefix);
  getRefFromFasta(refFile,ref);
  params[0]=1;
  params[1]=15;
  params[2]=15;
  strCat(outFiles[0],"_match.fastq");
  strCat(outFiles[1],"_partial.fastq");

  fprintf (stderr,"outFile1: %s\n", outFiles[0]);
  fprintf (stderr,"outFile2: %s\n", outFiles[1]);


  findReadsInFastq(ref, fastqFile, params,outFiles);


  //void findReadsInFastq(char** ref, char **fileName, int *parameters,char **outNames){

  for(ii=0;ii<2;ii++)free(outFiles[ii]);
  free(outFiles);
  return 0;
}
