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

static void skip_bracket_block(Parser* p, Token** res, char end, char end2, int rec_level) {
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
  if(num_parametrised_types == 0 && !func->is_default_implementation) return;

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

static char* generate_function_table_name(Function* func, char* prefix) {
  if(func->fancy_params == NULL) return NULL;
  char* res = NULL;

  strappend(res, MANGLED_NAME_PREFIX);
  strappend(res, prefix);
  strappend(res, func->decl.name);
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    strappend(res, "_");
    strappend(res, get_struct_name(func->fancy_params[i].type, false));
  }
  arrpush(res, '\0');

  return res;
}

static char* generate_struct_prefixed_name(Struct* s, char* prefix, bool ignore_parameter) {
  char* res = NULL;

  strappend(res, MANGLED_NAME_PREFIX);
  strappend(res, prefix);
  strappend(res, get_struct_name(s, false));

  if(!ignore_parameter) {
    strappend(res, "_");
    strappend(res, get_struct_name(s->parameter, true));
  }
  arrpush(res, '\0');

  return res;
}

static void generate_converted_struct_name(Struct* s) {
  if(s->tag_counter_name == NULL) {
    s->tag_counter_name = generate_struct_prefixed_name(s, "tag_counter_", true);
  }

  if(s->parameter == NULL) return;

  if(s->tag_value_name == NULL) {
    s->tag_value_name = generate_struct_prefixed_name(s, "tag_value_", false);
  }

  if(s->converted_name == NULL) {
    s->converted_name = generate_struct_prefixed_name(s, "", false);
  }
}

static void generate_function_names(Function* func) {
  if(func->table_name) return;
  func->table_name = generate_function_table_name(func, "table_");
  func->table_count_name = generate_function_table_name(func, "table_count_");
  func->function_pointer_type = generate_function_table_name(func, "pointer_");
}

static Struct* get_scope_variable(Function* func, char* word) {
  Struct* type = shget(func->scope, word);
  if(type) {
    return type;
  }

  // linear search here is fine.. i think
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    if(strcmp(func->fancy_params[i].name, word) == 0) {
      return func->fancy_params[i].type;
    }
  }

  return NULL;
}

static void free_tmp_storage(Token* tmp_storage, Emitter* emitter) {
  for(int i = 0; i < arrlen(tmp_storage); i++) {
    token_emit(&tmp_storage[i], emitter);
    free(tmp_storage[i].data);
  }
  arrfree(tmp_storage);
}

static void emit_int(char* name, int i, Emitter* emitter) {
  char buff[11] = {0};
  snprintf(buff, sizeof(buff), "%d", i);

  token_emit_cstr("int ", emitter);
  token_emit_cstr(name, emitter);
  token_emit_cstr(" = ", emitter);
  token_emit_cstr(buff, emitter);
  token_emit_cstr(";\n", emitter);
}

static void emit_extern_function_table(Emitter* emitter, Function* func, bool use_extern) {
  token_emit(&(Token){.kind = TOKEN_EOF}, emitter);
  token_emit_cstr("typedef", emitter);
  emitter->ignore_next_newline = true;
  declaration_emit_struct(func->decl.type, emitter);
  token_emit_cstr("(*", emitter);
  token_emit_cstr(func->function_pointer_type, emitter);
  token_emit_cstr(")", emitter);
  declaration_emit_function_arguments(func, emitter, false);
  if(func->is_abstract_header) {
    token_emit_cstr(");", emitter);
  }

  if(use_extern) {
    token_emit_cstr("extern ", emitter);
  }
  token_emit_cstr(func->function_pointer_type, emitter);
  token_emit_cstr("* ", emitter);
  token_emit_cstr(func->table_name, emitter);
  if(!use_extern) {
    token_emit_cstr(" = (void*)0", emitter);
  }
  token_emit_cstr(";", emitter);
}

