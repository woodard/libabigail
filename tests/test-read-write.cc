// -*- Mode: C++ -*-

#include <iostream>
#include "test-utils.h"
#include "abg-reader.h"

int
main()
{
  string suffix("tests/data/test-read-write/input0.xml");
  string path(abigail::tests::get_src_dir() + "/" + suffix);

  abigail::abi_corpus corpus("test");
  if (!abigail::reader::read_file(path.c_str(), corpus))
    return 1;

  //TODO: serialize the corpus into
  //builddir/output/test-read-write/ouput0.xml, so that an other
  //dedicated test diffs it.

  return 0;
}
