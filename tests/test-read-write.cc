// -*- Mode: C++ -*-

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "test-utils.h"
#include "abg-reader.h"
#include "abg-writer.h"

using std::string;
using std::ofstream;

using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from, and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_path;
  const char* out_path;
};// end struct InOutSpec


InOutSpec in_out_specs[] =
{
  {
    "data/test-read-write/test0.xml",
    "output/test-read-write/test0.xml"
  },
  {
    "data/test-read-write/test1.xml",
    "output/test-read-write/test1.xml"
  },
  {
    "data/test-read-write/test2.xml",
    "output/test-read-write/test2.xml"
  },
  {
    "data/test-read-write/test3.xml",
    "output/test-read-write/test3.xml"
  },
  {
    "data/test-read-write/test4.xml",
    "output/test-read-write/test4.xml"
  },
  {
    "data/test-read-write/test5.xml",
    "output/test-read-write/test5.xml"
  },
  {
    "data/test-read-write/test6.xml",
    "output/test-read-write/test6.xml"
  },
  {
    "data/test-read-write/test7.xml",
    "output/test-read-write/test7.xml"
  },
  {
    "data/test-read-write/test8.xml",
    "output/test-read-write/test8.xml"
  },
  {
    "data/test-read-write/test9.xml",
    "output/test-read-write/test9.xml"
  },
  {
    "data/test-read-write/test10.xml",
    "output/test-read-write/test10.xml"
  },
  // This should be the last entry.
  {NULL, NULL}
};

/// Walk the array of InOutSpecs above, read the input files it points
/// to, write it into the output it points to and diff them.
int
main()
{
  unsigned result = 1;

  bool is_ok = true;
  string in_path, out_path;
  for (InOutSpec *s = in_out_specs; s->in_path; ++s)
    {
      string input_suffix(s->in_path);
      in_path = abigail::tests::get_src_dir() + "/tests/" + input_suffix;
      abigail::translation_unit tu(input_suffix);
      if (!abigail::reader::read_file(in_path, tu))
	{
	  cerr << "failed to read " << in_path << "\n";
	  is_ok = false;
	  continue;
	}

      string output_suffix(s->out_path);
      out_path = abigail::tests::get_build_dir() + "/tests/" + output_suffix;
      if (!abigail::tests::ensure_parent_dir_created(out_path))
	{
	  cerr << "Could not create parent director for " << out_path;
	  is_ok = false;
	  return result;
	}

      ofstream of(out_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << out_path << "\n";
	  is_ok = false;
	  continue;
	}

      bool r = abigail::writer::write_to_ostream(tu, of);
      is_ok = (is_ok && r);
      of.close();
      string cmd = "diff -u " + in_path + " " + out_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}