static void emit_table_and_count_externs(Parser* p, Function* func) {
  func->is_header = true;

  if(p->keep_abstract_headers == true) return;
  func->is_abstract_header = true;
  generate_function_names(func);

  if(p->extra_emitter == NULL) return;

  emit_extern_function_table(p->extra_emitter, func, !p->use_constructors);

  if(p->use_constructors) {
    emit_int(func->table_count_name, 0, p->extra_emitter);
  } else {
    token_emit_cstr("extern int ", p->extra_emitter);
    token_emit_cstr(func->table_count_name, p->extra_emitter);
    token_emit_cstr(";", p->extra_emitter);
  }
}

static void emit_tag_values(Struct* s, Emitter* emitter) {
  for(int i = 0; i < arrlen(s->tag_names); i++) {
    emit_int(s->tag_names[i], i, emitter);
  }
}

static void emit_tag_value_constructor(char* tagname, char* counter, Emitter* emitter) {
  token_emit_cstr("void __attribute__((weak, constructor(101))) _constructor", emitter);
  token_emit_cstr(tagname, emitter);
  token_emit_cstr("() {\n  ", emitter);
  token_emit_cstr(tagname, emitter);
  token_emit_cstr(" = ", emitter);
  token_emit_cstr(counter, emitter);
  token_emit_cstr("++;\n", emitter);
  token_emit_cstr("}\n\n", emitter);
}

static void emit_function_table(Parser* p, Function* func, Emitter* emitter) {
  token_emit_cstr(func->function_pointer_type, emitter);
  token_emit_cstr("* ", emitter);
  token_emit_cstr(func->table_name, emitter);
  token_emit_cstr(" = (", emitter);
  token_emit_cstr(func->function_pointer_type, emitter);
  token_emit_cstr("[]){\n", emitter);

  int count = 0;
  int rec_lvl = 0;
  Struct* base = shget(p->structs, get_struct_name(func->fancy_params[rec_lvl].type, false));
  rec_lvl++;

  for(int i = 0; i < arrlen(base->tag_names); i++) {
    if(rec_lvl < arrlen(func->fancy_params)) {
      // todo: recursion
      // count += rec
      token_print_error(func->decl.tokens, LOGLEVEL_WARNING, "multimethod tables are not implemented yet%s", "");
      break;
    } else {
      count++;
      Function* impl = shget(func->implementations, base->tag_names[i]);
      if(impl == NULL) {
        impl = func->default_implementation;
      }
      if(impl == NULL) {
        token_emit_cstr("(void*)0,\n", emitter);
      } else {
        token_emit_cstr("(void*)&", emitter);
        token_emit_cstr(impl->converted_name, emitter);
        token_emit_cstr(",\n", emitter);
      }
    }
  }

  token_emit_cstr("};\n", emitter);
  emit_int(func->table_count_name, count, emitter);
}

static void emit_function_table_constructor(Parser* p, Function* func, Emitter* emitter) {
  token_emit_cstr("void __attribute__((weak, constructor(102))) _constructor", emitter);
  token_emit_cstr(func->table_name, emitter);
  token_emit_cstr("() {\n  ", emitter);

  token_emit_cstr("int size = ", emitter);
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    if(i > 0) {
      token_emit_cstr(" * ", emitter);
    }

    Struct* base = shget(p->structs, get_struct_name(func->fancy_params[i].type, false));
    token_emit_cstr(base->tag_counter_name, emitter);
  }
  token_emit_cstr(";\n  ", emitter);

  token_emit_cstr(func->table_name, emitter);
  token_emit_cstr(" = calloc(size, sizeof(void*));\n", emitter);

  if(func->default_implementation) {
    token_emit_cstr("  for(int i = 0; i < size; i++){\n    ", emitter);
    token_emit_cstr(func->table_name, emitter);
    token_emit_cstr("[i] = (void*)&", emitter);
    token_emit_cstr(func->default_implementation->converted_name, emitter);
    token_emit_cstr(";\n  }\n", emitter);
  }

  token_emit_cstr("}\n\n", emitter);
}

