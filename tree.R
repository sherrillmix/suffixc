treeSO<-'tree.so'
loader<-try(dyn.load(treeSO),TRUE)
if(any(grep("Error",loader))) stop(simpleError('Error loading tree c functions'))                                                                                                                                                     

tree<-function(ref,queries){
	if(any(grepl('[^ACTG]',c(ref,queries))))stop(simpleError('Only ACTG please'))
	.C('treeAlign',as.integer(rep(-99,length(queries))),as.character(ref),as.character(queries),as.integer(length(queries)))[[1]]
}

