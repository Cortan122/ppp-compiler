#pragma once

#include <stdbool.h>
#include <stdint.h>
#define SIZE 20
extern uint8_t array[SIZE][SIZE];

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

void Draw<Drawable* d>();
bool Intersect<Drawable* d1, Drawable* d2>();
