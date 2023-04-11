#include "parser.h"

#define strappend(arr, str)                     \
  do {                                          \
    size_t len = strlen((str));                 \
    memcpy(arraddnptr((arr), len), (str), len); \
  } while(0)

#define bracket_lookup_switch(rec_level) \
  case '(':                              \
  case '[':                              \
  case '{':                              \
    (rec_level)++;                       \
    break;                               \
  case ')':                              \
  case ']':                              \
  case '}':                              \
    (rec_level)--;

#define MANGLED_NAME_PREFIX "__ppp__"

static void skip_bracket_block(Parser* p, Token** res, char end, char end2) {
  int rec_level = 0;
  bool keep_looping = true;

  while(keep_looping) {
    Token tok = parser_peek_token(p);
    if(tok.kind == TOKEN_EOF) {
      token_print_error(&tok, LOGLEVEL_ERROR, "expected '%.1s' but found EOF", &end);
      exit(1);
    } else if(tok.kind == TOKEN_CHAR) {
      char val = *tok.data;
      switch(val) {
        bracket_lookup_switch(rec_level);
        /* fall through */
        default:
          if((val == end || val == end2) && rec_level == 0) keep_looping = false;
      }
    }

    parser_transfer_token(p, res);
  }
}

static void parse_declaration_list_until(Parser* p, Token** tok_dest, Declaration** decl_dest, char end) {
  while(1) {
    Token tok = parser_peek_token(p);
    if(token_eq_char(&tok, end)) {
      parser_transfer_token(p, tok_dest);
      break;
    }
    arrpush(*decl_dest, parser_parse_declaration(p));
  }
}

static bool is_typename(Parser* p, const char* word, bool* has_used_typedef) {
  if(strcmp(word, "void") == 0) return true;
  if(strcmp(word, "char") == 0) return true;
  if(strcmp(word, "short") == 0) return true;
  if(strcmp(word, "int") == 0) return true;
  if(strcmp(word, "long") == 0) return true;
  if(strcmp(word, "float") == 0) return true;
  if(strcmp(word, "double") == 0) return true;
  if(strcmp(word, "signed") == 0) return true;
  if(strcmp(word, "unsigned") == 0) return true;

  if(strcmp(word, "_Bool") == 0) return true;
  if(strcmp(word, "bool") == 0) return true;
  if(strcmp(word, "extern") == 0) return true;
  if(strcmp(word, "const") == 0) return true;

  if(has_used_typedef && *has_used_typedef) return false;

  Struct* s = shget(p->typedefs, word);
  if(s == NULL) return false;

  if(has_used_typedef) *has_used_typedef = true;
  return true;
}

static int consume_type_modifiers(Parser* p, Token** tok_dest, bool allow_words, bool has_used_typedef) {
  int res = 0;
  while(1) {
    Token tok = parser_peek_token(p);
    if(token_eq_char(&tok, '*') || token_eq_keyword(&tok, "const") || token_eq_keyword(&tok, "extern")) {
      parser_transfer_token(p, tok_dest);
      res++;
    } else if(allow_words && tok.kind == TOKEN_WORD) {
      if(is_typename(p, tok.data, &has_used_typedef)) {
        parser_transfer_token(p, tok_dest);
        res++;
      } else {
        break;
      }
    } else {
      break;
    }
  }
  return res;
}

static char* get_struct_name(Struct* s, bool use_converted_name) {
  if(s == NULL) {
    return "null";
  } else if(s->converted_name && use_converted_name) {
    return s->converted_name;
  } else if(s->name) {
    return s->name;
  } else if(s->aliased_to) {
    return get_struct_name(s->aliased_to, use_converted_name);
  } else {
    token_print_error(&s->tokens[0], LOGLEVEL_INFO, "mangaling primitive names is not implemented yet%s", "");
    return "primitive";
  }
}

