#pragma once

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
