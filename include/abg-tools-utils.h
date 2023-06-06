// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2023 Red Hat, Inc.

///@file

#ifndef __ABG_TOOLS_UTILS_H
#define __ABG_TOOLS_UTILS_H

#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include "abg-suppression.h"
#include "abg-elf-based-reader.h"

namespace abigail
{

namespace tools_utils
{

using std::ostream;
using std::istream;
using std::ifstream;
using std::string;
using std::set;
using std::shared_ptr;

const char* get_system_libdir();
const char* get_anonymous_struct_internal_name_prefix();
const char* get_anonymous_union_internal_name_prefix();
const char* get_anonymous_enum_internal_name_prefix();
const char* get_anonymous_subrange_internal_name_prefix();

bool file_exists(const string&);
bool is_regular_file(const string&);
bool file_has_dwarf_debug_info(const string& elf_file_path,
			       const vector<char**>& debug_info_root_paths);
bool file_has_ctf_debug_info(const string& elf_file_path,
			     const vector<char**>& debug_info_root_paths);
bool file_has_btf_debug_info(const string& elf_file_path,
			     const vector<char**>& debug_info_root_paths);
bool is_dir(const string&);
bool dir_exists(const string&);
bool dir_is_empty(const string &);
bool decl_names_equal(const string&, const string&);
bool maybe_get_symlink_target_file_path(const string& file_path,
					string& target_path);
bool base_name(string const& path,
	       string& file_name);
bool dir_name(string const &path,
	      string& path_dir_name,
	      bool keep_separator_at_end=false);
void real_path(const string&path, string& realpath);
bool ensure_dir_path_created(const string&);
bool ensure_parent_dir_created(const string&);
ostream& emit_prefix(const string& prog_name, ostream& out);
bool check_file(const string& path, ostream& out, const string& prog_name = "");
bool check_dir(const string& path, ostream& out, const string& prog_name="");
bool string_ends_with(const string&, const string&);
bool string_begins_with(const string&, const string&);
bool string_is_ascii(const string&);
bool string_is_ascii_identifier(const string&);
bool split_string(const string&, const string&, vector<string>&);
bool string_suffix(const string&, const string&, string&);
bool sorted_strings_common_prefix(vector<string>&, string&);
string get_library_version_string();
string get_abixml_version_string();
bool execute_command_and_get_output(const string&, vector<string>&);
bool get_dsos_provided_by_rpm(const string& rpm_path,
			      set<string>& provided_dsos);
string trim_white_space(const string&);
string trim_leading_string(const string& from, const string& to_trim);
void convert_char_stars_to_char_star_stars(const vector<char*>&,
					   vector<char**>&);

suppr::type_suppression_sptr
gen_suppr_spec_from_headers(const string& hdrs_root_dir);

suppr::type_suppression_sptr
gen_suppr_spec_from_headers(const string& hdrs_root_dir,
			    const vector<string>& hdr_files);

suppr::type_suppression_sptr
gen_suppr_spec_from_headers(const vector<string>& headers_root_dirs,
			    const vector<string>& header_files);

suppr::suppressions_type
gen_suppr_spec_from_kernel_abi_whitelists
   (const vector<string>& abi_whitelist_paths);

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

bool
find_file_under_dir(const string& root_dir,
		    const string& file_path_to_look_for,
		    string& result);

class temp_file;

/// Convenience typedef for a shared_ptr to @ref temp_file.
typedef shared_ptr<temp_file> temp_file_sptr;

/// A temporary file.
///
/// This is a helper file around the  mkstemp API.
///
/// Once the temporary file is created, users can interact with it
/// using an fstream.  They can also get the path to the newly
/// created temporary file.
///
/// When the instance of @ref temp_file is destroyed, the underlying
/// resources are de-allocated, the underlying temporary file is
/// closed and removed.
class temp_file
{
  struct priv;
  std::unique_ptr<priv> priv_;

  temp_file();

public:

  bool
  is_good() const;

  const char*
  get_path() const;

  std::fstream&
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
  ABITOOL_OK = 0,

  /// This bit is set if there is an application error.
  ///
  /// Its numerical value is 1.
  ABITOOL_ERROR = 1,

  /// This bit is set if the tool is invoked in an non appropriate
  /// manner.
  ///
  /// Its numerical value is 2.
  ABITOOL_USAGE_ERROR = 1 << 1,

  /// This bit is set if the ABIs being compared are different.
  ///
  /// Its numerical value is 4.
  ABITOOL_ABI_CHANGE = 1 << 2,

