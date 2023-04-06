#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int main(int argc, char** argv) {
  (void)argc;

  Emitter em = {.file = stdout};
  Lexer lexer = {0};
  lexer_open_file(&lexer, argv[1] ? argv[1] : "main.c");
  while(1) {
    Token t = lexer_next_token(&lexer);
    token_emit(&t, &em);
    if(t.kind == TOKEN_EOF) break;
    free(t.data);
  }

  lexer_delete(&lexer);
  return 0;
}
