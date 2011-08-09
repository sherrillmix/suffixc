treeSO<-'tree.so'
loader<-try(dyn.load(treeSO),TRUE)
if(any(grep("Error",loader))) stop(simpleError('Error loading tree c functions'))                                                                                                                                                     

tree<-function(ref,queries,maxMismatch=1){
	if(any(grepl('[^ACTG]',c(ref,queries))))stop(simpleError('Only ACTG please'))
	.C('treeAlign',as.integer(rep(99,length(queries))),as.character(ref),as.character(queries),as.integer(length(queries)),as.integer(maxMismatch))[[1]]
}

anyChanceOfMatch<-function(ref,queries,nGapMismatch=0){
	if(any(grepl('[^ACTG]',c(ref,queries))))stop(simpleError('Only ACTG please'))
	.C('anyChanceOfMatch',as.integer(rep(-99,length(queries))),as.character(ref),as.character(queries),as.integer(length(queries)),as.integer(nGapMismatch))[[1]]
}

alignFastq<-function(ref,fastqName,outPrefix,maxMismatch=2,minPartial=30,partialSumMatch=90){
	suffixes<-c('.match.fastq.gz','.partial.fastq.gz')
	outNames<-sprintf("%s%s",outPrefix,suffixes)
	.C('findReadsInFastq',as.character(ref),as.character(fastqName),as.integer(c(maxMismatch,minPartial,partialSumMatch)),as.character(outNames))
	return(TRUE)
}

#source("~/scripts/R/dna.R");ref<-read.fa('../../ref/HIV1U39362.fna');x<-read.fa('test.fa');x<-x[!grepl('[^ACTG]',x$seq),];source('tree.R');system.time(z<-tree(ref$seq,x$seq))
#source("~/scripts/R/dna.R");ref<-read.fa('../../ref/HIV1U39362.fna');source('tree.R');system.time(tree(ref$seq,x$seq[100000]))
#source("~/scripts/R/dna.R");ref<-read.fa('../../ref/HIV1U39362.fna');source('tree.R');system.time(alignFastq(ref$seq,'tmp.fastq.gz','tmp.out'))
