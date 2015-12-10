#include "tree.h"
#include <ctype.h>

int main (int argc, char *argv[]){
  int nMismatch=0,nThread=2;
  int c;
  int ii;
  char usage[500];
  char refFile[MAXSTRINGIN+1];
  char fastqFile[MAXSTRINGIN+1];
  sprintf(usage,"Usage: %s ref.fa reads.fastq [-m 2] [-t 4]\n  first argument: a reference sequence in a fasta file (if this is much more than 10kb then we could get memory problems)\n  second argument: a fastq file containing the reads to search\n  -t: (optional) specify how many threads to use (default: 2)\n  -m: (optional) specify how many mismatches to tolerate (default: 00)\n  -h: (optional) display this message and exit\n",argv[0]);
  while ((c = getopt (argc, argv, "hm:t:")) != -1){
    switch (c){
      case 'm':
        nMismatch = atoi(optarg);
        break;
      case 'y':
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
  fprintf (stderr,"nMismatch: %d\nnThread: %d\nrefFile: %s\nfastqFile: %s\n", nMismatch, nThread,refFile,fastqFile);

  //void findReadsInFastq(char** ref, char **fileName, int *parameters,char **outNames){

  return 0;
}
