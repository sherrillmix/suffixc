
#include <stdio.h>
#include "tree.h"

//http://www.jera.com/techinfo/jtns/jtn002.html
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)
int tests_run;

static char * test_fileFuncs(){
  char tmp[]="/tmp/XXXXXX";
  int temp_fd;
  FILE *uid_file;
  char buffer[MAXLINELENGTH];
  char buffer4[4][MAXLINELENGTH];
  gzFile gzTmp;
  temp_fd=mkstemp(tmp);
  uid_file = fdopen(temp_fd,"w");
  fprintf(uid_file,">Test\nACACATTT\nA\nT\n\nT");
  fclose(uid_file);
  getRefFromFasta(tmp,buffer);
  mu_assert("Error. getRefFromFasta not correct",strcmp(buffer,"ACACATTTAT"));
  
  strcpy(tmp,"/tmp/XXXXXX");
  temp_fd=mkstemp(tmp);
  uid_file = fdopen(temp_fd,"w");
  fprintf(uid_file,"@Test\nACACATTT\n+\nAAAAAAAA\n@Test2\nGCGCATTTGGG\n+\nAAAAAAAAAAA");
  fclose(uid_file);
  gzTmp=gzopen(tmp,"r");
  getSeqFromFastq(&gzTmp,(char**)buffer4);
  mu_assert("Error. getSeqFromFastq not correct",strcmp(buffer4[1],"ACACATTT"));
  mu_assert("Error. Second getSeqFromFastq not correct",strcmp(buffer4[1],"GCGCATTTGGG"));
  mu_assert("Error. Second getSeqFromFastq not correct",strcmp(buffer4[0],"Test2"));
  gzclose(gzTmp);

  gzTmp=gzopen(tmp,"w");
  gzputs(gzTmp,"@Test\nACACATTT\n+\nAAAAAAAA\n@Test2\nGCGCATTTGGG\n+\nAAAAAAAAAAA");
  gzclose(gzTmp);
  gzTmp=gzopen(tmp,"r");
  getSeqFromFastq(&gzTmp,(char**)buffer4);
  mu_assert("Error. getSeqFromFastq from gz not correct",strcmp(buffer4[1],"ACACATTT"));
  mu_assert("Error. Second getSeqFromFastq from gz not correct",strcmp(buffer4[1],"GCGCATTTGGG"));
  mu_assert("Error. Second getSeqFromFastq from gz not correct",strcmp(buffer4[0],"Test2"));



  //char template_name[]="/tmp/cmguiXXXXXX";
  //int temp_fd;

  //temp_fd=mkstemp(template_name);
  //uid_file = fdopen(temp_fd,"w");

    //if(gzputs(*out,buffers[ii])==-1)return(0);
  //char fileName[L_tmpnam];
  //tmpnam(fileName);
  //printf("%s",fileName);
  return(0);
}

