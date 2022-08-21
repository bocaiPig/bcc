#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
  // exception handing
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
    // if return value not equal to 0, error. 
    return 1;
  }
  // global main which is the entrance of program.
  // a simple example instrction of riscv, li a0, a1.
  printf("  .globl main\n");
  printf("main:\n");
  printf("  li a0, %d\n", atoi(argv[1]));
  printf("  ret\n");
  return 0;
}
