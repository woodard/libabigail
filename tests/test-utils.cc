// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this program; see the file COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.


#include "test-utils.h"

using std::string;

namespace abigail
{
namespace tests
{

/// Returns the absolute path to the source directory.
///
/// \return the absolute path tho the source directory.
const char*
get_src_dir()
{
#ifndef ABIGAIL_SRC_DIR
#error the macro ABIGAIL_SRC_DIR must be set at compile time
#endif

  static __thread const char* s(ABIGAIL_SRC_DIR);
  return s;
}

/// Returns the absolute path to the build directory.
///
/// \return the absolute path the build directory.
const char*
get_build_dir()
{
#ifndef ABIGAIL_BUILD_DIR
#error the macro ABIGAIL_BUILD_DIR must be set at compile time
#endif

  static __thread const char* s(ABIGAIL_BUILD_DIR);
  return s;
}

}//end namespace tests
}//end namespace abigail
