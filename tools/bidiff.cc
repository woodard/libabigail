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
#include "abg-reader.h"
#include "abg-dwarf-reader.h"

using std::string;
using std::ostream;
using std::cout;
using std::cerr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::corpus_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::tools::check_file;
using abigail::tools::guess_file_type;

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

      abigail::tools::file_type t1_type, t2_type;

      t1_type = guess_file_type(opts.file1);
      if (t1_type == abigail::tools::FILE_TYPE_UNKNOWN)
	{
	  cerr << "Unknown content type for file " << opts.file1 << "\n";
	  return true;
	}

      t2_type = guess_file_type(opts.file2);
      if (t2_type == abigail::tools::FILE_TYPE_UNKNOWN)
	{
	  cerr << "Unknown content type for file " << opts.file2 << "\n";
	  return true;
	}

      translation_unit_sptr t1, t2;
      corpus_sptr c1, c2;

      switch (t1_type)
	{
	case abigail::tools::FILE_TYPE_UNKNOWN:
	  cerr << "Unknown content type for file " << opts.file1 << "\n";
	  return true;
	  break;
	case abigail::tools::FILE_TYPE_NATIVE_BI:
	  t1 = abigail::xml_reader::read_translation_unit_from_file(opts.file1);
	  break;
	case abigail::tools::FILE_TYPE_ELF:
	  c1 = abigail::dwarf_reader::read_corpus_from_elf(opts.file1);
	  break;
	}

      switch (t2_type)
	{
	case abigail::tools::FILE_TYPE_UNKNOWN:
	  cerr << "Unknown content type for file " << opts.file2 << "\n";
	  return true;
	  break;
	case abigail::tools::FILE_TYPE_NATIVE_BI:
	  t2 = abigail::xml_reader::read_translation_unit_from_file(opts.file2);
	  break;
	case abigail::tools::FILE_TYPE_ELF:
	  c2 = abigail::dwarf_reader::read_corpus_from_elf(opts.file2);
	  break;
	}

      if (!t1 && !c1)
	{
	  cerr << "failed to read input file " << opts.file1 << "\n";
	  return true;
	}

      if (!t2 && !c2)
	{
	  cerr << "failed to read input file" << opts.file2 << "\n";
	  return true;
	}

      if (!!c1 != !!c2
	  || !!t1 != !!t2)
	{
	  cerr << "the two input should be of the same kind\n";
	  return true;
	}

      if (t1)
	{
	  translation_unit_diff_sptr changes = compute_diff(t1, t2);
	  changes->report(cout);
	}
      else if (c1)
	{
	  corpus_diff_sptr changes = compute_diff(c1, c2);
	  changes->report(cout);
	}

      return false;
    }

  return true;
}
