struct Triangle {
  int a;
  int b;
  int c;
};

struct Rectangle {
  int x;
  int y;
};

struct Circle {
  int r;
};

struct Figure {
  char* color;
} <
  struct Triangle;
  struct Rectangle;
  struct Circle;
>;
