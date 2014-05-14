// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2014 Red Hat, Inc.
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

// Author: Dodji Seketeli

/// @file
///
/// This program takes parameters to open an elf file, lookup a symbol
/// in its symbol tables and report what it sees.

#include <elf.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include "abg-dwarf-reader.h"
#include "abg-ir.h"

using std::cout;
using std::string;
using std::ostream;
using std::ostringstream;
using std::vector;

using abigail::dwarf_reader::lookup_symbol_from_elf;
using abigail::elf_symbol;

struct options
{
  bool	show_help;
  char* elf_path;
  char* symbol_name;
  bool	demangle;
  bool absolute_path;

  options()
    : show_help(false),
      elf_path(0),
      symbol_name(0),
      demangle(false),
      absolute_path(true)
  {}
};

static void
show_help(const string& progname)
{
  cout << "usage: " << progname << "[options ]<elf file> <symbol-name>\n"
       << "where [options] can be:\n"
       << "  --help  display this help string\n"
       << "  --demangle demangle the symbols from the symbol table\n"
       << "  --no-absolute-path do not show absolute paths in messages\n";
}

static void
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    {
      opts.show_help = true;
      return;
    }

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (!opts.elf_path)
	    opts.elf_path = argv[i];
	  else if (!opts.symbol_name)
	    opts.symbol_name = argv[i] ;
	  else
	    {
	      opts.show_help = true;
	      return;
	    }
	}
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.show_help = true;
	  return;
	}
      else if (!strcmp(argv[i], "--demangle"))
	opts.demangle = true;
      else if (!strcmp(argv[i], "--no-absolute-path"))
	opts.absolute_path = false;
      else
	opts.show_help = true;
    }
}

int
main(int argc, char* argv[])
{
  options opts;
  parse_command_line(argc, argv, opts);

  if (opts.show_help)
    {
      show_help(argv[0]);
      return 1;
    }
  assert(opts.elf_path != 0
	 && opts.symbol_name != 0);

  string p = opts.elf_path, n = opts.symbol_name;
  vector<elf_symbol> syms;
  if (!lookup_symbol_from_elf(p, n, opts.demangle, syms))
    {
      cout << "could not find symbol '"
	   << opts.symbol_name
	   << "' in file '";
      if (opts.absolute_path)
	cout << opts.elf_path << "'\n";
      else
	cout << basename(opts.elf_path);
      return 0;
    }

  elf_symbol sym = syms[0];
  cout << " found symbol '" << n << "'";
  if (n != sym.get_name())
    cout << " (" << sym.get_name() << ")";
    cout << ", an instance of "
	 << (elf_symbol::type) sym.get_type()
	 << " of " << sym.get_binding();
    if (syms.size() > 1 || !sym.get_version().is_empty())
      {
	cout << ", of version";
	if (syms.size () > 1)
	  cout << "s";
	cout << " ";
	for (vector<elf_symbol>::const_iterator i = syms.begin();
	     i != syms.end();
	     ++i)
	  {
	    if (i != syms.begin())
	      cout << ", ";
	    cout << "'" << i->get_version().str() << "'";
	  }
      }
    cout << '\n';

  return 0;
}
