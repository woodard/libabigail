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

/// @file
///
/// This program runs a diff between input dwarf files and compares
/// the resulting report with a reference report.  If the resulting
/// report is different from the reference report, the test has
/// failed.  Note that the comparison is done using the libabigail
/// library directly.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

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

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elfv0_path;
  const char* in_elfv1_path;
  const char* in_report_path;
  const char* out_report_path;
};// end struct InOutSpec

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-dwarf/test0-v0.o",
    "data/test-diff-dwarf/test0-v1.o",
    "data/test-diff-dwarf/test0-report.txt",
    "output/test-diff-dwarf/test0-report.txt"
  },
  {
    "data/test-diff-dwarf/test1-v0.o",
    "data/test-diff-dwarf/test1-v1.o",
    "data/test-diff-dwarf/test1-report.txt",
    "output/test-diff-dwarf/test1-report.txt"
  },
  {
    "data/test-diff-dwarf/test2-v0.o",
    "data/test-diff-dwarf/test2-v1.o",
    "data/test-diff-dwarf/test2-report.txt",
    "output/test-diff-dwarf/test2-report.txt"
  },
  {
    "data/test-diff-dwarf/test3-v0.o",
    "data/test-diff-dwarf/test3-v1.o",
    "data/test-diff-dwarf/test3-report.txt",
    "output/test-diff-dwarf/test3-report.txt"
  },
  {
    "data/test-diff-dwarf/test3-v0.o",
    "data/test-diff-dwarf/test3-v1.o",
    "data/test-diff-dwarf/test3-report.txt",
    "output/test-diff-dwarf/test3-report.txt"
  },
  {
    "data/test-diff-dwarf/test4-v0.o",
    "data/test-diff-dwarf/test4-v1.o",
    "data/test-diff-dwarf/test4-report.txt",
    "output/test-diff-dwarf/test4-report.txt"
  },
  {
    "data/test-diff-dwarf/test5-v0.o",
    "data/test-diff-dwarf/test5-v1.o",
    "data/test-diff-dwarf/test5-report.txt",
    "output/test-diff-dwarf/test5-report.txt"
  },
  {
    "data/test-diff-dwarf/test6-v0.o",
    "data/test-diff-dwarf/test6-v1.o",
    "data/test-diff-dwarf/test6-report.txt",
    "output/test-diff-dwarf/test6-report.txt"
  },
  {
    "data/test-diff-dwarf/test7-v0.o",
    "data/test-diff-dwarf/test7-v1.o",
    "data/test-diff-dwarf/test7-report.txt",
    "output/test-diff-dwarf/test7-report.txt"
  },
  {
    "data/test-diff-dwarf/test8-v0.o",
    "data/test-diff-dwarf/test8-v1.o",
    "data/test-diff-dwarf/test8-report.txt",
    "output/test-diff-dwarf/test8-report.txt"
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools::ensure_parent_dir_created;
  using abigail::dwarf_reader::read_corpus_from_elf;
  using abigail::comparison::compute_diff;
  using abigail::comparison::corpus_diff_sptr;

  bool is_ok = true;
  string in_elfv0_path, in_elfv1_path,
    ref_diff_report_path, out_diff_report_path;
  abigail::corpus_sptr corp0, corp1;

  for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
    {
      in_elfv0_path = get_src_dir() + "/tests/" + s->in_elfv0_path;
      in_elfv1_path = get_src_dir() + "/tests/" + s->in_elfv1_path;
      out_diff_report_path = get_build_dir() + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_diff_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_diff_report_path;
	  is_ok = false;
	  continue;
	}

      read_corpus_from_elf(in_elfv0_path,
			   /*debug_info_root_path=*/0,
			   corp0);
      read_corpus_from_elf(in_elfv1_path,
			   /*debug_info_root_path=*/0,
			   corp1);

      if (!corp0)
	{
	  cerr << "failed to read " << in_elfv0_path << "\n";
	  is_ok = false;
	  continue;
	}
      if (!corp1)
	{
	  cerr << "failed to read " << in_elfv1_path << "\n";
	  is_ok = false;
	  continue;
	}

      corp0->set_path(s->in_elfv0_path);
      corp1->set_path(s->in_elfv1_path);

      corpus_diff_sptr d = compute_diff(corp0, corp1);
      if (!d)
	{
	  cerr << "failed to compute diff\n";
	  is_ok = false;
	  continue;
	}

      ref_diff_report_path = get_src_dir() + "/tests/" + s->in_report_path;
      out_diff_report_path = get_build_dir() + "/tests/" + s->out_report_path;

      ofstream of(out_diff_report_path, std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << out_diff_report_path << "\n";
	  is_ok = false;
	  continue;
	}

      d->report(of);
      of.close();

      string cmd =
	"diff -u " + ref_diff_report_path + " " + out_diff_report_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}
