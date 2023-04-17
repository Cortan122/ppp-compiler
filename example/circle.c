#include "circle.h"

void Draw<Drawable<Circle>* c>() {
  for(int x = -c->!r; x < c->!r; x++){
    for(int y = -c->!r; y < c->!r; y++){
      if(x*x + y*y <= c->!r){
        array[c->!x + x][c->!y + y] |= 0xf;
      }
    }
  }
}
