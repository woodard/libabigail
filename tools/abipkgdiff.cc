// -*- Mode: C++ -*-
//
// Copyright (C) 2015-2016 Red Hat, Inc.
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
//
// Author: Sinny Kumari

/// @file

/// This program compares the ABIs of binaries inside two packages.
///
/// For now, the supported package formats are Deb and RPM, but
/// support for other formats would be greatly appreciated.
///
/// The program takes the two packages to compare as well as their
/// associated debug info packages.
///
/// The program extracts the content of the two packages into a
/// temporary directory , looks for the ELF binaries in there,
/// compares their ABIs and emit a report about the changes.
/// As this program uses libpthread to perform several tasks
/// concurrently, here is a coarse grain description of the sequence
/// of actions performed, including where things are done
/// concurrently.
///
/// (steps 1/ to /5 are performed in sequence)
///
/// 1/ the first package and its debug info are extracted concurrently.
/// One thread extracts the package (and maps its content) and another one
/// extracts the debug info package.
///
/// 2/ the second package and its debug info are extracted in parallel.
/// One thread extracts the package (and maps its content) and another one
/// extracts the debug info package.
///
/// 3/ the file system trees of extracted packages are traversed to
/// identify existing pairs and a list of arguments for future comparison
/// is made.  The trees are traversed concurrently.
///
/// 4/ comparisons are performed concurrently.
///
/// 5/ the reports are then emitted to standard output, always in the same
/// order.

// For package configuration macros.
#include "config.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <ftw.h>
#include <algorithm>
#include <map>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include <unistd.h>
#include <pthread.h>
#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-comparison.h"
#include "abg-suppression.h"
#include "abg-dwarf-reader.h"

using std::cout;
using std::cerr;
using std::string;
using std::ostream;
using std::vector;
using std::map;
using std::ostringstream;
using std::tr1::shared_ptr;
using abigail::tools_utils::maybe_get_symlink_target_file_path;
using abigail::tools_utils::file_exists;
using abigail::tools_utils::is_dir;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::check_file;
using abigail::tools_utils::ensure_dir_path_created;
using abigail::tools_utils::guess_file_type;
using abigail::tools_utils::string_ends_with;
using abigail::tools_utils::file_type;
using abigail::tools_utils::make_path_absolute;
using abigail::tools_utils::base_name;
using abigail::tools_utils::gen_suppr_spec_from_headers;
using abigail::tools_utils::get_default_system_suppression_file_path;
using abigail::tools_utils::get_default_user_suppression_file_path;
using abigail::tools_utils::load_default_system_suppressions;
using abigail::tools_utils::load_default_user_suppressions;
using abigail::tools_utils::abidiff_status;
using abigail::ir::corpus_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::dwarf_reader::read_context_sptr;
using abigail::dwarf_reader::create_read_context;
using abigail::dwarf_reader::get_soname_of_elf_file;
using abigail::dwarf_reader::get_type_of_elf_file;
using abigail::dwarf_reader::read_corpus_from_elf;

/// Set to true if the user wants to see verbose information about the
/// progress of what's being done.
static bool verbose;

/// The key for getting the thread-local elf_file_paths vector, which
/// contains the set of files of a given package.  The vector is populated
/// by a worker function that is invoked on each file contained in the
/// package, specifically by the
/// {first,second}_package_tree_walker_callback_fn() functions.  Its content
/// is relevant only until the mapping of the packages elf files is done.
static pthread_key_t elf_file_paths_tls_key;

/// A convenience typedef for a map of corpus diffs
typedef map<string, shared_ptr<ostringstream> > corpora_report_map;
/// This map is used to gather the computed diffs of ELF pairs
static corpora_report_map reports_map;

/// This map is used to keep environments for differing corpora
/// referenced. The environment needs to be kept alive longer than
/// all the objects that depend on it.
static map<corpus_diff_sptr, abigail::ir::environment_sptr> env_map;

/// This mutex is used to control access to the reports_map
static pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;
/// This mutex is used to control access to the pre-computed list of ELF pairs
static pthread_mutex_t arg_lock = PTHREAD_MUTEX_INITIALIZER;

/// This points to the set of options shared by all the routines of the
/// program.
static struct options *prog_options;

/// The options passed to the current program.
class options
{
  options();

public:
  string	wrong_option;
  string	wrong_arg;
  string	prog_name;
  bool		display_usage;
  bool		display_version;
  bool		missing_operand;
  bool		abignore;
  bool		parallel;
  string	package1;
  string	package2;
  string	debug_package1;
  string	debug_package2;
  string	devel_package1;
  string	devel_package2;
  bool		no_default_suppression;
  bool		keep_tmp_files;
  bool		compare_dso_only;
  bool		show_linkage_names;
  bool		show_redundant_changes;
  bool		show_locs;
  bool		show_added_syms;
  bool		show_added_binaries;
  bool		fail_if_no_debug_info;
  bool		show_identical_binaries;
  vector<string> suppression_paths;

  options(const string& program_name)
    : prog_name(program_name),
      display_usage(),
      display_version(),
      missing_operand(),
      abignore(true),
      parallel(true),
      no_default_suppression(),
      keep_tmp_files(),
      compare_dso_only(),
      show_linkage_names(true),
      show_redundant_changes(),
      show_locs(true),
      show_added_syms(true),
      show_added_binaries(true),
      fail_if_no_debug_info(),
      show_identical_binaries()
  {}
};

/// Abstract ELF files from the packages which ABIs ought to be
/// compared
class elf_file
{
private:
  elf_file();

public:
  string				path;
  string				name;
  string				soname;
  off_t 				size;
  abigail::dwarf_reader::elf_type	type;

  /// The path to the elf file.
  ///
  /// @param path the path to the elf file.
  elf_file(const string& path)
    : path(path)
   {
     abigail::tools_utils::base_name(path, name);
     get_soname_of_elf_file(path, soname);
     get_type_of_elf_file(path, type);
     struct stat estat;
     stat(path.c_str(), &estat);
     size = estat.st_size;
  }
};

/// A convenience typedef for a shared pointer to elf_file.
typedef shared_ptr<elf_file> elf_file_sptr;

/// A convenience typedef for a pointer to a function type that
/// the ftw() function accepts.
typedef int (*ftw_cb_type)(const char *, const struct stat*, int);

/// Abstract the result of comparing two packages.
///
/// This contains the the paths of the set of added binaries, removed
/// binaries, and binaries whic ABI changed.
struct abi_diff
{
  vector<elf_file_sptr> added_binaries;
  vector<elf_file_sptr> removed_binaries;
  vector<string> changed_binaries;

