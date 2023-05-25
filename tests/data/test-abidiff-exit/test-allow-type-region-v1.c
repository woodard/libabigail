struct C0
{
  int m0;
  int m1;
  int correctly_inserted;
  unsigned rh_kabi_reserved2;
  unsigned rh_kabi_reserved3;
  unsigned rh_kabi_reserved4;
  unsigned rh_kabi_reserved5;
};

struct C1
{
  int m0;
  char m1;
  char wrongly_inserted;
};

int
foo(struct C0 *c0, struct C1 *c1)
{
  return c0->m0 + c1->m0;
}
