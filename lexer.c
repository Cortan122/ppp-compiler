#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool is_char_word(char c) {
  return isalnum(c) || c == '_' || c == '$' || (c & 0x80);
}

static bool is_char_number(char c) {
  return isalnum(c) || c == '.';
}

static bool is_char_space(char c) {
  return (uint8_t)c <= ' ';
}

static char escaped_chararter_map(char letter) {
  // clang-format off
  switch(letter) {
    case '\\': return '\\';
    case '\'': return '\'';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'e': return '\e';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '"': return '"';
    default: return '\0';
  }
  // clang-format on
}

static void get_location_from_preprocessor_output(const Token* tok, Lexer* res) {
  assert(tok->kind == TOKEN_PREPROCESSOR);
  assert(tok->data[0] == '#');

  Lexer lexer = {0};
  Token first = {0};
  Token second = {0};
  lexer_open_memory(&lexer, tok->data + 1, tok->length - 1);
  first = lexer_next_token(&lexer);
  if(first.kind != TOKEN_NUMBER) goto defer;

  da_append(&first, '\0');
  char* endptr;
  int line = (int)strtol(first.data, &endptr, 0);
  if(endptr != first.data + first.length - 1) {
    token_print_error(&first, LOGLEVEL_ERROR, "'%s' is not a valid line number", first.data);
    goto defer;
  }

  second = lexer_next_token(&lexer);
  da_append(&second, '\0');
  if(second.kind != TOKEN_LONGSTRING) {
    token_print_error(&second, LOGLEVEL_ERROR, "expected filename string, but found '%s'", second.data);
    goto defer;
  }

  char* new_filename = strdup(second.data);
  res->current_location.line_num = line - 1;
  res->current_location.filename = new_filename;
  da_append(&res->allocated_strings, new_filename);

defer:;
  free(first.data);
  free(second.data);
  lexer_delete(&lexer);
}

char lexer_next_char(Lexer* l) {
  if(l->file == NULL) {
    if(l->memory_length == 0) {
      l->exhausted = true;
      return '\0';
    }
    l->memory_length--;
    return *(l->memory++);
  }

  int next = fgetc(l->file);
  if(next == EOF) {
    l->exhausted = true;
    return '\0';
  }
  return (char)next;
}

char lexer_peek_char(Lexer* l) {
  if(l->exhausted) return '\0';
  if(l->peeked_char == '\0') {
    l->peeked_char = lexer_next_char(l);
  }
  return l->peeked_char;
}

char lexer_drop_char(Lexer* l) {
  char res = lexer_peek_char(l);

  l->current_location.byte_num++;
  if(res == '\n') {
    l->current_location.line_num++;
    l->current_location.col_num = 0;
    // TODO: check if prev char is a backslash
    l->is_newline = true;
  } else {
    l->current_location.col_num++;
    if(!is_char_space(res)) l->is_newline = false;
  }

  l->peeked_char = '\0';
  return res;
}

void lexer_lex_line(Lexer* l, Token* res) {
  while(!l->exhausted) {
    da_append(res, lexer_drop_char(l));
    if(res->data[res->length - 1] == '\n') {
      if(res->data[res->length - 2] != '\\') break;
      res->length -= 2;
    }
  }

  while(is_char_space(res->data[res->length - 1])) res->length--;
}

void lexer_lex_comment(Lexer* l, Token* res) {
  res->kind = TOKEN_CHAR;
  da_append(res, lexer_drop_char(l));
  char next2 = lexer_peek_char(l);

  if(next2 == '*') {
    res->kind = TOKEN_LONGCOMMENT;
    while(!l->exhausted) {
      da_append(res, lexer_drop_char(l));
      if(res->data[res->length - 1] == '/' && res->data[res->length - 2] == '*') {
        break;
      }
    }
  } else if(next2 == '/') {
    res->kind = TOKEN_SHORTCOMMENT;
    lexer_lex_line(l, res);
  }
}

