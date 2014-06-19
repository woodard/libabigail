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

// Author: Dodji Seketeli

/// @file read ELF binaries containing DWARF, save them in XML corpus
/// files and diff the corpus files against reference XML corpus
/// files.

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-ir.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::ofstream;
using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from, and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elf_path;
  const char* in_abi_path;
  const char* out_abi_path;
};// end struct InOutSpec


InOutSpec in_out_specs[] =
{
  {
    "data/test-read-dwarf/test0",
    "data/test-read-dwarf/test0.abi",
    "output/test-read-dwarf/test0.abi"
  },
  {
    "data/test-read-dwarf/test1",
    "data/test-read-dwarf/test1.abi",
    "output/test-read-dwarf/test1.abi"
  },
  {
    "data/test-read-dwarf/test2.so",
    "data/test-read-dwarf/test2.so.abi",
    "output/test-read-dwarf/test2.so.abi"
  },
  {
    "data/test-read-dwarf/test3.so",
    "data/test-read-dwarf/test3.so.abi",
    "output/test-read-dwarf/test3.so.abi"
  },
  // This should be the last entry.
  {NULL, NULL, NULL}
};

int
main()
{
  unsigned result = 1;

  bool is_ok = true;
  string in_elf_path, in_abi_path, out_abi_path;
  abigail::corpus_sptr corp;

  for (InOutSpec* s = in_out_specs; s->in_elf_path; ++s)
    {
      in_elf_path = abigail::tests::get_src_dir() + "/tests/" + s->in_elf_path;
      abigail::dwarf_reader::read_corpus_from_elf(in_elf_path,
						  /*debug_info_root_path=*/0,
						  corp);
      if (!corp)
	{
	  cerr << "failed to read " << in_elf_path << "\n";
	  is_ok = false;
	  continue;
	}
      corp->set_path(s->in_elf_path);

      out_abi_path =
	abigail::tests::get_build_dir() + "/tests/" + s->out_abi_path;
      if (!abigail::tools::ensure_parent_dir_created(out_abi_path))
	{
	  cerr << "Could not create parent director for " << out_abi_path;
	  is_ok = false;
	  return result;
	}

      ofstream of(out_abi_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << out_abi_path << "\n";
	  is_ok = false;
	  continue;
	}

      bool r =
	abigail::xml_writer::write_corpus_to_native_xml(corp,
							/*indent=*/0,
							of);
      is_ok = (is_ok && r);
      of.close();

      in_abi_path = abigail::tests::get_src_dir() + "/tests/" + s->in_abi_path;
      string cmd = "diff -u " + in_abi_path + " " + out_abi_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}
