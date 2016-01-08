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

// Author: Dodji Seketeli

/// @file
///
/// This program tests that the representation of types by the
/// internal representation of libabigail is stable through reading
/// from ELF/DWARF, constructing an internal represenation, saving that
/// internal presentation to the abixml format, reading from that
/// abixml format and constructing an internal representation from it
/// again.
///
/// This program thus compares the internal representation that is
/// built from reading from ELF/DWARF and the one that is built from
/// the abixml (which itself results from the serialization of the
/// first internal representation to abixml).
///
/// The comparison is expected to yield the empty set.


#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"
#include "abg-dwarf-reader.h"
#include "abg-comparison.h"

using std::string;
using std::ofstream;
using std::cerr;

// A set of elf files to test type stability for.
const char* elf_paths[] =
{
  "data/test-types-stability/pr19139-DomainNeighborMapInst.o",
  "data/test-types-stability/pr19202-libmpi_gpfs.so.5.0",
  "data/test-types-stability/pr19026-libvtkIOSQL-6.1.so.1",
  "data/test-types-stability/pr19138-elf0",
  // The below should always be the last element of array.
  0
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;

  bool is_ok = true;

  for (const char** p = elf_paths; p && *p; ++p)
    {
      string abidw = get_build_dir() + "/tools/abidw";
      string elf_path = get_src_dir() + "/tests/" + *p;
      string cmd = abidw + " --abidiff " + elf_path;
      if (system(cmd.c_str()))
	{
	  cerr << "IR stability issue detected for binary "
	       << elf_path;
	  is_ok = false;
	}
    }

  return !is_ok;
}
