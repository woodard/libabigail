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
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <elf.h>
#include <elfutils/libdw.h>
#include "abg-tools-utils.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostream;
using std::vector;
using std::map;
using std::tr1::shared_ptr;
using abigail::tools_utils::guess_file_type;
using abigail::tools_utils::file_type;
using abigail::tools_utils::make_path_absolute;

vector<string> dir_elf_files_path;

struct options
{
  bool display_usage;
  bool missing_operand;
  string pkg1;
  string pkg2;
  string debug_pkg1;
  string debug_pkg2;

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

  elf_file(string path, string name, elf_type type, string soname)
    : name(name),
      path(path),
      soname(soname),
      type(type)
    { }

  ~elf_file()
  {
    delete this;
  }
};

struct abi_changes
{
  vector <string> added_binaries;
  vector <string> removed_binaries;
  vector <string> abi_changes;
}abi_diffs;

struct package
{
  string pkg_path;
  string extracted_pkg_dir_path;
//   string pkg_name;
  abigail::tools_utils::file_type pkg_type;
  bool is_debuginfo_pkg;
  map<string, elf_file*> dir_elf_files_map;
  shared_ptr<package> debuginfo_pkg;

  package(string path, string dir, abigail::tools_utils::file_type file_type,
          bool is_debuginfo = false )
  : pkg_path(path),
    pkg_type(file_type),
    is_debuginfo_pkg(is_debuginfo)
    {
      const char *tmpdir = getenv("TMPDIR");
      if (tmpdir != NULL)
        extracted_pkg_dir_path = tmpdir;
      else
        extracted_pkg_dir_path = "/tmp";
      extracted_pkg_dir_path = extracted_pkg_dir_path + "/" + dir;
    }

  ~package()
    {
      string cmd = "rm -rf " + extracted_pkg_dir_path;
      system(cmd.c_str());
    }
};

typedef shared_ptr<package> package_sptr;

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] <bi-package1> <bi-package2>\n"
      << " where options can be:\n"
      << " --debug-info-pkg1 <path>  Path of debug-info package of bi-pacakge1\n"
      << " --debug-info-pkg2 <path>  Path of debug-info package of bi-pacakge2\n"
      << " --help                    Display help message\n";
}

string
get_soname(Elf *elf, GElf_Ehdr *ehdr)
{
  string result;

    for (int i = 0; i < ehdr->e_phnum; ++i)
    {
      GElf_Phdr phdr_mem;
      GElf_Phdr *phdr = gelf_getphdr (elf, i, &phdr_mem);

      if (phdr != NULL && phdr->p_type == PT_DYNAMIC)
        {
          Elf_Scn *scn = gelf_offscn (elf, phdr->p_offset);
          GElf_Shdr shdr_mem;
          GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
          int maxcnt = (shdr != NULL
                        ? shdr->sh_size / shdr->sh_entsize : INT_MAX);
          assert (shdr == NULL || shdr->sh_type == SHT_DYNAMIC);
          Elf_Data *data = elf_getdata (scn, NULL);
          if (data == NULL)
	    break;

          for (int cnt = 0; cnt < maxcnt; ++cnt)
            {
              GElf_Dyn dynmem;
              GElf_Dyn *dyn = gelf_getdyn (data, cnt, &dynmem);
              if (dyn == NULL)
                continue;

              if (dyn->d_tag == DT_NULL)
                break;

              if (dyn->d_tag != DT_SONAME)
                continue;

              // XXX Don't rely on SHDR
              result = elf_strptr (elf, shdr->sh_link, dyn->d_un.d_val);
	      break;
            }
          break;
        }
    }

    return result;
}


elf_type
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

const bool
extract_rpm(const string& pkg_path, const string &extracted_pkg_dir_path)
{
  string cmd = "mkdir " + extracted_pkg_dir_path + " && cd " +
    extracted_pkg_dir_path + " && rpm2cpio " + pkg_path + " | cpio -dium --quiet";

  if (!system(cmd.c_str()))
          return true;
  return false;

}

