/*
 * Compile this to emit BTF debug info with:
 *
 * gcc -c -gbtf test0.c
 */

struct S;
typedef struct S S;

union U;
typedef union U U;

S*
fn0(S* p, U* u)
{
  if (u)
    ;

  return p;
}
