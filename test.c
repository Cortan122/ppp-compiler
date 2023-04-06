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

typedef void (*TestFunc)(const char*);
typedef struct TestCase {
  TestFunc func;
  char* name;
  bool loop_check;
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
  Parser parser = {0};
  parser_read_file(&parser, input_file);

  for(int i = 0; i < shlen(parser.structs); i++) {
    printf("struct %s;\n", parser.structs[i].key);
  }
  for(int i = 0; i < shlen(parser.typedefs); i++) {
    if(parser.typedefs[i].value) {
      printf("typedef struct %s %s;\n", parser.typedefs[i].value->name, parser.typedefs[i].key);
    } else {
      printf("typedef ... %s;\n", parser.typedefs[i].key);
    }
  }

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

TestCase test_cases[] = {
    TESTCASE(debug_tokens),
    TESTCASE(declarations),
    TESTCASE(named_types),
    TESTCASE_L(emit_tokens),
    TESTCASE_L(emit_declarations),
};
int test_cases_count = sizeof(test_cases) / sizeof(*test_cases);

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
