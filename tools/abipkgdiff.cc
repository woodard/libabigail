// -*- Mode: C++ -*-
//
// Copyright (C) 2015 Red Hat, Inc.
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

// For package configuration macros.
#include "config.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <ftw.h>
#include <map>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include "abg-tools-utils.h"
#include "abg-comparison.h"
#include "abg-dwarf-reader.h"

using std::cout;
using std::cerr;
using std::string;
using std::ostream;
using std::vector;
using std::map;
using std::tr1::shared_ptr;
using abigail::tools_utils::guess_file_type;
using abigail::tools_utils::file_type;
using abigail::tools_utils::make_path_absolute;
using abigail::tools_utils::abidiff_status;
using abigail::ir::corpus_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::suppression_sptr;
using abigail::comparison::suppressions_type;
using abigail::comparison::read_suppressions;
using abigail::dwarf_reader::get_soname_of_elf_file;
using abigail::dwarf_reader::get_type_of_elf_file;
using abigail::dwarf_reader::read_corpus_from_elf;

/// Set to true if the user wants to see verbose information about the
/// progress of what's being done.
static bool verbose;

/// This contains the set of files of a given package.  It's populated
/// by a worker function that is invoked on each file contained in the
/// package.  So this global variable is filled by the
/// file_tree_walker_callback_fn() function.  Its content is relevant
/// only during the time after which the current package has been
/// analyzed and before we start analyzing the next package.
static vector<string> elf_file_paths;

/// The options passed to the current program.
struct options
{
  bool		display_usage;
  bool		missing_operand;
  string	package1;
  string	package2;
  string	debug_package1;
  string	debug_package2;
  bool		keep_tmp_files;
  bool		compare_dso_only;
  bool		show_linkage_names;
  bool		show_redundant_changes;
  bool		show_added_syms;
  bool		show_added_binaries;
  bool		fail_if_no_debug_info;
  vector<string> suppression_paths;

  options()
    : display_usage(),
      missing_operand(),
      keep_tmp_files(),
      compare_dso_only(),
      show_linkage_names(true),
      show_redundant_changes(),
      show_added_syms(true),
      show_added_binaries(true),
      fail_if_no_debug_info()
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
  }
};

/// A convenience typedef for a shared pointer to elf_file.
typedef shared_ptr<elf_file> elf_file_sptr;

/// Abstract the result of comparing two packages.
///
/// This contains the the paths of the set of added binaries, removed
/// binaries, and binaries whic ABI changed.
struct abi_diff
{
  vector <string> added_binaries;
  vector <string> removed_binaries;
  vector <string> changed_binaries;

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

/// Abstracts a package.
class package
{
  string				path_;
  string				extracted_dir_path_;
  abigail::tools_utils::file_type	type_;
  bool					is_debug_info_;
  map<string, elf_file_sptr>		path_elf_file_sptr_map_;
  shared_ptr<package>			debug_info_package_;

public:

  /// Constructor for the @ref package type.
  ///
  /// @param path the path to the package.
  ///
  /// @parm dir the temporary directory where to extract the content
  /// of the package.
  ///
  /// @param is_debug_info true if the pacakge is a debug info package.
  package(const string&			path,
	  const string&			dir,
          bool					is_debug_info = false)
    : path_(path),
      is_debug_info_(is_debug_info)
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

  /// Test if the current package is a debug info package.
  ///
  /// @return true iff the current package is a debug info package.
  bool
  is_debug_info() const
  {return is_debug_info_;}

  /// Set the flag that says if the current package is a debug info package.
  ///
  /// @param f the new flag.
  void
  is_debug_info(bool f)
  {is_debug_info_ = f;}

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
      cerr << "Erasing temporary extraction directory "
	   << extracted_dir_path()
	   << " ...";

    string cmd = "rm -rf " + extracted_dir_path();
    system(cmd.c_str());

    if (verbose)
      cerr << " DONE\n";
  }

  /// Erase the content of all the temporary extraction directories.
  void
  erase_extraction_directories() const
  {
    erase_extraction_directory();
    if (debug_info_package())
      debug_info_package()->erase_extraction_directory();
  }
};

