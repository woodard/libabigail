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

/// This program gives abi changes for avilable binaries inside two
/// packages. It takes input as two packages (e.g. .rpms, .tar, .deb) and
/// optional corresponding debug-info packages. The program extracts pacakges
/// and looks for avilable ELF binaries in each pacakge and gives results for
/// possible abi changes occured between two pacakges.

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
using abigail::ir::corpus_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::corpus_diff_sptr;
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

  options()
    : display_usage(false),
      missing_operand(false)
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

/// Abstract the result of comparing two package.
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
struct package
{
  string				path;
  string				extracted_package_dir_path;
  abigail::tools_utils::file_type	type;
  bool					is_debug_info;
  map<string, elf_file_sptr>		path_elf_file_sptr_map;
  shared_ptr<package>			debug_info_package;

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
    : path(path),
      is_debug_info(is_debug_info)
  {
    type = guess_file_type(path);
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir != NULL)
      extracted_package_dir_path = tmpdir;
    else
      extracted_package_dir_path = "/tmp";
    extracted_package_dir_path = extracted_package_dir_path + "/" + dir;
  }

  /// Erase the content of the temporary extraction directory that has
  /// been populated by the @ref extract_package() function;
  void
  erase_extraction_directory() const
  {
    if (verbose)
      cerr << "Erasing temporary extraction directory "
	   << extracted_package_dir_path
	   << " ...";

    string cmd = "rm -rf " + extracted_package_dir_path;
    system(cmd.c_str());

    if (verbose)
      cerr << " DONE\n";
  }

  /// Erase the content of all the temporary extraction directories.
  void
  erase_extraction_directories() const
  {
    erase_extraction_directory();
    if (debug_info_package)
      debug_info_package->erase_extraction_directory();
  }
};

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
      << " --debug-info-pkg1|--d1 <path>  Path of debug-info package of package1\n"
      << " --debug-info-pkg2|--d2 <path>  Path of debug-info package of package2\n"
      << " --verbose                      Emit verbose progress messages\n"
      << " --help                         Display help message\n";
}

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
	 << "to "
	 << extracted_package_dir_path
	 << " ...";

  string cmd = "test -d " +
    extracted_package_dir_path +
    " && rm -rf " + extracted_package_dir_path;

  system(cmd.c_str());

  cmd = "mkdir " + extracted_package_dir_path + " && cd " +
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

