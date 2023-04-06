#pragma once

#include "token.h"

struct Declaration;
typedef struct Struct {
  Token* tokens;
  char* name;
  struct Declaration* members;
} Struct;

typedef struct Declaration {
  Token* tokens;
  char* name;
  Struct* type;
} Declaration;

void declaration_print_struct(Struct* s, int rec_lvl);
void declaration_print_debug(Declaration* d, int rec_lvl);
void declaration_delete_struct(Struct* s);
void declaration_delete(Declaration* d);
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "token.h"

#define da_append(da, item)                                                   \
  do {                                                                        \
    if((da)->length >= (da)->capacity) {                                      \
      size_t new_capacity = (da)->capacity * 2;                               \
      if(new_capacity == 0) new_capacity = 16;                                \
      (da)->data = realloc((da)->data, new_capacity * sizeof((da)->data[0])); \
      (da)->capacity = new_capacity;                                          \
    }                                                                         \
    (da)->data[(da)->length++] = (item);                                      \
  } while(0)

typedef struct Lexer {
  char peeked_char;
  bool exhausted;
  bool is_newline;
  Loc current_location;

  FILE* file;
  const char* memory;
  size_t memory_length;

  Token peeked_token;

  struct {
    char** data;
    size_t length;
    size_t capacity;
  } allocated_strings;
} Lexer;

void lexer_parse_escape_sequence(Lexer* l, Token* res);
void lexer_lex_string(Lexer* l, Token* res, char end);
void lexer_lex_line(Lexer* l, Token* res);
void lexer_lex_comment(Lexer* l, Token* res);

void lexer_open_file(Lexer* l, const char* filename);
void lexer_open_memory(Lexer* l, const char* text, size_t length);
void lexer_delete(Lexer* l);

char lexer_next_char(Lexer* l);
char lexer_peek_char(Lexer* l);
char lexer_drop_char(Lexer* l);

Token lexer_next_token(Lexer* l);
Token lexer_peek_token(Lexer* l);
Token lexer_drop_token(Lexer* l);
#pragma once

#include "declaration.h"
#include "lexer.h"
#include "stb_ds.h"

typedef struct NamedType {
  char* key;
  Struct* value;
} NamedType;

typedef struct Parser {
  Lexer lexer;
  Declaration* top_level;
  NamedType* typedefs;
  NamedType* structs;
} Parser;

bool parser_parse_struct(Parser* p, Struct* res);
Declaration parser_parse_declaration(Parser* p);
bool parser_parse_line(Parser* p);

Token parser_peek_token(Parser* p);
void parser_transfer_token(Parser* p, Token** dest);

void parser_delete(Parser* p);
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum LogLevel {
  LOGLEVEL_ERROR = 0,
  LOGLEVEL_WARNING,
  LOGLEVEL_INFO,
} LogLevel;

typedef enum TokenKind {
  TOKEN_EOF = 0,
  TOKEN_INVALID,
  TOKEN_CHAR,
  TOKEN_WORD,
  TOKEN_LONGCOMMENT,
  TOKEN_SHORTCOMMENT,
  TOKEN_PREPROCESSOR,
  TOKEN_LONGSTRING,
  TOKEN_SHORTSTRING,
  TOKEN_NUMBER,
} TokenKind;

typedef struct Loc {
  const char* filename;
  int line_num;
  int col_num;
  int byte_num;
} Loc;

typedef struct Token {
  char* data;
  size_t length;
  size_t capacity;
  TokenKind kind;
  Loc location;
} Token;

void token_print_debug(Token* tok);
void token_print_error(Token* tok, LogLevel level, const char* msg, const char* printf_arg);
bool token_eq_keyword(Token* tok, const char* keyword);
bool token_eq_char(Token* tok, char val);