  /// Test if the current diff carries changes.
  ///
  /// @return true iff the current diff carries changes.
  bool
  has_changes()
  {
    return (!added_binaries.empty()
	    || !removed_binaries.empty()
	    ||!changed_binaries.empty());
  }
};

class package;

/// Convenience typedef for a shared pointer to a @ref package.
typedef shared_ptr<package> package_sptr;

/// Abstracts a package.
class package
{
public:

  /// The kind of package we are looking at.
  enum kind
  {
    /// Main package. Contains binaries to ABI-compare.
    KIND_MAIN = 0,
    /// Devel package.  Contains public headers files in which public
    /// types are defined.
    KIND_DEVEL,
    /// Debug info package.  Contains the debug info for the binaries
    /// int he main packge.
    KIND_DEBUG_INFO,
    /// Source package.  Contains the source of the binaries in the
    /// main package.
    KIND_SRC
  };

private:
  string				path_;
  string				extracted_dir_path_;
  abigail::tools_utils::file_type	type_;
  kind					kind_;
  map<string, elf_file_sptr>		path_elf_file_sptr_map_;
  package_sptr				debug_info_package_;
  package_sptr				devel_package_;
  suppressions_type			private_types_suppressions_;

public:
  /// Constructor for the @ref package type.
  ///
  /// @param path the path to the package.
  ///
  /// @parm dir the temporary directory where to extract the content
  /// of the package.
  ///
  /// @param pkg_kind the kind of package.
  package(const string&			path,
	  const string&			dir,
          kind					pkg_kind = package::KIND_MAIN)
    : path_(path),
      kind_(pkg_kind)
  {
    type_ = guess_file_type(path);
    if (type_ == abigail::tools_utils::FILE_TYPE_DIR)
      extracted_dir_path_ = path;
    else
      extracted_dir_path_ = extracted_packages_parent_dir() + "/" + dir;
  }

  /// Getter of the path of the package.
  ///
  /// @return the path of the package.
  const string&
  path() const
  {return path_;}

  /// Setter of the path of the package.
  ///
  /// @param s the new path.
  void
  path(const string& s)
  {path_ = s;}

  /// Getter for the path to the root dir where the packages are
  /// extracted.
  ///
  /// @return the path to the root dir where the packages are
  /// extracted.
  static const string&
  extracted_packages_parent_dir();

  /// Getter for the path to the directory where the packages are
  /// extracted for the current thread.
  ///
  /// @return the path to the directory where the packages are
  /// extracted for the current thread.
  const string&
  extracted_dir_path() const
  {return extracted_dir_path_;}

  /// Setter for the path to the directory where the packages are
  /// extracted for the current thread.
  ///
  /// @param p the new path.
  void
  extracted_dir_path(const string& p)
  {extracted_dir_path_ = p;}

  /// Getter for the file type of the current package.
  ///
  /// @return the file type of the current package.
  abigail::tools_utils::file_type
  type() const
  {return type_;}

  /// Setter for the file type of the current package.
  ///
  /// @param t the new file type.
  void type(abigail::tools_utils::file_type t)
  {type_ = t;}

  /// Get the package kind
  ///
  /// @return the package kind
  kind
  get_kind() const
  {return kind_;}

  /// Set the package kind
  ///
  /// @param k the package kind.
  void
  set_kind(kind k)
  {kind_ = k;}

  /// Getter for the path <-> elf_file map.
  ///
  /// @return the the path <-> elf_file map.
  const map<string, elf_file_sptr>&
  path_elf_file_sptr_map() const
  {return path_elf_file_sptr_map_;}

  /// Getter for the path <-> elf_file map.
  ///
  /// @return the the path <-> elf_file map.
  map<string, elf_file_sptr>&
  path_elf_file_sptr_map()
  {return path_elf_file_sptr_map_;}

  /// Getter for the debug info package associated to the current
  /// package.
  ///
  /// @return the debug info package associated to the current
  /// package.
  const shared_ptr<package>&
  debug_info_package() const
  {return debug_info_package_;}

  /// Setter for the debug info package associated to the current
  /// package.
  ///
  /// @param p the new debug info package.
  void
  debug_info_package(const shared_ptr<package> p)
  {debug_info_package_ = p;}

  /// Getter for the devel package associated to the current package.
  ///
  /// @return the devel package associated to the current package.
  const package_sptr&
  devel_package() const
  {return devel_package_;}

  /// Setter of the devel package associated to the current package.
  ///
  /// @param p the new devel package associated to the current package.
  void
  devel_package(const package_sptr& p)
  {devel_package_ = p;}

  /// Getter of the specifications to suppress change reports about
  /// private types.
  ///
  /// @return the vector of specifications to suppress change reports
  /// about private types.
  const suppressions_type&
  private_types_suppressions() const
  {return private_types_suppressions_;}

  /// Getter of the specifications to suppress change reports about
  /// private types.
  ///
  /// @return the vector of specifications to suppress change reports
  /// about private types.
  suppressions_type&
  private_types_suppressions()
  {return private_types_suppressions_;}

  /// Erase the content of the temporary extraction directory that has
  /// been populated by the @ref extract_package() function;
  void
  erase_extraction_directory() const
  {
    if (type() == abigail::tools_utils::FILE_TYPE_DIR)
      // If we are comparing two directories, do not erase the
      // directory as it was provided by the user; it's not a
      // temporary directory we created ourselves.
      return;

    if (verbose)
      emit_prefix("abipkgdiff", cerr)
	<< "Erasing temporary extraction directory "
	<< extracted_dir_path()
	<< " ...";

    string cmd = "rm -rf " + extracted_dir_path();
    if (system(cmd.c_str()))
      {
	if (verbose)
	  emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      }
    else
      {
	if (verbose)
	  emit_prefix("abipkgdiff", cerr) << " DONE\n";
      }
  }

  /// Erase the content of all the temporary extraction directories.
  void
  erase_extraction_directories() const
  {
    erase_extraction_directory();
    if (debug_info_package())
      debug_info_package()->erase_extraction_directory();
    if (devel_package())
      devel_package()->erase_extraction_directory();
  }
};

/// Arguments passed to the package extraction functions.
struct package_descriptor
{
  package &pkg;
  const options& opts;
  ftw_cb_type callback;
};

/// Arguments passed to the comparison workers.
struct compare_args
{
  const elf_file	elf1;
  const string&	debug_dir1;
  const suppressions_type&	private_types_suppr1;
  const elf_file	elf2;
  const string&	debug_dir2;
  const suppressions_type&	private_types_suppr2;
  const options&	opts;

