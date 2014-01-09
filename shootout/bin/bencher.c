
#include <stdio.h>
#include <stdlib.h>

#define TESTS 5 
//7
char *tests[TESTS] = {
   "binary-trees 20" ,
//   "chameneos-redux 2000000", 
   "fannkuch 12", 
   "mandelbrot 16000",
   "regex-dna regexdna-input.txt",
   "spectral-norm 10000" //,
//   "thread-ring 50000000"
};

int main(int argc, char *argv[] ){
  system("cat bin/header.txt");
  int i;
  char buff[512];
  for(i=0 ; i < TESTS ; i++) {
     sprintf(buff,"bin/load %s", tests[i]);
     system(buff);
  }
  system("cat bin/footer.txt");
  return 0;
}
