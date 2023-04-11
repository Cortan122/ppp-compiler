#include "token.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ERROR_COLOR "\x1b[31m"
#define WARNING_COLOR "\x1b[95m"
#define INFO_COLOR "\x1b[36m"
#define RESET_COLOR "\x1b[0m"
#define LOC_COLOR "\x1b[97m"

const char* LogLevel_colors[] = {
    [LOGLEVEL_ERROR] = ERROR_COLOR,
    [LOGLEVEL_WARNING] = WARNING_COLOR,
    [LOGLEVEL_INFO] = INFO_COLOR,
};

const char* LogLevel_names[] = {
    [LOGLEVEL_ERROR] = "error",
    [LOGLEVEL_WARNING] = "warning",
    [LOGLEVEL_INFO] = "info",
};

const char* TokenKind_names[] = {
    "EOF",
    "INVALID",
    "CHAR",
    "WORD",
    "LONGCOMMENT",
    "SHORTCOMMENT",
    "PREPROCESSOR",
    "PREPROCESSOR_LINENUM",
    "LONGSTRING",
    "SHORTSTRING",
    "NUMBER",
};

void token_print_debug(Token* tok) {
  const char* tmp = strrchr(tok->location.filename, '/');
  const char* name = tmp ? tmp + 1 : tok->location.filename;

  int line = tok->location.line_num + 1;
  int col = tok->location.col_num + 1;
  const char* type = TokenKind_names[tok->kind];
  printf("%s[%s:%d:%d] data = %.*s\n", type, name, line, col, (int)tok->length, tok->data);
}

void token_print_error(Token* tok, LogLevel level, const char* msg, const char* printf_arg) {
  int line = tok->location.line_num + 1;
  int col = tok->location.col_num + 1;
  fprintf(stderr, LOC_COLOR "%s:%d:%d: " RESET_COLOR, tok->location.filename, line, col);

  const char* color = LogLevel_colors[level];
  fprintf(stderr, "%s%s: " RESET_COLOR, color, LogLevel_names[level]);
  fprintf(stderr, msg, printf_arg);
  fprintf(stderr, "\n");
}

static bool is_sticky_token(TokenKind kind) {
  switch(kind) {
    case TOKEN_EOF:
    case TOKEN_INVALID:
    case TOKEN_CHAR:
    case TOKEN_LONGCOMMENT:
    case TOKEN_SHORTCOMMENT:
    case TOKEN_PREPROCESSOR:
    case TOKEN_PREPROCESSOR_LINENUM:
    case TOKEN_LONGSTRING:
    case TOKEN_SHORTSTRING:
      return false;

    case TOKEN_WORD:
    case TOKEN_NUMBER:
      return true;
  }
  // unreachable
  return true;
}

static void emit_spaces(Emitter* emitter, Loc loc, bool force_space) {
  if(emitter->cursor.filename == NULL) {
    emitter->cursor = loc;
    emitter->cursor.col_num = 0;
  }
  if(loc.filename == NULL) {
    if(force_space) {
      fprintf(emitter->file, " ");
      emitter->cursor.col_num++;
    }
    return;
  }

  if(strcmp(emitter->cursor.filename, loc.filename) != 0) {
    fprintf(emitter->file, "\n");
    emitter->cursor = loc;
    emitter->cursor.col_num = 0;
  }

  int line_delta = loc.line_num - emitter->cursor.line_num;
  if(emitter->delete_repeted_empty_lines && line_delta > 2) line_delta = 2;
  if(emitter->ignore_next_newline) {
    emitter->ignore_next_newline = false;
    line_delta = 0;
  }
  for(int i = 0; i < line_delta; i++) {
    fprintf(emitter->file, "\n");
    emitter->cursor.col_num = 0;
  }

  int col_delta = loc.col_num - emitter->cursor.col_num;
  if(emitter->ignore_next_indent) {
    emitter->ignore_next_indent = false;
    col_delta = 0;
  }
  if(col_delta > 0) {
    fprintf(emitter->file, "%*s", col_delta, "");
  }

  if(col_delta <= 0 && line_delta <= 0 && force_space) {
    fprintf(emitter->file, " ");
    loc.col_num++;
  }

  emitter->cursor = loc;
}

void token_emit(Token* tok, Emitter* emitter) {
  if(emitter == NULL) return;

  emit_spaces(emitter, tok->location, is_sticky_token(tok->kind) && is_sticky_token(emitter->last_token_kind));

  int length_written = 0;
  switch(tok->kind) {
    case TOKEN_INVALID:
      fprintf(emitter->file, "/* INVALID TOKEN %.*s */%n", (int)tok->length, tok->data, &length_written);
      break;
    case TOKEN_EOF:
      if(emitter->cursor.col_num) {
        fprintf(emitter->file, "\n");
      }
      break;

    case TOKEN_CHAR:
    case TOKEN_WORD:
    case TOKEN_NUMBER:
    case TOKEN_SHORTCOMMENT:
      fprintf(emitter->file, "%.*s%n", (int)tok->length, tok->data, &length_written);
      break;

    case TOKEN_LONGCOMMENT:
    case TOKEN_PREPROCESSOR:
      fprintf(emitter->file, "%.*s\n", (int)tok->length, tok->data);
      emitter->cursor.line_num++;
      emitter->cursor.col_num = 0;
      break;
    case TOKEN_PREPROCESSOR_LINENUM:
      fprintf(emitter->file, "%.*s\n", (int)tok->length, tok->data);
      emitter->cursor.filename = NULL;
      break;

    case TOKEN_LONGSTRING:
    case TOKEN_SHORTSTRING: {
      char quote = tok->kind == TOKEN_LONGSTRING ? '"' : '\'';
      fprintf(emitter->file, "%c", quote);
      for(size_t i = 0; i < tok->length; i++) {
        char c = tok->data[i];
        if(c < ' ' || c == '"' || c == '\'' || c == '\\') {
          fprintf(emitter->file, "\\x%02x", (uint8_t)c);
          length_written += 4;
        } else {
          fprintf(emitter->file, "%c", c);
          length_written++;
        }
      }
      fprintf(emitter->file, "%c", quote);
      length_written += 2;
    } break;
  }

  emitter->cursor.col_num += length_written;
  emitter->last_token_kind = tok->kind;
}

void token_emit_cstr(const char* keyword, Emitter* emitter) {
  if(emitter == NULL) return;

  if(*keyword == '\n') {
    fprintf(emitter->file, "\n");
    emitter->cursor.line_num++;
    emitter->cursor.col_num = 0;
    keyword++;
  }
  fprintf(emitter->file, "%s", keyword);
  emitter->cursor.col_num += strlen(keyword);
  emitter->last_token_kind = TOKEN_WORD;
}

bool token_eq_keyword(Token* tok, const char* keyword) {
  if(tok->kind != TOKEN_WORD) return false;
  size_t keyword_len = strlen(keyword);
  if(keyword_len != tok->length) return false;
  return memcmp(tok->data, keyword, tok->length) == 0;
}

bool token_eq_char(Token* tok, char val) {
  if(tok->kind != TOKEN_CHAR) return false;
  return *tok->data == val;
}

Token token_from_keyword(const char* keyword) {
  Token res = {0};
  res.kind = TOKEN_WORD;
  res.data = strdup(keyword);
  res.length = strlen(keyword);
  return res;
}
