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

using std::cout;
using std::cerr;
using std::string;
using std::ostream;

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

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] <bi-package1> <bi-package2>\n"
      << " where options can be:\n"
      << " --debug-info-pkg1 <path>  Path of debug-info package of bi-pacakge1\n"
      << " --debug-info-pkg2 <path>  Path of debug-info package of bi-pacakge2\n"
      << " --help                    Display help message\n";
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
            opts.pkg1 = argv[i];
          else if (opts.pkg2.empty())
            opts.pkg2 = argv[i];
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
          opts.debug_pkg1 = argv[j];
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
          opts.debug_pkg2 = argv[j];
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
  cout << "Package1 = " << opts.pkg1 << std::endl;
  cout << "package2 = " << opts.pkg2 << std::endl;
  if (!opts.debug_pkg1.empty())
    cout << "Debug-info pacakge " << opts.debug_pkg1 << std::endl;
  if (!opts.debug_pkg2.empty())
    cout << "Debug-info package2 = " << opts.debug_pkg2 << std::endl;
}