static void emit_function_constructor(Function* func, Emitter* emitter) {
  if(func->fancy_params == NULL) return;
  if(func->base == NULL) return;
  if(func->is_default_implementation) return;

  if(func->base->table_name == NULL) {
    generate_function_names(func->base);
    emit_extern_function_table(emitter, func->base, true);
  }

  token_emit_cstr("\nvoid __attribute__((weak, constructor(103))) _constructor", emitter);
  token_emit_cstr(func->converted_name, emitter);
  token_emit_cstr("() {\n  ", emitter);
  token_emit_cstr(func->base->table_name, emitter);

  token_emit_cstr("[", emitter);
  for(int i = 0; i < arrlen(func->fancy_params); i++) {
    if(i != 0) {
      token_emit_cstr("+", emitter);
    }
    token_emit_cstr(func->fancy_params[i].type->tag_value_name, emitter);
    for(int j = 0; j < i; j++) {
      token_emit_cstr("*", emitter);
      token_emit_cstr(func->fancy_params[j].type->tag_counter_name, emitter);
    }
  }
  token_emit_cstr("]", emitter);

  token_emit_cstr(" = &", emitter);
  token_emit_cstr(func->converted_name, emitter);
  token_emit_cstr(";\n", emitter);
  token_emit_cstr("}\n\n", emitter);
}

static bool check_valid_subtype(Struct* base, Struct* parameterized) {
  char* name1 = get_struct_name(parameterized->parameter, true);
  for(int i = 0; i < arrlen(base->subtypes); i++) {
    char* name2 = get_struct_name(base->subtypes[i].type, true);
    if(strcmp(name1, name2) == 0) return true;
  }

  Token* tokens = parameterized->parameter->tokens;
  Token* last = &tokens[arrlen(tokens) - 1];
  token_print_error(last, LOGLEVEL_ERROR, "invalid parameter value '%s'", last->data);

  return false;
}

static void emit_struct_counter_name(Parser* p, Struct* s) {
  generate_converted_struct_name(s);
  token_emit_cstr("int __attribute__((weak)) ", p->extra_emitter);
  token_emit_cstr(s->tag_counter_name, p->extra_emitter);
  token_emit_cstr(" = 0;\n", p->extra_emitter);
}

static void try_emit_parameterized_struct(Parser* p, Struct* res) {
  generate_converted_struct_name(res);

  if(p->defined_specialized_structs == NULL) {
    sh_new_arena(p->defined_specialized_structs);
  }

  if(shget(p->defined_specialized_structs, res->converted_name) == NULL) {
    Struct* base = shget(p->structs, get_struct_name(res, false));
    if(check_valid_subtype(base, res)) {
      arrpush(base->tag_names, res->tag_value_name);
      declaration_emit_parameter_struct(res, p->extra_emitter, base, p->use_constructors);
    }
    shput(p->defined_specialized_structs, res->converted_name, res);
  }
}

static bool preemit_subtypes(Parser* p, Struct* res, int start_from) {
  if(!p->preemit_structs) return false;
  if(!res) return false;
  if(!res->tokens_members_pos) return false;
  if(!res->subtypes) return false;
  if(res->parameter) return false;

  for(int i = start_from; i < arrlen(res->subtypes); i++) {
    Struct* tmp = calloc(1, sizeof(Struct));
    tmp->name = res->name;
    tmp->tokens = res->tokens;
    tmp->parameter = res->subtypes[i].type;
    try_emit_parameterized_struct(p, tmp);

    tmp->parameter = NULL;
    tmp->tokens = NULL;
    arrpush(res->expanded_subtypes, tmp);
  }

  if(start_from == 0) {
    generate_converted_struct_name(res);
    token_emit_cstr("extern int ", p->extra_emitter);
    token_emit_cstr(res->tag_counter_name, p->extra_emitter);
    token_emit_cstr(";", p->extra_emitter);
  }

  return true;
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
  try_emit_parameterized_struct(p, res);
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
    if(token_eq_char(&tok, '+') && val) {
      parser_parse_type_extention(p, val);
      return false;
    } else if(!token_eq_char(&tok, '{')) {
      goto no_body;
    }
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

  generate_converted_struct_name(res.type);
  return res;
}