static void generate_converted_function_name(Function* func) {
  if(func->converted_name) return;
  if(func->fancy_params == NULL) return;

  int num_parametrised_types = 0;
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    num_parametrised_types += func->fancy_params[i].type->parameter != NULL;
  }
  if(num_parametrised_types == 0) return;

  strappend(func->converted_name, MANGLED_NAME_PREFIX);
  strappend(func->converted_name, func->decl.name);
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    strappend(func->converted_name, "_");
    strappend(func->converted_name, get_struct_name(func->fancy_params[i].type, false));
    strappend(func->converted_name, "_");
    strappend(func->converted_name, get_struct_name(func->fancy_params[i].type->parameter, true));
  }
  arrpush(func->converted_name, '\0');
}

static void generate_converted_struct_name(Struct* s) {
  if(s->converted_name) return;
  if(s->parameter == NULL) return;

  strappend(s->converted_name, MANGLED_NAME_PREFIX);
  strappend(s->converted_name, get_struct_name(s, false));

  strappend(s->converted_name, "_");
  strappend(s->converted_name, get_struct_name(s->parameter, true));
  arrpush(s->converted_name, '\0');
}

static Declaration* get_scope_variable(Function* func, char* word) {
  // linear search here is fine.. i think
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    if(strcmp(func->fancy_params[i].name, word) == 0) {
      return &func->fancy_params[i];
    }
  }

  return NULL;
}

void parser_transfer_token(Parser* p, Token** dest) {
  Token tok = lexer_drop_token(&p->lexer);
  if(dest) {
    arrpush(*dest, tok);
  } else {
    token_emit(&tok, p->default_emitter);
    free(tok.data);
  }
}

Token parser_peek_token(Parser* p) {
  Token res = lexer_peek_token(&p->lexer);
  while(1) {
    switch(res.kind) {
      case TOKEN_LONGCOMMENT:
      case TOKEN_SHORTCOMMENT:
      case TOKEN_PREPROCESSOR:
      case TOKEN_PREPROCESSOR_LINENUM:
        parser_transfer_token(p, NULL);
        res = lexer_peek_token(&p->lexer);
        break;
      default:
        return res;
    }
  }
  return res;
}

void parser_parse_struct_parameter(Parser* p, Struct* res) {
  Token tok = parser_peek_token(p);
  res->parameter = calloc(1, sizeof(Struct));

  if(!parser_parse_type(p, res->parameter)) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected type name, but found '%s'", tok.data);
    exit(1);
  }

  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '>')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '>', but found '%s'", tok.data);
    exit(1);
  }

  parser_transfer_token(p, &res->tokens);
  generate_converted_struct_name(res);
  if(shget(p->defined_specialized_structs, res->converted_name) == NULL) {
    Struct* base = shget(p->structs, get_struct_name(res, false));
    declaration_emit_parameter_struct(res, p->extra_emitter, base);
    shput(p->defined_specialized_structs, res->converted_name, res);
  }
}

bool parser_parse_struct_subtypes(Parser* p, Struct* res, bool force_parameter) {
  if(!p->allow_fancy_structs) return false;

  Token tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '<')) return false;
  parser_transfer_token(p, &res->tokens);
  res->tokens_subtypes_pos = arrlen(res->tokens);

  if(res->tokens_members_pos && !force_parameter) {
    parse_declaration_list_until(p, &res->tokens, &res->subtypes, '>');
  } else {
    parser_parse_struct_parameter(p, res);
  }

  return true;
}

bool parser_parse_struct(Parser* p, Struct* res) {
  Token tok = parser_peek_token(p);
  if(!token_eq_keyword(&tok, "struct")) return false;
  parser_transfer_token(p, &res->tokens);

  tok = parser_peek_token(p);
  if(tok.kind == TOKEN_WORD) {
    res->name = tok.data;
    Struct* val = shget(p->structs, res->name);
    if(val == NULL || val->tokens_members_pos == 0) {
      shput(p->structs, res->name, res);
    }
    parser_transfer_token(p, &res->tokens);

    tok = parser_peek_token(p);
    if(!token_eq_char(&tok, '{')) goto no_body;
  } else if(!token_eq_char(&tok, '{')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '{' but found '%s'", tok.data);
    exit(1);
  }
  parser_transfer_token(p, &res->tokens);
  res->tokens_members_pos = arrlen(res->tokens);

  parse_declaration_list_until(p, &res->tokens, &res->members, '}');

no_body:;
  parser_parse_struct_subtypes(p, res, false);

  return true;
}

