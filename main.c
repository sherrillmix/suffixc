#include "tree.h"

int main (int argc, char *argv[]){
  int nMismatch=0, minLength=20;
  //int nThread=2,
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
  int params[2];
  //\n  -t: (optional) specify how many threads to use (default: 2)
  sprintf(usage,"Usage: %s ref.fa reads.fastq [-m 2] [-t 4]\n  first argument: a reference sequence in a fasta file (if this is much more than 10kb then we could get memory problems)\n  second argument: a fastq file containing the reads to search\n  -o: (optional) the output prefix for outfiles (default: suffixrOut)\n  -l: (optional) specify how many long a match has to be considered a partial match. A partial match on both ends counts as a match (default: 20)\n  -m: (optional) specify how many mismatches to tolerate (default: 0)\n  -h: (optional) display this message and exit\n",argv[0]);
  while ((c = getopt (argc, argv, "hm:t:o:l:")) != -1){
    switch (c){
      case 'l':
        minLength=atoi(optarg);
        break;
      case 'o':
        strcpy(outPrefix,optarg);
        break;
      case 'm':
        nMismatch = atoi(optarg);
        break;
      //case 't':
        //nThread = atoi(optarg);
        //break;
      case 'h':
        fprintf(stderr,"%s",usage);
        return(3);
      case '?':
        if (optopt<33)
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
  fprintf (stderr,"nMismatch: %d\nrefFile: %s\nfastqFile: %s\noutPrefix: %s\n", nMismatch,refFile,fastqFile,outPrefix);
  getRefFromFasta(refFile,ref);
  params[0]=nMismatch;
  params[1]=minLength;
  strcpy(outFiles[0],outPrefix);
  strcpy(outFiles[1],outPrefix);
  strCat(outFiles[0],"_match.fastq.gz");
  strCat(outFiles[1],"_partial.fastq.gz");

  fprintf (stderr,"outFile1: %s\n", outFiles[0]);
  fprintf (stderr,"outFile2: %s\n", outFiles[1]);


  findReadsInFastq(ref, fastqFile, params,outFiles);

  for(ii=0;ii<2;ii++)free(outFiles[ii]);
  free(outFiles);
  return 0;
}
