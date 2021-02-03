#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

typedef struct BufHdr{
  size_t len; 
  size_t cap;
  char buf[0]; //actual buffer
}BufHdr;


//takes buf and gives BufHdr
#define buf__hdr(b)((BufHdr *)((char *)b- offsetof(BufHdr, buf))) 

//increases the sizeof buffer by n
#define buf__fit(b,n)((buf_len(b)+(n) <= buf_cap(b))? 0:((b)=(buf__grow((b), buf_len(b) +(n), sizeof(*b)))))

//returns the length and cap of buffer
#define buf_len(b)((b)?buf__hdr(b)->len:0)
#define buf_cap(b)((b)?buf__hdr(b)->cap:0)

//pushes x into the buffer and incresese the len by 1
#define buf_push(b, x) (buf__fit(b, 1),(b)[buf_len(b)]=(x), buf__hdr(b)->len++)

#define buf_free(b)((b)? free(buf__hdr(b)),(b)=NULL:0)

void *xmalloc(size_t _size){
  void *ptr = malloc(_size);
  if(!ptr){
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
}

void fatal(const char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  printf("FATAL: ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

void *xrealloc(void *ptr,size_t _size){
  ptr = realloc(ptr, _size);
  if(!ptr){
    perror("xremalloc failed");
    exit(1);
  }
  return ptr;
}

//takes in a buffer, length and the size of element in buffer
void *buf__grow(const void *b, size_t new_len, size_t elem_size){

  //new size of the buffer
  size_t new_size = offsetof(BufHdr, buf) + new_len* elem_size;
  BufHdr * new_hdr;
  if(b){
    //just reaccocate the buffer with new size
    new_hdr = xrealloc(buf__hdr(b), new_size);
  }
  else{
    //make new buffer with new size
    new_hdr = xmalloc(new_size);
    new_hdr->len = 0;
  }
  new_hdr -> cap = new_len;
  return new_hdr->buf;
}

void test_buf(){
  int *adsf = NULL;
  assert(buf_len(adsf) ==0 );
  enum{ N = 1024};
  for(int i = 0;i < N; i++){
    buf_push(adsf, i);
  }
  assert(buf_len(adsf) == N);
  for(int i = 0; i< buf_len(adsf); i++){
    assert(adsf[i]==i);
  }
  buf_free(adsf);
  assert(buf_len(adsf) == 0);
  assert(adsf == NULL);

}

typedef struct InternStr{
  size_t len;
  const char *str;
} InternStr;

static InternStr *interns;

const char *str_intern_range(const char *start, const char *end){
  size_t len = end - start;
  for(size_t i=0;i< buf_len(interns); i++){
    if(interns[i].len == len && strncmp(interns[i].str, start, len) == 0){
      return interns[i].str;
    }
  }
  char *str = xmalloc(len+1);
  memcpy(str, start, len);
  str[len] = 0;
  buf_push(interns, ((InternStr){len, str}));
  return str;
}


const char *str_intern(const char *str){
  return str_intern_range(str, str + strlen(str));
}
// lexing: translating char stream to token stream

void str_intern_test(){
  char x[]= "hello";
  char y[]= "hello";
  assert(x != y);
  assert(str_intern(x) == str_intern(y));
}

typedef enum TokenKind{
  TOKEN_INT = 128,
  TOKEN_NAME ,
} TokenKind;

const char *token_kind_name(TokenKind kind){
  char buf[256];
  switch(kind){
    case TOKEN_INT:
      strcpy(buf, "integer");
      break;
    case TOKEN_NAME:
      strcpy(buf, "name");
      break;
    default:
      if(kind < 128 && isprint(kind)){
        sprintf(buf, "%c",kind);
      }else{
        sprintf(buf, "<ASCII %d>",kind);
      }
  }
}

typedef struct Token{
  TokenKind kind;
  const char *start;
  const char *end;
  union{
    //val for int start,end for string
    uint64_t val;
    const char *name;
  };
} Token;

const char *keyword_if;
const char *keyword_for;
const char *keyword_while;

void init_keywords(){
  keyword_if = str_intern("if");
  keyword_for = str_intern("for");
  keyword_while = str_intern("while");
}

Token token;
const char *stream;

void next_token(){
  token.start = stream;
  switch(*stream){
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':{
               uint64_t val = 0;
               while(isdigit(*stream)){
                 val *= 10; //shifts the digit
                 val += *stream++ -'0'; //converts asci character into int
               }
               token.kind = TOKEN_INT;
               token.val = val;
               break;
             }
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '_':{
               const char *start = stream++;
               while(isalnum(*stream) || *stream == '_'){
                 stream ++;
               }
               token.kind = TOKEN_NAME;
               token.name = str_intern_range(token.start, stream);
               break;
             }
    default:
             token.kind = *stream++;
             break;
  }
  token.end = stream;
}

void print_token(Token token){
  switch(token.kind){
    case TOKEN_INT:
      printf("Token int: %ld\n",token.val);
      break;
    case TOKEN_NAME:
      printf("TOken Name: %*s\n",(int)(token.end- token.start), token.start);
      break;
    default:
      printf("Token '%c'\n",token.kind);
      break;
  }
}
bool is_token(TokenKind Kind){
  return token.kind == Kind;
}

inline bool is_token_name(const char *name){
  return token.kind == TOKEN_NAME && token.name == name;
}

bool match_token(TokenKind kind){
  if(is_token(kind)){
    next_token();
    return true;
  }
  else{
    return false;
  }
}
bool expect_token(TokenKind kind){
  if(is_token(kind)){
    next_token();
    return true;
  }
  else{
    fatal("expected token: %s, got %s",token_kind_name(token.kind), token_kind_name(token.kind));
    return false;
  }
}

/*
 * expr3 = INT | '(' expr ')'
 * expr2 = [-]expr3;
 * expr1 = expr ([/*]expr1)
 * expr0 = expr ([+-]expr1)
 * expr = expr0
 */
void parse_expr();
void parse_expr3(){
  if (is_token(TOKEN_INT)){
    next_token();
  } else if(match_token('(')){
    parse_expr();
    expect_token(')');
  }else{
    fatal("expected integer or (, got %s", token_kind_name(token.kind));
  }
}

void parse_expr2(){
  if(match_token('-')){
    parse_expr3();
  } else{
    parse_expr3();
  }
}


void parse_expr1(){
  parse_expr2();
  while(is_token('*') || is_token('/')){
    char op = token.kind;
    next_token();
    parse_expr2();
  }
}


void parse_expr0(){
  parse_expr1();
  while(is_token('+') || is_token('-')){
    char op = token.kind;
    next_token();
    parse_expr1();
  }
}
void parse_expr(){
  parse_expr0();
}

void init_stream(const char *str){
  stream = str;
  next_token();
}

void parse_text(){
  const char * expr = "1";
  init_stream(expr);
  parse_expr();
}

void lex_test(){
  char *source = "XY+(XY)24fheo,5425+5245samrat";
  stream = source;
  next_token();
  while(token.kind){
    print_token(token);
    next_token();
  }
}

int main(){
  parse_text();
  return 0;
}
