#include "declaration.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void declaration_print_struct(Struct* s, int rec_lvl) {
  Token last = s->tokens[arrlen(s->tokens) - 1];
  bool has_extra_bracket = token_eq_char(&last, '}');
  printf("%*sStruct(tokens = %ld, name = %s) \n", rec_lvl * 2, "", arrlen(s->tokens), s->name);
  rec_lvl++;
  for(int i = 0; i < arrlen(s->tokens) - has_extra_bracket; i++) {
    printf("%*s", rec_lvl * 2, "");
    token_print_debug(&s->tokens[i]);
  }

  for(int i = 0; i < arrlen(s->members); i++) {
    declaration_print_debug(&s->members[i], rec_lvl);
  }
  if(has_extra_bracket) {
    printf("%*s", rec_lvl * 2, "");
    token_print_debug(&last);
  }
  rec_lvl--;
}

void declaration_print_debug(Declaration* d, int rec_lvl) {
  printf("%*sDeclaration(tokens = %ld, name = %s)\n", rec_lvl * 2, "", arrlen(d->tokens), d->name);
  rec_lvl++;

  if(d->type) {
    declaration_print_struct(d->type, rec_lvl);
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

void declaration_delete_struct(Struct* s) {
  for(int i = 0; i < arrlen(s->tokens); i++) {
    free(s->tokens[i].data);
  }
  arrfree(s->tokens);

  for(int i = 0; i < arrlen(s->members); i++) {
    declaration_delete(&s->members[i]);
  }
  arrfree(s->members);
  free(s);
}

void declaration_delete(Declaration* d) {
  if(d->type) {
    declaration_delete_struct(d->type);
  }

  for(int i = 0; i < arrlen(d->tokens); i++) {
    free(d->tokens[i].data);
  }
  arrfree(d->tokens);
}
