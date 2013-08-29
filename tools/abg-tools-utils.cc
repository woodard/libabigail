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

///@file

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include "abg-tools-utils.h"

using std::string;

namespace abigail
{
namespace tools
{
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

/// Return the directory part of a file path.
///
/// @param path the file path to consider
///
/// @param dirnam the resulting directory part, or "." if the couldn't
/// figure out anything better (for now; maybe we should do something
/// better than this later ...).
///
/// @return true upon successful completion, false otherwise (okay,
/// for now it always return true, but that might change in the future).
bool
dir_name(std::string const& path,
	std::string& dirnam)
{
  if (path.empty())
    {
      dirnam = ".";
      return true;
    }

  char *p = strdup(path.c_str());
  char *r = ::dirname(p);
  dirnam = r;
  free(p);
  return true;
}

/// Return the file name part of a file part.
///
/// @param path the file path to consider.
///
/// @param file_name the name part of the file to consider.
///
///@return true upon successful completion, false otherwise (okay it
///always return true for now, but that might change in the future).
bool
base_name(string const &path,
	 string& file_name)
{
  if (path.empty())
    {
      file_name = ".";
      return true;
    }

  char *p = strdup(path.c_str());
  char *f = ::basename(p);
  file_name = f;
  free(p);
  return true;
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

  string parent;
  if (dir_name(path, parent))
    is_ok = ensure_dir_path_created(parent);

  return is_ok;
}

}//end namespace tools
}//end namespace abigail