/// Extract the content of a package.
///
/// @param package the package we are looking at.
static bool
extract_package(const package& package)
{
  switch(package.type)
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
      if (!extract_rpm(package.path, package.extracted_package_dir_path))
        {
          cerr << "Error while extracting package" << package.path << "\n";
          return false;
        }
      return true;
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
/// @return true iff the comparison yields differences between the two
/// files, false otherwise.
static bool
compare(const elf_file& elf1, const string& debug_dir1,
	const elf_file& elf2, const string& debug_dir2)
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
      cerr << "could not read file '"
	   << elf1.path
	   << "' properly\n";
      return false;
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
      cerr << "could not find the read file '"
	   << elf2.path
	   << "' properly\n";
      return false;
    }

  if (verbose)
    cerr << " DONE\n";

  if (verbose)
    cerr << "  Comparing the ABI of the two files ...";

  corpus_diff_sptr diff = compute_diff(corpus1, corpus2);

  if (verbose)
    cerr << "DONE\n";

  bool has_changes = diff->has_changes();
  if (has_changes)
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

  return has_changes;
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
/// @param true upon successful completion, false otherwise.
static bool
create_maps_of_package_content(package& package)
{
  elf_file_paths.clear();
  if (verbose)
    cerr << "Analyzing the content of package "
	 << package.path
	 << " extracted to "
	 << package.extracted_package_dir_path
	 << " ...";

  if (ftw(package.extracted_package_dir_path.c_str(),
	  file_tree_walker_callback_fn,
	  16))
    {
      cerr << "Error while inspecting files in package"
	   << package.extracted_package_dir_path << "\n";
      return false;
    }

  for (vector<string>::const_iterator file =
	 elf_file_paths.begin();
       file != elf_file_paths.end();
       ++file)
    {
      elf_file_sptr e (new elf_file(*file));
      if (!e->type != abigail::dwarf_reader::ELF_TYPE_DSO)
	continue;

      if (e->soname.empty())
	package.path_elf_file_sptr_map[e->name] = e;
      else
	package.path_elf_file_sptr_map[e->soname] = e;
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
/// @return true upon successful completion, false otherwise.
static bool
extract_package_and_map_its_content(package& package)
{
  if (!extract_package(package))
    return false;

  bool result = true;
  if (!package.is_debug_info)
    result |= create_maps_of_package_content(package);

  return result;
}

/// Prepare the packages for comparison.
///
/// This function extracts the content of the package and maps it.
///
/// @return true upon successful completion, false otherwise.
static bool
prepare_packages(package& first_package,
		 package& second_package)
{
    if (!extract_package_and_map_its_content(first_package)
	|| !extract_package_and_map_its_content(second_package))
    return false;

    if ((first_package.debug_info_package
	 && !extract_package(*first_package.debug_info_package))
	|| (second_package.debug_info_package
	    && !extract_package(*second_package.debug_info_package)))
      return false;

    return true;
}

/// Compare the ABI of two packages
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @param diff out parameter.  If this function returns true, then
/// this parameter is set to the result of the comparison.
///
/// @return true iff the comparison yields differences between the two
/// packages.
static bool
compare(package& first_package, package& second_package, abi_diff& diff)
{
  if (!prepare_packages(first_package, second_package))
    return false;

  // Setting debug-info path of libraries
  string debug_dir1, debug_dir2, relative_debug_path = "/usr/lib/debug/";
  if (first_package.debug_info_package
      && second_package.debug_info_package)
    {
      debug_dir1 =
	first_package.debug_info_package->extracted_package_dir_path +
	relative_debug_path;
      if (second_package.debug_info_package)
	debug_dir2 =
	  second_package.debug_info_package->extracted_package_dir_path +
	  relative_debug_path;
    }

  bool has_abi_changes = false;
  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map.begin();
       it != first_package.path_elf_file_sptr_map.end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map.find(it->first);

      if (iter != second_package.path_elf_file_sptr_map.end()
	  && (iter->second->type == abigail::dwarf_reader::ELF_TYPE_DSO
	      || iter->second->type == abigail::dwarf_reader::ELF_TYPE_EXEC))
	{
	  has_abi_changes |= compare(*it->second, debug_dir1,
				     *iter->second, debug_dir2);
	  second_package.path_elf_file_sptr_map.erase(iter);
	  if (has_abi_changes)
	    diff.changed_binaries.push_back(it->second->name);
	}
      else
	diff.removed_binaries.push_back(it->second->name);
    }

  for (map<string, elf_file_sptr>::iterator it =
	 second_package.path_elf_file_sptr_map.begin();
       it != second_package.path_elf_file_sptr_map.end();
       ++it)
    diff.added_binaries.push_back(it->second->name);

  if (diff.removed_binaries.size())
    {
      cout << "Removed binaries\n";
      for (vector<string>::iterator it = diff.removed_binaries.begin();
	   it != diff.removed_binaries.end(); ++it)
	cout << *it << "\n";
    }

  if (diff.added_binaries.size())
    {
      cout << "Added binaries\n";
      for (vector<string>::iterator it = diff.added_binaries.begin();
	   it != diff.added_binaries.end(); ++it)
	cout << *it << "\n";
    }

  erase_created_temporary_directories(first_package, second_package);

  return diff.has_changes();
}

/// Compare the ABI of two packages.
///
/// @param first_package the first package to consider.
///
/// @param second_package the second package to consider.
///
/// @return true if the comparison yields ABI differences between the
/// two packages.
static bool
compare(package& first_package, package& second_package)
{
  abi_diff diff;
  return compare(first_package, second_package, diff);
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
      else if (!strcmp(argv[i], "--verbose"))
	verbose = true;
      else if (!strcmp(argv[i], "--help"))
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
    first_package->debug_info_package =
      package_sptr(new package(opts.debug_package1,
			       "debug_package1",
			       /*is_debug_info=*/true));

  if (!opts.debug_package2.empty())
    second_package->debug_info_package =
      package_sptr(new package(opts.debug_package2,
			       "debug_package2",
			       /*is_debug_info=*/true));

  switch (first_package->type)
    {
    case abigail::tools_utils::FILE_TYPE_RPM:
      if (!(second_package->type == abigail::tools_utils::FILE_TYPE_RPM))
	{
	  cerr << opts.package2 << " should be an RPM file\n";
	  return 1;
	}
      break;

    default:
      cerr << opts.package1 << " should be a valid package file \n";
      return 1;
    }

  return compare(*first_package, *second_package);
}
