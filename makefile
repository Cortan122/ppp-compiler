MAKEFLAGS += -j4
WARNINGS=-Wall -Wextra -Werror=vla
CFLAGS=-O2 $(WARNINGS)

SRC=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJS=token.o lexer.o parser.o declaration.o

all: main test
wrapper: wrapper.o $(OBJS)
main: main.o $(OBJS)
test: test.o $(OBJS)

ifneq ($(OS),Windows_NT)
all: wrapper
endif

depend.mk: $(SRC) $(HEADERS)
	$(CC) -MM -MG $(SRC) > depend.mk

include depend.mk

clean:
	rm -f *.o depend.mk main test wrapper "test outputs/"tmp.*

run_tests: test
	./test "test inputs/"*

rewrite_tests: test
	./test --rewrite "test inputs/"*

stb_ds.h:
	curl --silent -O https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h
