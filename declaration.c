#include "declaration.h"

#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wsign-compare"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void declaration_print_struct(Struct* s, int rec_lvl) {
  if(s->is_primitive) {
    for(int i = 0; i < arrlen(s->tokens); i++) {
      printf("%*s", rec_lvl * 2, "");
      token_print_debug(&s->tokens[i]);
    }
    return;
  }

  printf("%*sStruct(tokens = %d, name = %s) \n", rec_lvl * 2, "", (int)arrlen(s->tokens), s->name);
  rec_lvl++;
  for(int i = 0; i < arrlen(s->tokens); i++) {
    printf("%*s", rec_lvl * 2, "");
    token_print_debug(&s->tokens[i]);

    if(s->tokens_members_pos == i + 1) {
      for(int j = 0; j < arrlen(s->members); j++) {
        declaration_print_debug(&s->members[j], rec_lvl);
      }
    }

    if(s->tokens_subtypes_pos == i + 1) {
      if(s->parameter) {
        declaration_print_struct(s->parameter, rec_lvl);
      }
      for(int j = 0; j < arrlen(s->subtypes); j++) {
        declaration_print_debug(&s->subtypes[j], rec_lvl);
      }
    }
  }
  rec_lvl--;
}

void declaration_print_function(Function* func, int rec_lvl) {
  printf("%*sFunction(is_header = %d, prams = %d)\n", rec_lvl * 2, "", func->is_header, (int)arrlen(func->fancy_params));
  rec_lvl++;
  declaration_print_debug(&func->decl, rec_lvl);

  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    declaration_print_debug(&func->fancy_params[i], rec_lvl);
  }

  rec_lvl--;
  if(rec_lvl == 0) {
    printf("\n");
  }
}

void declaration_print_debug(Declaration* d, int rec_lvl) {
  int total_tokens = arrlen(d->tokens);
  if(d->type && d->type->is_primitive) {
    total_tokens += arrlen(d->type->tokens);
  }

  printf("%*sDeclaration(tokens = %d, name = %s)\n", rec_lvl * 2, "", total_tokens, d->name);
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
  for(int i = 0; i < arrlen(s->tokens); i++) {
    token_emit(&s->tokens[i], emitter);
    if(s->tokens_members_pos == i + 1) {
      token_emit_cstr(" int tag; ", emitter);
      token_emit_cstr("struct {", emitter);
      for(int i = 0; i < arrlen(s->members); i++) {
        declaration_emit(&s->members[i], emitter);
      }
      token_emit(&s->tokens[++i], emitter);
      token_emit_cstr(" head; ", emitter);

      token_emit_cstr("union {", emitter);
      for(int i = 0; i < arrlen(s->subtypes); i++) {
        Token name = token_from_keyword("s___");
        snprintf(name.data, name.length, "s%02x", i);
        arrins(s->subtypes[i].tokens, arrlen(s->subtypes[i].tokens) - 2, name);
        declaration_emit(&s->subtypes[i], emitter);
      }
      token_emit_cstr(" } tail;", emitter);

      Token end_bracket = s->tokens[s->tokens_subtypes_pos];
      end_bracket.data = "}";
      token_emit(&end_bracket, emitter);
      i = s->tokens_subtypes_pos;
    }
  }
}

void declaration_emit_struct(Struct* s, Emitter* emitter) {
  if(emitter->convert_structs && s->subtypes) {
    if(s->tokens_members_pos) {
      declaration_emit_fancy_struct(s, emitter);
    } else {
      token_print_error(&s->tokens[0], LOGLEVEL_INFO, "converting struct names is not implemented yet%s", "");
    }
    return;
  }

  for(int i = 0; i < arrlen(s->tokens); i++) {
    token_emit(&s->tokens[i], emitter);
    if(s->tokens_members_pos == i + 1) {
      for(int i = 0; i < arrlen(s->members); i++) {
        declaration_emit(&s->members[i], emitter);
      }
    }

    if(s->tokens_subtypes_pos == i + 1) {
      for(int j = 0; j < arrlen(s->subtypes); j++) {
        declaration_emit(&s->subtypes[j], emitter);
      }
      if(s->parameter) {
        declaration_emit_struct(s->parameter, emitter);
      }
    }
  }
}

void declaration_emit_function(Function* func, Emitter* emitter) {
  if(func->fancy_params == NULL) {
    declaration_emit(&func->decl, emitter);
    return;
  }

  if(emitter->convert_structs) {
    token_print_error(&func->decl.tokens[0], LOGLEVEL_INFO, "converting function headers is not implemented yet%s", "");
  } else {
    declaration_emit_struct(func->decl.type, emitter);

    for(int i = 0; i < arrlen(func->decl.tokens); i++) {
      token_emit(&func->decl.tokens[i], emitter);

      if(func->tokens_prams_pos == i + 1) {
        for(int j = 0; j < arrlen(func->fancy_params); j++) {
          if(j != 0) {
            token_emit(&func->decl.tokens[++i], emitter);
          }
          declaration_emit(&func->fancy_params[j], emitter);
        }
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

void declaration_delete_function(Function* func) {
  declaration_delete(&func->decl);

  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    declaration_delete(&func->fancy_params[i]);
  }
  arrfree(func->fancy_params);
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

  if(s->parameter) {
    declaration_delete_struct(s->parameter);
  }
  free(s);
}

void declaration_delete(Declaration* d) {
  if(d->type) {
    declaration_delete_struct(d->type);
    d->type = NULL;
  }

  for(int i = 0; i < arrlen(d->tokens); i++) {
    free(d->tokens[i].data);
  }
  arrfree(d->tokens);
}