/// Getter for the path to the parent directory under which packages
/// extracted by the current thread are placed.
///
/// @return the path to the parent directory under which packages
/// extracted by the current thread are placed.
const string&
package::extracted_packages_parent_dir()
{
  // I tried to declare this in thread-local storage, but GCC 4.4.7
  //won't let me.  So for now, I am just making it static.  I'll deal
  //with this later when time of multi-threading comes.

  //static __thread string p;
  static string p;

  if (p.empty())
    {
      const char *tmpdir = getenv("TMPDIR");

      if (tmpdir != NULL)
	p = tmpdir;
      else
	p = "/tmp";

      using abigail::tools_utils::get_random_number_as_string;

      p = p + "/libabigail-tmp-dir-" + get_random_number_as_string();
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
  out << "usage: " << prog_name << " [options] <package1> <package2>\n"
      << " where options can be:\n"
      << " --debug-info-pkg1|--d1 <path>  path of debug-info package of package1\n"
      << " --debug-info-pkg2|--d2 <path>  path of debug-info package of package2\n"
      << " --suppressions|--suppr <path>  specify supression specification path\n"
      << " --keep-tmp-files               don't erase created temporary files\n"
      << " --dso-only                     compare shared libraries only\n"
      << " --no-linkage-name		  do not display linkage names of "
                                          "added/removed/changed\n"
      << " --redundant                    display redundant changes\n"
      << " --no-added-syms                do not display added functions or variables\n"
      << " --no-added-binaries            do not display added binaries\n"
      << " --fail-no-dbg                  fail if no debug info was found\n"
      << " --verbose                      emit verbose progress messages\n"
      << " --help|-h                      display this help message\n";
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
    cerr << "Extracting package "
	 << package_path
	 << " to "
	 << extracted_package_dir_path
	 << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  system(cmd.c_str());

  cmd = "mkdir -p " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && rpm2cpio " + package_path +
    " | cpio -dium --quiet";

  if (system(cmd.c_str()))
    {
      if (verbose)
	cerr << " FAILED\n";
      return false;
    }

  if (verbose)
    cerr << " DONE\n";

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
    cerr << "Extracting package "
	 << package_path
	 << "to "
	 << extracted_package_dir_path
	 << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  system(cmd.c_str());

  cmd = "mkdir -p " + extracted_package_dir_path + " && dpkg -x " +
    package_path + " " + extracted_package_dir_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	cerr << " FAILED\n";
      return false;
    }

  if (verbose)
    cerr << " DONE\n";

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
    cerr << "Extracting tar archive "
	 << package_path
	 << " to "
	 << extracted_package_dir_path
	 << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  system(cmd.c_str());

  cmd = "mkdir -p " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && tar -xf " + package_path;

  if (system(cmd.c_str()))
    {
      if (verbose)
	cerr << " FAILED\n";
      return false;
    }

  if (verbose)
    cerr << " DONE\n";

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
    cerr << "Erasing temporary extraction parent directory "
	 << package::extracted_packages_parent_dir()
	 << " ...";

  string cmd = "rm -rf " + package::extracted_packages_parent_dir();
  system(cmd.c_str());

  if (verbose)
    cerr << "DONE\n";
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
          cerr << "Error while extracting package" << package.path() << "\n";
          return false;
        }
      return true;
#else
      cerr << "Support for rpm hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_RPM
      break;
    case abigail::tools_utils::FILE_TYPE_DEB:
#ifdef WITH_DEB
      if (!extract_deb(package.path(), package.extracted_dir_path()))
        {
          cerr << "Error while extracting package" << package.path() << "\n";
          return false;
        }
      return true;
#else
      cerr << "Support for deb hasn't been enabled.  Please consider "
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
          cerr << "Error while extracting GNU tar archive"
	       << package.path() << "\n";
          return false;
        }
      return true;
#else
      cerr << "Support for GNU tar hasn't been enabled.  Please consider "
	"enabling it at package configure time\n";
      return false;
#endif // WITH_TAR
      break;

    default:
      return false;
    }
  return true;
}

