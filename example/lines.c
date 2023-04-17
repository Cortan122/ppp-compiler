#include "lines.h"

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
