#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "token.h"

#define return_defer(value) \
  do {                      \
    result = (value);       \
    goto defer;             \
  } while(0)

#define OUTPUT_FOLDER "test outputs"
#define TESTCASE(x) \
  { .func = testfunc_##x, .name = #x }
#define TESTCASE_L(x) \
  { .func = testfunc_##x, .name = #x, .loop_check = true }
#define TESTCASE_LL(x) \
  { .func = testfunc_##x, .name = #x, .loop_check = true, .ignore_some_files = true }

typedef void (*TestFunc)(const char*);
typedef struct TestCase {
  TestFunc func;
  char* name;
  bool loop_check;
  bool ignore_some_files;
} TestCase;

void testfunc_debug_tokens(const char* input_file) {
  Lexer lexer = {0};
  lexer_open_file(&lexer, input_file);
  while(1) {
    Token t = lexer_next_token(&lexer);
    if(t.kind == TOKEN_EOF) break;
    token_print_debug(&t);
    free(t.data);
  }

  lexer_delete(&lexer);
}

void testfunc_declarations(const char* input_file) {
  Parser parser = {0};
  parser_read_file(&parser, input_file);

  for(int i = 0; i < arrlen(parser.top_level); i++) {
    declaration_print_debug(&parser.top_level[i], 0);
  }

  parser_delete(&parser);
}

void testfunc_named_types(const char* input_file) {
  Emitter em = {.file = stdout};
  Parser parser = {0};
  parser_read_file(&parser, input_file);
  parser_emit_typedefs(&parser, &em, true);

  parser_delete(&parser);
}

void testfunc_emit_tokens(const char* input_file) {
  Emitter em = {.file = stdout};
  Lexer lexer = {0};
  lexer_open_file(&lexer, input_file);
  while(1) {
    Token t = lexer_next_token(&lexer);
    token_emit(&t, &em);
    free(t.data);
    if(t.kind == TOKEN_EOF) break;
  }

  lexer_delete(&lexer);
}

void testfunc_emit_declarations(const char* input_file) {
  Emitter em = {.file = stdout};
  Parser parser = {0};
  parser_read_file(&parser, input_file);
  parser_emit_declarations(&parser, &em);

  parser_delete(&parser);
}

void testfunc_fancy_declarations(const char* input_file) {
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, input_file);

  for(int i = 0; i < arrlen(parser.top_level); i++) {
    declaration_print_debug(&parser.top_level[i], 0);
  }

  parser_delete(&parser);
}

void testfunc_emit_fancy_declarations(const char* input_file) {
  Emitter em = {.file = stdout};
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, input_file);
  parser_emit_declarations(&parser, &em);

  parser_delete(&parser);
}

void testfunc_convert_structs(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, input_file);
  parser_emit_declarations(&parser, &em);

  parser_delete(&parser);
}

void testfunc_functions(const char* input_file) {
  Parser parser = {0};
  parser_read_file(&parser, input_file);

  for(int i = 0; i < arrlen(parser.funcs); i++) {
    declaration_print_function(parser.funcs[i], 0);
  }

  parser_delete(&parser);
}

void testfunc_fancy_functions(const char* input_file) {
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, input_file);

  for(int i = 0; i < arrlen(parser.funcs); i++) {
    declaration_print_function(parser.funcs[i], 0);
  }

  parser_delete(&parser);
}

void testfunc_emit_functions(const char* input_file) {
  Emitter em = {.file = stdout};
  Parser parser = {0};
  parser_read_file(&parser, input_file);
  parser_emit_typedefs(&parser, &em, false);
  parser_emit_functions(&parser, &em);

  parser_delete(&parser);
}

void testfunc_emit_fancy_functions(const char* input_file) {
  Emitter em = {.file = stdout};
  Parser parser = {.allow_fancy_structs = true};
  parser_read_file(&parser, input_file);
  parser_emit_typedefs(&parser, &em, false);
  parser_emit_functions(&parser, &em);

  parser_delete(&parser);
}

void testfunc_convert_functions(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .keep_abstract_headers = true};
  parser_read_file(&parser, input_file);
  parser_emit_typedefs(&parser, &em, false);
  parser_emit_functions(&parser, &em);

  parser_delete(&parser);
}

void testfunc_double_emittion(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .decl_emitter = &em, .default_emitter = &em};
  parser_read_file(&parser, input_file);

  parser_delete(&parser);
}

void testfunc_triple_emittion(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);

  parser_delete(&parser);
}

void testfunc_going_deeper(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);

  parser_delete(&parser);
}

void testfunc_final_tables(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);
  parser_emit_final_tables(&parser, &em);

  parser_delete(&parser);
}

void testfunc_constructors(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true, .add_line_directives = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true, .use_constructors = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);
  parser_emit_final_tables(&parser, &em);

  parser_delete(&parser);
}

