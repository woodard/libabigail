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

#include <cstring>
#include <string>
#include <iostream>
#include "abg-comparison.h"
#include "abg-tools-utils.h"

using std::string;
using std::ostream;
using std::cout;
using std::cerr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::tools::check_file;

struct options
{
  string file1;
  string file2;
};//end struct options;

void
display_usage(const string prog_name, ostream& out)
{
  out << "usage: " << prog_name << "[options] [<bi-file1> <bi-file2>]\n"
      << " where options can be:\n"
      << "  --help display this message\n";
}

/// Parse the command line and set the options accordingly.
///
/// @param argc the number of words on the command line
///
/// @param argv the command line, which is an array of words.
///
/// @param opts the options data structure.  This is set by the
/// function iff it returns true.
///
/// @return true if the command line could be parsed and opts filed,
/// false otherwise.
bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.file1.empty())
	    opts.file1 = argv[i];
	  else if (opts.file2.empty())
	    opts.file2 = argv[i];
	  else
	    return false;
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
      return true;
    }

  if (!opts.file1.empty() && !opts.file2.empty())
    {
      if (!check_file(opts.file1, cerr))
	return true;

      if (!check_file(opts.file2, cerr))
	return true;

      translation_unit_sptr t1(new translation_unit(opts.file1));
      translation_unit_sptr t2(new translation_unit(opts.file2));

      if (!t1->read())
	{
	  cerr << "failed to read input file " << t1->get_path() << "\n";
	  return true;
	}

      if (!t2->read())
	{
	  cerr << "failed to read input file" << t2->get_path() << "\n";
	  return true;
	}

      translation_unit_diff_sptr changes = compute_diff(t1, t2);
      changes->report(cout);

      return false;
    }

  return true;
}
