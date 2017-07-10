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

#include <tr1/memory>
#include <string>
#include <ostream>
#include <istream>
#include <iostream>
#include <abg-suppression.h>

namespace abigail
{

namespace tools_utils
{

using std::ostream;
using std::istream;
using std::ifstream;
using std::string;
using std::tr1::shared_ptr;

const char* get_system_libdir();
bool file_exists(const string&);
bool is_regular_file(const string&);
bool is_dir(const string&);
bool dir_exists(const string&);
bool dir_is_empty(const string &);
bool maybe_get_symlink_target_file_path(const string& file_path,
					string& target_path);
bool base_name(string const& path,
	       string& file_name);
bool dir_name(string const &path,
	      string& path_dir_name);
bool ensure_dir_path_created(const string&);
bool ensure_parent_dir_created(const string&);
ostream& emit_prefix(const string& prog_name, ostream& out);
bool check_file(const string& path, ostream& out, const string& prog_name = "");
bool check_dir(const string& path, ostream& out, const string& prog_name="");
bool string_ends_with(const string&, const string&);
bool string_begins_with(const string&, const string&);
bool string_is_ascii(const string&);
bool string_is_ascii_identifier(const string&);

suppr::type_suppression_sptr
gen_suppr_spec_from_headers(const string& hdrs_root_dir);

bool
gen_suppr_spec_from_kernel_abi_whitelist(const string& abi_whitelist_path,
					 suppr::suppressions_type& s);

bool
get_vmlinux_path_from_kernel_dist(const string&	from,
				  string&		vmlinux_path);

bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  const string&	debug_info_root_path,
				  string&		vmlinux_path,
				  vector<string>&	module_paths);

bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  string&		vmlinux_path,
				  vector<string>&	module_paths);

string
get_default_system_suppression_file_path();

string
get_default_user_suppression_file_path();

void
load_default_system_suppressions(suppr::suppressions_type&);

void
load_default_user_suppressions(suppr::suppressions_type&);

class temp_file;

/// Convenience typedef for a shared_ptr to @ref temp_file.
typedef shared_ptr<temp_file> temp_file_sptr;

/// A temporary file.
///
/// This is a helper file around the  mkstemp API.
///
/// Once the temporary file is created, users can interact with it
/// using an iostream.  They can also get the path to the newly
/// created temporary file.
///
/// When the instance of @ref temp_file is destroyed, the underlying
/// resources are de-allocated, the underlying temporary file is
/// closed and removed.
class temp_file
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  temp_file();

public:

  bool
  is_good() const;

  const char*
  get_path() const;

  std::iostream&
  get_stream();

  static temp_file_sptr
  create();
}; // end class temp_file

size_t
get_random_number();

string
get_random_number_as_string();

/// The different types of files understood the bi* suite of tools.
enum file_type
{
  /// A file type we don't know about.
  FILE_TYPE_UNKNOWN,
  /// The native xml file format representing a translation unit.
  FILE_TYPE_NATIVE_BI,
  /// An elf file.  Read this kind of file should yield an
  /// abigail::corpus type.
  FILE_TYPE_ELF,
  /// An archive (AR) file.
  FILE_TYPE_AR,
  // A native abixml file format representing a corpus of one or
  // several translation units.
  FILE_TYPE_XML_CORPUS,
  // A native abixml file format representing a corpus group of one or
  // several corpora.
  FILE_TYPE_XML_CORPUS_GROUP,
  // A zip file, possibly containing a corpus of one of several
  // translation units.
  FILE_TYPE_ZIP_CORPUS,
  /// An RPM (.rpm) binary file
  FILE_TYPE_RPM,
  /// An SRPM (.src.rpm) file
  FILE_TYPE_SRPM,
  /// A DEB (.deb) binary file
  FILE_TYPE_DEB,
  /// A plain directory
  FILE_TYPE_DIR,
  /// A tar archive.  The archive can be compressed with the popular
  /// compression schemes recognized by GNU tar.
  FILE_TYPE_TAR
};

/// Exit status for abidiff and abicompat tools.
///
/// It's actually a bit mask.  The value of each enumerator is a power
/// of two.
enum abidiff_status
{
  /// This is for when the compared ABIs are equal.
  ///
  /// Its numerical value is 0.
  ABIDIFF_OK = 0,

  /// This bit is set if there an application error.
  ///
  /// Its numerical value is 1.
  ABIDIFF_ERROR = 1,

  /// This bit is set if the tool is invoked in an non appropriate
  /// manner.
  ///
  /// Its numerical value is 2.
  ABIDIFF_USAGE_ERROR = 1 << 1,

  /// This bit is set if the ABIs being compared are different.
  ///
  /// Its numerical value is 4.
  ABIDIFF_ABI_CHANGE = 1 << 2,

  /// This bit is set if the ABIs being compared are different *and*
  /// are incompatible.
  ///
  /// Its numerical value is 8.
  ABIDIFF_ABI_INCOMPATIBLE_CHANGE = 1 << 3
};

abidiff_status
operator|(abidiff_status, abidiff_status);

abidiff_status
operator&(abidiff_status, abidiff_status);

abidiff_status&
operator|=(abidiff_status&l, abidiff_status r);

bool
abidiff_status_has_error(abidiff_status s);

bool
abidiff_status_has_abi_change(abidiff_status s);

bool
abidiff_status_has_incompatible_abi_change(abidiff_status s);

ostream&
operator<<(ostream& output, file_type r);

file_type guess_file_type(istream& in);

file_type guess_file_type(const string& file_path);

bool
get_rpm_name(const string& str, string& name);

bool
get_rpm_arch(const string& str, string& arch);

bool
get_deb_name(const string& str, string& name);

bool
file_is_kernel_package(const string& file_path,
		       file_type file_type);

bool
file_is_kernel_debuginfo_package(const string& file_path,
				 file_type file_type);

std::tr1::shared_ptr<char>
make_path_absolute(const char*p);

corpus_group_sptr
build_corpus_group_from_kernel_dist_under(const string&	root,
					  const string		debug_info_root,
					  const string&	vmlinux_path,
					  vector<string>&	suppr_paths,
					  vector<string>&	kabi_wl_paths,
					  suppr::suppressions_type&	supprs,
					  bool				verbose,
					  environment_sptr&		env);

extern const char* PRIVATE_TYPES_SUPPR_SPEC_NAME;
}// end namespace tools_utils

/// A macro that expands to aborting the program when executed.
///
/// Before aborting, the macro emits informatin about the source
/// location where it was expanded.
#define ABG_ASSERT_NOT_REACHED \
  do {									\
    std::cerr << "in " << __FUNCTION__					\
	      << " at: " << __FILE__ << ":" << __LINE__			\
	      << ": execution should not have reached this point!\n";	\
      abort();								\
  } while (false)
}//end namespace abigail
