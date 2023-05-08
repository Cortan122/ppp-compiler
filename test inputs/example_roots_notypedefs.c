#include <stdio.h>
#include <math.h>

struct OneRoot {
  double x;
};

struct TwoRoots {
  double x1;
  double x2;
};

struct NoRoot {
  char* msg;
};

struct Root {} <
  struct OneRoot;
  struct TwoRoots;
  struct NoRoot;
>;

void Print<struct Root* root>() = 0;

void Print<struct Root<struct OneRoot>* root>() {
  printf("x = %.2f\n", root->!x);
}

void Print<struct Root<struct TwoRoots>* root>() {
  printf("x1 = %.2f\n", root->!x1);
  printf("x2 = %.2f\n", root->!x2);
}

void Print<struct Root<struct NoRoot>* root>() {
  printf("error: %s\n", root->!msg);
}

void EvalOneRoot(double a, double b, struct Root* root) {
  struct Root<struct OneRoot> res;
  res!x = (-b) / (2 * a);
  struct Root* resptr = &res;
  *root = *resptr;
}

void EvalTwoRoots(double a, double b, double d, struct Root* root) {
  struct Root<struct TwoRoots> res;
  res!x1 = (-b - d) / (2 * a);
  res!x2 = (-b + d) / (2 * a);
  struct Root* resptr = &res;
  *root = *resptr;
}

void EvalNoRoot(struct Root* root) {
  struct Root<struct NoRoot> res;
  res!msg = "No roots found!";
  struct Root* resptr = &res;
  *root = *resptr;
}

void SolveQuadratic(double a, double b, double c, struct Root* root) {
  double d = b*b - 4*a*c;
  if(d == 0) {
    EvalOneRoot(a, b, root);
  } else if(d > 0) {
    EvalTwoRoots(a, b, d, root);
  } else {
    EvalNoRoot(root);
  }
}

int main() {
  double a,b,c;
  struct Root root;
  printf("Input a, b, and c:\n");
  scanf("%lf%lf%lf", &a, &b, &c);
  SolveQuadratic(a, b, c, &root);
  Print<&root>();
  return 0;
}
