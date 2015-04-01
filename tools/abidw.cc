// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2015 Red Hat, Inc.
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

#include <unistd.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <tr1/memory>
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using std::tr1::shared_ptr;

struct options
{
  string		in_file_path;
  string		out_file_path;
  shared_ptr<char>	di_root_path;
  bool			check_alt_debug_info_path;
  bool			show_base_name_alt_debug_info_path;
  bool			write_architecture;
  bool			load_all_types;

  options()
    : check_alt_debug_info_path(),
      show_base_name_alt_debug_info_path(),
      write_architecture(true),
      load_all_types()
  {}
};

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] [<path-to-elf-file>]\n"
      << " where options can be: \n"
      << "  --help display this message\n"
      << "  --debug-info-dir <dir-path> look for debug info under 'dir-path'\n"
      << "  --out-file <file-path> write the output to 'file-path'\n"
      << "  --no-architecture do not emit architecture info in the output\n"
      << "  --check-alternate-debug-info <elf-path> check alternate debug info "
		"of <elf-path>\n"
      << "  --check-alternate-debug-info-base-name <elf-path> check alternate "
    "debug info of <elf-path>, and show its base name\n"
      << "  --load-all-types read all types including those not reachable from"
         "exported declarations\n"
    ;
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
      else if (!strcmp(argv[i], "--debug-info-dir"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.out_file_path.empty())
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
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
      else if (!strcmp(argv[i], "--no-architecture"))
	opts.write_architecture = false;
      else if (!strcmp(argv[i], "--check-alternate-debug-info")
	       || !strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.in_file_path.empty())
	    return false;
	  if (!strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	    opts.show_base_name_alt_debug_info_path = true;
	  opts.check_alt_debug_info_path = true;
	  opts.in_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--load-all-types"))
	opts.load_all_types = true;
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
      display_usage(argv[0], cout);
      return 1;
    }

  assert(!opts.in_file_path.empty());
  if (!abigail::tools_utils::check_file(opts.in_file_path, cerr))
    return 1;

  abigail::tools_utils::file_type type =
    abigail::tools_utils::guess_file_type(opts.in_file_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF
      && type != abigail::tools_utils::FILE_TYPE_AR)
    {
      cerr << opts.in_file_path << " is not an ELF file\n";
      return 1;
    }

  using abigail::corpus;
  using abigail::corpus_sptr;
  using abigail::translation_units;
  using abigail::dwarf_reader::read_context;
  using abigail::dwarf_reader::read_context_sptr;
  using abigail::dwarf_reader::read_corpus_from_elf;
  using abigail::dwarf_reader::create_read_context;
  using namespace abigail;

  char* p = opts.di_root_path.get();
  corpus_sptr corp;
  read_context_sptr c = create_read_context(opts.in_file_path, &p,
					    opts.load_all_types);
  read_context& ctxt = *c;

  if (opts.check_alt_debug_info_path)
    {
      bool has_alt_di = false;
      string alt_di_path;
      abigail::dwarf_reader::status status =
	abigail::dwarf_reader::has_alt_debug_info(ctxt,
						  has_alt_di, alt_di_path);
      if (status & abigail::dwarf_reader::STATUS_OK)
	{
	  if (alt_di_path.empty())
	    ;
	  else
	    {
	      if (opts.show_base_name_alt_debug_info_path)
		tools_utils::base_name(alt_di_path, alt_di_path);

	      cout << "found the alternate debug info file '"
		   << alt_di_path
		   << "'\n";
	    }
	  return 0;
	}
      else
	{
	  cerr << "could not find alternate debug info file\n";
	  return 1;
	}
    }

  dwarf_reader::status s = read_corpus_from_elf(ctxt, corp);
  if (!corp)
    {
      if (s == dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
	{
	  if (p == 0)
	    {
	      cerr <<
		"Could not read debug info from "
		   << opts.in_file_path << "\n";

	      cerr << "You might want to supply the root directory where "
		"to search debug info from, using the "
		"--debug-info-dir option "
		"(e.g --debug-info-dir /usr/lib/debug)\n";
	    }
	  else
	    {
	      cerr << "Could not read debug info for '" << opts.in_file_path
		   << "' from debug info root directory '" << p
		   << "'\n";
	    }
	}
      else if (s == dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
	cerr << "Could not read ELF symbol information from "
	     << opts.in_file_path << "\n";

      return 1;
    }
  else
    {
      if (!opts.write_architecture)
	corp->set_architecture_name("");
      abigail::xml_writer::write_corpus_to_native_xml(corp, 0, cout);
    }

  return 0;
}
