#include <stdint.h>
#include <stdio.h>

#include "lines.h"
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
