struct C0
{
  int m0;
  int wrongly_inserted;
  int m1;
  char rh_kabi_reserved1[50];
};

struct C1
{
  int m0;
  char m1;
  int m2;
};

int
foo(struct C0 *c0, struct C1 *c1)
{
  return c0->m0 + c1->m0;
}
