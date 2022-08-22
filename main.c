#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
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
  int Val;
  char *Loc;      // location in string
  int Len;        // Length of token
};


// string which we input
static char *CurrentInput;

// Print error information
// Fmt is input string, '...' is changeable arguments
static void error(char *Fmt, ...) {
  va_list VA;
  va_start(VA, Fmt);
  vfprintf(stderr, Fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
  exit(1);
}

// print error loction, exit at the same time
static void verrorAt(char *Loc, char *Fmt, va_list VA) {
  // print original information
  fprintf(stderr, "%s\n", CurrentInput);
  // print error information
  int Pos = Loc - CurrentInput;
  // make up 'Pos' spaces
  fprintf(stderr, "%*s", Pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, Fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
  exit(1);
}

// char error
static void errorAt(char *Loc, char *Fmt, ...) {
  va_list VA;
  va_start(VA,Fmt);
  verrorAt(Loc, Fmt, VA);
}

// token error
static void errorTok(Token *Tok, char *Fmt, ...) {
  va_list VA;
  va_start(VA, Fmt);
  verrorAt(Tok->Loc, Fmt, VA);
}


// Judge whether token is equal to input str.
// the function of memcmp is compare two str.
// = return 0, < return <0, > return >0
static bool equal(Token *Tok, char *Str) {
  return memcmp(Tok->Loc, Str, Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

// skip specific String.
static Token *skip(Token *Tok, char *Str) {
  if(!equal(Tok, Str))
    errorTok(Tok, "expect '%s'", Str);
  return Tok->Next;
}

// return Value of TK_NUM
static int getNumber(Token *Tok) {
  if(Tok->Kind != TK_NUM)
    errorTok(Tok, "expect a number");
  return Tok->Val;
}

// generate a new token
static Token *newToken(TokenKind Kind, char *Start, char *End) {
  Token *Tok = calloc(1, sizeof(Token));
  Tok->Kind = Kind;
  Tok->Loc = Start;
  Tok->Len = End - Start;
  return Tok;
}

// analysis token
static Token *tokenize() {
  char *P = CurrentInput;
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
      Cur->Val = strtoul(P, &P, 10);
      Cur->Len = P - OldPtr;
      continue;
    }

    // analysis operator
    if(ispunct(*P)) {
      Cur->Next = newToken(TK_PUNCT, P, P + 1);
      Cur = Cur->Next;
      ++P;
      continue;
    }

    // handing errors
    //error("invalid token: %c", *P);
    errorAt(P, "invalid token");
  }
  // end of analysis, we add a TK_EOF to present end
  Cur->Next = newToken(TK_EOF, P, P);
  return Head.Next;
}

/*******************
// generate Abstract Syntax Tree
*******************/
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // number
} NodeKind;

// AST binary tree
typedef struct Node Node;
struct Node {
  NodeKind Kind; // kind of node
  Node *LHS;     // left-hand side
  Node *RHS;     // right-hand side
  int Val;       // store ND_NUM's value
};

// start a new node
struct Node *newNode(NodeKind Kind) {
  Node *Nd = calloc(1, sizeof(Node));
  Nd->Kind = Kind;
  return Nd;
}

// start a new binary tree node
static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS) {
  Node *Nd = newNode(Kind);
  Nd->LHS = LHS;
  Nd->RHS = RHS;
  return Nd;
}

// start a new number node
static Node *newNum(int Val) {
  Node *Nd = newNode(ND_NUM);
  Nd->Val = Val;
  return Nd;
}

// expr = mul("+" mul | "-" mul)*
// mul = primary ("*" primary | "/" primary)*
// primary = "(" expr ")" | num
static Node *expr(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);

// analysis + & -
// expr = mul("+" mul | "-" mul)
static Node *expr(Token **Rest, Token *Tok) {
  // mul
  Node *Nd = mul(&Tok, Tok);

  while(true) {
    if(equal(Tok, "+")) {
      Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next));
      continue;
    }
    if(equal(Tok, "-")) {
      Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// analysis * & /
static Node *mul(Token **Rest, Token *Tok) {
  // primary
  Node *Nd = primary(&Tok, Tok);

  while(true) {
    if(equal(Tok, "*")) {
      Nd = newBinary(ND_MUL, Nd, primary(&Tok, Tok->Next));
      continue;
    }
    if(equal(Tok, "/")){
      Nd = newBinary(ND_DIV, Nd, primary(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *primary(Token **Rest, Token *Tok) {
  if(equal(Tok, "(")) {
    Node *Nd = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ")");
    return Nd;
  }
  if(Tok->Kind == TK_NUM) {
    Node *Nd = newNum(Tok->Val);
    *Rest = Tok->Next;
    return Nd;
  }

  errorTok(Tok, "expected an expression");
  return NULL;
}

/*****************
 * 1. semantics analysis
 * 2. code generate
 * **************/

// Record stack depth
static int Depth;

// push stack
// sp is stack point
static void push(void) {
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

// pop stack
static void pop(char *Reg) {
  printf("  ld %s, 0(sp)\n", Reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

// generate expression
static void genExpr(Node *Nd) {
  if(Nd->Kind == ND_NUM) {
    printf("  li a0, %d\n", Nd->Val);
    return;
  }

  genExpr(Nd->RHS);
  push();
  genExpr(Nd->LHS);
  pop("a1");

  switch(Nd->Kind) {
  case ND_ADD:
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB:
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL:
    printf(" mul a0, a0, a1\n");
    return;
  case ND_DIV:
    printf(" div a0, a0, a1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}



int main(int argc, char ** argv) {
  // exception handing
  if (argc != 2) {
    //fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
    // if return value not equal to 0, error. 
    //return 1;
    error("%s: invalid number of arguments.\n",argv[0]);
  }
  CurrentInput = argv[1];
  Token *Tok = tokenize();
  
  Node *Node = expr(&Tok, Tok);
  
  if(Tok->Kind != TK_EOF)
    errorTok(Tok, "extra token");

  printf("  .globl main\n");
  printf("main:\n");
  
  genExpr(Node);

  printf("  ret\n");
  
  // if stack is not null, error.
  assert(Depth == 0);
  
  return 0;
}