bool parser_parse_type(Parser* p, Struct* res) {
  if(parser_parse_struct(p, res)) {
    res->tokens_modifier_pos = arrlen(res->tokens);
    consume_type_modifiers(p, &res->tokens, false, false);
    return true;
  }

  Token tok = parser_peek_token(p);
  if(!is_typename(p, tok.data, NULL)) {
    return false;
  }

  Struct* truetype = shget(p->typedefs, tok.data);
  if(truetype != NULL) {
    res->aliased_to = truetype;
    parser_transfer_token(p, &res->tokens);
    parser_parse_struct_subtypes(p, res, true);
    res->tokens_modifier_pos = arrlen(res->tokens);
  }

  consume_type_modifiers(p, &res->tokens, true, truetype != NULL);
  res->is_primitive = true;
  return true;
}

Declaration parser_force_declaration(Parser* p) {
  Token tok = parser_peek_token(p);
  Declaration res = {0};
  res.location = tok.location;
  res.type = calloc(1, sizeof(Struct));

  if(!parser_parse_type(p, res.type)) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected type name, but found '%s'", tok.data);
    exit(1);
  }

  tok = parser_peek_token(p);
  if(tok.kind == TOKEN_WORD) {
    res.name = tok.data;
    parser_transfer_token(p, &res.tokens);
  } else {
    // TODO: headers have optional names
    token_print_error(&tok, LOGLEVEL_ERROR, "expected parameter name, but found '%s'", tok.data);
    exit(1);
  }

  return res;
}

Declaration parser_parse_declaration(Parser* p) {
  Declaration res = {0};
  res.location = parser_peek_token(p).location;
  res.type = calloc(1, sizeof(Struct));

  Token name = {0};
  if(!parser_parse_type(p, res.type)) {
    free(res.type);
    res.type = NULL;
  } else {
    name = parser_peek_token(p);
    if(name.kind == TOKEN_WORD) {
      res.name = name.data;
    }
  }
  skip_bracket_block(p, &res.tokens, ';', '\0');

  if(res.name == NULL && arrlen(res.tokens) > 1) {
    Token last = res.tokens[arrlen(res.tokens) - 2];
    if(last.kind == TOKEN_WORD) {
      res.name = last.data;
    } else {
      if(name.data && !token_eq_char(&name, ';')) {
        token_print_error(&name, LOGLEVEL_WARNING, "expected declaration name, but found '%s'", name.data);
      }
      token_print_error(&last, LOGLEVEL_WARNING, "extracting the names of function pointers is not implemented yet%s", "");
    }
  }

  return res;
}

bool parser_parse_variable(Parser* p, Declaration* var) {
  Token* tmp_storage = NULL;
  bool result = false;

  parser_transfer_token(p, NULL);
  Token tok = parser_peek_token(p);
  if(tok.kind != TOKEN_CHAR) goto defer;

  if(token_eq_char(&tok, '!')) {
    parser_transfer_token(p, &tmp_storage);
    Token last = var->type->tokens[arrlen(var->type->tokens)];
    if(token_eq_char(&last, '*')) {
      token_emit_cstr("->tail", p->default_emitter);
    } else {
      token_emit_cstr(".tail", p->default_emitter);
    }
    result = true;
    goto defer;
  }

  if(token_eq_char(&tok, '.')) {
    Token last = var->type->tokens[arrlen(var->type->tokens)];
    if(token_eq_char(&last, '*')) {
      token_emit_cstr("->head", p->default_emitter);
    } else {
      token_emit_cstr(".head", p->default_emitter);
    }
  }

  if(!token_eq_char(&tok, '-')) goto defer;
  parser_transfer_token(p, &tmp_storage);
  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '>')) goto defer;
  parser_transfer_token(p, &tmp_storage);
  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '!')) goto defer;
  parser_transfer_token(p, &tmp_storage);

  // TODO: check if field actually exists
  token_emit_cstr("->tail.", p->default_emitter);

  result = true;
