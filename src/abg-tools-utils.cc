// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2016 Red Hat, Inc.
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
#include <time.h>
#include <fts.h>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <libgen.h>
#include <ext/stdio_filebuf.h> // For __gnu_cxx::stdio_filebuf
#include <fstream>
#include <iostream>
#include <sstream>

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include <abg-ir.h>
#include "abg-tools-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

using std::string;

namespace abigail
{

using namespace abigail::suppr;

/// @brief Namespace for a set of utility function used by tools based
/// on libabigail.
namespace tools_utils
{

/// Get the value of $libdir variable of the autotools build
/// system.  This is where shared libraries are usually installed.
///
/// @return a constant string (doesn't have to be free-ed by the
/// caller) that represent the value of the $libdir variable in the
/// autotools build system, or NULL if it's not set.
const char*
get_system_libdir()
{
#ifndef ABIGAIL_ROOT_SYSTEM_LIBDIR
#error the macro ABIGAIL_ROOT_SYSTEM_LIBDIR must be set at compile time
#endif

  static __thread const char* system_libdir(ABIGAIL_ROOT_SYSTEM_LIBDIR);
  return system_libdir;
}

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

/// Get the stat struct (as returned by the lstat() function of the C
/// library) of a file.  Note that the function uses lstat, so that
/// callers can detect symbolic links.
///
/// @param path the path to the function to stat.
///
/// @param s the resulting stat struct.
///
/// @return true iff the stat function completed successfully.
static bool
get_stat(const string& path,
	 struct stat* s)
{return (lstat(path.c_str(), s) == 0);}

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

/// Test if path is a path to a regular file or a symbolic link to a
/// regular file.
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

  if (S_ISREG(st.st_mode))
    return true;

  string symlink_target_path;
  if (maybe_get_symlink_target_file_path(path, symlink_target_path))
    return is_regular_file(symlink_target_path);

  return false;
}

/// Tests if a given path is a directory or a symbolic link to a
/// directory.
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

  if (S_ISDIR(st.st_mode))
    return true;

  string symlink_target_path;
  if (maybe_get_symlink_target_file_path(path, symlink_target_path))
    return is_dir(symlink_target_path);

  return false;
}

