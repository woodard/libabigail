// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "test-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-tools-utils.h"
#include "abg-reader.h"
#include "abg-writer.h"

struct InOutSpec
{
  const char* in_path;
  const char* out_path;
};// end struct InOutSpec

/// This is an aggregate that specifies where the test gets the
/// elements that it reads to build an archive.  It also specifies
/// where to write the output result of the element that is written
/// back to disk, for diffing purposes.
InOutSpec archive_elements[] =
{
  {
    "data/test-write-read-archive/test0.xml",
    "output/test-write-read-archive/test0.xml",
  },
  {
    "data/test-write-read-archive/test1.xml",
    "output/test-write-read-archive/test2.xml",
  },
  {
    "data/test-write-read-archive/test2.xml",
    "output/test-write-read-archive/test2.xml",
  },
  {
    "data/test-write-read-archive/test3.xml",
    "output/test-write-read-archive/test3.xml",
  },
  {
    "data/test-write-read-archive/test4.xml",
    "output/test-write-read-archive/test4.xml",
  },
  // This should be the last entry.
  {NULL, NULL}
};

#define NUM_ARCHIVES_ELEMENTS \
  ((sizeof(archive_elements) / sizeof(InOutSpec)) -1)

/// Where to write the archive, and where to read it from to get the
/// base for the diffing.
const InOutSpec archive_spec =
{
  "data/test-write-read-archive/archive.abi",
  "output/test-write-read-archive/archive.abi"
};

using std::string;
using std::cerr;
using std::ofstream;
using abg_compat::shared_ptr;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::translation_unit;
using abigail::xml_reader::read_corpus_from_file;
using abigail::xml_writer::write_corpus_to_archive;

int
main()
{
  // Read the elements into abigail::translation_unit and stick them
  // into an abigail::corpus.
  string in_path, out_path;
  bool is_ok = true;

  out_path =
    abigail::tests::get_build_dir() + "/tests/" + archive_spec.out_path;

  if (!abigail::tools_utils::ensure_parent_dir_created(out_path))
    {
      cerr << "Could not create parent director for " << out_path;
      return 1;
    }

  corpus_sptr abi_corpus(new corpus(out_path));

  for (InOutSpec *s = archive_elements; s->in_path; ++s)
    {
      in_path = abigail::tests::get_src_dir() + "/tests/" + s->in_path;
      abigail::translation_unit_sptr tu =
	abigail::xml_reader::read_translation_unit_from_file(in_path);
      if (!tu || tu->is_empty())
	{
	  cerr << "failed to read " << in_path << "\n";
	  is_ok = false;
	  continue;
	}

      string file_name;
      abigail::tools_utils::base_name(tu->get_path(), file_name);
      tu->set_path(file_name);
      abi_corpus->add(tu);
    }

  if (!write_corpus_to_archive(abi_corpus))
    {
      cerr  << "failed to write archive file: " << abi_corpus->get_path();
      return 1;
    }

  // Diff the archive members.
  //
  // Basically, re-read the corpus from disk, walk the loaded
  // translation units, write them back and diff them against their
  // reference.

  abi_corpus->drop_translation_units();
  if (abi_corpus->get_translation_units().size())
    {
      cerr << "In-memory object of abi corpus at '"
	   << abi_corpus->get_path()
	   << "' still has translation units after call to "
	      "corpus::drop_translation_units!";
      return false;
    }

  if (read_corpus_from_file(abi_corpus) != NUM_ARCHIVES_ELEMENTS)
    {
      cerr << "Failed to load the abi corpus from path '"
	   << abi_corpus->get_path()
	   << "'";
      return 1;
    }

  if (abi_corpus->get_translation_units().size() != NUM_ARCHIVES_ELEMENTS)
    {
      cerr << "Read " << abi_corpus->get_translation_units().size()
	   << " elements from the abi corpus at "
	   << abi_corpus->get_path()
	   << " instead of "
	   << NUM_ARCHIVES_ELEMENTS
	   << "\n";
      return 1;
    }

  for (unsigned i = 0; i < NUM_ARCHIVES_ELEMENTS; ++i)
    {
      InOutSpec& spec = archive_elements[i];
      out_path =
	abigail::tests::get_build_dir() + "/tests/" + spec.out_path;
      using abigail::xml_writer::write_translation_unit;
      bool wrote =
	write_translation_unit(*abi_corpus->get_translation_units()[i],
			       /*indent=*/0, out_path);
      if (!wrote)
	{
	  cerr << "Failed to serialize translation_unit to '"
	       << out_path
	       << "'\n";
	  is_ok = false;
	}

      string ref =
	abigail::tests::get_src_dir() + "/tests/" + spec.in_path;
      string cmd = "diff -u " + ref + " " + out_path;

      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}
