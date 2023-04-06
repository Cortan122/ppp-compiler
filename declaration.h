#pragma once

#include "token.h"

struct Declaration;
typedef struct Struct {
  Token* tokens;
  char* name;
  struct Declaration* members;
} Struct;

typedef struct Declaration {
  Token* tokens;
  char* name;
  Struct* type;
} Declaration;

void declaration_print_struct(Struct* s, int rec_lvl);
void declaration_print_debug(Declaration* d, int rec_lvl);
void declaration_delete_struct(Struct* s);
void declaration_delete(Declaration* d);