Declaration parser_parse_declaration(Parser* p) {
  Declaration res = {0};
  res.location = parser_peek_token(p).location;
  res.type = calloc(1, sizeof(Struct));

  Token name = {0};
  if(!parser_parse_type(p, res.type)) {
    declaration_delete_struct(res.type);
    res.type = NULL;
  } else {
    name = parser_peek_token(p);
    if(name.kind == TOKEN_WORD) {
      res.name = name.data;
    }
  }
  skip_bracket_block(p, &res.tokens, ';', '\0', 0);

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

bool parser_parse_variable(Parser* p, Struct* type) {
  Token* tmp_storage = NULL;
  bool result = false;

  parser_transfer_token(p, NULL);
  Token tok = parser_peek_token(p);
  if(tok.kind != TOKEN_CHAR) goto defer;

  if(token_eq_char(&tok, ';')) {
    result = true;
    goto defer;
  }

  if(token_eq_char(&tok, '!')) {
    parser_transfer_token(p, &tmp_storage);
    Token last = type->tokens[arrlen(type->tokens)];
    if(token_eq_char(&last, '*')) {
      token_emit_cstr("->tail", p->default_emitter);
    } else {
      token_emit_cstr(".tail", p->default_emitter);
    }
    tok = parser_peek_token(p);
    if(tok.kind == TOKEN_WORD) {
      token_emit_cstr(".", p->default_emitter);
    }
    result = true;
    goto defer;
  }

  if(token_eq_char(&tok, '.')) {
    Token last = type->tokens[arrlen(type->tokens)];
    if(token_eq_char(&last, '*')) {
      token_emit_cstr("->head", p->default_emitter);
    } else {
      token_emit_cstr(".head", p->default_emitter);
    }
    result = true;
    goto defer;
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
  free_tmp_storage(tmp_storage, result ? NULL : p->default_emitter);
  return result;
}

bool parser_parse_function_call(Parser* p) {
  parser_transfer_token(p, NULL);
  Token tok = parser_peek_token(p);

  if(token_eq_char(&tok, '(')) {
    token_print_error(&tok, LOGLEVEL_WARNING, "calling a parametric function like a normal one%s", "");
    return false;
  } else if(!token_eq_char(&tok, '<')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '<' or '(', but found '%s'", tok.data);
    return false;
  }

  Token* prams = NULL;
  parser_transfer_token(p, &prams);
  skip_bracket_block(p, &prams, '>', '\0', 0);

  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '(')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected function arguments, but found '%s'", tok.data);
    free_tmp_storage(prams, NULL);
    return false;
  }

  parser_transfer_token(p, NULL);
  for(int i = 1; i < arrlen(prams) - 1; i++) {
    token_emit(&prams[i], p->extra_emitter);
  }
  free_tmp_storage(prams, NULL);
  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, ')')) {
    token_emit_cstr(",", p->extra_emitter);
  }
  skip_bracket_block(p, NULL, ')', ';', 1);

  return true;
}

bool parser_parse_initializer(Parser* p, Function* func, Struct* type) {
  Token tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '{')) {
    return false;
  }

  token_emit_cstr("{.tag = ", p->extra_emitter);
  token_emit_cstr(type->tag_value_name, p->extra_emitter);
  token_emit_cstr(", .head = ", p->extra_emitter);
  skip_bracket_block(p, NULL, '}', ';', 0);

  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '<')) {
    token_emit_cstr("}", p->extra_emitter);
    return true;
  }

  Token* prams = NULL;
  parser_transfer_token(p, &prams);
  token_emit_cstr(", .tail = ", p->extra_emitter);
  parser_inside_function(p, func);

  tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '>')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '>', but found '%s'", tok.data);
    return true;
  }

  parser_transfer_token(p, &prams);
  free_tmp_storage(prams, NULL);
  token_emit_cstr("}", p->extra_emitter);

  return true;
}