defer:;
  if(!result) {
    token_print_error(&tok, LOGLEVEL_WARNING, "expected '!' or '->!', but found '%s'", tok.data);
  }
  for(int i = 0; i < arrlen(tmp_storage); i++) {
    if(!result) {
      token_emit(&tmp_storage[i], p->default_emitter);
    }
    free(tmp_storage[i].data);
  }
  arrfree(tmp_storage);
  return result;
}

void parser_inside_function(Parser* p, Function* func) {
  int rec_level = 0;
  bool keep_looping = true;

  while(keep_looping) {
    Token tok = parser_peek_token(p);
    if(tok.kind == TOKEN_EOF) {
      token_print_error(&tok, LOGLEVEL_ERROR, "expected end of function code but found EOF%s", "");
      exit(1);
    } else if(tok.kind == TOKEN_CHAR) {
      switch(*tok.data) {
        bracket_lookup_switch(rec_level);
      }
      if(rec_level == 0) keep_looping = false;
    } else if(tok.kind == TOKEN_WORD) {
      Declaration* var;
      if(shget(p->polymorphic_functions, tok.data)) {
        // todo: parse_function_call()
        token_print_error(&tok, LOGLEVEL_INFO, "calling functuons is not implemented yet%s", "");
      } else if((var = get_scope_variable(func, tok.data))) {
        parser_parse_variable(p, var);
      }
    }

    parser_transfer_token(p, NULL);
  }
}

bool parser_parse_function(Parser* p, Function* func) {
  Struct* tmp = calloc(1, sizeof(Struct));
  if(!parser_parse_type(p, tmp)) goto defer_type;
  func->decl.type = tmp;

  Token tok = parser_peek_token(p);
  if(tok.kind == TOKEN_WORD) {
    func->decl.name = tok.data;
    parser_transfer_token(p, &func->decl.tokens);
  } else {
    token_print_error(&tok, LOGLEVEL_INFO, "expected function name, but found '%s'", tok.data);
    goto defer;
  }

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '<') && p->allow_fancy_structs) {
    shput(p->polymorphic_functions, func->decl.name, func);
    parser_transfer_token(p, &func->decl.tokens);
    func->tokens_prams_pos = arrlen(func->decl.tokens);
    while(1) {
      arrpush(func->fancy_params, parser_force_declaration(p));

      tok = parser_peek_token(p);
      if(token_eq_char(&tok, '>')) {
        parser_transfer_token(p, &func->decl.tokens);
        break;
      } else if(token_eq_char(&tok, ',')) {
        parser_transfer_token(p, &func->decl.tokens);
      } else {
        token_print_error(&tok, LOGLEVEL_ERROR, "expected ',' or '>', but found '%s'", tok.data);
        exit(1);
      }
    }
  }

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '(')) {
    func->tokens_args_pos = arrlen(func->decl.tokens);
    skip_bracket_block(p, &func->decl.tokens, ')', '\0');
  } else {
    goto defer;
  }

  generate_converted_function_name(func);

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '{')) {
    declaration_emit_function(func, p->decl_emitter);
    if(p->go_deeper) {
      parser_inside_function(p, func);
    } else {
      skip_bracket_block(p, NULL, '}', '\0');
    }
    return true;
  } else if(token_eq_char(&tok, ';')) {
    parser_transfer_token(p, &func->decl.tokens);
    func->is_header = true;
  } else if(token_eq_char(&tok, '=')) {
    token_print_error(&tok, LOGLEVEL_INFO, "abstract '= 0' headers are not implemented yet%s", "");
  } else {
    // there is a lot of __attribute__ and __asm__
    // this is fine 🔥
    func->is_header = false;
  }

  declaration_emit_function(func, p->decl_emitter);
  return true;
defer:;
  declaration_emit(&func->decl, p->decl_emitter);
defer_type:;
  func->decl.type = NULL;
  declaration_delete_function(func);
  declaration_delete_struct(tmp);
  return false;
}

