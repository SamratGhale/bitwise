#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>

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

#define buf_free(b)((b)? free(buf__hdr(b)):0)

//takes in a buffer, length and the size of element in buffer
void *buf__grow(const void *b, size_t new_len, size_t elem_size){

  //new size of the buffer
  size_t new_size = offsetof(BufHdr, buf) + new_size * elem_size;
  BufHdr * new_hdr;
  if(b){
    //just reaccocate the buffer with new size
    new_hdr = realloc(buf__hdr(b), new_size);
  }
  else{
    //make new buffer with new size
    new_hdr = malloc(new_size);
    new_hdr->len = 0;
  }
  new_hdr -> cap = new_len;
  return new_hdr->buf;
}

void test_buf(){
  int *b  = NULL;
  buf_push(b, 1);
  buf_push(b, 1);
  buf_push(b, 1);
  buf_push(b, 1);
}

// lexing: translating char stream to token stream

typedef enum TokenKind{
  TOKEN_INT = 128,
  TOKEN_NAME ,
} TokenKind;

typedef struct Token{
  TokenKind kind;
  union{
    //val for int start,end for string
    uint64_t val;
    struct{
      const char *start;
      const char *end;
    };
  };
} Token;

Token token;
const char *stream;

void next_token(){
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
               token.start = start;
               token.end = stream;
               break;
             }
    default:
             token.kind = *stream++;
             break;
  }
}

void print_token(Token token){
  switch(token.kind){
    case TOKEN_INT:
      printf("Token int: %d\n",token.val);
      break;
    case TOKEN_NAME:
      printf("TOken Name: %.*s",token.end- token.start, token.start);
      break;
    default:
      printf("Token '%c'\n",token.kind);
      break;
  }
}

void lex_test(){
  char *source = "+()24fheo,5425+5245samrat";
  stream = source;
  next_token();
  while(token.kind){
    print_token(token);
    next_token();
  }
}

int main(){
  lex_test();
  return 0;
}
