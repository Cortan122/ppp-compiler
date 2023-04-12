#include <stdint.h>
#include <stdio.h>

#define SIZE 20

typedef struct VerticalLine {
  int x;
  int y1;
  int y2;
} VerticalLine;

typedef struct HorizontalLine {
  int y;
  int x1;
  int x2;
} HorizontalLine;

typedef struct Box {
  int x1;
  int y1;
  int x2;
  int y2;
} Box;

typedef struct Drawable {}<
  struct VerticalLine;
  struct HorizontalLine;
  struct Box;
> Drawable;

uint8_t array[SIZE][SIZE] = {0};

// 4 bits -- udlr
const char* rom[] = {
    " ",
    "╶",
    "╴",
    "─",
    "╷",
    "┌",
    "┐",
    "┬",
    "╵",
    "└",
    "┘",
    "┴",
    "│",
    "├",
    "┤",
    "┼",
};

void print_array() {
  for(int i = 0; i < SIZE; i++) {
    for(int j = 0; j < SIZE; j++) {
      printf("%s", rom[array[i][j]]);
    }
    printf("\n");
  }
}

void Draw<Drawable* d>() = 0;

void Draw<Drawable<VerticalLine>* vl>() {
  array[vl->!y1][vl->!x] |= 4;  // 0b0100
  for(int i = vl->!y1 + 1; i < vl->!y2; i++) {
    array[i][vl->!x] |= 12;  // 0b1100
  }
  array[vl->!y2][vl->!x] |= 8;  // 0b1000
}

void Draw<Drawable<HorizontalLine>* hl>() {
  array[hl->!y][hl->!x1] |= 1;  // 0b0001
  for(int i = hl->!x1 + 1; i < hl->!x2; i++) {
    array[hl->!y][i] |= 3;  // 0b0011
  }
  array[hl->!y][hl->!x2] |= 2;  // 0b0010
}

void Draw<Drawable<Box>* b>() {
  Drawable<VerticalLine> vl = {}<{.x = b->!x1, .y1 = b->!y1, .y2 = b->!y2}>;
  Draw<&vl>();
  vl!x = b->!x2;
  Draw<&vl>();

  Drawable<HorizontalLine> hl = {}<{.y = b->!y1, .x1 = b->!x1, .x2 = b->!x2}>;
  Draw<&hl>();
  hl!y = b->!y2;
  Draw<&hl>();
}

int main() {
  Drawable* objects[4];

  VerticalLine vl = {.x = 3, .y1 = 2, .y2 = 10};
  HorizontalLine hl = {.y = 3, .x1 = 2, .x2 = 10};
  Box b1 = {.x1 = 5, .y1 = 5, .x2 = 15, .y2 = 10};
  Box b2 = {.x1 = 1, .y1 = 11, .x2 = 18, .y2 = 13};

  Drawable<VerticalLine> d_vl;
  Drawable<HorizontalLine> d_hl;
  Drawable<Box> d_b1;
  Drawable<Box> d_b2;

  d_vl! = vl;
  d_hl! = hl;
  d_b1! = b1;
  d_b2! = b2;

  objects[0] = &d_vl;
  objects[1] = &d_hl;
  objects[2] = &d_b1;
  objects[3] = &d_b2;

  for(int i = 0; i < 4; i++){
    Draw<objects[i]>();
  }

  print_array();
  return 0;
}
