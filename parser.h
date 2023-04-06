#pragma once

#include "declaration.h"
#include "lexer.h"
#include "stb_ds.h"

typedef struct NamedType {
  char* key;
  Struct* value;
} NamedType;

typedef struct Parser {
  Lexer lexer;
  Declaration* top_level;
  NamedType* typedefs;
  NamedType* structs;
} Parser;

bool parser_parse_struct(Parser* p, Struct* res);
Declaration parser_parse_declaration(Parser* p);
bool parser_parse_line(Parser* p);

Token parser_peek_token(Parser* p);
void parser_transfer_token(Parser* p, Token** dest);

void parser_read_file(Parser* p, const char* filename);
void parser_emit_declarations(Parser* p, Emitter* emitter);

void parser_delete(Parser* p);
