#include "declaration.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void declaration_print_debug(Declaration* d, int rec_lvl) {
  printf("%*sDeclaration(tokens = %ld, name = %s)\n", rec_lvl * 2, "", arrlen(d->tokens), d->name);
  rec_lvl++;

  if(d->type->tokens) {
    Token last = d->type->tokens[arrlen(d->type->tokens) - 1];
    bool has_extra_bracket = token_eq_char(&last, '}');
    printf("%*sStruct(tokens = %ld, name = %s) \n", rec_lvl * 2, "", arrlen(d->type->tokens), d->type->name);
    rec_lvl++;
    for(int i = 0; i < arrlen(d->type->tokens) - has_extra_bracket; i++) {
      printf("%*s", rec_lvl * 2, "");
      token_print_debug(&d->type->tokens[i]);
    }

    for(int i = 0; i < arrlen(d->type->members); i++) {
      declaration_print_debug(&d->type->members[i], rec_lvl);
    }
    if(has_extra_bracket) {
      printf("%*s", rec_lvl * 2, "");
      token_print_debug(&last);
    }
    rec_lvl--;
  }

  for(int i = 0; i < arrlen(d->tokens); i++) {
    printf("%*s", rec_lvl * 2, "");
    token_print_debug(&d->tokens[i]);
  }

  rec_lvl--;
  if(rec_lvl == 0) {
    printf("\n");
  }
}

void declaration_delete(Declaration* d) {
  for(int i = 0; i < arrlen(d->type->tokens); i++) {
    free(d->type->tokens[i].data);
  }
  arrfree(d->type->tokens);

  for(int i = 0; i < arrlen(d->type->members); i++) {
    declaration_delete(&d->type->members[i]);
  }
  arrfree(d->type->members);
  free(d->type);

  for(int i = 0; i < arrlen(d->tokens); i++) {
    free(d->tokens[i].data);
  }
  arrfree(d->tokens);
}