/// A callback function invoked by the ftw() function while walking
/// the directory of files extracted from a given package.
///
/// @param fpath the path to the file being considered.
///
/// @param stat the stat struct of the file.
static int
file_tree_walker_callback_fn(const char *fpath,
			     const struct stat *,
			     int /*flag*/)
{
  struct stat s;
  lstat(fpath, &s);

  if (!S_ISLNK(s.st_mode))
    {
      if (guess_file_type(fpath) == abigail::tools_utils::FILE_TYPE_ELF)
	elf_file_paths.push_back(fpath);
    }
  return 0;
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
///
/// @param elf2 the second eld file to consider.
///
/// @param debug_dir2 the directory where the debug info file for @p
/// elf2 is stored.
///
/// @param opts the options the current program has been called with.
///
/// @return the status of the comparison.
static abidiff_status
compare(const elf_file& elf1,
	const string&	debug_dir1,
	const elf_file& elf2,
	const string&	debug_dir2,
	const options&	opts)
{
  char *di_dir1 = (char*) debug_dir1.c_str(),
    *di_dir2 = (char*) debug_dir2.c_str();

  if (verbose)
    cerr << "Comparing the ABIs of file "
	 << elf1.path
	 << " and "
	 << elf2.path
	 << "...\n";

  abigail::dwarf_reader::status c1_status = abigail::dwarf_reader::STATUS_OK,
    c2_status = abigail::dwarf_reader::STATUS_OK;

  if (verbose)
    cerr << "  Reading file "
	 << elf1.path
	 << " ...";

  corpus_sptr corpus1 = read_corpus_from_elf(elf1.path, &di_dir1,
					     /*load_all_types=*/false,
					     c1_status);
  if (!(c1_status & abigail::dwarf_reader::STATUS_OK))
    {
      if (verbose)
	cerr << "could not read file '"
	     << elf1.path
	     << "' properly\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (opts.fail_if_no_debug_info
      && (c1_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND))
    {
      cerr << "Could not find debug info file";
      if (di_dir1 && strcmp(di_dir1, ""))
	cerr << " under " << di_dir1 << "\n";
      else
	cerr << "\n";

      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (verbose)
    cerr << " DONE\n";

  if (verbose)
    cerr << "  Reading file "
	 << elf2.path
	 << " ...";

  corpus_sptr corpus2 = read_corpus_from_elf(elf2.path, &di_dir2,
					     /*load_all_types=*/false,
					     c2_status);
  if (!(c2_status & abigail::dwarf_reader::STATUS_OK))
    {
      if (verbose)
	cerr << "could not find the read file '"
	     << elf2.path
	     << "' properly\n";
      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (opts.fail_if_no_debug_info
      && (c2_status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND))
    {
      cerr << "Could not find debug info file";
      if (di_dir1 && strcmp(di_dir2, ""))
	cerr << " under " << di_dir2 << "\n";
      else
	cerr << "\n";

      return abigail::tools_utils::ABIDIFF_ERROR;
    }

  if (verbose)
    cerr << " DONE\n";

  if (verbose)
    cerr << "  Comparing the ABI of the two files ...";

  diff_context_sptr ctxt(new diff_context);
  set_diff_context_from_opts(ctxt, opts);
  corpus_diff_sptr diff = compute_diff(corpus1, corpus2, ctxt);

  if (verbose)
    cerr << "DONE\n";

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  if (diff->has_net_changes())
    status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
  if (diff->has_incompatible_changes())
    status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;
  if ((status & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
      || (verbose && diff->has_changes()))
    {
      const string prefix = "  ";

      cout << "================ changes of '"
	   << elf1.name
	   << "'===============\n";

      diff->report(cout, prefix);

      cout << "================ end of changes of '"
	   << elf1.name
	   << "'===============\n\n";
    }

  if (verbose)
    cerr << "Comparing the ABIs of file "
	 << elf1.path
	 << " and "
	 << elf2.path
	 << " is DONE\n";

  return status;
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
			       const options& opts)
{
  elf_file_paths.clear();
  if (verbose)
    cerr << "Analyzing the content of package "
	 << package.path()
	 << " extracted to "
	 << package.extracted_dir_path()
	 << " ...";

  if (ftw(package.extracted_dir_path().c_str(),
	  file_tree_walker_callback_fn,
	  16))
    {
      cerr << "Error while inspecting files in package"
	   << package.extracted_dir_path() << "\n";
      return false;
    }

  for (vector<string>::const_iterator file =
	 elf_file_paths.begin();
       file != elf_file_paths.end();
       ++file)
    {
      elf_file_sptr e (new elf_file(*file));
      if (opts.compare_dso_only)
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO)
	    continue;
	}
      else
	{
	  if (e->type != abigail::dwarf_reader::ELF_TYPE_DSO
	      && e->type != abigail::dwarf_reader::ELF_TYPE_EXEC)
	    continue;
	}

      if (e->soname.empty())
	package.path_elf_file_sptr_map()[e->name] = e;
      else
	package.path_elf_file_sptr_map()[e->soname] = e;
    }

  if (verbose)
    cerr << " DONE\n";
  return true;
}

/// Extract the content of a package and map its content.
///
/// The extracting is done to a temporary
///
/// @param package the package to consider.
///
/// @param opts the options the current package has been called with.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_package_and_map_its_content(package& package,
				    const options& opts)
{
  if (!extract_package(package))
    return false;

  bool result = true;
  if (!package.is_debug_info())
    result |= create_maps_of_package_content(package, opts);

  return result;
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
  if (!extract_package_and_map_its_content(first_package, opts)
      || !extract_package_and_map_its_content(second_package, opts))
    return false;

  if ((first_package.debug_info_package()
	 && !extract_package(*first_package.debug_info_package()))
      || (second_package.debug_info_package()
	    && !extract_package(*second_package.debug_info_package())))
      return false;

    return true;
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

  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map().begin();
       it != first_package.path_elf_file_sptr_map().end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map().find(it->first);

      if (iter != second_package.path_elf_file_sptr_map().end()
	  && (iter->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC))
	{
	  abidiff_status s = compare(*it->second, debug_dir1,
				     *iter->second, debug_dir2,
				     opts);
	  second_package.path_elf_file_sptr_map().erase(iter);
	  if (s & abigail::tools_utils::ABIDIFF_ABI_CHANGE)
	    diff.changed_binaries.push_back(it->second->name);
	  status |= s;
	}
      else
	{
	  diff.removed_binaries.push_back(it->second->name);
	  status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;
	  status |= abigail::tools_utils::ABIDIFF_ABI_CHANGE;
	}
    }

  for (map<string, elf_file_sptr>::iterator it =
	 second_package.path_elf_file_sptr_map().begin();
       it != second_package.path_elf_file_sptr_map().end();
       ++it)
    diff.added_binaries.push_back(it->second->name);

  if (diff.removed_binaries.size())
    {
      cout << "Removed binaries:\n";
      for (vector<string>::iterator it = diff.removed_binaries.begin();
	   it != diff.removed_binaries.end(); ++it)
	cout << "  " << *it << "\n";
    }

  if (opts.show_added_binaries && diff.added_binaries.size())
    {
      cout << "Added binaries:\n";
      for (vector<string>::iterator it = diff.added_binaries.begin();
	   it != diff.added_binaries.end(); ++it)
	cout << "  " << *it << "\n";
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
            return false;
        }
      else if (!strcmp(argv[i], "--debug-info-pkg1")
	       || !strcmp(argv[i], "--d1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
              opts.missing_operand = true;
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
              return true;
            }
          opts.debug_package2 =
	    abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--keep-tmp-files"))
	opts.keep_tmp_files = true;
      else if (!strcmp(argv[i], "--dso-only"))
	opts.compare_dso_only = true;
      else if (!strcmp(argv[i], "--no-linkage-name"))
	opts.show_linkage_names = false;
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant_changes = true;
      else if (!strcmp(argv[i], "--no-added-syms"))
		 opts.show_added_syms = false;
      else if (!strcmp(argv[i], "--no-added-binaries"))
	opts.show_added_binaries = false;
      else if (!strcmp(argv[i], "--fail-no-dbg"))
	opts.fail_if_no_debug_info = true;
      else if (!strcmp(argv[i], "--verbose"))
	verbose = true;
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
      else
        return false;
    }

  return true;
}

