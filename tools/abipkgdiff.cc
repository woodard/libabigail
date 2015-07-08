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
using abigail::dwarf_reader::get_soname_from_elf;

vector<string> elf_file_paths;

struct options
{
  bool display_usage;
  bool missing_operand;
  string package1;
  string package2;
  string debug_package1;
  string debug_package2;

  options()
    : display_usage(false),
      missing_operand(false)
  {}
};

enum elf_type
{
  ELF_TYPE_EXEC,
  ELF_TYPE_DSO,
  ELF_TYPE_UNKNOWN
};

struct elf_file
{
  string name;
  string path;
  string soname;
  elf_type type;

  elf_file(const string& path,
	   const string& name,
	   const string& soname,
	   elf_type type)
    : name(name),
      path(path),
      soname(soname),
      type(type)
  {}
};

typedef shared_ptr<elf_file> elf_file_sptr;

struct abi_changes
{
  vector <string> added_binaries;
  vector <string> removed_binaries;
  vector <string> abi_changes;
} abi_diffs;

struct package
{
  string path;
  string extracted_package_dir_path;
  abigail::tools_utils::file_type type;
  bool is_debug_info;
  map<string, elf_file_sptr> path_elf_file_sptr_map;
  shared_ptr<package> debug_info_package;

  package(const string& path, const string& dir,
	  abigail::tools_utils::file_type file_type,
          bool is_debug_info = false)
    : path(path),
      type(file_type),
      is_debug_info(is_debug_info)
  {
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir != NULL)
      extracted_package_dir_path = tmpdir;
    else
      extracted_package_dir_path = "/tmp";
    extracted_package_dir_path = extracted_package_dir_path + "/" + dir;
  }

  void
  erase_extraction_directory() const
  {
    string cmd = "rm -rf " + extracted_package_dir_path;
    system(cmd.c_str());
  }

  void
  erase_extraction_directories() const
  {
    erase_extraction_directory();
    if (debug_info_package)
      debug_info_package->erase_extraction_directory();
  }
};

typedef shared_ptr<package> package_sptr;

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] <package1> <package2>\n"
      << " where options can be:\n"
      << " --debug-info-pkg1|--d1 <path>  Path of debug-info package of package1\n"
      << " --debug-info-pkg2|--d2 <path>  Path of debug-info package of package2\n"
      << " --help                         Display help message\n";
}

static elf_type
elf_file_type(const GElf_Ehdr* ehdr)
{
  switch (ehdr->e_type)
    {
    case ET_DYN:
      return ELF_TYPE_DSO;
      break;
    case ET_EXEC:
      return ELF_TYPE_EXEC;
      break;
    default:
      return ELF_TYPE_UNKNOWN;
      break;
    }
}

static bool
extract_rpm(const string& pkg_path, const string &extracted_package_dir_path)
{
  string cmd = "mkdir " + extracted_package_dir_path + " && cd " +
    extracted_package_dir_path + " && rpm2cpio " + pkg_path +
    " | cpio -dium --quiet";

  if (!system(cmd.c_str()))
    return true;
  return false;
}

static void
erase_created_temporary_directories(const package& first_package,
				    const package& second_package)
{
  first_package.erase_extraction_directories();
  second_package.erase_extraction_directories();
}

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

static void
compare(const elf_file& elf1, const string& debug_dir1,
	const elf_file& elf2, const string& debug_dir2)
{
  cout << "Changes between " << elf1.name << " and " << elf2.name;
  cout << "  =======>\n";
  string cmd = "abidiff " +
    elf1.path + " " + elf2.path;
  if (!debug_dir1.empty())
    cmd += " --debug-info-dir1 " + debug_dir1;
  if (!debug_dir2.empty())
    cmd += " --debug-info-dir2 " + debug_dir2;
  system(cmd.c_str());
}

static bool
create_maps_of_package_content(package& package)
{
  elf_file_paths.clear();
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
      int fd = open((*file).c_str(), O_RDONLY);
      if(fd == -1)
	return false;
      elf_version (EV_CURRENT);
      Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
      GElf_Ehdr ehdr_mem;
      GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
      string soname;
      elf_type e = elf_file_type(ehdr);
      if (e == ELF_TYPE_DSO)
        {
          if (! get_soname_from_elf(elf, soname))
            return false;
        }

      string file_base_name(basename(const_cast<char*>((*file).c_str())));
      if (soname.empty())
	package.path_elf_file_sptr_map[file_base_name] =
	  elf_file_sptr(new elf_file((*file), file_base_name, soname, e));
      else
	package.path_elf_file_sptr_map[soname] =
	  elf_file_sptr( new elf_file((*file), file_base_name, soname, e));
    }

  return true;
}

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

static bool
compare(package& first_package, package& second_package)
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

  for (map<string, elf_file_sptr>::iterator it =
	 first_package.path_elf_file_sptr_map.begin();
       it != first_package.path_elf_file_sptr_map.end();
       ++it)
    {
      map<string, elf_file_sptr>::iterator iter =
	second_package.path_elf_file_sptr_map.find(it->first);

      if (iter != second_package.path_elf_file_sptr_map.end()
	  && (iter->second->type == ELF_TYPE_DSO
	      || iter->second->type == ELF_TYPE_EXEC))
	{
	  compare(*it->second, debug_dir1,
		  *iter->second, debug_dir2);
	  second_package.path_elf_file_sptr_map.erase(iter);
	}
      else
	abi_diffs.removed_binaries.push_back(it->second->name);
    }

  for (map<string, elf_file_sptr>::iterator it =
	 second_package.path_elf_file_sptr_map.begin();
       it != second_package.path_elf_file_sptr_map.end();
       ++it)
    abi_diffs.added_binaries.push_back(it->second->name);

  if (abi_diffs.removed_binaries.size())
    {
      cout << "Removed binaries\n";
      for (vector<string>::iterator it = abi_diffs.removed_binaries.begin();
	   it != abi_diffs.removed_binaries.end(); ++it)
	cout << *it << "\n";
    }

  if (abi_diffs.added_binaries.size())
    {
      cout << "Added binaries\n";
      for (vector<string>::iterator it = abi_diffs.added_binaries.begin();
	   it != abi_diffs.added_binaries.end(); ++it)
	cout << *it << "\n";
    }

  erase_created_temporary_directories(first_package, second_package);

  return true;
}

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

  package_sptr first_package(new package(opts.package1, "package1",
					 guess_file_type(opts.package1)));

  package_sptr second_package(new package(opts.package2, "package2",
					  guess_file_type(opts.package2)));

  if (!opts.debug_package1.empty())
    first_package->debug_info_package =
      package_sptr(new package(opts.debug_package1, "debug_package1",
			       guess_file_type(opts.debug_package1),
			       /*is_debug_info=*/true));

  if (!opts.debug_package2.empty())
    second_package->debug_info_package =
      package_sptr(new package(opts.debug_package2, "debug_package2",
			       guess_file_type(opts.debug_package2),
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