  /// Constructor for compare_args, which is used to pass
  /// information to the comparison threads.
  ///
  /// @param elf1 the first elf file to consider.
  ///
  /// @param debug_dir1 the directory where the debug info file for @p
  /// elf1 is stored.
  ///
  /// @param elf2 the second elf file to consider.
  ///
  /// @param debug_dir2 the directory where the debug info file for @p
  /// elf2 is stored.
  ///
  /// @param opts the options the current program has been called with.
  compare_args(const elf_file &elf1, const string& debug_dir1,
	       const suppressions_type& priv_types_suppr1,
	       const elf_file &elf2, const string& debug_dir2,
	       const suppressions_type& priv_types_suppr2,
	       const options& opts)
    : elf1(elf1), debug_dir1(debug_dir1),
      private_types_suppr1(priv_types_suppr1),
      elf2(elf2), debug_dir2(debug_dir2),
      private_types_suppr2(priv_types_suppr2),
      opts(opts)
  {}
};
/// A convenience typedef for arguments passed to the comparison workers.
typedef shared_ptr<compare_args> compare_args_sptr;

/// Getter for the path to the parent directory under which packages
/// extracted by the current thread are placed.
///
/// @return the path to the parent directory under which packages
/// extracted by the current thread are placed.
const string&
package::extracted_packages_parent_dir()
{
  // I tried to declare this in thread-local storage, but GCC 4.4.7
  // won't let me.  So for now, I am just making it static.  I'll deal
  // with this later when I have to.

  //static __thread string p;
  static string p;

  if (p.empty())
    {
      const char *cachedir = getenv("XDG_CACHE_HOME");

      if (cachedir != NULL)
        p = cachedir;
      else
        {
	  p = getenv("HOME");
	  if (p.empty())
	    p = "~";
	  p += "/.cache/libabigail";
	  // Create directory $HOME/.cache/libabigail/ if it doesn't
	  // exist
	  bool cache_dir_is_created = ensure_dir_path_created(p);
	  assert(cache_dir_is_created);
        }
      using abigail::tools_utils::get_random_number_as_string;

      string libabigail_tmp_dir_template = p;
      libabigail_tmp_dir_template += "/abipkgdiff-tmp-dir-XXXXXX";

      if (!mkdtemp(const_cast<char*>(libabigail_tmp_dir_template.c_str())))
	abort();

      p = libabigail_tmp_dir_template;
    }

  return p;
}

/// A convenience typedef for shared_ptr of package.
typedef shared_ptr<package> package_sptr;

/// Show the usage of this program.
///
/// @param prog_name the name of the program.
///
/// @param out the output stream to emit the usage to .
static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] <package1> <package2>\n"
    << " where options can be:\n"
    << " --debug-info-pkg1|--d1 <path>  path of debug-info package of package1\n"
    << " --debug-info-pkg2|--d2 <path>  path of debug-info package of package2\n"
    << " --devel-pkg1|--devel1 <path>   path of devel package of pakage1\n"
    << " --devel-pkg2|--devel2 <path>   path of devel package of pakage1\n"
    << " --suppressions|--suppr <path>  specify supression specification path\n"
    << " --keep-tmp-files               don't erase created temporary files\n"
    << " --dso-only                     compare shared libraries only\n"
    << " --no-linkage-name		do not display linkage names of "
    "added/removed/changed\n"
    << " --redundant                    display redundant changes\n"
    << " --no-show-locs                 do not show location information\n"
    << " --no-added-syms                do not display added functions or variables\n"
    << " --no-added-binaries            do not display added binaries\n"
    << " --no-abignore                  do not look for *.abignore files\n"
    << " --no-parallel                  do not execute in parallel\n"
    << " --fail-no-dbg                  fail if no debug info was found\n"
    << " --show-identical-binaries      show the names of identical binaries\n"
    << " --verbose                      emit verbose progress messages\n"
    << " --help|-h                      display this help message\n"
    << " --version|-v                   display program version information"
    " and exit\n";
}

#ifdef WITH_RPM

/// Extract an RPM package.
///
/// @param package_path the path to the package to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// package to.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_rpm(const string& package_path,
	    const string& extracted_package_dir_path)
{
  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting package "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << "command " << cmd << " FAILED\n";
    }

  cmd = "mkdir -p " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && rpm2cpio " + package_path +
    " | cpio -dium --quiet";

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_RPM

#ifdef WITH_DEB

/// Extract a Debian binary package.
///
/// @param package_path the path to the package to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// package to.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_deb(const string& package_path,
	    const string& extracted_package_dir_path)
{
  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting package "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << "command "  << cmd <<  " FAILED\n";
    }

  cmd = "mkdir -p " + extracted_package_dir_path + " && dpkg -x " +
    package_path + " " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_DEB

#ifdef WITH_TAR

/// Extract a GNU Tar archive.
///
/// @param package_path the path to the archive to extract.
///
/// @param extracted_package_dir_path the path where to extract the
/// archive to.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_tar(const string& package_path,
	    const string& extracted_package_dir_path)
{
  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Extracting tar archive "
      << package_path
      << " to "
      << extracted_package_dir_path
      << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << "command " << cmd << " FAILED\n";
    }

  cmd = "mkdir -p " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && tar -xf " + package_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << " FAILED\n";
      return false;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr) << " DONE\n";

  return true;
}

#endif // WITH_TAR

/// Erase the temporary directories created for the extraction of two
/// packages.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
static void
erase_created_temporary_directories(const package& first_package,
				    const package& second_package)
{
  first_package.erase_extraction_directories();
  second_package.erase_extraction_directories();
}

/// Erase the root of all the temporary directories created by the
/// current thread.
static void
erase_created_temporary_directories_parent()
{
  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Erasing temporary extraction parent directory "
      << package::extracted_packages_parent_dir()
      << " ...";

  string cmd = "rm -rf " + package::extracted_packages_parent_dir();
  if (system(cmd.c_str()))
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << "FAILED\n";
    }
  else
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr) << "DONE\n";
    }
}

