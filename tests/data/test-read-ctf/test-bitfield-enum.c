/* gcc -gctf -c test-bitfield-enum.c -o test-bitfield-enum.o */
enum event_kind
{
  NO_EVENT,
};

struct input
{
  enum event_kind kind : 16;
};

struct input e;
