// -*- mode: C++ -*-

#include "abg-hash.h"

namespace abigail
{

namespace hashing
{

// Mix 3 32 bits values reversibly.  Borrowed from hashtab.c in gcc tree.
#define abigail_hash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<< 8); \
  c -= a; c -= b; c ^= ((b&0xffffffff)>>13); \
  a -= b; a -= c; a ^= ((c&0xffffffff)>>12); \
  b -= c; b -= a; b = (b ^ (a<<16)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>> 5)) & 0xffffffff; \
  a -= b; a -= c; a = (a ^ (c>> 3)) & 0xffffffff; \
  b -= c; b -= a; b = (b ^ (a<<10)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>>15)) & 0xffffffff; \
}

/// Produce good hash value combining val1 and val2.  This is copied
/// from tree.c in GCC.
size_t
combine_hashes(size_t val1, size_t val2)
{
  /* the golden ratio; an arbitrary value.  */
  size_t a = 0x9e3779b9;
  abigail_hash_mix(a, val1, val2);
  return val2;
}

}//end namespace hash
}//end namespace abigail
