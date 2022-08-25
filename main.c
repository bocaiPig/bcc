#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/******************
* lexical analysis
* terminal sympol analysis
******************/

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

static bool startsWith(char *Str, char *SubStr) {
  // compare LHS and RHS, the N th char whether equal
  return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

// read punct
static int readPunct(char *Ptr) {
  // judge 2 bit operator
  if(startsWith(Ptr, "==") || startsWith(Ptr, "!=") || startsWith(Ptr, "<=") ||startsWith(Ptr, ">="))
    return 2;
  // judge 1 bit operator
  return ispunct(*Ptr) ? 1 : 0;
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
    int PunctLen = readPunct(P);
    if(PunctLen) {
      Cur->Next = newToken(TK_PUNCT, P, P + PunctLen);
      Cur = Cur->Next;
      P = P + PunctLen;
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
  ND_NEG, // minus sign
  ND_NUM, // number
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
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

// statt a new signal tree node
static Node *newUnary(NodeKind Kind, Node *Expr) {
  Node *Nd = newNode(Kind);
  Nd->LHS = Expr;
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

// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("+" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = ("(" expr ")" | num
static Node *expr(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
static Node *unary(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);

// analysis expression
// expr = equality
static Node *expr(Token **Rest, Token *Tok) {
  return equality(Rest, Tok);
}

// analysis relational
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **Rest, Token *Tok) {
  // relational
  Node *Nd = relational(&Tok, Tok);

  // ("==" relational | "!=" relational)*
  while(true) {
    // "==" relational
    if (equal(Tok, "==")) {
      Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next));
      continue;
    }

    // "!=" relational
    if(equal(Tok, "!=")) {
      Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// analysis compare relationship
// ralational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **Rest, Token *Tok){
  // add
  Node *Nd = add(&Tok, Tok);

  while(true) {
    // "<"
    if(equal(Tok, "<")) {
      Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next));
      continue;
    }
    if(equal(Tok, "<=")) {
      Nd= newBinary(ND_LE, Nd, add(&Tok, Tok->Next));
      continue;
    }
    if(equal(Tok, ">")) {
      Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd);
      continue;
    }
    if(equal(Tok, ">=")) {
      Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd);
      continue;
    }
    *Rest = Tok;
    return Nd;
  }
}

// analysis add & subtract
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **Rest, Token *Tok) {
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

// analysis mul & div
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **Rest, Token *Tok) {
  Node *Nd = unary(&Tok, Tok);

  while(true) {
    if(equal(Tok, "*")) {
      Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next));
      continue;
    }
    if(equal(Tok, "/")) {
      Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

// analysis unary
// unary = ("+" | "-") unary | primary
static Node *unary(Token **Rest, Token *Tok) {
  if(equal(Tok, "+")){
    return unary(Rest, Tok->Next);
  }
  if(equal(Tok, "-")) {
    return newUnary(ND_NEG, unary(Rest, Tok->Next));
  }

  return primary(Rest, Tok);
}

// analysis brackets & num
// primary = "(" expr ")" | num
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

  errorTok(Tok, "expected an expression!");
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
  // generate root node
  switch(Nd->Kind) {
    case ND_NUM:
      printf("  li a0, %d\n", Nd->Val);
      return;
    // minus
    case ND_NEG:
      genExpr(Nd->LHS);
      printf("  neg a0, a0\n");\
      return;
    default:
      break;
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
  case ND_EQ:
  case ND_NE:
    printf(" xor a0, a0, a1\n");

    if(Nd->Kind == ND_EQ)
      printf(" seqz a0, a0\n");
    else
      printf(" snez a0, a0\n");
    return;
  case ND_LT:
    printf(" slt a0, a0, a1\n");
    return;
  case ND_LE:
    printf(" slt a0, a1, a0\n");
    printf(" xori a0, a0, 1\n");
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
