struct S0
{
  int private_data_member0;
  char private_data_member1;
  int suppressed_added_member;
};

struct S1
{
  int m0;
  char non_suppressed_added_member;
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
