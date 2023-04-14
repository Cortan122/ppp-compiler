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

int main(int argc, char** argv) {
  printf("hello world\n");

  Pipe cpp_pipe = make_pipe();
  Pipe cc_pipe = make_pipe();
  Pid cpp = exec_file_with_pipe("gcc", (char*[]){"gcc", "-E", argv[1], NULL}, cpp_pipe, false, true);
  Pid cc = exec_file_with_pipe("gcc", (char*[]){"gcc", "-Wno-incompatible-pointer-types", "-x", "c", "-", NULL}, cc_pipe, true, false);
  FILE* input_file = fdopen(cpp_pipe.read, "r");
  FILE* output_file = fdopen(cc_pipe.write, "w");

  Emitter em = {.file = output_file, .convert_structs = true};
  Parser parser = {.allow_fancy_structs = true, .go_deeper = true};
  parser.decl_emitter = &em;
  parser.default_emitter = &em;
  parser.extra_emitter = &em;

  lexer_reset(&parser.lexer);
  parser.lexer.current_location.filename = argv[1];
  parser.lexer.file = input_file;
  while(parser_parse_line(&parser)) {
  }
  token_emit(&(Token){.kind = TOKEN_EOF}, parser.default_emitter);
  parser_emit_final_tables(&parser, &em);
  parser_delete(&parser);

  fclose(output_file);
  wait_pid(cpp);
  wait_pid(cc);

  return 0;
}