/// Extract the content of a package.
///
/// @param package the package we are looking at.
static bool
extract_package(const package& package)
{
  switch(package.type())
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
#ifdef WITH_RPM
      if (!extract_rpm(package.path(), package.extracted_dir_path()))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting package" << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for rpm hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_RPM
      break;
    case abigail::tools_utils::FILE_TYPE_DEB:
#ifdef WITH_DEB
      if (!extract_deb(package.path(), package.extracted_dir_path()))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting package" << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for deb hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_DEB
      break;

    case  abigail::tools_utils::FILE_TYPE_DIR:
      // The input package is just a directory that contains binaries,
      // there is nothing to extract.
      break;

    case abigail::tools_utils::FILE_TYPE_TAR:
#ifdef WITH_TAR
      if (!extract_tar(package.path(), package.extracted_dir_path()))
        {
          emit_prefix("abipkgdiff", cerr)
	    << "Error while extracting GNU tar archive "
	    << package.path() << "\n";
          return false;
        }
      return true;
#else
      emit_prefix("abipkgdiff", cerr)
	<< "Support for GNU tar hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_TAR
      break;

    default:
      return false;
    }
  return true;
}
/// A wrapper to call extract_package in a separate thread.
///
/// @param pkg the package we want to extract.
///
/// @return via pthread_exit() a pointer to a boolean value of true upon
/// successful completion, false otherwise.
static void
pthread_routine_extract_package(void *pkg)
{
  const package &package = *static_cast<class package*>(pkg);
  pthread_exit(new bool(extract_package(package)));
}

/// A callback function invoked by the ftw() function while walking
/// the directory of files extracted from the first package.
///
/// @param fpath the path to the file being considered.
///
/// @param stat the stat struct of the file.
static int
first_package_tree_walker_callback_fn(const char *fpath,
				      const struct stat *,
				      int /*flag*/)
{
  string path = fpath;
  // If path is a symbolic link, then set it to the path of its target
  // file.
  maybe_get_symlink_target_file_path(path, path);
  if (guess_file_type(path) == abigail::tools_utils::FILE_TYPE_ELF)
    {
      vector<string> *elf_file_paths
	= static_cast<vector<string>*>(pthread_getspecific(elf_file_paths_tls_key));
      elf_file_paths->push_back(path);
    }
  return 0;
}

/// A callback function invoked by the ftw() function while walking
/// the directory of files extracted from the second package.
///
/// @param fpath the path to the file being considered.
///
/// @param stat the stat struct of the file.
static int
second_package_tree_walker_callback_fn(const char *fpath,
				       const struct stat *,
				       int /*flag*/)
{
  string path = fpath;
  // If path is a symbolic link, then set it to the path of its target
  // file.
  maybe_get_symlink_target_file_path(path, path);
  if (guess_file_type(path) == abigail::tools_utils::FILE_TYPE_ELF)
    {
      vector<string> *elf_file_paths
	= static_cast<vector<string>*>(pthread_getspecific(elf_file_paths_tls_key));
      elf_file_paths->push_back(path);
    }
  /// We go through the files of the newer (second) pkg to look for
  /// suppression specifications, matching the "*.abignore" name pattern.
  else if (prog_options->abignore && string_ends_with(fpath, ".abignore"))
    prog_options->suppression_paths.push_back(fpath);

  return 0;
}

/// Check that the suppression specification files supplied are
/// present.  If not, emit an error on stderr.
///
/// @param opts the options instance to use.
///
/// @return true if all suppression specification files are present,
/// false otherwise.
static bool
maybe_check_suppression_files(const options& opts)
{
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    if (!check_file(*i, cerr, opts.prog_name))
      return false;

  return true;
}

/// Update the diff context from the @ref options data structure.
///
/// @param ctxt the diff context to update.
///
/// @param opts the instance of @ref options to consider.
static void
set_diff_context_from_opts(diff_context_sptr ctxt,
			   const options& opts)
{
  ctxt->default_output_stream(&cout);
  ctxt->error_output_stream(&cerr);
  ctxt->show_redundant_changes(opts.show_redundant_changes);
  ctxt->show_locs(opts.show_locs);
  ctxt->show_linkage_names(opts.show_linkage_names);
  ctxt->show_added_fns(opts.show_added_syms);
  ctxt->show_added_vars(opts.show_added_syms);
  ctxt->show_added_symbols_unreferenced_by_debug_info
    (opts.show_added_syms);

  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
     | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
     | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY);

  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);
  ctxt->add_suppressions(supprs);
}

