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
