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

bool Intersect<Drawable* d1, Drawable* d2>() = 0;

bool Intersect<Drawable* d1, Drawable* d2>() {
  if(d1->tag > d2->tag) {
    return Intersect<d2, d1>();
  }
  return false;
}

bool Intersect<Drawable<VerticalLine>* vl, Drawable<HorizontalLine>* hl>() {
  return hl->!x1 <= vl->!x && vl->!x <= hl->!x2 && vl->!y1 <= hl->!y && hl->!y <= vl->!y2;
}

bool Intersect<Drawable<Box>* rectA, Drawable<Box>* rectB>() {
  return rectA->!x1 <= rectB->!x2 && rectA->!x2 >= rectB->!x1 && rectA->!y1 >= rectB->!y2 && rectA->!y2 <= rectB->!y1;
}

bool Intersect<Drawable<VerticalLine>* line, Drawable<Box>* b>() {
  Drawable<VerticalLine> vl = {}<{.x = b->!x1, .y1 = b->!y1, .y2 = b->!y2}>;
  if(Intersect<&vl, line>()) return true;
  vl!x = b->!x2;
  if(Intersect<&vl, line>()) return true;

  Drawable<HorizontalLine> hl = {}<{.y = b->!y1, .x1 = b->!x1, .x2 = b->!x2}>;
  if(Intersect<&hl, line>()) return true;
  hl!y = b->!y2;
  if(Intersect<&hl, line>()) return true;
  return false;
}
