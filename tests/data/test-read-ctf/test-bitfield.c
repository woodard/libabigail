/* gcc -gctf -c test-bitfield.c -o test-bitfield.o */
struct foo
{
  unsigned bar : 2;
  unsigned baz : 1;
};

struct foobar 
{
  unsigned char bar : 2;
  unsigned char baz : 1;
};

struct foo f;
struct foobar fb;
