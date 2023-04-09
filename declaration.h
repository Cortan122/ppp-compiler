#pragma once

#include "token.h"

struct Declaration;
typedef struct Struct {
  Token* tokens;
  char* name;
  struct Declaration* members;
  int tokens_members_pos;

  struct Declaration* subtypes;
  int tokens_subtypes_pos;
  bool is_primitive;

  struct Struct* aliased_to;
  struct Struct* parameter;
  char* converted_name;
  int tokens_modifier_pos;
} Struct;

typedef struct Declaration {
  Token* tokens;
  char* name;
  Struct* type;
  Loc location;
  bool is_typedef;
} Declaration;

typedef struct Function {
  Declaration decl;

  int tokens_prams_pos;
  int tokens_args_pos;
  Declaration* fancy_params;
  bool is_header;
  char* converted_name;
} Function;

void declaration_print_function(Function* func, int rec_lvl);
void declaration_print_struct(Struct* s, int rec_lvl);
void declaration_print_debug(Declaration* d, int rec_lvl);

void declaration_emit_fancy_struct(Struct* s, Emitter* emitter);
void declaration_emit_struct(Struct* s, Emitter* emitter);
void declaration_emit_function(Function* func, Emitter* emitter);
void declaration_emit(Declaration* d, Emitter* emitter);

void declaration_delete_function(Function* func);
void declaration_delete_struct(Struct* s);
void declaration_delete(Declaration* d);
