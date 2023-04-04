// this is a normal comment

/* this is a fucked up /* nested comment */
char* contrary_to_popular_belief = "this is not a comment";
// this is a multiline \
comment

void normal_function_header(char nothig_to_see_here) {}
#error surprise preprocessor directive "\
" aaaa

// strings
char* octal = "\1111";
char* hex = "\x41\x41\x41\x41";
char* normal_escapes = "\r\f\v\t\e\b\n";
char* empty = "";
char* backslash1 = "\"";
char* backslash2 = "\\";
char* backslash3 = "\\\"";
char* backslash4 = "\\\\";
char* multiline = "hello\
world";

// numbers
float floaty = 1.1f;
double doubly = 1.1;
long longish = 122l;
long long longest = 122ll;
