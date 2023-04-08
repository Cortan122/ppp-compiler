struct Circle {
  int r;
};

struct Figure {} <struct Circle;>;

// Круг разместится внутри круга
int Intersect<struct Figure<struct Circle> *c1, struct Figure<struct Circle> *c2>() {
  return c1->!r < c2->!r;
}
