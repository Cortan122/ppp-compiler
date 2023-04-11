#pragma once

#include "declaration.h"
#include "lexer.h"
#include "stb_ds.h"

typedef struct NamedType {
  char* key;
  Struct* value;
} NamedType;

typedef struct NamedFunction {
  char* key;
  Function* value;
} NamedFunction;

typedef struct Parser {
  Lexer lexer;
  Declaration* top_level;
  NamedType* typedefs;
  NamedType* structs;
  Function** funcs;

  bool allow_fancy_structs;
  bool go_deeper;
  bool keep_abstract_headers;

  Emitter* default_emitter;
  Emitter* decl_emitter;
  Emitter* extra_emitter;

  NamedType* defined_specialized_structs;
  NamedFunction* polymorphic_functions;
} Parser;

void parser_parse_struct_parameter(Parser* p, Struct* res);
bool parser_parse_struct_subtypes(Parser* p, Struct* res, bool force_parameter);
bool parser_parse_struct(Parser* p, Struct* res);
bool parser_parse_type(Parser* p, Struct* res);

void parser_inside_function(Parser* p, Function* func);

Declaration parser_force_declaration(Parser* p);
Declaration parser_parse_declaration(Parser* p);
bool parser_parse_function(Parser* p, Function* func);
bool parser_parse_line(Parser* p);

Token parser_peek_token(Parser* p);
void parser_transfer_token(Parser* p, Token** dest);

void parser_read_file(Parser* p, const char* filename);
void parser_emit_declarations(Parser* p, Emitter* emitter);
void parser_emit_functions(Parser* p, Emitter* emitter);
void parser_emit_typedefs(Parser* p, Emitter* emitter, bool print_unknowns);

void parser_delete(Parser* p);
