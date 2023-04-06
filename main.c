#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int main(int argc, char** argv) {
  (void)argc;

  Parser parser = {0};
  lexer_open_file(&parser.lexer, argv[1] ? argv[1] : "main.c");
  while(parser_parse_line(&parser)) {
  }
  for(int i = 0; i < arrlen(parser.top_level); i++) {
    declaration_print_debug(&parser.top_level[i], 0);
  }
  parser_delete(&parser);
  return 0;
}
