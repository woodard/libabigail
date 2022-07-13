/* Compile with gcc -Wall -g -gctf -shared -o libfoo.so foo.c */

struct S
{
  int m0;
  char m1;
};

int
f(struct S* s)
{return s->m0;}
