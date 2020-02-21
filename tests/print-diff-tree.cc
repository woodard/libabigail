// -*- Mode: C++ -*-

// Copyright (C) 2013-2020 Red Hat, Inc.
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
//

#include <cstring>
#include <iostream>
#include "abg-comparison.h"
#include "abg-dwarf-reader.h"

using std::cout;
using std::cerr;
using std::ostream;
using std::string;

using abigail::comparison::diff;
using abigail::comparison::diff_sptr;
using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::print_diff_tree;
using abigail::comparison::apply_filters;
using namespace abigail;
using namespace abigail::dwarf_reader;

struct options
{
  bool display_help;
  bool categorize_redundancy;
  bool apply_filters;
  string elf1;
  string elf2;

  options()
    : display_help(false),
      categorize_redundancy(false),
      apply_filters(false)
  {}
};

static void
display_help(const string& prog_name,
	     ostream& out)
{
  out << prog_name << " [options] <elf lib1> <elf lib2>\n"
      << " where options can be:\n"
      << " --categorize-redundancy  categorize diff node redundancy\n"
      << " --apply-filters  apply the generic categorization filters\n"
      << " --help  display this message\n";
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
	  if (opts.elf1.empty())
	    opts.elf1 = argv[i];
	  else if (opts.elf2.empty())
	    opts.elf2 = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--help"))
	opts.display_help = true;
      else if (!strcmp(argv[i], "--categorize-redundancy"))
	opts.categorize_redundancy = true;
      else if (!strcmp(argv[i], "--apply-filters"))
	opts.apply_filters = true;
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

  if (opts.display_help)
    {
      display_help(argv[0], cout);
      return 0;
    }

  if (!opts.elf1.empty() && !opts.elf2.empty())
    {
      dwarf_reader::status c1_status, c2_status;
      corpus_sptr c1, c2;

      environment_sptr env(new environment);
      vector<char**> di_roots;
      c1 = dwarf_reader::read_corpus_from_elf(opts.elf1, di_roots, env.get(),
					      /*load_all_types=*/false,
					      c1_status);
      if (c1_status != dwarf_reader::STATUS_OK)
	{
	  cerr << "Failed to read elf file " << opts.elf1 << "\n";
	  return 1;
	}

      c2 = dwarf_reader::read_corpus_from_elf(opts.elf2, di_roots, env.get(),
					      /*load_all_types=*/false,
					      c2_status);
      if (c2_status != dwarf_reader::STATUS_OK)
	{
	  cerr << "Failed to read elf file " << opts.elf2 << "\n";
	  return 1;
	}

      corpus_diff_sptr diff = compute_diff(c1, c2);
      if (!diff)
	{
	  cerr << "Could not compute ABI diff between elf files "
	       << opts.elf1 << " and " << opts.elf2;
	  return 1;
	}

      if (opts.categorize_redundancy)
	categorize_redundancy(diff);

      if (opts.apply_filters)
	apply_filters(diff);

      print_diff_tree(diff, cout);
      return 0;
    }
  return 1;
}

void
print_diff_tree(abigail::comparison::corpus_diff* diff_tree)
{
  print_diff_tree(diff_tree, std::cout);
}

void
print_diff_tree(abigail::comparison::corpus_diff_sptr diff_tree)
{
  print_diff_tree(diff_tree, std::cout);
}
