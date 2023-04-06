WARNINGS=-Wall -Wextra -Werror=vla
CFLAGS=-O2 $(WARNINGS)

all: main test
main: main.o token.o lexer.o parser.o declaration.o
test: test.o token.o lexer.o parser.o declaration.o
token.o: token.h
lexer.o: lexer.h token.h
declaration.o: declaration.h stb_ds.h token.h
parser.o: parser.h lexer.h declaration.h token.h stb_ds.h

clean:
	rm -f *.o main test "test outputs/"tmp.*

run_tests: test
	./test "test inputs/"*

rewrite_tests: test
	./test --rewrite "test inputs/"*

stb_ds.h:
	curl --silent -O https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h
