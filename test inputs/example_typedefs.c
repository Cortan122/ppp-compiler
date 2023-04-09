typedef struct Triangle {
  int a;
  int b;
  int c;
} Triangle;

typedef struct Rectangle {
  int x;
  int y;
} Rectangle;

typedef struct Circle {
  int r;
} Circle;

typedef struct Figure {
  char* color;
} <
  Triangle;
  Rectangle;
  Circle;
> Figure;

bool Intersect<Figure *first, Figure *second>() = 0;

// Прямоугольник разместится внутри прямоугольника
bool Intersect<Figure<Rectangle> *r1, Figure<Rectangle> *r2>() {
  return ((r1->!x < r2->!x) && (r1->!y < r2->!y)) ||
         ((r1->!x < r2->!y) && (r1->!y < r2->!x));
}

// Круг разместится внутри круга
bool Intersect<Figure<Circle> *c1, Figure<Circle> *c2>() {
  return c1->!r < c2->!r;
}
