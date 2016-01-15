// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2016 Red Hat, Inc.
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
#include <unistd.h>
#include <pthread.h>
#include "abg-ir.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::ofstream;
using std::cerr;
using abigail::tests::get_build_dir;
using abigail::dwarf_reader::read_corpus_from_elf;

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
  {
    "data/test-read-dwarf/test4.so",
    "data/test-read-dwarf/test4.so.abi",
    "output/test-read-dwarf/test4.so.abi"
  },
  {
    "data/test-read-dwarf/test5.o",
    "data/test-read-dwarf/test5.o.abi",
    "output/test-read-dwarf/test5.o.abi"
  },
  {
    "data/test-read-dwarf/test6.so",
    "data/test-read-dwarf/test6.so.abi",
    "output/test-read-dwarf/test6.so.abi"
  },
  {
    "data/test-read-dwarf/test7.so",
    "data/test-read-dwarf/test7.so.abi",
    "output/test-read-dwarf/test7.so.abi"
  },
  {
    "data/test-read-dwarf/test8-qualified-this-pointer.so",
    "data/test-read-dwarf/test8-qualified-this-pointer.so.abi",
    "output/test-read-dwarf/test8-qualified-this-pointer.so.abi"
  },
  {
    "data/test-read-dwarf/test9-pr18818-clang.so",
    "data/test-read-dwarf/test9-pr18818-clang.so.abi",
    "output/test-read-dwarf/test9-pr18818-clang.so.abi",
  },
  {
    "data/test-read-dwarf/test10-pr18818-gcc.so",
    "data/test-read-dwarf/test10-pr18818-gcc.so.abi",
    "output/test-read-dwarf/test10-pr18818-gcc.so.abi",
  },
  {
    "data/test-read-dwarf/test11-pr18828.so",
    "data/test-read-dwarf/test11-pr18828.so.abi",
    "output/test-read-dwarf/test11-pr18828.so.abi",
  },
  {
    "data/test-read-dwarf/test12-pr18844.so",
    "data/test-read-dwarf/test12-pr18844.so.abi",
    "output/test-read-dwarf/test12-pr18844.so.abi",
  },
  {
    "data/test-read-dwarf/test13-pr18894.so",
    "data/test-read-dwarf/test13-pr18894.so.abi",
    "output/test-read-dwarf/test13-pr18894.so.abi",
  },
  {
    "data/test-read-dwarf/test14-pr18893.so",
    "data/test-read-dwarf/test14-pr18893.so.abi",
    "output/test-read-dwarf/test14-pr18893.so.abi",
  },
  {
    "data/test-read-dwarf/test15-pr18892.so",
    "data/test-read-dwarf/test15-pr18892.so.abi",
    "output/test-read-dwarf/test15-pr18892.so.abi",
  },
  {
    "data/test-read-dwarf/test16-pr18904.so",
    "data/test-read-dwarf/test16-pr18904.so.abi",
    "output/test-read-dwarf/test16-pr18904.so.abi",
  },
  {
    "data/test-read-dwarf/test17-pr19027.so",
    "data/test-read-dwarf/test17-pr19027.so.abi",
    "output/test-read-dwarf/test17-pr19027.so.abi",
  },
  {
    "data/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so",
    "data/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
    "output/test-read-dwarf/test18-pr19037-libvtkRenderingLIC-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so",
    "data/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so.abi",
    "output/test-read-dwarf/test19-pr19023-libtcmalloc_and_profiler.so.abi",
  },
  {
    "data/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so",
    "data/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so.abi",
    "output/test-read-dwarf/test20-pr19025-libvtkParallelCore-6.1.so.abi",
  },
  {
    "data/test-read-dwarf/test21-pr19092.so",
    "data/test-read-dwarf/test21-pr19092.so.abi",
    "output/test-read-dwarf/test21-pr19092.so.abi",
  },
  {
    "data/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so",
    "data/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so.abi",
    "output/test-read-dwarf/test22-pr19097-libstdc++.so.6.0.17.so.abi",
  },
  // This should be the last entry.
  {NULL, NULL, NULL}
};

