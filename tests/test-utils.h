// -*- Mode: C++ -*-

#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <string>

namespace abigail
{
namespace tests
{

const std::string& get_src_dir();
const std::string& get_build_dir();
bool is_dir(const std::string&);
bool ensure_dir_path_created(const std::string&);
bool ensure_parent_dir_created(const std::string&);

}//end namespace tests
}//end namespace abigail
#endif //__TEST_UTILS_H__
