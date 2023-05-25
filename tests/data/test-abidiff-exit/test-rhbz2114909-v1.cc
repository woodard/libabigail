namespace ns1
{
class base_1
{
  int m0;
};

class base_2
{
  int m0;
};
}

namespace ns2
{

class base_1
{
  int m0;
};

class C : public ns1::base_2
{
  int m0;
  char m1;
public:
  C()
  {}
};
}

void
foo(ns2::C&)
{
}
