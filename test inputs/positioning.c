# 6 "test inputs/positioning.c"
int lol[-1];

#if 0
#error this section exists to get referenced
int lol[-1]; // comment
#endif
# 9 "test inputs/positioning.c"

typedef struct Figure {
  char* color[-1]; // comment 2
} <
  struct whatever;
> Figure;
