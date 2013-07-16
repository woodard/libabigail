// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <cstring>
#include <cstdlib>
#include "test-utils.h"

using std::string;

namespace abigail
{
namespace tests
{

/// Returns the absolute path to the source directory.
///
/// \return the absolute path tho the source directory.
const std::string&
get_src_dir()
{
#ifndef ABIGAIL_SRC_DIR
#error the macro ABIGAIL_SRC_DIR must be set at compile time
#endif

  static string s(ABIGAIL_SRC_DIR);
  return s;
}

/// Returns the absolute path to the build directory.
///
/// \return the absolute path the build directory.
const std::string&
get_build_dir()
{
#ifndef ABIGAIL_BUILD_DIR
#error the macro ABIGAIL_BUILD_DIR must be set at compile time
#endif

  static string s(ABIGAIL_BUILD_DIR);
  return s;
}

/// Tests whether #path is a directory.
///
/// \return true iff #path is a directory.
bool
is_dir(const string& path)
{
  struct stat st;
  memset(&st, 0, sizeof (st));

  if (stat(path.c_str(), &st) != 0)
    return false;

  return !!S_ISDIR(st.st_mode);
}

/// Ensures #dir_path is a directory and is created.  If #dir_path is
/// not created, this function creates it.
///
/// \return true if #dir_path is a directory that is already present,
/// of if the function has successfuly created it.
bool
ensure_dir_path_created(const string& dir_path)
{
  struct stat st;
  memset(&st, 0, sizeof (st));

  int stat_result = 0;

  stat_result = stat(dir_path.c_str(), &st);
  if (stat_result == 0)
    {
      // A file or directory already exists with the same name.
      if (!S_ISDIR (st.st_mode))
	return false;
      return true;
    }

  string cmd;
  cmd = "mkdir -p " + dir_path;

  if (system(cmd.c_str()))
    return false;

  return true;
}

/// Ensures that the parent directory of #path is created.
///
/// \return true if the parent directory of #path is already present,
/// or if this function has successfuly created it.
bool
ensure_parent_dir_created(const string& path)
{
  bool is_ok = false;

  if (path.empty())
    return is_ok;

  char * p = strdup(path.c_str());
  char *parent = dirname(p);
  is_ok = ensure_dir_path_created(parent);
  free(p);

  return is_ok;

}

}//end namespace tests
}//end namespace abigail
