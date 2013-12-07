// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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
// Author: Dodji Seketeli

/// @file
///
/// This program reads an elf file, try to load its debug info (in
/// DWARF format) and emit it back in a set of "text sections" in native
/// libabigail XML format.

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using abigail::tools::check_file;

struct options
{
  string in_file_path;
  string out_file_path;
};

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << "[options] [<path-to-elf-file>]\n"
      << " where options can be: \n"
      << "  --help display this message\n"
      << "  --out-file <file-path> write the output to 'file-path'\n";
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
	  if (opts.in_file_path.empty())
	    opts.in_file_path = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--out-file"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.out_file_path.empty())
	    return false;

	  opts.out_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--help"))
	return false;
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
      display_usage(argv[0], cerr);
      return 1;
    }

  assert(!opts.in_file_path.empty());
  if (!check_file(opts.in_file_path, cerr))
    return 1;

  using abigail::corpus;
  using abigail::corpus_sptr;
  using abigail::translation_units;
  using abigail::dwarf_reader::read_corpus_from_elf;

  corpus_sptr corp = read_corpus_from_elf(opts.in_file_path);
  if (!corp)
    {
      cerr << "Could not read debug info from " << opts.in_file_path << "\n";
      return 1;
    }

  cout << "for corpus " << corp->get_path() << ":\n";
  for (translation_units::const_iterator it =
	 corp->get_translation_units().begin();
       it != corp->get_translation_units().end();
       ++it)
    {
      cout << "translation unit: " << (*it)->get_path() << ":\n";
      dump(*it, cout);
      cout << "\n";
    }

  return 0;
}
