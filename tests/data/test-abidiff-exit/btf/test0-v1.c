/*
 * Compile this to emit BTF debug info with:
 *
 * gcc -c -gbtf test0.c
 */

typedef enum ENUM_TYPE
{
  E0_ENUM_TYPE = 0,
  E1_ENUM_TYPE= 1,
  E2_ENUM_TYPE= 2
} ENUM_TYPE;

typedef enum ANOTHER_ENUM_TYPE
{
  E0_ANOTHER_ENUM_TYPE = 0,
  E1_ANOTHER_ENUM_TYPE= 1,
  E2_ANOTHER_ENUM_TYPE= 2
} ANOTHER_ENUM_TYPE;

typedef union u_type
{
  ENUM_TYPE *m0;
  ANOTHER_ENUM_TYPE *m1;
  char *m2;
} u_type;

typedef struct foo_type
{
  int *m0;
  volatile char *m1;
  unsigned *m2;
  const volatile unsigned char *m3;
  float m4[10];
  volatile const u_type *m5;
} foo_type;

int
fn0(foo_type* p, int a)
{
  *p->m0 = a;
  return a;
}

struct foo_type foos[2] = {0};