bool parser_parse_local_variable(Parser* p, Function* func) {
  char* name = NULL;
  Struct* type = calloc(1, sizeof(Struct));
  if(!parser_parse_type(p, type)) goto defer;
  declaration_emit_struct(type, p->decl_emitter);

  if(type->parameter == NULL) goto defer;

  Token tok = parser_peek_token(p);
  if(tok.kind == TOKEN_WORD) {
    name = strdup(tok.data);
    parser_transfer_token(p, NULL);
  } else {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected variable name, but found '%s'", tok.data);
    goto defer;
  }

  if(shget(func->scope, name)) {
    token_print_error(&tok, LOGLEVEL_WARNING, "attempting to redefine local variable '%s'", tok.data);
    goto defer;
  } else {
    shput(func->scope, name, type);
  }

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '=')) {
    parser_transfer_token(p, NULL);
    parser_parse_initializer(p, func, type);
  } else {
    token_emit_cstr("= {.tag = ", p->extra_emitter);
    token_emit_cstr(type->tag_value_name, p->extra_emitter);
    token_emit_cstr("}", p->extra_emitter);
  }

  return true;
defer:;
  free(name);
  declaration_delete_struct(type);
  return false;
}

void parser_inside_function(Parser* p, Function* func) {
  int rec_level = 0;
  bool keep_looping = true;

  while(keep_looping) {
    Token tok = parser_peek_token(p);
    if(tok.kind == TOKEN_EOF) {
      token_print_error(&tok, LOGLEVEL_ERROR, "expected end of function code but found EOF%s", "");
      token_print_error(&func->decl.tokens[0], LOGLEVEL_INFO, "in function '%s'", func->decl.name);
      exit(1);
    } else if(tok.kind == TOKEN_CHAR) {
      switch(*tok.data) {
        bracket_lookup_switch(rec_level);
      }
      if(rec_level == 0) keep_looping = false;
    } else if(tok.kind == TOKEN_WORD) {
      Struct* var;
      if(shget(p->polymorphic_functions, tok.data)) {
        parser_parse_function_call(p);
      } else if(shget(p->typedefs, tok.data)) {
        parser_parse_local_variable(p, func);
      } else if((var = get_scope_variable(func, tok.data))) {
        parser_parse_variable(p, var);
      }

      tok = parser_peek_token(p);
      if(tok.kind != TOKEN_WORD) continue;
    }

    parser_transfer_token(p, NULL);
  }
}

bool parser_parse_type_extention(Parser* p, Struct* base) {
  bool result = true;
  Token* tmp_storage = NULL;
  parser_transfer_token(p, &tmp_storage);

  Token tok = parser_peek_token(p);
  if(!token_eq_char(&tok, '<')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '<', but found '%s'", tok.data);
    goto defer;
  }
  parser_transfer_token(p, &tmp_storage);

  int old_subtypes_count = arrlen(base->subtypes);
  parse_declaration_list_until(p, &tmp_storage, &base->subtypes, '>');
  preemit_subtypes(p, base, old_subtypes_count);

  result = true;
defer:;
  free_tmp_storage(tmp_storage, result ? NULL : p->default_emitter);
  return result;
}

