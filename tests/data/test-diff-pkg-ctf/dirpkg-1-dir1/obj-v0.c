// Compile with:
// g++ -gctf -shared -o libobj-v0.so obj-v0.cc

struct S
{
  int mem0;
};

void
bar(struct S *s)
{}
