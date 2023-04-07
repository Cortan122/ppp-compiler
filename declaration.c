#include "declaration.h"

#include <stdio.h>
#include <stdlib.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void declaration_print_struct(Struct* s, int rec_lvl) {
  printf("%*sStruct(tokens = %d, name = %s) \n", rec_lvl * 2, "", (int)arrlen(s->tokens), s->name);
  rec_lvl++;
  for(int i = 0; i < arrlen(s->tokens); i++) {
    printf("%*s", rec_lvl * 2, "");
    token_print_debug(&s->tokens[i]);

    if(s->tokens_header_len == i + 1) {
      for(int j = 0; j < arrlen(s->members); j++) {
        declaration_print_debug(&s->members[j], rec_lvl);
      }
    }

    if(i == arrlen(s->tokens) - 2) {
      for(int j = 0; j < arrlen(s->subtypes); j++) {
        declaration_print_debug(&s->subtypes[j], rec_lvl);
      }
    }
  }
  rec_lvl--;
}

void declaration_print_debug(Declaration* d, int rec_lvl) {
  printf("%*sDeclaration(tokens = %d, name = %s)\n", rec_lvl * 2, "", (int)arrlen(d->tokens), d->name);
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

void declaration_emit_fancy_struct(Struct* s, Emitter* emitter) {
  for(int i = 0; i < arrlen(s->tokens) - 2; i++) {
    token_emit(&s->tokens[i], emitter);
    if(s->tokens_header_len == i + 1) {
      token_emit_cstr(" int tag; ", emitter);
      token_emit_cstr("struct {", emitter);
      for(int i = 0; i < arrlen(s->members); i++) {
        declaration_emit(&s->members[i], emitter);
      }
      token_emit(&s->tokens[++i], emitter);
      token_emit_cstr(" head; ", emitter);

      token_emit_cstr("union {", emitter);
      for(int i = 0; i < arrlen(s->subtypes); i++) {
        declaration_emit(&s->subtypes[i], emitter);
        // TODO: add phony name token
      }
      token_emit_cstr(" } tail;", emitter);
      token_emit_cstr("\n}", emitter);  // TODO: use location of '>'
    }
  }
}

void declaration_emit_struct(Struct* s, Emitter* emitter) {
  if(emitter->convert_structs && s->subtypes) {
    if(s->tokens_header_len) {
      declaration_emit_fancy_struct(s, emitter);
    } else {
      // TODO
    }
    return;
  }

  for(int i = 0; i < arrlen(s->tokens); i++) {
    token_emit(&s->tokens[i], emitter);
    if(s->tokens_header_len == i + 1) {
      for(int i = 0; i < arrlen(s->members); i++) {
        declaration_emit(&s->members[i], emitter);
      }
    }

    if(i == arrlen(s->tokens) - 2) {
      for(int j = 0; j < arrlen(s->subtypes); j++) {
        declaration_emit(&s->subtypes[j], emitter);
      }
    }
  }
}

void declaration_emit(Declaration* d, Emitter* emitter) {
  if(d->type) {
    declaration_emit_struct(d->type, emitter);
  }

  for(int i = 0; i < arrlen(d->tokens); i++) {
    token_emit(&d->tokens[i], emitter);
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

  for(int i = 0; i < arrlen(s->subtypes); i++) {
    declaration_delete(&s->subtypes[i]);
  }
  arrfree(s->subtypes);
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
