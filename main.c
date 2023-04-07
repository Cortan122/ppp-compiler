#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int main(int argc, char** argv) {
  (void)argc;

  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, argv[1] ? argv[1] : "main.c");
  parser_emit_declarations(&parser, &em);
  // for(int i = 0; i < arrlen(parser.top_level); i++) {
  //   declaration_print_debug(&parser.top_level[i], 0);
  // }

  parser_delete(&parser);
  return 0;
}
