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
#include <fstream>
#include "abg-tools-utils.h"

using std::string;

namespace abigail
{
namespace tools
{

using std::ostream;
using std::istream;
using std::ifstream;
using std::string;

#define DECLARE_STAT(st) \
  struct stat st; \
  memset(&st, 0, sizeof(st))

static bool
get_stat(const string& path,
	 struct stat* s)
{return (stat(path.c_str(), s) == 0);}

/// Tests whether \a path exists;
bool
file_exists(const string& path)
{
  DECLARE_STAT(st);

  return get_stat(path, &st);
}

/// Test if path is a path to a regular file.
///
/// @param path the path to consider.
///
/// @return true iff path is a regular path.
bool
is_regular_file(const string& path)
{
  DECLARE_STAT(st);

  if (!get_stat(path, &st))
    return false;

  return !!S_ISREG(st.st_mode);
}

/// Tests whether #path is a directory.
///
/// \return true iff #path is a directory.
bool
is_dir(const string& path)
{
  DECLARE_STAT(st);

  if (!get_stat(path, &st))
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
dir_name(string const& path,
	 string& dirnam)
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

/// Check if a given path exists and is readable.
///
/// @param path the path to consider.
///
/// @param out the out stream to report errors to.
///
/// @return true iff path exists and is readable.
bool
check_file(const string& path,
	   ostream& out)
{
  if (!file_exists(path))
    {
      out << "file " << path << " does not exist\n";
      return false;
    }

  if (!is_regular_file(path))
    {
      out << path << " is not a regular file\n";
      return false;
    }

  return true;
}

/// Guess the type of the content of an input stream.
///
/// @param in the input stream to guess the content type for.
///
/// @return the type of content guessed.
file_type
guess_file_type(istream& in)
{
  const unsigned BUF_LEN = 13;
  const unsigned NB_BYTES_TO_READ = 12;

  char buf[BUF_LEN];
  memset(buf, 0, BUF_LEN);

  std::streampos initial_pos = in.tellg();
  in.read(buf, NB_BYTES_TO_READ);
  in.seekg(initial_pos);

  if (in.gcount() < 4 || in.bad())
    return FILE_TYPE_UNKNOWN;

  if (buf[0] == 0x7f
      && buf[1] == 'E'
      && buf[2] == 'L'
      && buf[3] == 'F')
    return FILE_TYPE_ELF;

  if (buf[0] == '<'
      && buf[1] == 'a'
      && buf[2] == 'b'
      && buf[3] == 'i'
      && buf[4] == '-'
      && buf[5] == 'i'
      && buf[6] == 'n'
      && buf[7] == 's'
      && buf[8] == 't'
      && buf[9] == 'r'
      && buf[10] == ' ')
    return FILE_TYPE_NATIVE_BI;

  if (buf[0]     == '<'
      && buf[1]  == 'a'
      && buf[2]  == 'b'
      && buf[3]  == 'i'
      && buf[4]  == '-'
      && buf[5]  == 'c'
      && buf[6]  == 'o'
      && buf[7]  == 'r'
      && buf[8]  == 'p'
      && buf[9]  == 'u'
      && buf[10] == 's'
      && buf[11] == ' ')
    return FILE_TYPE_XML_CORPUS;

  if (buf[0]    == 'P'
      && buf[1] == 'K'
      && buf[2] == 0x03
      && buf[3] == 0x04)
    return FILE_TYPE_ZIP_CORPUS;

  return FILE_TYPE_UNKNOWN;
}

/// Guess the type of the content of an file.
///
/// @param file_path the path to the file to consider.
///
/// @return the type of content guessed.
file_type
guess_file_type(const std::string& file_path)
{
  ifstream in(file_path.c_str(), ifstream::binary);
  file_type r = guess_file_type(in);
  in.close();
  return r;
}

}//end namespace tools
}//end namespace abigail
