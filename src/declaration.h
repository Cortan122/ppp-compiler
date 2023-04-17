#pragma once

#include "token.h"

struct Declaration;
typedef struct Struct {
  Token* tokens;
  char* name;
  struct Declaration* members;
  int tokens_members_pos;

  struct Declaration* subtypes;
  struct Struct** expanded_subtypes;
  int tokens_subtypes_pos;
  bool is_primitive;
  char** tag_names;

  struct Struct* aliased_to;
  struct Struct* parameter;
  char* converted_name;
  char* tag_value_name;
  char* tag_counter_name;
  int tokens_modifier_pos;
} Struct;

typedef struct Declaration {
  Token* tokens;
  char* name;
  Struct* type;
  Loc location;
  bool is_typedef;
} Declaration;

typedef struct NamedType {
  char* key;
  Struct* value;
} NamedType;

struct Function;
typedef struct NamedFunction {
  char* key;
  struct Function* value;
} NamedFunction;

typedef struct Function {
  Declaration decl;

  int tokens_prams_pos;
  int tokens_args_pos;
  Declaration* fancy_params;
  bool is_header;
  char* converted_name;

  bool is_abstract_header;
  bool has_normal_prams;
  char* table_name;
  char* table_count_name;
  char* function_pointer_type;

  NamedType* scope;
  NamedFunction* implementations;
  struct Function* base;
} Function;

void declaration_print_function(Function* func, int rec_lvl);
void declaration_print_struct(Struct* s, int rec_lvl);
void declaration_print_debug(Declaration* d, int rec_lvl);

void declaration_emit_parameter_struct(Struct* s, Emitter* emitter, Struct* base, bool weak);
void declaration_emit_fancy_struct(Struct* s, Emitter* emitter);
void declaration_emit_struct(Struct* s, Emitter* emitter);
void declaration_emit_function_arguments(Function* func, Emitter* emitter, bool name_only);
void declaration_emit_fancy_function(Function* func, Emitter* emitter);
void declaration_emit_function(Function* func, Emitter* emitter);
void declaration_emit(Declaration* d, Emitter* emitter);

void declaration_delete_function(Function* func);
void declaration_delete_struct(Struct* s);
void declaration_delete(Declaration* d);