// The global pointer to the testsuite paths.
InOutSpec *iospec = in_out_specs;
// Lock to help atomically increment iospec.
pthread_mutex_t spec_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
// No lock needed here, since is_ok is only ever re-set to false.
bool is_ok = true;

// These prefixes don't change during the program's lifetime, so
// we only get them once.
const string out_abi_base = string(get_build_dir()) + "/tests/";
const string in_elf_base  = string(abigail::tests::get_src_dir()) + "/tests/";
const string in_abi_base = in_elf_base;

void
handle_in_out_spec(void)
{
  string in_elf_path, in_abi_path, out_abi_path;
  abigail::ir::environment_sptr env;
  InOutSpec *s;

  while (true)
  {
    pthread_mutex_lock(&spec_lock);
    if (iospec->in_elf_path)
      s = iospec++;
    else
      s = NULL;
    pthread_mutex_unlock(&spec_lock);

    if (!s)
      pthread_exit(NULL);
    in_elf_path = in_elf_base + s->in_elf_path;
    env.reset(new abigail::ir::environment);
    abigail::dwarf_reader::status status =
      abigail::dwarf_reader::STATUS_UNKNOWN;
    abigail::corpus_sptr corp =
      read_corpus_from_elf(in_elf_path,
			   /*debug_info_root_path=*/0,
			   env.get(),
			   /*load_all_types=*/false,
			   status);
    if (!corp)
      {
	cerr << "failed to read " << in_elf_path << "\n";
	is_ok = false;
	continue;
      }
    corp->set_path(s->in_elf_path);
    // Do not take architecture names in comparison so that these
    // test input binaries can come from whatever arch the
    // programmer likes.
    corp->set_architecture_name("");

    out_abi_path = out_abi_base + s->out_abi_path;
    if (!abigail::tools_utils::ensure_parent_dir_created(out_abi_path))
      {
	cerr << "Could not create parent directory for " << out_abi_path;
	is_ok = false;
	exit(1);
      }

    ofstream of(out_abi_path.c_str(), std::ios_base::trunc);
    if (!of.is_open())
      {
	cerr << "failed to read " << out_abi_path << "\n";
	is_ok = false;
	continue;
      }

    pthread_mutex_lock(&write_lock);
    is_ok =
      abigail::xml_writer::write_corpus_to_native_xml(corp,
						      /*indent=*/0,
						      of);
    pthread_mutex_unlock(&write_lock);
    of.close();

    string abidw = string(get_build_dir()) + "/tools/abidw";
    string cmd = abidw + " --abidiff " + in_elf_path;
    if (system(cmd.c_str()))
      {
	cerr << "ABIs differ:\n"
	     << in_elf_path
	     << "\nand:\n"
	     << out_abi_path
	     << "\n";
	is_ok = false;
      }

    in_abi_path = in_abi_base +  s->in_abi_path;
    cmd = "diff -u " + in_abi_path + " " + out_abi_path;
    if (system(cmd.c_str()))
      is_ok = false;
  }
}

int
main(int argc, char *argv[])
{
  // Number of currently online processors in the system.
  size_t nprocs = sysconf(_SC_NPROCESSORS_ONLN);
  // All the pthread_ts we've created.
  pthread_t *pthr = new pthread_t[nprocs];

  if (argc == 2)
    {
      if (argv[1] == string("--no-parallel"))
	nprocs = 1;
      else
	{
	  cerr << "unrecognized option\n";
	  cerr << "usage: " << argv[0] << " [--no-parallel]\n" ;
	  return 1;
	}
    }

  assert(nprocs >= 1);

  for (size_t i = 0; i < nprocs; ++i)
    pthread_create(&pthr[i], NULL,
		   (void*(*)(void*))handle_in_out_spec,
		   NULL);

  for (size_t i = 0; i < nprocs; ++i)
    pthread_join(pthr[i], NULL);

  delete pthr;

  return !is_ok;
}
