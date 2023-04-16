#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int main(int argc, char** argv) {
  FILE* output1 = argc > 2 ? fopen(argv[2], "w") : stdout;
  FILE* output2 = argc > 3 ? fopen(argv[3], "w") : stderr;

  Emitter em1 = {.file = output1, .convert_structs = true, .add_line_directives = true};
  Emitter em2 = {.file = output2};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true};
  parser.decl_emitter = &em1;
  parser.default_emitter = &em1;
  parser.extra_emitter = &em1;

  char* filename = argc > 1 ? argv[1] : "main.c";
  parser_read_file(&parser, filename);
  parser_emit_final_tables(&parser, &em1);
  if(argc > 2) {
    parser_emit_declarations(&parser, &em2);
    parser_emit_functions(&parser, &em2);
  }

  if(argc > 2) fclose(output1);
  if(argc > 3) fclose(output2);

  parser_delete(&parser);
  return 0;
}