bool parser_parse_function(Parser* p, Function* func) {
  Struct* tmp = calloc(1, sizeof(Struct));
  if(!parser_parse_type(p, tmp)) goto defer_type;
  func->decl.type = tmp;

  Token tok = parser_peek_token(p);
  if(tok.kind == TOKEN_WORD) {
    func->decl.name = tok.data;
    parser_transfer_token(p, &func->decl.tokens);
  } else if(token_eq_char(&tok, '+')) {
    Struct* real_type = func->decl.type->aliased_to;
    if(real_type == NULL) {
      token_print_error(&tok, LOGLEVEL_WARNING, "trying to expand non parametric type '%s'", func->decl.type->name);
      goto defer;
    }

    parser_parse_type_extention(p, real_type);
    goto defer_type;
  } else {
    token_print_error(&tok, LOGLEVEL_INFO, "expected function name, but found '%s'", tok.data);
    goto defer;
  }

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '<') && p->allow_fancy_structs) {
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
    skip_bracket_block(p, &func->decl.tokens, ')', '\0', 0);
    func->has_normal_prams = func->tokens_args_pos + 2 != arrlen(func->decl.tokens);
  } else {
    goto defer;
  }

  tok = parser_peek_token(p);
  if(func->tokens_prams_pos) {
    Function* val = shget(p->polymorphic_functions, func->decl.name);
    if(token_eq_char(&tok, '=') || token_eq_char(&tok, ';')) {
      if(!val || !val->is_abstract_header) {
        shput(p->polymorphic_functions, func->decl.name, func);
      }
    } else if(val) {
      // TODO: func->converted_name or...
      char* name = func->fancy_params[0].type->tag_value_name;
      if(name == NULL) {
        val->default_implementation = func;
        func->is_default_implementation = true;
      } else {
        shput(val->implementations, name, func);
      }
      func->base = val;
    } else {
      token_print_error(&tok, LOGLEVEL_ERROR, "parametric function defined before its header%s", "");
    }
  }

  generate_converted_function_name(func);

  if(token_eq_char(&tok, '{')) {
    declaration_emit_function(func, p->decl_emitter);
    if(p->go_deeper) {
      parser_inside_function(p, func);
    } else {
      skip_bracket_block(p, NULL, '}', '\0', 0);
    }
    if(p->use_constructors) {
      emit_function_constructor(func, p->extra_emitter);
    }
    return true;
  } else if(token_eq_char(&tok, ';')) {
    parser_transfer_token(p, &func->decl.tokens);
    func->is_header = true;
  } else if(token_eq_char(&tok, '=')) {
    emit_table_and_count_externs(p, func);
    skip_bracket_block(p, &func->decl.tokens, ';', '\0', 0);
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
    preemit_subtypes(p, d.type, 0);
  } else if(token_eq_keyword(&tok, "struct")) {
    Declaration d = parser_parse_declaration(p);
    arrpush(p->top_level, d);
    declaration_emit(&d, p->decl_emitter);
  } else {
    Function* func = calloc(1, sizeof(Function));
    if(!parser_parse_function(p, func)) {
      skip_bracket_block(p, NULL, ';', '}', 0);
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

void parser_set_emitter(Parser* p, Emitter* emitter) {
  p->decl_emitter = emitter;
  p->default_emitter = emitter;
  p->extra_emitter = emitter;
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

void parser_emit_final_tables(Parser* p, Emitter* emitter) {
  if(p->use_constructors) {
    parser_emit_final_constructors(p, emitter);
    return;
  }

  for(int i = 0; i < shlen(p->structs); i++) {
    emit_tag_values(p->structs[i].value, emitter);
  }

  for(int i = 0; i < shlen(p->polymorphic_functions); i++) {
    emit_function_table(p, p->polymorphic_functions[i].value, emitter);
  }
}

void parser_emit_final_constructors(Parser* p, Emitter* emitter) {
  token_emit_cstr("void* calloc(unsigned long nmemb, unsigned long size);\n", emitter);

  for(int i = 0; i < shlen(p->structs); i++) {
    Struct* s = p->structs[i].value;
    if(s->tokens_subtypes_pos) {
      emit_struct_counter_name(p, s);
    }

    for(int j = 0; j < arrlen(s->tag_names); j++) {
      emit_tag_value_constructor(s->tag_names[j], s->tag_counter_name, emitter);
    }
  }

  for(int i = 0; i < shlen(p->polymorphic_functions); i++) {
    if(!p->polymorphic_functions[i].value->is_abstract_header) continue;

    emit_function_table_constructor(p, p->polymorphic_functions[i].value, emitter);
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
