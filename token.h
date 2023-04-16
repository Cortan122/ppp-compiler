#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

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
  TOKEN_PREPROCESSOR_LINENUM,
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

typedef struct Emitter {
  FILE* file;
  Loc cursor;
  TokenKind last_token_kind;
  bool delete_repeted_empty_lines;
  bool ignore_next_indent;
  bool ignore_next_newline;
  bool convert_structs;
  bool add_line_directives;
} Emitter;

void token_print_debug(Token* tok);
void token_print_error(Token* tok, LogLevel level, const char* msg, const char* printf_arg);

void token_emit(Token* tok, Emitter* emitter);
void token_emit_cstr(const char* keyword, Emitter* emitter);

bool token_eq_keyword(Token* tok, const char* keyword);
bool token_eq_char(Token* tok, char val);
Token token_from_keyword(const char* keyword);
