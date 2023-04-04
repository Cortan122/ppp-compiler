#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "token.h"

int main(int argc, char** argv) {
  (void)argc;

  Lexer lexer = {0};
  lexer_open_file(&lexer, argv[1] ?: "main.c");
  while(1) {
    Token t = lexer_next_token(&lexer);
    if(t.kind == TOKEN_EOF) break;
    token_print_debug(&t);
    free(t.data);
  }

  lexer_delete(&lexer);
  return 0;
}
