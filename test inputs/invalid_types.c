typedef struct;
typedef struct foo;
typedef struct bar baz;
typedef {int f;} struct quux;

typedef struct fnord {}<122;> fum;
typedef struct shme {}<zot;> foobar;
typedef struct fred {}<int wibble;> derp;
typedef struct baz {}<int;> fnord;
typedef struct fum {}<122 fred;> shme;

struct;
struct foo1;
struct bar1 baz1;
{int f;} struct quux1;

struct fnord1 {}<122;> fum1;
struct shme1 {}<zot;> foobar1;
struct fred1 {}<int wibble;> derp1;
struct baz1 {}<int;> fnord1;
struct fum1 {}<122 fred1;> shme1;
