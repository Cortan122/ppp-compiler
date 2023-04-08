#include "parser.h"

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
        case '(':
        case '[':
        case '{':
          rec_level++;
          break;
        case ')':
        case ']':
        case '}':
          rec_level--;
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

static int consume_type_modifiers(Parser* p, Token** tok_dest, bool allow_words) {
  int res = 0;
  bool has_used_typedef = false;
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

void parser_transfer_token(Parser* p, Token** dest) {
  Token tok = lexer_drop_token(&p->lexer);
  // TODO: default emitter?
  if(dest) {
    arrpush(*dest, tok);
  } else {
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
  if(p->allow_fancy_structs) {
    tok = parser_peek_token(p);
    if(!token_eq_char(&tok, '<')) return true;
    parser_transfer_token(p, &res->tokens);
    res->tokens_subtypes_pos = arrlen(res->tokens);

    if(res->tokens_members_pos) {
      parse_declaration_list_until(p, &res->tokens, &res->subtypes, '>');
    } else {
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
    }
  }

  return true;
}

bool parser_parse_type(Parser* p, Struct* res) {
  if(parser_parse_struct(p, res)) {
    consume_type_modifiers(p, &res->tokens, false);
    return true;
  }

  Token tok = parser_peek_token(p);
  if(!is_typename(p, tok.data, NULL)) {
    return false;
  }

  consume_type_modifiers(p, &res->tokens, true);
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

bool parser_parse_function(Parser* p, Function* func) {
  Struct* tmp = calloc(1, sizeof(Struct));
  if(!parser_parse_type(p, tmp)) goto defer;

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
    parser_transfer_token(p, &func->decl.tokens);
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
    skip_bracket_block(p, &func->decl.tokens, ')', '\0');
  } else {
    goto defer;
  }

  tok = parser_peek_token(p);
  if(token_eq_char(&tok, '{')) {
    // TODO: we have to go deeper
    skip_bracket_block(p, NULL, '}', '\0');
  } else if(token_eq_char(&tok, ';')) {
    parser_transfer_token(p, &func->decl.tokens);
    func->is_header = true;
  }

  func->decl.type = tmp;
  return true;
defer:;
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
  } else if(token_eq_keyword(&tok, "struct")) {
    arrpush(p->top_level, parser_parse_declaration(p));
  } else {
    Function func = {0};
    if(!parser_parse_function(p, &func)) {
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

void parser_delete(Parser* p) {
  for(int i = 0; i < arrlen(p->top_level); i++) {
    declaration_delete(&p->top_level[i]);
  }
  arrfree(p->top_level);

  for(int i = 0; i < arrlen(p->funcs); i++) {
    declaration_delete_function(&p->funcs[i]);
  }
  arrfree(p->funcs);

  shfree(p->typedefs);
  shfree(p->structs);
  lexer_delete(&p->lexer);
}