void lexer_parse_escape_sequence(Lexer* l, Token* res) {
  char c = lexer_peek_char(l);
  switch(c) {
    case '\n': {
      lexer_drop_char(l);
      return;
    }
    case 'x': {
      lexer_drop_char(l);
      char hexstring[3] = {lexer_drop_char(l), lexer_drop_char(l), '\0'};
      da_append(res, (uint8_t)strtol(hexstring, NULL, 16));
      return;
    }
    case 'u': {
      token_print_error(res, LOGLEVEL_ERROR, "unicode escape sequences are not implemented yet", "");
      for(int i = 0; i < 5; i++) lexer_drop_char(l);
      return;
    }
    case 'U': {
      token_print_error(res, LOGLEVEL_ERROR, "unicode escape sequences are not implemented yet", "");
      for(int i = 0; i < 9; i++) lexer_drop_char(l);
      return;
    }
  }

  if(c >= '0' && c <= '7') {
    char octstring[4] = {0};
    int i = 0;
    while((c >= '0' && c <= '7') && i < 3) {
      octstring[i++] = lexer_drop_char(l);
      c = lexer_peek_char(l);
    }
    da_append(res, (uint8_t)strtol(octstring, NULL, 8));
    return;
  }

  char out = escaped_chararter_map(c);
  if(out == '\0') {
    token_print_error(res, LOGLEVEL_WARNING, "unknown escape char '%.1s'", &c);
    da_append(res, lexer_drop_char(l));
  } else {
    lexer_drop_char(l);
    da_append(res, out);
  }
}

void lexer_lex_string(Lexer* l, Token* res, char end) {
  lexer_drop_char(l);
  while(!l->exhausted) {
    char next = lexer_peek_char(l);
    if(next == '\\') {
      lexer_drop_char(l);
      lexer_parse_escape_sequence(l, res);
    } else if(next == end) {
      lexer_drop_char(l);
      break;
    } else {
      da_append(res, lexer_drop_char(l));
    }
  }
}

Token lexer_next_token(Lexer* l) {
  Token res = {0};

  while(is_char_space(lexer_peek_char(l))) {
    if(l->exhausted) return res;
    lexer_drop_char(l);
  }

  char next = lexer_peek_char(l);
  res.location = l->current_location;
  if('0' <= next && next <= '9') {
    res.kind = TOKEN_NUMBER;
    while(!l->exhausted && is_char_number(lexer_peek_char(l))) {
      da_append(&res, lexer_drop_char(l));
    }
  } else if(is_char_word(next)) {
    res.kind = TOKEN_WORD;
    while(!l->exhausted && is_char_word(lexer_peek_char(l))) {
      da_append(&res, lexer_drop_char(l));
    }
  } else if(next == '/') {
    lexer_lex_comment(l, &res);
  } else if(next == '"') {
    res.kind = TOKEN_LONGSTRING;
    lexer_lex_string(l, &res, '"');
  } else if(next == '\'') {
    res.kind = TOKEN_SHORTSTRING;
    lexer_lex_string(l, &res, '\'');
  } else if(next == '#') {
    if(!l->is_newline) {
      token_print_error(&res, LOGLEVEL_ERROR, "stray # in program%s", "");
      res.kind = TOKEN_INVALID;
      da_append(&res, lexer_drop_char(l));
    } else {
      res.kind = TOKEN_PREPROCESSOR;
      lexer_lex_line(l, &res);
      get_location_from_preprocessor_output(&res, l);
    }
  } else {
    res.kind = TOKEN_CHAR;
    da_append(&res, lexer_drop_char(l));
  }

  return res;
}

void lexer_reset(Lexer* l) {
  l->is_newline = true;
  l->current_location = (Loc){0};
  if(l->file) {
    fclose(l->file);
    l->file = NULL;
  }
}

void lexer_open_file(Lexer* l, const char* filename) {
  lexer_reset(l);
  l->current_location.filename = filename;
  l->file = fopen(filename, "r");
  if(l->file == NULL) {
    fprintf(stderr, "ERROR: opening file '%s': %s\n", filename, strerror(errno));
    exit(1);
  }
}

void lexer_open_memory(Lexer* l, const char* text, size_t length) {
  lexer_reset(l);
  l->current_location.filename = "<memory>";
  l->memory = text;
  l->memory_length = length;
}

void lexer_delete(Lexer* l) {
  if(l->file) {
    fclose(l->file);
    l->file = NULL;
  }
  for(size_t i = 0; i < l->allocated_strings.length; i++) {
    free(l->allocated_strings.data[i]);
  }
  free(l->allocated_strings.data);
}
