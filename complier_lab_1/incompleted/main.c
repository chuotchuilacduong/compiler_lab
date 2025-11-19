/*
  * To complile, use the command:
  * gcc -I. -o main scanner.c reader.c charcode.c token.c error.c
  * 
  * To complile and run an example with one command:
  * gcc -I. -o main main.c reader.c charcode.c token.c error.c && ./main ../test/example1.kpl
  * 
  * To complile a unittest file, use the command:
  * gcc -I. -I/opt/homebrew/include -L/opt/homebrew/lib -o main main.c reader.c charcode.c token.c error.c -lcmocka
  * 
  * Some explainations:
  * -I. : include current directory to look for header files
  * -I/opt/homebrew/include : include cmocka header files
  * -L/opt/homebrew/lib : link cmocka library
  * -lcmocka : link cmocka library
  */

/******************************************************************/
// #include "main.h"

#include <stdio.h>
#include <stdlib.h>

#include "scanner.c"

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("scanner: no input file.\n");
    return -1;
  }

  if (scan(argv[1]) == IO_ERROR) {
    printf("Can\'t read input file!\n");
    return -1;
  }
    
  return 0;
}