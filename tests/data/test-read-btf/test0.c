/*
 * Compile this to emit BTF debug info with:
 *
 * gcc -c -gbtf test0.c
 */

typedef enum ENUM_TYPE
{
  E0_ENUM_TYPE = 0,
  E1_ENUM_TYPE= 1
} ENUM_TYPE;

typedef enum ANOTHER_ENUM_TYPE
{
  E0_ANOTHER_ENUM_TYPE = 0,
  E1_ANOTHER_ENUM_TYPE= 1
} ANOTHER_ENUM_TYPE;

typedef union u_type
{
  ENUM_TYPE *m0;
  ANOTHER_ENUM_TYPE *m1;
} u_type;

typedef struct foo_type
{
  const int *m0;
  volatile char *m1;
  unsigned *m2;
  const volatile unsigned char *m3;
  float m4[10];
  volatile const u_type *m5;
} foo_type;

void
fn0(const foo_type* p __attribute__((unused)))
{
}

struct foo_type foos[2] = {0};
