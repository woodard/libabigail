// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2015 Red Hat, Inc.
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include <fstream>
#include <iostream>
#include <abg-ir.h>
#include "abg-tools-utils.h"

using std::string;

namespace abigail
{

/// @brief Namespace for a set of utility function used by tools based
/// on libabigail.
namespace tools_utils
{

/// The bitwise 'OR' operator for abidiff_status bit masks.
///
/// @param l the left hand side operand of the OR operator.
///
/// @param r the right hand side operand of the OR operator.
///
/// @return the result of the OR expression.
abidiff_status
operator|(abidiff_status l, abidiff_status r)
{return static_cast<abidiff_status>(static_cast<unsigned>(l)
				     | static_cast<unsigned>(r));}

/// The bitwise 'AND' operator for abidiff_status bit masks.
///
/// @param l the left hand side operand of the AND operator.
///
/// @param r the right hand side operand of the AND operator.
///
/// @return the result of the AND expression.
abidiff_status
operator&(abidiff_status l, abidiff_status r)
{return static_cast<abidiff_status>(static_cast<unsigned>(l)
				     & static_cast<unsigned>(r));}

/// The |= operator.
///
/// @param l the left hand side operand of the operator.
///
/// @param r the right hand side operand of the operator.
///
/// @param the resulting bit mask.
abidiff_status&
operator|=(abidiff_status&l, abidiff_status r)
{
  l = static_cast<abidiff_status>(static_cast<unsigned>(l)
				  | static_cast<unsigned>(r));
  return l;
}

/// Test if an instance of @param abidiff_status bits mask represents
/// an error.
///
/// This functions tests if the @ref ABIDIFF_ERROR bit is set in the
/// given bits mask.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its ABIDIFF_ERROR bit set.
bool
abidiff_status_has_error(abidiff_status s)
{return s & ABIDIFF_ERROR;}

/// Test if an instance of @param abidiff_status bits mask represents
/// an abi change.
///
/// This functions tests if the @ref ABIDIFF_ABI_CHANGE bit is set in the
/// given bits mask.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its @ref ABIDIFF_ABI_CHANGE bit set.
bool
abidiff_status_has_abi_change(abidiff_status s)
{return s & ABIDIFF_ABI_CHANGE;}

/// Test if an instance of @param abidiff_status bits mask represents
/// an incompatible abi change.
///
/// This functions tests if the @ref ABIDIFF_INCOMPATIBLE_ABI_CHANGE
/// bit is set in the given bits mask.  Note that the this bit is set
/// then the bit @ref ABIDIFF_ABI_CHANGE must be set as well.
///
/// @param s the bit mask to consider.
///
/// @return true iff @p s has its @ref ABIDIFF_INCOMPATIBLE ABI_CHANGE
/// set.
bool
abidiff_status_has_incompatible_abi_change(abidiff_status s)
{return s & ABIDIFF_ABI_INCOMPATIBLE_CHANGE;}

#define DECLARE_STAT(st) \
  struct stat st; \
  memset(&st, 0, sizeof(st))

/// Get the stat struct (as returned by the stat() function of the C library)
/// of a file.
///
/// @param path the path to the function to stat.
///
/// @param s the resulting stat struct.
///
/// @return true iff the stat function completed successfully.
static bool
get_stat(const string& path,
	 struct stat* s)
{return (stat(path.c_str(), s) == 0);}

/// Tests whether a path exists;
///
/// @param path the path to test for.
///
/// @return true iff the path at @p path exist.
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

/// Tests if a given path is a directory.
///
/// @param path the path to test for.
///
/// @return true iff @p path is a directory.
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
	 string& dir_name)
{
  if (path.empty())
    {
      dir_name = ".";
      return true;
    }

  char *p = strdup(path.c_str());
  char *r = ::dirname(p);
  dir_name = r;
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

ostream&
operator<<(ostream& output,
	   file_type r)
{
  string repr;

  switch(r)
    {
    case FILE_TYPE_UNKNOWN:
      repr = "unknown file type";
      break;
    case FILE_TYPE_NATIVE_BI:
      repr = "native binary instrumentation file type";
      break;
    case FILE_TYPE_ELF:
      repr = "ELF file type";
      break;
    case FILE_TYPE_AR:
      repr = "archive file type";
      break;
    case FILE_TYPE_XML_CORPUS:
      repr = "native XML corpus file type";
      break;
    case FILE_TYPE_ZIP_CORPUS:
      repr = "native ZIP corpus file type";
      break;
    case FILE_TYPE_RPM:
      repr = "RPM file type";
      break;
    case FILE_TYPE_SRPM:
      repr = "SRPM file type";
      break;
    }

  output << repr;
  return output;
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

  if (buf[0] == '!'
      && buf[1] == '<'
      && buf[2] == 'a'
      && buf[3] == 'r'
      && buf[4] == 'c'
      && buf[5] == 'h'
      && buf[6] == '>')
    return FILE_TYPE_AR;

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

  if ((unsigned char)buf[0]    == 0xed
      && (unsigned char) buf[1] == 0xab
      && (unsigned char)buf[2] == 0xee
      &&  (unsigned char)buf[3] == 0xdb)
    {
        if (buf[7] == 0x00)
          return FILE_TYPE_RPM;
        else if (buf[7] == 0x01)
          return FILE_TYPE_SRPM;
        else
          return FILE_TYPE_UNKNOWN;
    }

  return FILE_TYPE_UNKNOWN;
}

/// Guess the type of the content of an file.
///
/// @param file_path the path to the file to consider.
///
/// @return the type of content guessed.
file_type
guess_file_type(const string& file_path)
{
  ifstream in(file_path.c_str(), ifstream::binary);
  file_type r = guess_file_type(in);
  in.close();
  return r;
}

struct malloced_char_star_deleter
{
  void
  operator()(char* ptr)
  {free(ptr);}
};

/// Return a copy of the path given in argument, turning it into an
/// absolute path by prefixing it with the concatenation of the result
/// of get_current_dir_name() and the '/' character.
///
/// The result being an shared_ptr to char*, it should manage its
/// memory by itself and the user shouldn't need to wory too much for
/// that.
///
/// @param p the path to turn into an absolute path.
///
/// @return a shared pointer to the resulting absolute path.
std::tr1::shared_ptr<char>
make_path_absolute(const char*p)
{
  using std::tr1::shared_ptr;

  shared_ptr<char> result;

  if (p && p[0] != '/')
    {
      shared_ptr<char> pwd(get_current_dir_name(),
			   malloced_char_star_deleter());
      string s = string(pwd.get()) + "/" + p;
      result.reset(strdup(s.c_str()), malloced_char_star_deleter());
    }
  else
    result.reset(strdup(p), malloced_char_star_deleter());

  return result;
}

}//end namespace tools_utils

using abigail::ir::function_decl;

/// Dump (to the standard error stream) two sequences of strings where
/// each string represent one of the functions in the two sequences of
/// functions given in argument to this function.
///
/// @param a_begin the begin iterator for the first input sequence of
/// functions.
///
/// @parm a_end the end iterator for the first input sequence of
/// functions.
///
/// @param b_begin the begin iterator for the second input sequence of
/// functions.
///
/// @param b_end the end iterator for the second input sequence of functions.
void
dump_functions_as_string(std::vector<function_decl*>::const_iterator a_begin,
			 std::vector<function_decl*>::const_iterator a_end,
			 std::vector<function_decl*>::const_iterator b_begin,
			 std::vector<function_decl*>::const_iterator b_end)
{abigail::fns_to_str(a_begin, a_end, b_begin, b_end, std::cerr);}

/// Dump (to the standard error output stream) a pretty representation
/// of the signatures of two sequences of functions.
///
/// @param a_begin the start iterator of the first input sequence of functions.
///
/// @param a_end the end iterator of the first input sequence of functions.
///
/// @param b_begin the start iterator of the second input sequence of functions.
///
/// @param b_end the end iterator of the second input sequence of functions.
void
dump_function_names(std::vector<function_decl*>::const_iterator a_begin,
		    std::vector<function_decl*>::const_iterator a_end,
		    std::vector<function_decl*>::const_iterator b_begin,
		    std::vector<function_decl*>::const_iterator b_end)
{
  std::vector<function_decl*>::const_iterator i;
  std::ostream& o = std::cerr;
  for (i = a_begin; i != a_end; ++i)
    o << (*i)->get_pretty_representation() << "\n";

  o << "  ->|<-  \n";
  for (i = b_begin; i != b_end; ++i)
    o << (*i)->get_pretty_representation() << "\n";
  o << "\n";
}

/// Compare two functions that are in a vector of functions.
///
/// @param an iterator to the beginning of the the sequence of functions.
///
/// @param f1_index the index of the first function to compare.
///
/// @param f2_inde the index of the second function to compare
bool
compare_functions(vector<function_decl*>::const_iterator base,
		  unsigned f1_index, unsigned f2_index)
{
  function_decl* fn1 = base[f1_index];
  function_decl* fn2 = base[f2_index];

  return *fn1 == *fn2;
}

}//end namespace abigail
