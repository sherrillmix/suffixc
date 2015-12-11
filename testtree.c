
#include <stdio.h>
#include "tree.h"

//http://www.jera.com/techinfo/jtns/jtn002.html
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)
int tests_run;


int foo=7;
int bar=7;

char string1[1000];
char string2[1000];

static char * test_simpleCharFuncs(){
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
  revString("",string2);
  mu_assert("Error. reverseString not correct",strcmp("",string2)==0);
  revString("Z",string2);
  mu_assert("Error. reverseString not correct",strcmp("Z",string2)==0);
  revString("1234567890ABCabc--",string2);
  mu_assert("Error. reverseString not correct",strcmp("--cbaCBA0987654321",string2)==0);
  return(0);
}

static char * test_bar(){
  mu_assert("Error. Bar != 7",bar==7);
  return(0);
}

static char * all_tests() {
  mu_run_test(test_simpleCharFuncs);
  mu_run_test(test_bar);
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
