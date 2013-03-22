// -*- Mode: C++ -*-

#include <string>
#include <fstream>
#include "test-utils.h"
#include "abg-reader.h"
#include "abg-writer.h"

using std::string;
using std::ofstream;

int
main()
{
  unsigned result = 1;

  string input_suffix("tests/data/test-read-write/input0.xml");
  string path(abigail::tests::get_src_dir() + "/" + input_suffix);

  abigail::abi_corpus corpus("test");
  if (!abigail::reader::read_file(path.c_str(), corpus))
    return result;

  string output_suffix("tests/output/test-read-write/input0.xml");
  path = abigail::tests::get_build_dir() + "/" + output_suffix;
  if (!abigail::tests::ensure_parent_dir_created(path))
    return result;

  ofstream of(path.c_str(), std::ios_base::trunc);
  if (!of.is_open())
    return result;

  bool is_ok = abigail::writer::write_to_ostream(corpus, of);
  of.close();

  return !is_ok;
}
