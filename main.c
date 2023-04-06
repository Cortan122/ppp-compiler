#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int main(int argc, char** argv) {
  (void)argc;

  Emitter em = {.file = stdout};
  Parser parser = {0};
  parser_read_file(&parser, argv[1] ? argv[1] : "main.c");
  parser_emit_declarations(&parser, &em);

  parser_delete(&parser);
  return 0;
}
