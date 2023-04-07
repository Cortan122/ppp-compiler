#pragma once

#include "token.h"

struct Declaration;
typedef struct Struct {
  Token* tokens;
  char* name;
  struct Declaration* members;
  int tokens_header_len;

  struct Declaration* subtypes;
} Struct;

typedef struct Declaration {
  Token* tokens;
  char* name;
  Struct* type;
  Loc location;
  bool is_typedef;
} Declaration;

void declaration_print_struct(Struct* s, int rec_lvl);
void declaration_print_debug(Declaration* d, int rec_lvl);

void declaration_emit_fancy_struct(Struct* s, Emitter* emitter);
void declaration_emit_struct(Struct* s, Emitter* emitter);
void declaration_emit(Declaration* d, Emitter* emitter);

void declaration_delete_struct(Struct* s);
void declaration_delete(Declaration* d);