void testfunc_full_compilation(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true, .add_line_directives = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true, .use_constructors = true, .preemit_structs = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);
  parser_emit_final_tables(&parser, &em);

  parser_delete(&parser);
}

void testfunc_preemit_structs(const char* input_file) {
  Emitter em = {.file = stdout, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .preemit_structs = true};
  parser_set_emitter(&parser, &em);
  parser_read_file(&parser, input_file);

  parser_delete(&parser);
}

TestCase test_cases[] = {
    TESTCASE(debug_tokens),
    TESTCASE(declarations),
    TESTCASE_LL(named_types),
    TESTCASE_L(emit_tokens),
    TESTCASE_L(emit_declarations),
    TESTCASE(fancy_declarations),
    TESTCASE_L(emit_fancy_declarations),
    TESTCASE_L(convert_structs),
    TESTCASE(functions),
    TESTCASE(fancy_functions),
    TESTCASE_LL(emit_functions),
    TESTCASE_LL(emit_fancy_functions),
    TESTCASE_LL(convert_functions),
    TESTCASE_L(double_emittion),
    TESTCASE_L(triple_emittion),
    TESTCASE_L(going_deeper),
    TESTCASE_L(final_tables),
    TESTCASE(constructors),
    TESTCASE(full_compilation),
    TESTCASE_L(preemit_structs),
};
int test_cases_count = sizeof(test_cases) / sizeof(*test_cases);

bool endsWith(const char* restrict str, const char* restrict suffix) {
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if(lensuffix > lenstr) return false;
  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

bool diff_files(const char* file1, const char* file2) {
  bool result = false;
  FILE* f1 = NULL;
  FILE* f2 = NULL;

  f1 = fopen(file1, "rb");
  if(f1 == NULL) {
    perror(file1);
    return_defer(false);
  }
  f2 = fopen(file2, "rb");
  if(f2 == NULL) {
    perror(file2);
    return_defer(false);
  }

  while(!feof(f1) && !feof(f2)) {
    if(fgetc(f1) != fgetc(f2)) return_defer(false);
  }
  return_defer(feof(f1) == feof(f2));

defer:;
  if(f1) fclose(f1);
  if(f2) fclose(f2);
  return result;
}

void run_test(const char* input_file, const char* output_file, TestFunc func) {
  if(freopen(output_file, "wb", stdout) == NULL) {
    perror(output_file);
    exit(1);
  }
  func(input_file);
  fclose(stdout);
}

bool run_testcase(const char* input_file, TestCase tc, bool rewrite, bool loop) {
  char namebuf[100];
  const char* tmp = strrchr(input_file, '/');
  const char* input_name = tmp ? tmp + 1 : input_file;

  int input_name_len = strlen(input_name);
  tmp = strrchr(input_name, '.');
  if(tmp) {
    input_name_len = tmp - input_name;
  }
  char* extra = rewrite ? "" : "tmp.";

  snprintf(namebuf, sizeof(namebuf) - 1, "%s/%s%s_%.*s.txt", OUTPUT_FOLDER, extra, tc.name, input_name_len, input_name);
  run_test(input_file, namebuf, tc.func);

  if(rewrite) return false;

  char namebuf2[100];
  snprintf(namebuf2, sizeof(namebuf2) - 1, "%s/%s_%.*s.txt", OUTPUT_FOLDER, tc.name, input_name_len, input_name);

  const char* correct_output = loop ? input_file : namebuf2;
  if(diff_files(correct_output, namebuf)) {
    if(remove(namebuf)) {
      fprintf(stderr, "Error deleting file %s: %s\n", namebuf, strerror(errno));
    }
    fprintf(stderr, "Test %s(\x1b[36m'%s'\x1b[0m) succeeded!!\n", tc.name, input_name);

    if(tc.ignore_some_files) {
      if(endsWith(input_file, "redef.c")) return false;
      if(endsWith(input_file, "cpp_output.c")) return false;
      if(endsWith(input_file, "cpp_output_2.c")) return false;
    }

    if(tc.loop_check && !loop) {
      return run_testcase(correct_output, tc, false, true);
    }
    return false;
  } else {
    fprintf(stderr, "Test %s(\x1b[31m'%s'\x1b[0m) failed!!\n", tc.name, input_name);
    fprintf(stderr, "\x1b[36mHINT:\x1b[0m Run this command, to see what's changed\n");
    fprintf(stderr, "diff --color=always '%s' '%s'\n\n", correct_output, namebuf);
    return true;
  }
}

int main(int argc, char** argv) {
  bool rewrite_tests = false;
  if(argc > 1 && strcmp(argv[1], "--rewrite") == 0) {
    rewrite_tests = true;
    argv++;
    argc--;
  }

  bool res = false;
  for(int j = 0; j < test_cases_count; j++) {
    TestCase tc = test_cases[j];
    for(int i = 1; i < argc; i++) {
      res |= run_testcase(argv[i], tc, rewrite_tests, false);
    }
  }
  return res;
}