/// Compare the ABI two elf files, using their associated debug info.
///
/// The result of the comparison is emitted to standard output.
///
/// @param elf1 the first elf file to consider.
///
/// @param debug_dir1 the directory where the debug info file for @p
/// elf1 is stored.
/// The result of the comparison is saved to a global corpus map.
///
/// @param elf2 the second eld file to consider.
/// @args the list of argument sets used for comparison
///
/// @param debug_dir2 the directory where the debug info file for @p
/// elf2 is stored.
///
/// @param opts the options the current program has been called with.
///
/// @param env the environment encapsulating the entire comparison.
///
/// @param diff the shared pointer to be set to the result of the comparison.
///
/// @return the status of the comparison.
static abidiff_status
compare(const elf_file& elf1,
	const string&	debug_dir1,
	const suppressions_type& priv_types_supprs1,
	const elf_file& elf2,
	const string&	debug_dir2,
	const suppressions_type& priv_types_supprs2,
	const options&	opts,
	abigail::ir::environment_sptr	&env,
	corpus_diff_sptr	&diff,
	diff_context_sptr	&ctxt)
{
  char *di_dir1 = (char*) debug_dir1.c_str(),
	*di_dir2 = (char*) debug_dir2.c_str();

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABIs of file "
      << elf1.path
      << " and "
      << elf2.path
      << "...\n";

  abigail::dwarf_reader::status c1_status = abigail::dwarf_reader::STATUS_OK,
    c2_status = abigail::dwarf_reader::STATUS_OK;

  ctxt.reset(new diff_context);
  set_diff_context_from_opts(ctxt, opts);
  suppressions_type& supprs = ctxt->suppressions();
  bool files_suppressed = (file_is_suppressed(elf1.path, supprs)
			   ||file_is_suppressed(elf2.path, supprs));

  if (files_suppressed)
    {
      if (verbose)
	emit_prefix("abipkgdiff", cerr)
	  << "  input file "
	  << elf1.path << " or " << elf2.path
	  << " has been suppressed by a suppression specification.\n"
	  << " Not reading any of them\n";
      return abigail::tools_utils::ABIDIFF_OK;
    }

  // Add the first private type suppressions set to the set of
  // suppressions.
  for (suppressions_type::const_iterator i = priv_types_supprs1.begin();
       i != priv_types_supprs1.end();
       ++i)
    supprs.push_back(*i);

  // Add the second private type suppressions set to the set of
  // suppressions.
  for (suppressions_type::const_iterator i = priv_types_supprs2.begin();
       i != priv_types_supprs2.end();
       ++i)
    supprs.push_back(*i);

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "  Reading file "
      << elf1.path
      << " ...\n";

  corpus_sptr corpus1;
  {
    read_context_sptr c = create_read_context(elf1.path, &di_dir1, env.get(),
					      /*load_all_types=*/false);
    add_read_context_suppressions(*c, priv_types_supprs1);
    corpus1 = read_corpus_from_elf(*c, c1_status);

    if (!(c1_status & abigail::dwarf_reader::STATUS_OK))
      {
	if (verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not read file '"
	    << elf1.path
	    << "' properly\n";
	return abigail::tools_utils::ABIDIFF_ERROR;
      }
  }

  if (opts.fail_if_no_debug_info
      && (c1_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND))
    {
      emit_prefix("abipkgdiff", cerr) << "Could not find debug info file";
      if (di_dir1 && strcmp(di_dir1, ""))
	emit_prefix("abipkgdiff", cerr) << " under " << di_dir1 << "\n";
      else
	emit_prefix("abipkgdiff", cerr) << "\n";

      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << " DONE reading file "
      << elf1.path
      << "\n";

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "  Reading file "
      << elf2.path
      << " ...\n";

  corpus_sptr corpus2;
  {
    read_context_sptr c = create_read_context(elf2.path, &di_dir2, env.get(),
					      /*load_all_types=*/false);
    add_read_context_suppressions(*c, priv_types_supprs2);
    corpus2 = read_corpus_from_elf(*c, c2_status);

    if (!(c2_status & abigail::dwarf_reader::STATUS_OK))
      {
	if (verbose)
	  emit_prefix("abipkgdiff", cerr)
	    << "Could not find the read file '"
	    << elf2.path
	    << "' properly\n";
	return abigail::tools_utils::ABIDIFF_ERROR;
      }
  }

  if (opts.fail_if_no_debug_info
      && (c2_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND))
    {
      emit_prefix("abipkgdiff", cerr)
	<< "Could not find debug info file";
      if (di_dir1 && strcmp(di_dir2, ""))
	emit_prefix("abipkgdiff", cerr)
	  << " under " << di_dir2 << "\n";
      else
	emit_prefix("abipkgdiff", cerr) << "\n";

      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << " DONE reading file " << elf2.path << "\n";

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "  Comparing the ABIs of: \n"
      << "    " << elf1.path << "\n"
      << "    " << elf2.path << "\n";

  diff = compute_diff(corpus1, corpus2, ctxt);

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Comparing the ABIs of file "
      << elf1.path
      << " and "
      << elf2.path
      << " is DONE\n";

  abidiff_status s = abigail::tools_utils::ABIDIFF_OK;
  if (diff->has_net_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
  if (diff->has_incompatible_changes())
    s |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

  return s;
}

/// A wrapper to call compare in a separate thread.
/// The result of the comparison is saved to a global corpus map.
///
/// @args the vector of argument sets used for comparison.
///
/// @return the status of the comparison via pthread_exit().
static void
pthread_routine_compare(vector<compare_args_sptr> *args)
{
  abidiff_status s, status = abigail::tools_utils::ABIDIFF_OK;
  compare_args_sptr a;
  corpus_diff_sptr diff;
  diff_context_sptr ctxt;

  while (true)
    {
      pthread_mutex_lock(&arg_lock);
      if (args->empty())
	a = compare_args_sptr();
      else
	{
	  a = *args->begin();
	  args->erase(args->begin());
	}
      pthread_mutex_unlock(&arg_lock);

      if (!a)
	break;

      abigail::ir::environment_sptr env(new abigail::ir::environment);
      status |= s = compare(a->elf1, a->debug_dir1, a->private_types_suppr1,
			    a->elf2, a->debug_dir2, a->private_types_suppr2,
			    a->opts, env, diff, ctxt);

      const string key = a->elf1.path;
      if ((s & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
	  || (verbose && diff->has_changes()))
	{
	  const string prefix = "  ";
	  shared_ptr<ostringstream> out(new ostringstream);
	  diff->report(*out, prefix);
	  pthread_mutex_lock(&map_lock);
	  reports_map[key] = out;
	  // We need to keep the environment around, until the corpus is
	  // report()-ed.
	  env_map[diff] = env;
	  pthread_mutex_unlock(&map_lock);
	}
      else
	{
	  pthread_mutex_lock(&map_lock);
	  if (a->opts.show_identical_binaries)
	    {
	      shared_ptr<ostringstream> out(new ostringstream);
	      *out << "No ABI change detected\n";
	      reports_map[key] = out;
	      env_map[diff] = env;
	    }
	  else
	    reports_map[key] = shared_ptr<ostringstream>();
	  pthread_mutex_unlock(&map_lock);
	}
    }

  pthread_exit(new abidiff_status(status));
}

/// Create maps of the content of a given package.
///
/// The maps contain relevant metadata about the content of the
/// files.  These maps are used afterwards during the comparison of
/// the content of the package.  Note that the maps are stored in the
/// object that represents that package.
///
/// @param package the package to consider.
///
/// @param opts the options the current program has been called with.
///
/// @param true upon successful completion, false otherwise.
static bool
create_maps_of_package_content(package& package,
			       const options& opts,
			       ftw_cb_type callback)
{
  vector<string> *elf_file_paths = new vector<string>;
  pthread_setspecific(elf_file_paths_tls_key, elf_file_paths);

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Analyzing the content of package "
      << package.path()
      << " extracted to "
      << package.extracted_dir_path()
      << " ...\n";

  if (ftw(package.extracted_dir_path().c_str(), callback, 16))
    {
      emit_prefix("abipkgdiff", cerr)
	<< "Error while inspecting files in package"
	<< package.extracted_dir_path() << "\n";
      return false;
    }

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << "Found " << elf_file_paths->size() << " files in "
      << package.extracted_dir_path() << "\n";

  for (vector<string>::const_iterator file =
	 elf_file_paths->begin();
       file != elf_file_paths->end();
       ++file)
    {
      elf_file_sptr e (new elf_file(*file));
      if (opts.compare_dso_only)
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO)
	    {
	      if (verbose)
		emit_prefix("abipkgdiff", cerr)
		  << "skipping non-DSO file " << e->path << "\n";
	      continue;
	    }
	}
      else
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO
	      && e->type != abigail::dwarf_reader::ELF_TYPE_EXEC
              && e->type != abigail::dwarf_reader::ELF_TYPE_PI_EXEC)
	    {
	      if (verbose)
		emit_prefix("abipkgdiff", cerr)
		  << "skipping non-DSO non-executable file " << e->path << "\n";
	      continue;
	    }
	}

      if (e->soname.empty())
	package.path_elf_file_sptr_map()[e->name] = e;
      else
	package.path_elf_file_sptr_map()[e->soname] = e;
    }

  pthread_setspecific(elf_file_paths_tls_key, /*value=*/NULL);
  delete elf_file_paths;

  if (verbose)
    emit_prefix("abipkgdiff", cerr)
      << " Analysis of " << package.path() << " DONE\n";
  return true;
}

/// If devel packages were associated to the main package we are
/// looking at, use the names of the header files (extracted from the
/// package) to generate suppression specification to filter out types
/// that are not defined in those header files.
///
/// Filtering out types not defined in publi headers amounts to filter
/// out types that are deemed private to the package we are looking
/// at.
///
/// If the function succeeds, the generated private type suppressions
/// are available by invoking the
/// package::private_types_suppressions() accessor of the @p pkg
/// parameter.
///
/// @param pkg the main package we are looking at.
///
/// @return true iff suppression specifications were generated for
/// types private to the package.
static bool
maybe_create_private_types_suppressions(package& pkg)
{
  if (!pkg.private_types_suppressions().empty())
    return false;

  package_sptr devel_pkg = pkg.devel_package();
  if (!devel_pkg
      || !file_exists(devel_pkg->extracted_dir_path())
      || !is_dir(devel_pkg->extracted_dir_path()))
    return false;

  string headers_path = devel_pkg->extracted_dir_path();
  if (devel_pkg->type() == abigail::tools_utils::FILE_TYPE_RPM
      ||devel_pkg->type() == abigail::tools_utils::FILE_TYPE_DEB)
    // For RPM and DEB packages, header files are under the
    // /usr/include sub-directories.
    headers_path += "/usr/include";

  if (!is_dir(headers_path))
    return false;

  suppression_sptr suppr =
    gen_suppr_spec_from_headers(headers_path);

  if (suppr)
    pkg.private_types_suppressions().push_back(suppr);

  return suppr;
}

static inline bool
pthread_join(pthread_t thr)
{
  bool *thread_retval;
  if (!pthread_join(thr, reinterpret_cast<void**>(&thread_retval)))
    {
      bool retval = *thread_retval;
      delete thread_retval;
      return retval;
    }
  else
    return false;
}

/// Extract the content of a package and map its content.
/// Also extract its accompanying debuginfo package.
///
/// The extracting is done to a temporary directory.
///
/// @param a the set of arguments needed for successful extraction;
/// specifically the package itself, the options the current package has been
/// called with and a callback to traverse the directory structure.
///
/// @return via pthread_exit() true upon successful completion, false
/// otherwise.
static void
pthread_routine_extract_pkg_and_map_its_content(package_descriptor *a)
{
  pthread_t thr_pkg, thr_debug, thr_devel;
  package& package = a->pkg;
  const options& opts = a->opts;
  ftw_cb_type callback = a->callback;
  bool has_debug_info_pkg, has_devel_pkg, result = true;

  // The debug-info package usually takes longer to extract than the main
  // package plus that package's mapping for ELFs and optionally suppression
  // specs, so we run it ASAP.
  if ((has_debug_info_pkg = package.debug_info_package()))
    {
      if (pthread_create(&thr_debug, /*attr=*/NULL,
			 reinterpret_cast<void*(*)(void*)>(pthread_routine_extract_package),
			 package.debug_info_package().get()))
	{
	  result = false;
	  goto exit;
	}

      // Wait for debug-info package extraction to complete if we're
      // not running in parallel.
      if (!opts.parallel)
	{
	  result = pthread_join(thr_debug);
	  if (!result)
	    goto exit;
	}
    }

  if ((has_devel_pkg = package.devel_package()))
    {
      // A devel package was provided for 'package'.  Let's extract it
      // too.
      if (pthread_create(&thr_devel, /*attr=*/NULL,
			 reinterpret_cast<void*(*)(void*)>
			 (pthread_routine_extract_package),
			 package.devel_package().get()))
	{
	  result = false;
	  goto exit;
	}

      // Wait for devel package extraction to complete if we're
      // not running in parallel.
      if (!opts.parallel)
	{
	  result = pthread_join(thr_devel);
	  if (!result)
	    goto exit;
	}
    }

  // Extract the package itself.
  if (pthread_create(&thr_pkg, /*attr=*/NULL,
		     reinterpret_cast<void*(*)(void*)>(pthread_routine_extract_package),
		     &package))
    result = false;

  // We need to wait for the package's successful extraction, if we
  // want to do a mapping on its files.
  if (result)
    result = pthread_join(thr_pkg);

  // If extracting the package failed, there's no sense in going further.
  if (result)
    result = create_maps_of_package_content(package, opts, callback);

  // Wait for devel package extraction to finish
  if (has_devel_pkg && opts.parallel)
    result &= pthread_join(thr_devel);

  maybe_create_private_types_suppressions(package);

  // Let's wait for debug package extractions to finish before
  // we exit.
  if (has_debug_info_pkg && opts.parallel)
    result &= pthread_join(thr_debug);

exit:
  pthread_exit(new bool(result));

}

/// Prepare the packages for comparison.
///
/// This function extracts the content of each package and maps it.
///
/// @param first_package the first package to prepare.
///
/// @param second_package the second package to prepare.
///
/// @param opts the options the current program has been called with.
///
/// @return true upon successful completion, false otherwise.
static bool
prepare_packages(package&	first_package,
		 package&	second_package,
		 const options& opts)
{
  bool result = true;
  const int npkgs = 2;
  pthread_t extract_thread[npkgs];

  package_descriptor ea[] =
    {
      {
	first_package,
	opts,
	first_package_tree_walker_callback_fn
      },
      {
	second_package,
	opts,
	second_package_tree_walker_callback_fn
      },
    };

  // Since we can't pass custom arguments to the callback of ftw(),
  // and we are going to walk two directory trees in parallel, we
  // need a separate thread-local vector of files for each package.
  pthread_key_create(&elf_file_paths_tls_key, /*destructor=*/NULL);

  for (int i = 0; i < npkgs; ++i)
    {
      pthread_create(&extract_thread[i], /*attr=*/NULL,
		     reinterpret_cast<void*(*)(void*)>(pthread_routine_extract_pkg_and_map_its_content),
		     &ea[i]);

      if (!opts.parallel)
	// We're not running in parallel, so wait for the first package set to
	// finish extracting before starting to work on the second one.
	result &= pthread_join(extract_thread[i]);
    }

  if (opts.parallel)
    {
      // We're running in parallel, so we collect the threads here.
      for (int i = 0; i < npkgs; ++i)
	result &= pthread_join(extract_thread[i]);
    }

  pthread_key_delete(elf_file_paths_tls_key);
  return result;
}

/// Compare the added sizes of a ELF pair specified by @a1
/// with the sizes of a ELF pair from @a2.
///
/// Larger filesize strongly raises the possibility of larger debug-info,
/// hence longer diff time. For a package containing several relatively
/// large and small ELFs, it is often more efficient to start working on
/// the larger ones first. This function is used to order the pairs by
/// size, starting from the largest.
///
/// @a1 the first set of arguments containing also the size information about
/// the ELF pair being compared.
///
/// @a2 the second set of arguments containing also the size information about
/// the ELF pair being compared.
bool
elf_size_is_greater(const compare_args_sptr &a1, const compare_args_sptr &a2)
{
  off_t s1 = a1->elf1.size + a1->elf2.size;
  off_t s2 = a2->elf1.size + a2->elf2.size;

  return s1 > s2;
}

/// Compare the ABI of two packages
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param options the options the current program has been called
/// with.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @return the status of the comparison.
static abidiff_status
compare(package&	first_package,
	package&	second_package,
	const options&	opts,
	abi_diff&	diff)
{
  if (!prepare_packages(first_package, second_package, opts))
    return abigail::tools_utils::ABIDIFF_ERROR;

  // Setting debug-info path of libraries
  string debug_dir1, debug_dir2, relative_debug_path = "/usr/lib/debug/";
  if (first_package.debug_info_package()
      && second_package.debug_info_package())
    {
      debug_dir1 =
	first_package.debug_info_package()->extracted_dir_path() +
	relative_debug_path;
      if (second_package.debug_info_package())
	debug_dir2 =
	  second_package.debug_info_package()->extracted_dir_path() +
	  relative_debug_path;
    }

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;

  vector<compare_args_sptr> elf_pairs;

  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map().begin();
       it != first_package.path_elf_file_sptr_map().end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map().find(it->first);

      if (iter != second_package.path_elf_file_sptr_map().end()
	  && (iter->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC
              || iter->second->type == abigail::dwarf_reader::ELF_TYPE_PI_EXEC))
	{
	  elf_pairs.push_back
	    (compare_args_sptr (new compare_args
				(*it->second,
				 debug_dir1,
				 first_package.private_types_suppressions(),
				 *iter->second,
				 debug_dir2,
				 second_package.private_types_suppressions(),
				 opts)));
	}
      else
	{
	  diff.removed_binaries.push_back(it->second);
	  status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;
	  status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
	}
    }

  // Larger elfs are processed first, since it's usually safe to assume
  // their debug-info is larger as well, but the results are still
  // in a map ordered by looked up in elf.name order.
  std::sort(elf_pairs.begin(), elf_pairs.end(), elf_size_is_greater);

  size_t nprocs = opts.parallel ? sysconf(_SC_NPROCESSORS_ONLN) : 1;
  assert(nprocs >= 1);
  // There's no reason to spawn more threads than there are pairs to be diffed.
  nprocs = std::min(nprocs, elf_pairs.size());

  // if nprocs is zero, it means we have no binary to compare.

  // We've identified the elf couples to compare, let's spawn NPROCS threads
  // to do comparisons.
  pthread_t thr;
  vector<pthread_t> waitlist;
  for (size_t i = 0; i < nprocs; ++i)
    {
      if (!pthread_create(&thr, /*attr=*/NULL,
			  reinterpret_cast<void*(*)(void*)>(pthread_routine_compare),
			  &elf_pairs))
        // Record all the threads we will be waiting for.
	waitlist.push_back(thr);
    }

  // Let's iterate over the valid ELF pairs in-order again, this time
  // waiting for their diffs to come up from the other threads and reporting
  // them ASAP.
  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map().begin();
       it != first_package.path_elf_file_sptr_map().end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map().find(it->first);

      if (iter != second_package.path_elf_file_sptr_map().end()
	  && (iter->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC
              || iter->second->type == abigail::dwarf_reader::ELF_TYPE_PI_EXEC))
	{
	  second_package.path_elf_file_sptr_map().erase(iter);
	  while (true)
	    {
	      pthread_mutex_lock(&map_lock);
	      corpora_report_map::iterator d = reports_map.find(it->second->path);
	      pthread_mutex_unlock(&map_lock);

	      if (d == reports_map.end())
		// No result yet.
		continue;
	      if (!d->second)
		// The objects match.
		break;
	      else
		{
		  // Report exist, emit it.
		  string name = it->second->name;
		  diff.changed_binaries.push_back(name);
		  const string prefix = "  ";

		  cout << "================ changes of '"
		       << name
		       << "'===============\n";
		  cout << d->second->str();

		  cout << "================ end of changes of '"
		       << name
		       << "'===============\n\n";

		  pthread_mutex_lock(&map_lock);
		  reports_map.erase(d);
		  pthread_mutex_unlock(&map_lock);

		  break;
		}
	    }
	}
    }

  abidiff_status *s;
  // Join the comparison threads and collect the statuses.
  for (vector<pthread_t>::iterator it = waitlist.begin(); it != waitlist.end();
       ++it)
    {
      pthread_join(*it, reinterpret_cast<void **>(&s));
      status |= *s;
      delete s;
    }

  for (map<string, elf_file_sptr>::iterator it =
	 second_package.path_elf_file_sptr_map().begin();
       it != second_package.path_elf_file_sptr_map().end();
       ++it)
    diff.added_binaries.push_back(it->second);

  if (diff.removed_binaries.size())
    {
      cout << "Removed binaries:\n";
      for (vector<elf_file_sptr>::iterator it = diff.removed_binaries.begin();
	   it != diff.removed_binaries.end(); ++it)
	{
	  cout << "  " << (*it)->name << ", ";
	  string soname;
	  get_soname_of_elf_file((*it)->path, soname);
	  if (!soname.empty())
	    cout << "SONAME: " << soname;
	  else
	    cout << "no SONAME";
	  cout << "\n";
	}
    }

  if (opts.show_added_binaries && diff.added_binaries.size())
    {
      cout << "Added binaries:\n";
      for (vector<elf_file_sptr>::iterator it = diff.added_binaries.begin();
	   it != diff.added_binaries.end(); ++it)
	{
	  cout << "  " << (*it)->name << ", ";
	  string soname;
	  get_soname_of_elf_file((*it)->path, soname);
	  if (!soname.empty())
	    cout << "SONAME: " << soname;
	  else
	    cout << "no SONAME";
	  cout << "\n";
	}
    }

  if (!opts.keep_tmp_files)
    {
      erase_created_temporary_directories(first_package, second_package);
      erase_created_temporary_directories_parent();
    }

  return status;
}

