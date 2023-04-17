#include <stdint.h>
#include <stdio.h>

#define DRAWABLE_COUNT 6
#include "lines.h"
#include "circle.h"
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
  Drawable* objects[DRAWABLE_COUNT];

  VerticalLine vl = {.x = 3, .y1 = 2, .y2 = 10};
  HorizontalLine hl = {.y = 3, .x1 = 2, .x2 = 10};
  Box b1 = {.x1 = 5, .y1 = 5, .x2 = 15, .y2 = 10};
  Box b2 = {.x1 = 1, .y1 = 11, .x2 = 18, .y2 = 13};
  VerticalLine vl2 = {.x = 10, .y1 = 2, .y2 = 14};
  Circle circ = {.x = 15, .y = 15, .r = 4};

  Drawable<VerticalLine> d_vl;
  Drawable<HorizontalLine> d_hl;
  Drawable<Box> d_b1;
  Drawable<Box> d_b2;
  Drawable<VerticalLine> d_vl2;
  Drawable<Circle> d_circ;

  d_vl! = vl;
  d_hl! = hl;
  d_b1! = b1;
  d_b2! = b2;
  d_vl2! = vl2;
  d_circ! = circ;

  objects[0] = &d_vl;
  objects[1] = &d_hl;
  objects[2] = &d_b1;
  objects[3] = &d_b2;
  objects[4] = &d_vl2;
  objects[5] = &d_circ;

  for(int i = 0; i < DRAWABLE_COUNT; i++){
    Draw<objects[i]>();
  }

  print_array();

  for(int i = 0; i < DRAWABLE_COUNT; i++){
    for(int j = 0; j < DRAWABLE_COUNT; j++){
      if(Intersect<objects[i], objects[j]>()) {
        printf("Intersect(%d, %d)!!\n", i, j);
      }
    }
  }

  return 0;
}
