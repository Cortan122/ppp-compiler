#include "token.h"

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
