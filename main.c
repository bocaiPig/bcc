#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

// set enum type for every terminal symbol
typedef enum {
  TK_PUNCT, // operator such as '+' '-'
  TK_NUM,   // number
  TK_EOF,   // To indicate end of file
} TokenKind;

// Token of terminal symbol
typedef struct Token Token;
struct Token {
  TokenKind Kind; // TokenKind enum
  Token *Next;    // Indicate the next Token
  int val;
  char *loc;      // location in string
  int len;        // length of token
};

// Print error information
// fmt is input string, '...' is changeable arguments
static void error(char *fmt, ...) {
  va_list VA;
  va_start(VA, fmt);
  vfprintf(stderr, fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
  exit(1);
}

// Judge whether token is equal to input str.
// the function of memcmp is compare two str.
// = return 0, < return <0, > return >0
static bool equal(Token *Tok, char *Str) {
  return memcmp(Tok->loc, Str, Tok->len) == 0 && Str[Tok->len] == '\0';
}

// skip specific String.
static Token *skip(Token *Tok, char *Str) {
  if(!equal(Tok, Str))
    error("expect '%s'", Str);
  return Tok->Next;
}

// return value of TK_NUM
static int getNumber(Token *Tok) {
  if(Tok->Kind != TK_NUM)
    error("expect a number");
  return Tok->val;
}

// generate a new token
static Token *newToken(TokenKind Kind, char *Start, char *End) {
  Token *Tok = calloc(1, sizeof(Token));
  Tok->Kind = Kind;
  Tok->loc = Start;
  Tok->len = End - Start;
  return Tok;
}

// analysis token
static Token *tokenize(char *P) {
  Token Head = {};
  Token *Cur = &Head;

  while(*P) {
    // skip every space char such as enter, space
    if(isspace(*P)) {
      ++P;
      continue;
    }

    // analysis digit
    if(isdigit(*P)) {
      Cur->Next = newToken(TK_NUM, P, P);
      Cur = Cur->Next;
      const char *OldPtr = P;
      Cur->val = strtoul(P, &P, 10);
      Cur->len = P - OldPtr;
      continue;
    }

    // analysis operator
    if(*P == '+' || *P == '-') {
      Cur->Next = newToken(TK_PUNCT, P, P + 1);
      Cur = Cur->Next;
      ++P;
      continue;
    }

    // handing errors
    error("invalid token: %c", *P);
  }
  // end of analysis, we add a TK_EOF to present end
  Cur->Next = newToken(TK_EOF, P, P);
  return Head.Next;
}


int main(int argc, char ** argv) {
  // exception handing
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
    // if return value not equal to 0, error. 
    return 1;
  }
  Token *Tok = tokenize(argv[1]);
  printf("  .globl main\n");
  printf("main:\n");
  printf("  li a0, %d\n", getNumber(Tok));
  Tok = Tok->Next;

  while(Tok->Kind != TK_EOF) {
    if(equal(Tok, "+")) {
      Tok = Tok->Next;
      printf("  addi a0, a0, %d\n", getNumber(Tok));
      Tok = Tok->Next;
      continue;
    }

    Tok = skip(Tok, "-");
    printf("  addi a0, a0, -%d\n", getNumber(Tok));
    Tok = Tok->Next;
  }
  
  printf("  ret\n");
  return 0;
}
