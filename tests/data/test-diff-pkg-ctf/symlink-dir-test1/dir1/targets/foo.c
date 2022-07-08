/* Compile with gcc -Wall -gctf -g -shared -o libfoo.so foo.c.  */

struct S
{
  int m0;
};

int
f(struct S* s)
{return s->m0;}
