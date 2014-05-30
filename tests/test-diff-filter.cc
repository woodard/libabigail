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
/// This program runs a diff between input ELF files containing DWARF
/// debugging information and compares the resulting report with a
/// reference report.  If the resulting report is different from the
/// reference report, the test has failed.  Note that the comparison
/// is done using the bidiff command line comparison tool.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::cerr;

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.
struct InOutSpec
{
  const char* in_elfv0_path;
  const char* in_elfv1_path;
  const char* bidiff_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test0-report.txt",
    "output/test-diff-filter/test0-report.txt",
  },
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--harmless --no-linkage-names",
    "data/test-diff-filter/test01-report.txt",
    "output/test-diff-filter/test01-report.txt",
  },
  {
    "data/test-diff-filter/test1-v0.o",
    "data/test-diff-filter/test1-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test1-report.txt",
    "output/test-diff-filter/test1-report.txt",
  },
  {
    "data/test-diff-filter/test2-v0.o",
    "data/test-diff-filter/test2-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test2-report.txt",
    "output/test-diff-filter/test2-report.txt",
  },
  {
    "data/test-diff-filter/test3-v0.o",
    "data/test-diff-filter/test3-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test3-report.txt",
    "output/test-diff-filter/test3-report.txt",
  },
  {
    "data/test-diff-filter/test4-v0.o",
    "data/test-diff-filter/test4-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test4-report.txt",
    "output/test-diff-filter/test4-report.txt",
  },
  {
    "data/test-diff-filter/test5-v0.o",
    "data/test-diff-filter/test5-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test5-report.txt",
    "output/test-diff-filter/test5-report.txt",
  },
  {
    "data/test-diff-filter/test6-v0.o",
    "data/test-diff-filter/test6-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test6-report.txt",
    "output/test-diff-filter/test6-report.txt",
  },
  {
    "data/test-diff-filter/test7-v0.o",
    "data/test-diff-filter/test7-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test7-report.txt",
    "output/test-diff-filter/test7-report.txt",
  },
  {
    "data/test-diff-filter/test8-v0.o",
    "data/test-diff-filter/test8-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test8-report.txt",
    "output/test-diff-filter/test8-report.txt",
  },
  {
    "data/test-diff-filter/test9-v0.o",
    "data/test-diff-filter/test9-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test9-report.txt",
    "output/test-diff-filter/test9-report.txt",
  },
  {
    "data/test-diff-filter/test10-v0.o",
    "data/test-diff-filter/test10-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test10-report.txt",
    "output/test-diff-filter/test10-report.txt",
  },
  {
    "data/test-diff-filter/test11-v0.o",
    "data/test-diff-filter/test11-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test11-report.txt",
    "output/test-diff-filter/test11-report.txt",
  },
  {
    "data/test-diff-filter/test12-v0.o",
    "data/test-diff-filter/test12-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test12-report.txt",
    "output/test-diff-filter/test12-report.txt",
  },
  {
    "data/test-diff-filter/test13-v0.o",
    "data/test-diff-filter/test13-v1.o",
    "--no-linkage-names",
    "data/test-diff-filter/test13-report.txt",
    "output/test-diff-filter/test13-report.txt",
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools::ensure_parent_dir_created;

  bool is_ok = true;
  string in_elfv0_path, in_elfv1_path,
    bidiff_options, bidiff, cmd,
    ref_diff_report_path, out_diff_report_path;

    for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
      {
	in_elfv0_path = get_src_dir() + "/tests/" + s->in_elfv0_path;
	in_elfv1_path = get_src_dir() + "/tests/" + s->in_elfv1_path;
	bidiff_options = s->bidiff_options;
	ref_diff_report_path = get_src_dir() + "/tests/" + s->in_report_path;
	out_diff_report_path = get_build_dir() + "/tests/" + s->out_report_path;

	if (!ensure_parent_dir_created(out_diff_report_path))
	  {
	    cerr << "could not create parent directory for "
		 << out_diff_report_path;
	    is_ok = false;
	    continue;
	  }

	bidiff = get_build_dir() + "/tools/bidiff";
	bidiff += " " + bidiff_options;

	cmd = bidiff + " " + in_elfv0_path + " " + in_elfv1_path;
	cmd += " > " + out_diff_report_path;

	bool bidiff_ok = true;
	if (system(cmd.c_str()))
	  bidiff_ok = false;

	if (bidiff_ok)
	  {
	    cmd = "diff -u " + ref_diff_report_path
	      + " " + out_diff_report_path;
	    if (system(cmd.c_str()))
	      is_ok = false;
	  }
	else
	  is_ok = false;
      }

    return !is_ok;
}