/// Compare the ABI of two packages.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param opts the options the current program has been called with.
///
/// @return the status of the comparison.
static abidiff_status
compare(package& first_package, package& second_package, const options& opts)
{
  abi_diff diff;
  return compare(first_package, second_package, opts, diff);
}

/// Parse the command line of the current program.
///
/// @param argc the number of arguments in the @p argv parameter.
///
/// @param argv the array of arguemnts passed to the function.  The
/// first argument is the name of this program.
///
/// @param opts the resulting options.
///
/// @return true upon successful parsing.
static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
        {
          if (opts.package1.empty())
            opts.package1 = abigail::tools_utils::make_path_absolute(argv[i]).get();
          else if (opts.package2.empty())
            opts.package2 = abigail::tools_utils::make_path_absolute(argv[i]).get();
          else
	    {
	      opts.wrong_arg = argv[i];
	      return false;
	    }
        }
      else if (!strcmp(argv[i], "--debug-info-pkg1")
	       || !strcmp(argv[i], "--d1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.debug_package1 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--debug-info-pkg2")
	       || !strcmp(argv[i], "--d2"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.debug_package2 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--devel-pkg1")
	       || !strcmp(argv[i], "--devel1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.devel_package1 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--devel-pkg2")
	       || !strcmp(argv[i], "--devel2"))
        {
          int j = i + 1;
          if (j >= argc)
            {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
            }
          opts.devel_package2 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--no-default-suppression"))
	opts.no_default_suppression = true;
      else if (!strcmp(argv[i], "--keep-tmp-files"))
	opts.keep_tmp_files = true;
      else if (!strcmp(argv[i], "--dso-only"))
	opts.compare_dso_only = true;
      else if (!strcmp(argv[i], "--no-linkage-name"))
	opts.show_linkage_names = false;
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant_changes = true;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--no-added-syms"))
	opts.show_added_syms = false;
      else if (!strcmp(argv[i], "--no-added-binaries"))
	opts.show_added_binaries = false;
      else if (!strcmp(argv[i], "--fail-no-dbg"))
	opts.fail_if_no_debug_info = true;
      else if (!strcmp(argv[i], "--verbose"))
	verbose = true;
      else if (!strcmp(argv[i], "--no-abignore"))
	opts.abignore = false;
      else if (!strcmp(argv[i], "--no-parallel"))
	opts.parallel = false;
      else if (!strcmp(argv[i], "--show-identical-binaries"))
	opts.show_identical_binaries = true;
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
        {
          opts.display_usage = true;
          return true;
        }
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return true;
	}
      else
	{
	  if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	    opts.wrong_option = argv[i];
	  return false;
	}
    }

  return true;
}

