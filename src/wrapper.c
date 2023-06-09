#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

// TODO: windows compatibility
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
typedef pid_t Pid;
typedef int Fd;
typedef struct {
  Fd read;
  Fd write;
} Pipe;

Pipe make_pipe() {
  Pipe pip = {0};

  Fd pipefd[2];
  if(pipe(pipefd) < 0) {
    fprintf(stderr, "Could not create pipe: %s", strerror(errno));
    exit(1);
  }

  pip.read = pipefd[0];
  pip.write = pipefd[1];
  return pip;
}

Pid exec_file_with_pipe(char* name, char** arr, Pipe pipe, bool use_in, bool use_out) {
  Pid pid = fork();
  arr[0] = name;

  if(pid == -1) {
    perror("fork");
    exit(1);
  } else if(pid > 0) {
    if(use_in) close(pipe.read);
    if(use_out) close(pipe.write);
    return pid;
  } else {
    if(use_in) dup2(pipe.read, STDIN_FILENO);
    close(pipe.read);
    if(use_out) dup2(pipe.write, STDOUT_FILENO);
    close(pipe.write);

    execvp(name, arr);
    perror("execvp");
    fprintf(stderr, "can't run %s\n", name);
    exit(1);
  }
}

void wait_pid(Pid pid) {
  int status;
  waitpid(pid, &status, 0);
  if(status) {
    fprintf(stderr, "Child process exited with code %d\n", WEXITSTATUS(status));
    exit(1);
  }
}

bool run_parser(FILE* input_file, FILE* output_file, const char* input_filename) {
  Emitter em = {.file = output_file, .convert_structs = true, .add_line_directives = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true, .use_constructors = true, .preemit_structs = true};
  parser_set_emitter(&parser, &em);

  lexer_reset(&parser.lexer);
  parser.lexer.current_location.filename = input_filename;
  parser.lexer.file = input_file;
  while(parser_parse_line(&parser)) {
  }
  token_emit(&(Token){.kind = TOKEN_EOF}, parser.default_emitter);
  parser_emit_final_tables(&parser, &em);
  parser_delete(&parser);

  fclose(output_file);
  return parser.has_seen_error;
}

void setup_pipes(char* input_filename, char** options) {
  Pipe cpp_pipe = make_pipe();
  Pipe cc_pipe = make_pipe();
  Pid cpp = exec_file_with_pipe("gcc", (char*[]){"gcc", "-E", input_filename, NULL}, cpp_pipe, false, true);
  Pid cc = exec_file_with_pipe("gcc", options, cc_pipe, true, false);
  FILE* input_file = fdopen(cpp_pipe.read, "r");
  FILE* output_file = fdopen(cc_pipe.write, "w");

  run_parser(input_file, output_file, input_filename);

  wait_pid(cpp);
  wait_pid(cc);
}

char* parse_argv(int argc, char** argv, char*** options) {
  char* res = NULL;
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--") == 0) {
      res = argv[++i];
    } else if(argv[i][0] == '-') {
      arrpush(*options, argv[i]);
    } else if(res == NULL) {
      res = argv[i];
    } else {
      fprintf(stderr, "ppp-wrapper: way too many input files...\n");
      exit(1);
    }
  }

  if(res == NULL) {
    fprintf(stderr, "ppp-wrapper: no input file found...\n");
    exit(1);
  }
  return res;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    fprintf(stderr, "ppp-wrapper: [options...] file.c\n");
    exit(1);
  }

  char** options = NULL;
  arrpush(options, "gcc");
  arrpush(options, "-Wno-incompatible-pointer-types");
  char* file = parse_argv(argc, argv, &options);
  arrpush(options, "-x");
  arrpush(options, "c");
  arrpush(options, "-");
  arrpush(options, NULL);

  setup_pipes(file, options);
  arrfree(options);
  return 0;
}
