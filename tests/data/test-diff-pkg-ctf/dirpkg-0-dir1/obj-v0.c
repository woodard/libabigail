// Compile with:
// gcc -gctf -shared -o libobj-v0.so obj-v0.c

struct S
{
  int mem0;
};

void
bar(struct S *s)
{}
