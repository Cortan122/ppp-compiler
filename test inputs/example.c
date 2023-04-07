struct Triangle {int a, b, c;}; // Треугольник
struct Rectangle {int x, y;};   // Прямоугольник
struct Circle {int r;};         // Круг

struct Figure {} <struct Triangle;struct Rectangle;struct Circle;>;

// Вывод параметров прямоугольника
void PrintFigure<struct Figure<struct Rectangle> *r>() {
  printf("Rectangle: x = %d, y = %d", r->!x, r->!y);
}
// Вывод параметров треугольника
void PrintFigure<struct Figure<struct Triangle> *t>() {
  printf("Triangle: a = %d, b = %d, c = %d", t->!a, t->!b, t->!c);
}
// Вывод параметров круга
void PrintFigure<struct Figure<struct Circle> *c>() {
  printf("Circle: r = %d", c->!r);
}
