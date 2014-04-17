class B0
{
  char m0;

public:
  B0()
    : m0(0)
  {}
};

class B1
{
  unsigned m0;

public:
  B1()
    : m0(0)
  {}
};

class S : public B0, public B1
{
  char m0;

public:
  S()
    : m0(0)
  {}
};

void
foo(S&)
{}