static char * test_simpleCharFuncs(){
  char string1[1000];
  mu_assert("Error. covertCharToIndex roundtrip not correct",convertIndexToChar(convertCharToIndex('A'))=='A');
  mu_assert("Error. covertCharToIndex roundtrip not correct",convertIndexToChar(convertCharToIndex('C'))=='C');
  mu_assert("Error. covertCharToIndex roundtrip not correct",convertIndexToChar(convertCharToIndex('G'))=='G');
  mu_assert("Error. covertCharToIndex roundtrip not correct",convertIndexToChar(convertCharToIndex('T'))=='T');
  mu_assert("Error. complementBase not correct",complementBase('A')=='T');
  mu_assert("Error. complementBase not correct",complementBase('C')=='G');
  mu_assert("Error. complementBase not correct",complementBase('G')=='C');
  mu_assert("Error. complementBase not correct",complementBase('T')=='A');
  mu_assert("Error. complementBase roundtrip not correct",complementBase(complementBase('A'))=='A');
  mu_assert("Error. complementBase roundtrip not correct",complementBase(complementBase('C'))=='C');
  mu_assert("Error. complementBase roundtrip not correct",complementBase(complementBase('G'))=='G');
  mu_assert("Error. complementBase roundtrip not correct",complementBase(complementBase('T'))=='T');
  revString("",string1);
  mu_assert("Error. reverseString not correct",strcmp("",string1)==0);
  revString("Z",string1);
  mu_assert("Error. reverseString not correct",strcmp("Z",string1)==0);
  revString("1234567890ABCabc--",string1);
  mu_assert("Error. reverseString not correct",strcmp("--cbaCBA0987654321",string1)==0);
  mu_assert("Error. onlyACTG not correct",onlyACTG("A"));
  mu_assert("Error. onlyACTG not correct",onlyACTG(""));
  mu_assert("Error. onlyACTG not correct",onlyACTG("ACTG"));
  mu_assert("Error. onlyACTG not correct",onlyACTG("Z")==0);
  mu_assert("Error. onlyACTG not correct",onlyACTG("ACTGZ")==0);
  mu_assert("Error. onlyACTG not correct",onlyACTG("ACTGAAAAAAAAAAAAAAAAA-A")==0);
  return(0);
}


static char * test_helpers(){
  char **buffer1, **buffer2;
  buffer1=(char **)malloc(sizeof(char*)*4);
  buffer2=(char **)malloc(sizeof(char*)*4);
  int ii;
  for(ii=0;ii<4;ii++){
    buffer1[ii]=(char *)malloc(sizeof(char)*200);
    buffer2[ii]=(char *)malloc(sizeof(char)*200);
    sprintf(buffer1[ii],"AAAA%d",ii);
    sprintf(buffer2[ii],"BBB%d",ii);
  }
  switchBuffers((char **) buffer1,(char**) buffer2);
  //printf("%s\n",buffer1[1]);
  mu_assert("Error. Buffers not switched",strcmp(buffer1[1],"BBB1")==0 && strcmp(buffer2[3],"AAAA3")==0);
  //printf("%s\n",buffer1[2]);
  //printf("%s\n",buffer2[0]);
  strCat(buffer1[2],buffer2[0]);
  //printf("%s\n",buffer1[2]);
  mu_assert("Error. Strings not concatenated",strcmp(buffer1[2],"BBB2AAAA0")==0);
  for(ii=0;ii<4;ii++){free(buffer1[ii]);free(buffer2[ii]);}
  free(buffer1);
  free(buffer2);
  return(0);
}

static char * test_tree(){
  struct node *tree;
  struct node *tree2;
 // char query[100];
  //sprintf(query,"%s","ACTG");
  tree=buildTree("AAAACTGGGGG");
  tree2=buildTree("AAAA");
  mu_assert("Error. Tree count not correct",countNodes(tree)>0);
  mu_assert("Error. Tree count not correct",countNodes(tree2)==5);
  mu_assert("Error. String not found",findStringInTree(tree,"ACTG",tree,-1,0)>0);
  mu_assert("Error. False string found",findStringInTree(tree,"TTTTTT",tree,-1,0)<1);
  mu_assert("Error. False string found",findStringInTree(tree,"ACTT",tree,-1,0)<1);
  mu_assert("Error. Close string not found",findStringInTree(tree,"ACTT",tree,-1,1)>0);
  mu_assert("Error. False string found",findStringInTree(tree,"ATTT",tree,-1,1)<1);
  mu_assert("Error. Close string not found",findStringInTree(tree,"ATTT",tree,-1,2)>0);
  mu_assert("Error. Tree not destroyed",destroyTree(tree)==1);
  mu_assert("Error. Tree not destroyed",destroyTree(tree2)==1);
  return(0);
}

static char * all_tests() {
  mu_run_test(test_simpleCharFuncs);
  mu_run_test(test_helpers);
  mu_run_test(test_tree);
  mu_run_test(test_fileFuncs);
  return 0;
}

int main(int argc, char **argv) {
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;
}
