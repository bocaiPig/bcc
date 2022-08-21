#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
  // exception handing
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
    // if return value not equal to 0, error. 
    return 1;
  }
  // Expression which we input is stored in P
  char *P = argv[1];
  // global main which is the entrance of program.
  printf("  .globl main\n");
  printf("main:\n");
  // We devide expression into num (op num) (op num) style.
  // so we load the first num in a0
  printf("  li a0, %ld\n", strtol(P,&P,10));
  // start to analysis (op num)

  while (*P) {
    // + 
    if (*P == '+') {
      ++P;
      printf("  addi a0, a0, %ld\n", strtol(P, &P, 10));
      continue;
    }
    // -
    if(*P == '-') {
      ++P;
      printf("  addi a0, a0, -%ld\n", strtol(P, &P, 10));
      continue;
    }
    fprintf(stderr, "unexpected character: '%c'", *P);
    return 1;
  }
  printf("  ret\n");

  return 0;
}