int
main(int argc, char* argv[])
{
  options opts(argv[0]);
  prog_options = &opts;
  vector<package_sptr> packages;
  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.wrong_option.empty())
	emit_prefix("abipkgdiff", cerr)
	  << "unrecognized option: " << opts.wrong_option
	  << "\ntry the --help option for more information\n";
      else
	emit_prefix("abipkgdiff", cerr)
	  << "unrecognized argument: " << opts.wrong_arg
	  << "\ntry the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.missing_operand)
    {
      emit_prefix("abipkgdiff", cerr)
	<< "missing operand\n"
        "try the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_version)
    {
      string major, minor, revision;
      abigail::abigail_get_library_version(major, minor, revision);
      cout << major << "." << minor << "." << revision << "\n";
      return 0;
    }

    if (!opts.no_default_suppression && opts.suppression_paths.empty())
    {
      // Load the default system and user suppressions.
      string default_system_suppr_file =
	get_default_system_suppression_file_path();
      if (file_exists(default_system_suppr_file))
	opts.suppression_paths.push_back(default_system_suppr_file);

      string default_user_suppr_file =
	get_default_user_suppression_file_path();
      if (file_exists(default_user_suppr_file))
	opts.suppression_paths.push_back(default_user_suppr_file);
    }

  if (!maybe_check_suppression_files(opts))
    return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	    | abigail::tools_utils::ABIDIFF_ERROR);

  if (opts.package1.empty() || opts.package2.empty())
    {
      emit_prefix("abipkgdiff", cerr)
	<< "Please enter two packages to compare" << "\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  package_sptr first_package(new package(opts.package1, "package1"));

  package_sptr second_package(new package(opts.package2, "package2"));

  if (!opts.debug_package1.empty())
    first_package->debug_info_package
      (package_sptr(new package(opts.debug_package1,
				"debug_package1",
				/*pkg_kind=*/package::KIND_DEBUG_INFO)));

  if (!opts.debug_package2.empty())
    second_package->debug_info_package
      (package_sptr(new package(opts.debug_package2,
				"debug_package2",
				/*pkg_kind=*/package::KIND_DEBUG_INFO)));

  if (!opts.devel_package1.empty())
    first_package->devel_package
      (package_sptr(new package(opts.devel_package1,
				"devel_package1",
				/*pkg_kind=*/package::KIND_DEVEL)));
    ;

  if (!opts.devel_package2.empty())
    second_package->devel_package
      (package_sptr(new package(opts.devel_package2,
				"devel_package2",
				/*pkg_kind=*/package::KIND_DEVEL)));

  switch (first_package->type())
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_RPM)
	{
	  emit_prefix("abipkgdiff", cerr)
	    << opts.package2 << " should be an RPM file\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    case abigail::tools_utils::FILE_TYPE_DEB:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_DEB)
	{
	  emit_prefix("abipkgdiff", cerr)
	    << opts.package2 << " should be a DEB file\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    case abigail::tools_utils::FILE_TYPE_DIR:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_DIR)
	{
	  emit_prefix("abipkgdiff", cerr)
	    << opts.package2 << " should be a directory\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    case abigail::tools_utils::FILE_TYPE_TAR:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_TAR)
	{
	  emit_prefix("abipkgdiff", cerr)
	    << opts.package2 << " should be a GNU tar archive\n";
	  return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
		  | abigail::tools_utils::ABIDIFF_ERROR);
	}
      break;

    default:
      emit_prefix("abipkgdiff", cerr)
	<< opts.package1 << " should be a valid package file \n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  return compare(*first_package, *second_package, opts);
}