int
main(int argc, char* argv[])
{
  options opts;
  vector<package_sptr> packages;
  if (!parse_command_line(argc, argv, opts))
    {
      cerr << "unrecognized option\n"
        "try the --help option for more information\n";
      return 1;
    }

  if (opts.missing_operand)
    {
      cerr << "missing operand\n"
        "try the --help option for more information\n";
      return 1;
    }

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return 1;
    }

  if (opts.package1.empty() || opts.package2.empty())
    {
      cerr << "Please enter two packages to compare" << "\n";
      return 1;
    }

  package_sptr first_package(new package(opts.package1, "package1"));

  package_sptr second_package(new package(opts.package2, "package2"));

  if (!opts.debug_package1.empty())
    first_package->debug_info_package
      (package_sptr(new package(opts.debug_package1,
				"debug_package1",
				/*is_debug_info=*/true)));

  if (!opts.debug_package2.empty())
    second_package->debug_info_package
      (package_sptr(new package(opts.debug_package2,
				"debug_package2",
				/*is_debug_info=*/true)));

  switch (first_package->type())
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_RPM)
	{
	  cerr << opts.package2 << " should be an RPM file\n";
	  return 1;
	}
      break;

    case abigail::tools_utils::FILE_TYPE_DEB:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_DEB)
	{
	  cerr << opts.package2 << " should be a DEB file\n";
	  return 1;
	}
      break;

    case abigail::tools_utils::FILE_TYPE_DIR:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_DIR)
	{
	  cerr << opts.package2 << " should be a directory\n";
	  return 1;
	}
      break;

    case abigail::tools_utils::FILE_TYPE_TAR:
      if (second_package->type() != abigail::tools_utils::FILE_TYPE_TAR)
	{
	  cerr << opts.package2 << " should be a GNU tar archive\n";
	  return 1;
	}
      break;

    default:
      cerr << opts.package1 << " should be a valid package file \n";
      return 1;
    }

  return compare(*first_package, *second_package, opts);
}
