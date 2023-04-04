WARNINGS=-Wall -Wextra -Werror=vla
CFLAGS=-O2 $(WARNINGS)

all: main test
main: main.o token.o lexer.o
test: test.o token.o lexer.o
token.o: token.h
lexer.o: lexer.h

clean:
	rm -f *.o main test "test outputs/"tmp.*

run_tests: test
	./test "test inputs/"*

rewrite_tests: test
	./test --rewrite "test inputs/"*