static bool
extract_pkg(package_sptr pkg)
{
  switch(pkg->pkg_type)
    {
      case abigail::tools_utils::FILE_TYPE_RPM:
        if (!extract_rpm(pkg->pkg_path, pkg->extracted_pkg_dir_path))
        {
          cerr << "Error while extracting package" << pkg->pkg_path << endl;
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
callback(const char *fpath, const struct stat *st, int flag)
{
  struct stat s;
  lstat(fpath, &s);

  if (!S_ISLNK(s.st_mode))
  {
    if (guess_file_type(fpath) == abigail::tools_utils::FILE_TYPE_ELF)
      dir_elf_files_path.push_back(fpath);
  }
  return 0;
}

void
compute_abidiff (const elf_file* elf1, const string debug_dir1,
                 const elf_file* elf2, const string &debug_dir2)
{
  cout << "ABI change between binaries " << elf1->name << " and " << elf2->name;
  cout << "  =======>\n";
  string cmd = "abidiff " +
  elf1->path + " " + elf2->path;
  if (!debug_dir1.empty())
    cmd += " --debug-info-dir1 " + debug_dir1;
  if (!debug_dir2.empty())
    cmd += " --debug-info-dir2 " + debug_dir2;
  system(cmd.c_str());
}

static bool
pkg_diff(vector<package_sptr> &packages)
{
  for (vector< package_sptr>::iterator it = packages.begin() ; it != packages.end(); ++it)
    {
      if (!extract_pkg(*it))
        return false;

      // Getting files path available in packages
      if (!(*it)->is_debuginfo_pkg)
        {
          dir_elf_files_path.clear();
          if (!(ftw((*it)->extracted_pkg_dir_path.c_str(), callback, 16)))
            {
              for (vector<string>::const_iterator iter = dir_elf_files_path.begin();
                   iter != dir_elf_files_path.end(); ++iter)
                {
                  int fd = open((*iter).c_str(), O_RDONLY);
                  if(fd == -1)
                    return false;
                  elf_version (EV_CURRENT);
                  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
                  GElf_Ehdr ehdr_mem;
                  GElf_Ehdr *ehdr = gelf_getehdr (elf, &ehdr_mem);
                  string soname;
                  elf_type e = elf_file_type(ehdr);
                  if (e == ELF_TYPE_DSO)
                    soname = get_soname(elf, ehdr);

                  string file_base_name(basename(const_cast<char*>((*iter).c_str())));
                  if (soname.empty())
                    (*it)->dir_elf_files_map[file_base_name] =
                    new elf_file((*iter), file_base_name, e, soname);
                  else
                    (*it)->dir_elf_files_map[soname] =
                    new elf_file((*iter), file_base_name, e, soname);
                }
              }
          else
            cerr << "Error while getting list of files in package"
            << (*it)->extracted_pkg_dir_path << std::endl;
        }
    }
    // Setting debug-info path of libraries
    string debug_dir1, debug_dir2, relative_debug_path = "/usr/lib/debug/";
    if (packages[0]->debuginfo_pkg != NULL)
      {
        debug_dir1 = packages[2]->extracted_pkg_dir_path + relative_debug_path;
        if (packages[1]->debuginfo_pkg != NULL)
            debug_dir2 = packages[3]->extracted_pkg_dir_path + relative_debug_path;
      }
    else if (packages[1]->debuginfo_pkg != NULL)
        debug_dir2 = packages[2]->extracted_pkg_dir_path + relative_debug_path;

    for (map<string, elf_file*>::iterator it = packages[0]->dir_elf_files_map.begin();
          it != packages[0]->dir_elf_files_map.end();
          ++it)
      {

        map<string, elf_file*>::iterator iter =
          packages[1]->dir_elf_files_map.find(it->first);
        if (iter != packages[1]->dir_elf_files_map.end())
          {
           compute_abidiff(it->second, debug_dir1, iter->second, debug_dir2);
            packages[1]->dir_elf_files_map.erase(iter);
          }
        else
          abi_diffs.removed_binaries.push_back(it->second->name);
      }

      for (map<string, elf_file*>::iterator it = packages[1]->dir_elf_files_map.begin();
          it != packages[1]->dir_elf_files_map.end();
          ++it)
        {
          abi_diffs.added_binaries.push_back(it->second->name);
        }

      if (abi_diffs.removed_binaries.size())
        {
          cout << "Removed binaries\n";
          for (vector<string>::iterator it = abi_diffs.removed_binaries.begin();
               it != abi_diffs.removed_binaries.end(); ++it)
            {
              cout << *it << std::endl;
            }
        }

      if (abi_diffs.added_binaries.size())
        {
          cout << "Added binaries\n";
          for (vector<string>::iterator it = abi_diffs.added_binaries.begin();
               it != abi_diffs.added_binaries.end(); ++it)
            {
              cout << *it << std::endl;
            }
        }

    return true;
}

bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
        {
          if (opts.pkg1.empty())
            opts.pkg1 = abigail::tools_utils::make_path_absolute(argv[i]).get();
          else if (opts.pkg2.empty())
            opts.pkg2 = abigail::tools_utils::make_path_absolute(argv[i]).get();
          else
            return false;
        }
      else if (!strcmp(argv[i], "--debug-info-pkg1"))
        {
          int j = i + 1;
          if (j >= argc)
            {
              opts.missing_operand = true;
              return true;
            }
          opts.debug_pkg1 = abigail::tools_utils::make_path_absolute(argv[j]).get();
          ++i;
        }
      else if (!strcmp(argv[i], "--debug-info-pkg2"))
        {
          int j = i + 1;
          if (j >= argc)
            {
              opts.missing_operand = true;
              return true;
            }
          opts.debug_pkg2 = abigail::tools_utils::make_path_absolute(argv[j]).get();
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

  if (opts.pkg1.empty() || opts.pkg2.empty())
  {
    cerr << "Please enter two pacakges to diff" << endl;
    return 1;
  }
  packages.push_back(package_sptr (new package(
    opts.pkg1, "pkg1", guess_file_type(opts.pkg1))));
  packages.push_back(package_sptr(new package(
    opts.pkg2, "pkg2", guess_file_type(opts.pkg2))));
  if (!opts.debug_pkg1.empty())
    {
      packages.push_back(package_sptr (new package(
        opts.debug_pkg1, "debug_pkg1", guess_file_type(opts.debug_pkg1), true)));
      packages[0]->debuginfo_pkg = packages[packages.size() - 1];
    }
  if (!opts.debug_pkg2.empty())
    {
      packages.push_back(package_sptr (new package(
        opts.debug_pkg2, "debug_pkg2", guess_file_type(opts.debug_pkg2), true)));
      packages[1]->debuginfo_pkg = packages[packages.size() - 1];
    }

  switch (packages.at(0)->pkg_type)
    {
      case abigail::tools_utils::FILE_TYPE_RPM:
        if (!(packages.at(1)->pkg_type == abigail::tools_utils::FILE_TYPE_RPM))
          {
            cerr << opts.pkg2 << " should be an RPM file\n";
            return 1;
          }
        break;

      default:
        cerr << opts.pkg1 << " should be a valid package file \n";
        return 1;
    }

    return pkg_diff(packages);

}
