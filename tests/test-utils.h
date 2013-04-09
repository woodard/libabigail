// Copyright (C) 2013 Free Software Foundation, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License
// and a copy of the GCC Runtime Library Exception along with this
// program; see the files COPYING3 and COPYING.RUNTIME respectively.
// If not, see <http://www.gnu.org/licenses/>.

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