  /// This bit is set if the ABIs being compared are different *and*
  /// are incompatible.
  ///
  /// Its numerical value is 8.
  ABITOOL_ABI_INCOMPATIBLE_CHANGE = 1 << 3
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

/// A type used to time various part of the libabigail system.
class timer
{
  struct priv;
  std::unique_ptr<priv> priv_;

public:
  enum kind
  {
    /// Default timer kind.
    DEFAULT_TIMER_KIND = 0,
    /// This kind of timer starts upon instantiation.
    START_ON_INSTANTIATION_TIMER_KIND = 1,
  };

  timer (kind k = DEFAULT_TIMER_KIND);
  bool start();
  bool stop();
  time_t value_in_seconds() const;
  bool value(time_t& hours,
	     time_t& minutes,
	     time_t& seconds,
	     time_t& milliseconds) const;
  string value_as_string() const;
  ~timer();
}; //end class timer

ostream& operator<<(ostream&, const timer&);

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
rpm_contains_file(const string& rpm_path,
		  const string& file_name);

bool
file_is_kernel_debuginfo_package(const string& file_path,
				 file_type file_type);

std::shared_ptr<char>
make_path_absolute(const char*p);

char*
make_path_absolute_to_be_freed(const char*p);

corpus_group_sptr
build_corpus_group_from_kernel_dist_under(const string&	root,
					  const string		debug_info_root,
					  const string&	vmlinux_path,
					  vector<string>&	suppr_paths,
					  vector<string>&	kabi_wl_paths,
					  suppr::suppressions_type&	supprs,
					  bool				verbose,
					  environment&			env,
					  corpus::origin	requested_fe_kind = corpus::DWARF_ORIGIN);

class best_elf_based_reader_opts
{
public:
  bool show_all_types;
  bool linux_kernel_mode;
  string elf_file_path;
  vector<char**> debug_info_root_paths;
  environment &env;
  corpus::origin requested_fe_kind;

  best_elf_based_reader_opts( environment &e):
    show_all_types(false),
    linux_kernel_mode(true),
    env(e)
  {}

  ~best_elf_based_reader_opts()
  {
    debug_info_root_paths.clear();
  }
};

elf_based_reader_sptr
create_best_elf_based_reader(const string& elf_file_path,
			     const vector<char**>& debug_info_root_paths,
			     environment& env,
			     corpus::origin requested_debug_info_kind,
			     bool show_all_types,
			     bool linux_kernel_mode = false);

inline elf_based_reader_sptr
create_best_elf_based_reader( best_elf_based_reader_opts& reader_opts){
  return create_best_elf_based_reader(reader_opts.elf_file_path,
				      reader_opts.debug_info_root_paths,
				      reader_opts.env,
				      reader_opts.requested_fe_kind,
				      reader_opts.show_all_types,
				      reader_opts.linux_kernel_mode);
}

class options_base
{
  /// Check that the suppression specification files supplied are
  /// present.  If not, emit an error on stderr.
  ///
  /// @param progname the program name
  ///
  /// @return true if all suppression specification files are present,
  /// false otherwise.
  bool maybe_check_suppression_files(const char *progname);
public:
  vector<string>	     suppression_paths;
  vector<string>	     kabi_whitelist_paths;
  string		     wrong_option;
  bool                       missing_operand;
  bool			     show_stats;
  bool			     do_log;
#ifdef WITH_CTF
  bool			     use_ctf;
#endif
#ifdef WITH_BTF
  bool			     use_btf;
#endif
#ifdef WITH_DEBUG_SELF_COMPARISON
  bool			     debug_abidiff;
#endif
#ifdef WITH_DEBUG_TYPE_CANONICALIZATION
  bool			     debug_type_canonicalization;
  bool			     debug_die_canonicalization;
#endif
  vector<char*>	             di_root_paths;
  environment                env;
  best_elf_based_reader_opts reader_opts;

  options_base()
    : missing_operand(false),
      show_stats(false),
      do_log(false),
#ifdef WITH_CTF
      use_ctf(false),
#endif
#ifdef WITH_BTF
      use_btf(false),
#endif
#ifdef WITH_DEBUG_SELF_COMPARISON
      debug_abidiff(false),
#endif
#ifdef WITH_DEBUG_TYPE_CANONICALIZATION
      debug_type_canonicalization(false),
      debug_die_canonicalization(false),
#endif
      reader_opts(env)
  {}
  ~options_base()
  {
    for (vector<char*>::iterator i = di_root_paths.begin();
	 i != di_root_paths.end();
	 ++i)
      free(*i);
  }

  bool common_options( int argc, char* argv[], int &i, const char usage[]);
  bool complete_parse(const char *progname);
};

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

#endif //__ABG_TOOLS_UTILS_H
