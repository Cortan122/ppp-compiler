vpath %.c src
vpath %.h src
vpath %.o build/

MAKEFLAGS += -j4
WARNINGS=-Wall -Wextra -Werror=vla
CFLAGS=-Og -g $(WARNINGS) -I.

OBJDIR=build
SRC=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)
OBJS=token.o lexer.o parser.o declaration.o

all: main test
wrapper: wrapper.o $(OBJS)
main: main.o $(OBJS)
test: test.o $(OBJS)

ifneq ($(OS),Windows_NT)
all: wrapper
endif

$(OBJDIR)/depend.mk: $(SRC) $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CC) -MM -MG $(SRC) > $(OBJDIR)/depend.mk

include $(OBJDIR)/depend.mk

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o build/$@ $<

.o:
	$(CC) $(LDFLAGS) $(addprefix $(OBJDIR)/, $(^:$(OBJDIR)/%=%)) $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -rf $(OBJDIR) *.o depend.mk main test wrapper a.out "test outputs/"tmp.*

run_tests: test
	./test "test inputs/"*

rewrite_tests: test
	./test --rewrite "test inputs/"*

stb_ds.h:
	curl --silent -O https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h