bool parser_parse_line(Parser* p) {
  Token tok = parser_peek_token(p);
  if(tok.kind == TOKEN_EOF) return false;

  if(token_eq_keyword(&tok, "typedef")) {
    parser_transfer_token(p, NULL);
    Declaration d = parser_parse_declaration(p);
    d.location = tok.location;
    d.is_typedef = true;
    if(d.name) {
      shput(p->typedefs, d.name, d.type);
    }
    arrpush(p->top_level, d);
    declaration_emit(&d, p->decl_emitter);
  } else if(token_eq_keyword(&tok, "struct")) {
    Declaration d = parser_parse_declaration(p);
    arrpush(p->top_level, d);
    declaration_emit(&d, p->decl_emitter);
  } else {
    Function* func = calloc(1, sizeof(Function));
    if(!parser_parse_function(p, func)) {
      skip_bracket_block(p, NULL, ';', '}');
    } else {
      arrpush(p->funcs, func);
    }
  }

  return true;
}

void parser_read_file(Parser* p, const char* filename) {
  lexer_open_file(&p->lexer, filename);
  while(parser_parse_line(p)) {
  }
  if(p->default_emitter) {
    token_emit(&(Token){.kind = TOKEN_EOF}, p->default_emitter);
  }
}

void parser_emit_declarations(Parser* p, Emitter* emitter) {
  bool restore_drel = emitter->delete_repeted_empty_lines;
  emitter->delete_repeted_empty_lines = true;

  for(int i = 0; i < arrlen(p->top_level); i++) {
    emitter->ignore_next_indent = true;
    if(p->top_level[i].is_typedef) {
      Token t = token_from_keyword("typedef");
      t.location = p->top_level[i].location;
      token_emit(&t, emitter);
      free(t.data);
    }
    declaration_emit(&p->top_level[i], emitter);
  }

  token_emit(&(Token){.kind = TOKEN_EOF}, emitter);
  emitter->delete_repeted_empty_lines = restore_drel;
}

void parser_emit_functions(Parser* p, Emitter* emitter) {
  bool restore_drel = emitter->delete_repeted_empty_lines;
  emitter->delete_repeted_empty_lines = true;

  for(int i = 0; i < arrlen(p->funcs); i++) {
    declaration_emit_function(p->funcs[i], emitter);
    if(!p->funcs[i]->is_header) {
      token_emit_cstr(";", emitter);
    }
  }

  token_emit(&(Token){.kind = TOKEN_EOF}, emitter);
  emitter->delete_repeted_empty_lines = restore_drel;
}

void parser_emit_typedefs(Parser* p, Emitter* emitter, bool print_unknowns) {
  token_emit(&(Token){.kind = TOKEN_EOF}, emitter);

  for(int i = 0; i < shlen(p->structs); i++) {
    fprintf(emitter->file, "struct %s;\n", p->structs[i].key);
  }

  for(int i = 0; i < shlen(p->typedefs); i++) {
    Struct* struc = p->typedefs[i].value;
    if(struc == NULL) {
      if(print_unknowns) {
        fprintf(emitter->file, "typedef ... %s;\n", p->typedefs[i].key);
      }
    } else if(struc->is_primitive) {
      emitter->cursor.filename = NULL;
      token_emit_cstr("typedef", emitter);
      emitter->ignore_next_indent = true;
      for(int j = 0; j < arrlen(struc->tokens); j++) {
        token_emit(&struc->tokens[j], emitter);
      }
      token_emit_cstr(" ", emitter);
      token_emit_cstr(p->typedefs[i].key, emitter);
      token_emit_cstr(";\n", emitter);
    } else if(print_unknowns || struc->name != NULL) {
      fprintf(emitter->file, "typedef struct %s %s;\n", struc->name, p->typedefs[i].key);
    }
  }
}

void parser_delete(Parser* p) {
  for(int i = 0; i < arrlen(p->top_level); i++) {
    declaration_delete(&p->top_level[i]);
  }
  arrfree(p->top_level);

  for(int i = 0; i < arrlen(p->funcs); i++) {
    declaration_delete_function(p->funcs[i]);
  }
  arrfree(p->funcs);

  shfree(p->typedefs);
  shfree(p->structs);
  shfree(p->defined_specialized_structs);
  shfree(p->polymorphic_functions);
  lexer_delete(&p->lexer);
}