/// If a given file is a symbolic link, get the canonicalized absolute
/// path to the target file.
///
/// @param file_path the path to the file to consider.
///
/// @param target_path this parameter is set by the function to the
/// canonicalized path to the target file, if @p file_path is a
/// symbolic link.  In that case, the function returns true.
///
/// @return true iff @p file_path is a symbolic link.  In that case,
/// the function sets @p target_path to the canonicalized absolute
/// path of the target file.
bool
maybe_get_symlink_target_file_path(const string& file_path,
				   string& target_path)
{
  DECLARE_STAT(st);

  if (!get_stat(file_path, &st))
    return false;

  if (!S_ISLNK(st.st_mode))
    return false;

  char *link_target_path = realpath(file_path.c_str(), NULL);
  if (!link_target_path)
    return false;

  target_path = link_target_path;
  free(link_target_path);
  return true;
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

/// Emit a prefix made of the name of the program which is emitting a
/// message to an output stream.
///
/// The prefix is a string which looks like:
///
///   "<program-name> : "
///
/// @param prog_name the name of the program to use in the prefix.
/// @param out the output stream where to emit the prefix.
///
/// @return the output stream where the prefix was emitted.
ostream&
emit_prefix(const string& prog_name, ostream& out)
{
  if (!prog_name.empty())
    out << prog_name << ": ";
  return out;
}

/// Check if a given path exists and is readable.
///
/// @param path the path to consider.
///
/// @param out the out stream to report errors to.
///
/// @return true iff path exists and is readable.
bool
check_file(const string& path, ostream& out, const string& prog_name)
{
  if (!file_exists(path))
    {
      emit_prefix(prog_name, out) << "file " << path << " does not exist\n";
      return false;
    }

  if (!is_regular_file(path))
    {
      emit_prefix(prog_name, out) << path << " is not a regular file\n";
      return false;
    }

  return true;
}

/// Test if a given string ends with a particular suffix.
///
/// @param str the string to consider.
///
/// @param suffix the suffix to test for.
///
/// @return true iff string @p str ends with suffix @p suffix.
bool
string_ends_with(const string& str, const string& suffix)
{
  string::size_type str_len = str.length(), suffix_len = suffix.length();

  if (str_len < suffix_len)
    return false;
  return str.compare(str_len - suffix_len, suffix_len, suffix) == 0;
}

/// Test if a string is made of ascii characters.
///
/// @param str the string to consider.
///
/// @return true iff @p str is made of ascii characters.
bool
string_is_ascii(const string& str)
{
  for (string::const_iterator i = str.begin(); i != str.end(); ++i)
    if (!isascii(*i))
      return false;

  return true;
}

/// Test if a string is made of ascii characters which are identifiers
/// acceptable in C or C++ programs.
///
///
/// In the C++ spec, [lex.charset]/2, we can read:
///
/// "if the hexadecimal value for a universal-character-name [...]  or
///  string literal corresponds to a control character (in either of
///  the ranges 0x00–0x1F or 0x7F–0x9F, both inclusive) [...] the
///  program is ill-formed."
///
/// @param str the string to consider.
///
/// @return true iff @p str is made of ascii characters, and is an
/// identifier.
bool
string_is_ascii_identifier(const string& str)
{
  for (string::const_iterator i = str.begin(); i != str.end(); ++i)
    {
      unsigned char c = *i;
    if (!isascii(c)
	|| (c <= 0x1F) // Rule out control characters
	|| (c >= 0x7F && c <= 0x9F)) // Rule out special extended
				     // ascii characters.
      return false;
    }

  return true;
}


/// The private data of the @ref temp_file type.
struct temp_file::priv
{
  char*					path_template_;
  int						fd_;
  shared_ptr<__gnu_cxx::stdio_filebuf<char> >	filebuf_;
  shared_ptr<std::iostream>			iostream_;

  priv()
  {
    const char* templat = "/tmp/libabigail-tmp-file-XXXXXX";
    int s = strlen(templat);
    path_template_ = new char[s + 1];
    memset(path_template_, 0, s + 1);
    memcpy(path_template_, templat, s);

    fd_ = mkstemp(path_template_);
    if (fd_ == -1)
      return;

    using __gnu_cxx::stdio_filebuf;
    filebuf_.reset(new stdio_filebuf<char>(fd_,
					   std::ios::in | std::ios::out));
    iostream_.reset(new std::iostream(filebuf_.get()));
  }

  ~priv()
  {
    if (fd_ && fd_ != -1)
      {
	iostream_.reset();
	filebuf_.reset();
	close(fd_);
	remove(path_template_);
      }
    delete [] path_template_;
  }
};

/// Default constructor of @ref temp_file.
///
/// It actually creates the temporary file.
temp_file::temp_file()
  : priv_(new priv)
{}

/// Test if the temporary file has been created and is usable.
///
/// @return true iff the temporary file has been created and is
/// useable.
bool
temp_file::is_good() const
{return (priv_->fd_ && priv_->fd_ != -1);}

/// Return the path to the temporary file.
///
/// @return the path to the temporary file if it's usable, otherwise
/// return nil.
const char*
temp_file::get_path() const
{
  if (is_good())
    return priv_->path_template_;

  return 0;
}

/// Get the iostream to the temporary file.
///
/// Note that the current process is aborted if this member function
/// is invoked on an instance of @ref temp_file that is not usable.
/// So please test that the instance is usable by invoking the
/// temp_file::is_good() member function on it first.
///
/// @return the iostream to the temporary file.
std::iostream&
temp_file::get_stream()
{
  assert(is_good());
  return *priv_->iostream_;
}

/// Create the temporary file and return it if it's usable.
///
/// @return the newly created temporary file if it's usable, nil
/// otherwise.
temp_file_sptr
temp_file::create()
{
  temp_file_sptr result(new temp_file);
  if (result->is_good())
    return result;

  return temp_file_sptr();
}

/// Get a pseudo random number.
///
/// @return a pseudo random number.
size_t
get_random_number()
{
  static __thread bool initialized = false;

  if (!initialized)
    {
      srand(time(NULL));
      initialized = true;
    }

  return rand();
}

/// Get a pseudo random number as string.
///
/// @return a pseudo random number as string.
string
get_random_number_as_string()
{
  std::ostringstream o;
  o << get_random_number();

  return o.str();
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
    case FILE_TYPE_DEB:
      repr = "Debian binary file type";
      break;
    case FILE_TYPE_DIR:
      repr = "Directory type";
      break;
    case FILE_TYPE_TAR:
      repr = "GNU tar archive type";
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
  const unsigned BUF_LEN = 264;
  const unsigned NB_BYTES_TO_READ = 263;

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
    {
      if (strstr(buf, "debian-binary"))
	return FILE_TYPE_DEB;
      else
	return FILE_TYPE_AR;
    }

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

  if ((unsigned char) buf[0]    == 0xed
      && (unsigned char) buf[1] == 0xab
      && (unsigned char) buf[2] == 0xee
      && (unsigned char) buf[3] == 0xdb)
    {
        if (buf[7] == 0x00)
          return FILE_TYPE_RPM;
        else if (buf[7] == 0x01)
          return FILE_TYPE_SRPM;
        else
          return FILE_TYPE_UNKNOWN;
    }

  if (buf[257]    == 'u'
      && buf[258] == 's'
      && buf[259] == 't'
      && buf[260] == 'a'
      && buf[261] == 'r')
    return FILE_TYPE_TAR;

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
  if (is_dir(file_path))
    return FILE_TYPE_DIR;

  if (string_ends_with(file_path, ".tar")
      || string_ends_with(file_path, ".tar.gz")
      || string_ends_with(file_path, ".tgz")
      || string_ends_with(file_path, ".tar.bz2")
      || string_ends_with(file_path, ".tbz2")
      || string_ends_with(file_path, ".tbz")
      || string_ends_with(file_path, ".tb2")
      || string_ends_with(file_path, ".tar.xz")
      || string_ends_with(file_path, ".txz")
      || string_ends_with(file_path, ".tar.lzma")
      || string_ends_with(file_path, ".tar.lz")
      || string_ends_with(file_path, ".tlz")
      || string_ends_with(file_path, ".tar.Z")
      || string_ends_with(file_path, ".taz")
      || string_ends_with(file_path, ".tz"))
    return FILE_TYPE_TAR;

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

/// The name of the artificial private type suppression specification
/// that is auto-generated by libabigail to suppress change reports
/// about types that are not defined in public headers.
const char* PRIVATE_TYPES_SUPPR_SPEC_NAME =
  "Artificial private types suppression specification";

/// This is a sub-routine of gen_suppr_spec_from_headers.
///
/// @param entry if this file represents a regular (or symlink) file,
/// then its file name is going to be added to the vector returned by
/// type_suppression::get_source_locations_to_keep().
///
/// @param if @p entry represents a file, then its file name is going
/// to be added to the vector returned by the method
/// type_suppression::get_source_locations_to_keep of this instance.
/// If this smart pointer is nil then a new instance @ref
/// type_suppression is created and this variable is made to point to
/// it.
static void
handle_fts_entry(const FTSENT *entry,
		 type_suppression_sptr& suppr)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return;

  string fname = entry->fts_name;
  if (!fname.empty())
    {
      if (string_ends_with(fname, ".h")
	  || string_ends_with(fname, ".hpp")
	  || string_ends_with(fname, ".hxx"))
	{
	  if (!suppr)
	    suppr.reset(new type_suppression(PRIVATE_TYPES_SUPPR_SPEC_NAME,
					     /*type_name_regexp=*/"",
					     /*type_name=*/""));
	  suppr->set_is_artificial(true);
	  suppr->set_drops_artifact_from_ir(true);
	  suppr->get_source_locations_to_keep().insert(fname);
	}
    }
}

/// Generate a type suppression specification that suppresses ABI
/// changes for types defines in source files that are *NOT* in a give
/// header root dir.
///
/// @param headers_root_dir ABI changes in types defined in files
/// *NOT* found in this directory tree are going be suppressed.
///
/// @return the resulting type suppression generated, if any file was
/// found in the directory tree @p headers_root_dir.
type_suppression_sptr
gen_suppr_spec_from_headers(const string& headers_root_dir)
{
  char* paths[] = {const_cast<char*>(headers_root_dir.c_str()), 0};
  type_suppression_sptr result;

  FTS *file_hierarchy = fts_open(paths, FTS_LOGICAL|FTS_NOCHDIR, NULL);
  if (!file_hierarchy)
    return result;

  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    handle_fts_entry(entry, result);
  fts_close(file_hierarchy);
  return result;
}

/// Get the path to the default system suppression file.
///
/// @return a copy of the default system suppression file.
string
get_default_system_suppression_file_path()
{
  string default_system_suppr_path;

  const char *s = getenv("LIBABIGAIL_DEFAULT_SYSTEM_SUPPRESSION_FILE");
  if (s)
    default_system_suppr_path = s;

  if (default_system_suppr_path.empty())
    default_system_suppr_path =
      get_system_libdir() + string("/libabigail/default.abignore");

  return default_system_suppr_path;
}

/// Get the path to the default user suppression file.
///
/// @return a copy of the default user suppression file.
string
get_default_user_suppression_file_path()
{
  string default_user_suppr_path;
  const char *s = getenv("LIBABIGAIL_DEFAULT_USER_SUPPRESSION_FILE");

  if (s == NULL)
    {
      s = getenv("HOME");
      if (s == NULL)
	return "";
      default_user_suppr_path  = s;
      if (default_user_suppr_path.empty())
	default_user_suppr_path = "~";
      default_user_suppr_path += "/.abignore";
    }
  else
    default_user_suppr_path = s;

  return default_user_suppr_path;
}

/// Load the default system suppression specification file and
/// populate a vector of @ref suppression_sptr with its content.
///
/// The default system suppression file is located at
/// $libdir/libabigail/default-libabigail.abignore.
///
/// @param supprs the vector to add the suppression specifications
/// read from the file to.
void
load_default_system_suppressions(suppr::suppressions_type& supprs)
{
  string default_system_suppr_path =
    get_default_system_suppression_file_path();

  read_suppressions(default_system_suppr_path, supprs);
}

/// Load the default user suppression specification file and populate
/// a vector of @ref suppression_sptr with its content.
///
/// The default user suppression file is located at $HOME~/.abignore.
///
/// @param supprs the vector to add the suppression specifications
/// read from the file to.
void
load_default_user_suppressions(suppr::suppressions_type& supprs)
{
  string default_user_suppr_path =
    get_default_user_suppression_file_path();

  read_suppressions(default_user_suppr_path, supprs);
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
