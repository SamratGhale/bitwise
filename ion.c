#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

void syntax_error(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("Syntax error");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

/**
 * Buffer header for stretchy buffer 
 */
typedef struct BufHdr {
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
#define buf_end(b)((b) + buf_len(b)) 

//pushes x into the buffer and incresese the len by 1
#define buf_push(b, ...) (buf__fit((b), 1),(b)[buf__hdr(b)->len++]=(__VA_ARGS__))

#define buf_free(b)((b)? free(buf__hdr(b)),(b)=NULL:0)

//Safe malloc
void* xmalloc(size_t _size) {
  void* ptr = malloc(_size);
  if (!ptr) {
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
}

void fatal(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("FATAL: ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

void* xrealloc(void* ptr, size_t _size) {
  ptr = realloc(ptr, _size);
  if (!ptr) {
    perror("xremalloc failed");
    exit(1);
  }
  return ptr;
}

#define MAX(a,b)((a)>(b)?(a):(b))

//takes in a buffer, length and the size of element in buffer
void* buf__grow(const void* b, size_t new_len, size_t elem_size) {

  //new size of the buffer
  assert(buf_cap(b) <= (SIZE_MAX - 1) / 2);
  size_t new_cap = MAX(1 + 2 * buf_cap(b), new_len);
  assert(new_len <= new_cap);
  assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf)));

  size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
  BufHdr* new_hdr;
  if (b) {
    //just reaccocate the buffer with new size
    new_hdr = xrealloc(buf__hdr(b), new_size);
  }
  else {
    //make new buffer with new size
    new_hdr = xmalloc(new_size);
    new_hdr->len = 0;
  }
  new_hdr->cap = new_len;
  return new_hdr->buf;
}

void test_buf() {
  int* adsf = NULL;
  assert(buf_len(adsf) == 0);
  int n = 1024;
  for (int i = 0; i < n; i++) {
    buf_push(adsf, i);
  }
  assert(buf_len(adsf) == n);
  for (int i = 0; i < buf_len(adsf); i++) {
    assert(adsf[i] == i);
  }
  buf_free(adsf);
  assert(buf_len(adsf) == 0);
  assert(adsf == NULL);
}

typedef struct Intern {
  size_t len;
  const char* str;
} Intern;

static Intern* interns;

const char* str_intern_range(const char* start, const char* end) {
  size_t len = end - start;

  for (Intern* it = interns; it != buf_end(interns); it++) {
    if (it->len == len && strncmp(it->str, start, len) == 0) {
      return it->str;
    }
  }

  char* str = xmalloc(len + 1);
  memcpy(str, start, len);
  str[len] = 0;
  buf_push(interns, (Intern){len, str});
  return str;
}


const char* str_intern(const char* str) {
  return str_intern_range(str, str + strlen(str));
}
// lexing: translating char stream to token stream

void str_intern_test() {
  char x[] = "hello";
  char y[] = "hello";
  assert(x != y);
  assert(str_intern(x) == str_intern(y));
}

typedef enum TokenKind {
  TOKEN_INT = 128,
  TOKEN_NAME,
} TokenKind;

size_t copy_token_kind_str(char* dest, size_t dest_size, TokenKind kind) {
  size_t n = 0;
  switch (kind) {
  case TOKEN_INT:
    n = snprintf(dest, dest_size , "integer");
    break;
  case TOKEN_NAME:
    n = snprintf(dest, dest_size , "name");
    break;
  default:
    if (kind < 128 && isprint(kind)) {
      n = snprintf(dest, dest_size , "%c", kind);
    }
    else {
      n = snprintf(dest, dest_size , "<ASCII %d>", kind);
    }
  }
  return n;
}

const char* token_kind_str(TokenKind kind) {
  static char buf[256];
  size_t n = copy_token_kind_str(buf, sizeof(buf), kind);
  assert(n + 1 <= sizeof(buf));
  return buf;
}

typedef struct Token {
  TokenKind kind;
  const char* start;
  const char* end;
  union {
    uint64_t int_val;
    const char* name;
  };
} Token;

//Global variables, state machine
const char* keyword_if;
const char* keyword_for;
const char* keyword_while;

void init_keywords() {
  keyword_if = str_intern("if");
  keyword_for = str_intern("for");
  keyword_while = str_intern("while");
}

Token token;
const char* stream;

uint8_t char_to_digit[256] = {
  ['0'] = 0,
  ['1'] = 1,
  ['2'] = 2,
  ['3'] = 3,
  ['4'] = 4,
  ['5'] = 5,
  ['6'] = 6,
  ['7'] = 7,
  ['8'] = 8,
  ['9'] = 9,
  ['a'] = ['A'] = 10,
  ['b'] = ['B'] = 11,
  ['c'] = ['C'] = 12,
  ['d'] = ['D'] = 13,
  ['e'] = ['E'] = 14,
  ['f'] = ['F'] = 15,
};

uint64_t scan_int() {
  uint64_t base = 10;

  if (*stream == '0') {
    stream++;

    if (tolower(*stream) == 'x') {
      //Hexadecimal
      stream++;
      base = 16;
    }
    else {
      syntax_error("Invalid integer literal suffix '%c'", *stream);
      stream++;
    }
  }


  uint64_t val = 0;
  while (isdigit(*stream)) {
    uint64_t digit = char_to_digit[*stream];

    if (digit == 0 && *stream != '0') {
      syntax_error("Invalid integer literal suffix ")
	}

    if (val > (UINT64_MAX - digit) / 10) {
      syntax_error("Integer literal overflow\n");
      while (isdigit(*stream)) {
	stream++;
      }
      val = 0;
    }
    val = val * 10 + digit;
  }
  return val;
}

//Sets the token and stream to to next thing in the code
void next_token() {
  token.start = stream;
  switch (*stream) {
  case '0':case '1': case '2':case '3':case '4': case '5': case '6':  case '7':case '8': case '9': {
    token.kind = TOKEN_INT;
    token.int_val = scan_int();
    break;
  }
  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':case 'g': case 'h':
  case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':case 'o': case 'p':
  case 'q': case 'r': case 's': case 't': case 'u': case 'v':case 'w': case 'x':
  case 'y': case 'z': case 'A': case 'B': case 'C': case 'D':case 'E': case 'F':
  case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':case 'M': case 'N':
  case 'O': case 'X': case 'Y': case 'Z': case '_': {
    while (isalnum(*stream) || *stream == '_') {
      stream++;
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

void print_token(Token token) {
  switch (token.kind) {
  case TOKEN_INT:
    printf("Token int: %llu\n", token.int_val);
    break;
  case TOKEN_NAME:
    printf("TOken Name: %.*s\n", (int)(token.end - token.start), token.start);
    break;
  default:
    printf("Token '%c'\n", token.kind);
    break;
  }
}
inline bool is_token(TokenKind Kind) {
  return token.kind == Kind;
}

inline bool is_token_name(const char *name){
  return token.kind == TOKEN_NAME && token.name == name;
}

inline bool match_token(TokenKind kind) {
  if (is_token(kind)) {
    next_token();
    return true;
  }
  else {
    return false;
  }
}
inline bool expect_token(TokenKind kind) {
  if (is_token(kind)) {
    next_token();
    return true;
  }
  else {
    char buf[256];
    copy_token_kind_str(buf, sizeof(buf), kind);
    fatal("Expected token %s, got %s ", buf, token_kind_str(token.kind));
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
int parse_expr();
int parse_expr3() {
  if (is_token(TOKEN_INT)) {
    int val = token.int_val;
    next_token();
    return val;
  }
  else if (match_token('(')) {
    int val = parse_expr();
    expect_token(')');
    return val;
  }
  else {
    fatal("expected integer or (, got %s", token_kind_str(token.kind));
    return 0;
  }
}

int parse_expr2() {
  if (match_token('-')) {
    return -parse_expr2();
  }
  else {
    return parse_expr3();
  }
}


int parse_expr1() {
  int val = parse_expr2();
  while (is_token('*') || is_token('/')) {
    char op = token.kind;
    next_token();

    int rval = parse_expr2();
    if(op == '*'){
      val *= rval;
    }else{
      assert(op == '/');
      assert(rval != 0);
      val /= rval;
    }
  }
  return val;
}


int parse_expr0() {
  int val = parse_expr1();
  while (is_token('+') || is_token('-')) {
    char op = token.kind;
    next_token();
    int rval = parse_expr1();
    if(op == '+'){
      val += rval;
    }else{
      assert(op == '-');
      val -= rval;
    }
  }
  return val;
}
int parse_expr() {
  return parse_expr0();
}

void init_stream(const char* str) {
  stream = str;
  next_token();
}

#define assert_token(x)      assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_NAME))
#define assert_token_int(x)  assert(token.int_val== (x) && match_token(TOKEN_INT))
#define assert_token_eof(x)  assert(is_token(0));

void lex_test() {
  const char* str = "XY+(XY)_HELLO1,234+994";
  init_stream(str);
  assert_token_name("XY");
  assert_token('+');
  assert_token('(');
  assert_token_name("XY");
  assert_token(')');
  assert_token_name("_HELLO1");
  assert_token(',');
  assert_token_int(234);
  assert_token('+');
  assert_token_int(994);
  assert_token_eof();

  //init_stream("2147483648");
  //assert_token_int(2147483647);
  //assert_token_eof();
}

#undef assert_token_eof
#undef assert_token_int
#undef assert_token_name
#undef assert_token

int parse_test_expr(const char* expr){
  init_stream(expr);
  return parse_expr();
}
void parse_test() {
  assert(parse_test_expr("1+1") == 2);
  printf("1*2*(2+3)= %d \n",parse_test_expr("1*2*(2+3)"));
}


int main() {
  lex_test();
  //str_intern_test();
  parse_test();
  return 0;
}
