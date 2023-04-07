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
    shput(p->structs, res->name, res);
    parser_transfer_token(p, &res->tokens);

    tok = parser_peek_token(p);
    if(!token_eq_char(&tok, '{')) goto no_body;
  } else if(!token_eq_char(&tok, '{')) {
    token_print_error(&tok, LOGLEVEL_ERROR, "expected '{' but found '%s'", tok.data);
    exit(1);
  }
  parser_transfer_token(p, &res->tokens);
  res->tokens_header_len = arrlen(res->tokens);

  parse_declaration_list_until(p, &res->tokens, &res->members, '}');

no_body:;
  if(p->allow_fancy_structs) {
    tok = parser_peek_token(p);
    if(!token_eq_char(&tok, '<')) return true;
    parser_transfer_token(p, &res->tokens);

    parse_declaration_list_until(p, &res->tokens, &res->subtypes, '>');
  }

  return true;
}

Declaration parser_parse_declaration(Parser* p) {
  Declaration res = {0};
  res.location = parser_peek_token(p).location;
  res.type = calloc(1, sizeof(Struct));

  if(!parser_parse_struct(p, res.type)) {
    free(res.type);
    res.type = NULL;
  }
  skip_bracket_block(p, &res.tokens, ';', '\0');

  if(arrlen(res.tokens) > 1) {
    Token last = res.tokens[arrlen(res.tokens) - 2];
    if(last.kind == TOKEN_WORD) {
      res.name = last.data;
    } else {
      token_print_error(&last, LOGLEVEL_WARNING, "extracting the names of long declarations is not implemented yet%s", "");
    }
  }

  return res;
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
    skip_bracket_block(p, NULL, ';', '}');
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

  shfree(p->typedefs);
  shfree(p->structs);
  lexer_delete(&p->lexer);
}
