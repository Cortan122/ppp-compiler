#pragma once

#include "declaration.h"
#include "lexer.h"
#include "stb_ds.h"

typedef struct Parser {
  Lexer lexer;
  Declaration* top_level;
  struct {
    char* key;
    Struct* value;
  }* named_types;
} Parser;

bool parser_parse_struct(Parser* p, Struct* res);
Declaration parser_parse_declaration(Parser* p);
bool parser_parse_line(Parser* p);

Token parser_peek_token(Parser* p);
void parser_transfer_token(Parser* p, Token** dest);

void parser_delete(Parser* p);
