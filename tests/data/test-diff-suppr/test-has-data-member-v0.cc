struct S0
{
  int private_data_member0;
  char private_data_member1;
};

struct S1
{
  int m0;
};


void
foo(S0&)
{
}

int
bar(S1* s)
{
  return s->m0;
}
