// -*- Mode: C++ -*-

#include "test-utils.h"

using std::string;

namespace abigail
{
namespace tests
{

std::string&
get_src_dir()
{
#ifndef ABIGAIL_SRC_DIR
#error the macro ABIGAIL_SRC_DIR must be set at compile time
#endif

  static string s(ABIGAIL_SRC_DIR);
  return s;
}

std::string&
get_build_dir()
{
#ifndef ABIGAIL_BUILD_DIR
#error the macro ABIGAIL_BUILD_DIR must be set at compile time
#endif

  static string s(ABIGAIL_BUILD_DIR);
  return s;
}

}//end namespace tests
}//end namespace abigail
