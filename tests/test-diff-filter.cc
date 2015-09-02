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
/// This program runs a diff between input ELF files containing DWARF
/// debugging information and compares the resulting report with a
/// reference report.  If the resulting report is different from the
/// reference report, the test has failed.  Note that the comparison
/// is done using the abidiff command line comparison tool.
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
  const char* abidiff_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test0-report.txt",
    "output/test-diff-filter/test0-report.txt",
  },
  {
    "data/test-diff-filter/test0-v0.o",
    "data/test-diff-filter/test0-v1.o",
    "--harmless --no-linkage-name --no-redundant",
    "data/test-diff-filter/test01-report.txt",
    "output/test-diff-filter/test01-report.txt",
  },
  {
    "data/test-diff-filter/test1-v0.o",
    "data/test-diff-filter/test1-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test1-report.txt",
    "output/test-diff-filter/test1-report.txt",
  },
  {
    "data/test-diff-filter/test2-v0.o",
    "data/test-diff-filter/test2-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test2-report.txt",
    "output/test-diff-filter/test2-report.txt",
  },
  {
    "data/test-diff-filter/test3-v0.o",
    "data/test-diff-filter/test3-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test3-report.txt",
    "output/test-diff-filter/test3-report.txt",
  },
  {
    "data/test-diff-filter/test4-v0.o",
    "data/test-diff-filter/test4-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test4-report.txt",
    "output/test-diff-filter/test4-report.txt",
  },
  {
    "data/test-diff-filter/test5-v0.o",
    "data/test-diff-filter/test5-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test5-report.txt",
    "output/test-diff-filter/test5-report.txt",
  },
  {
    "data/test-diff-filter/test6-v0.o",
    "data/test-diff-filter/test6-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test6-report.txt",
    "output/test-diff-filter/test6-report.txt",
  },
  {
    "data/test-diff-filter/test7-v0.o",
    "data/test-diff-filter/test7-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test7-report.txt",
    "output/test-diff-filter/test7-report.txt",
  },
  {
    "data/test-diff-filter/test8-v0.o",
    "data/test-diff-filter/test8-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test8-report.txt",
    "output/test-diff-filter/test8-report.txt",
  },
  {
    "data/test-diff-filter/test9-v0.o",
    "data/test-diff-filter/test9-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test9-report.txt",
    "output/test-diff-filter/test9-report.txt",
  },
  {
    "data/test-diff-filter/test10-v0.o",
    "data/test-diff-filter/test10-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test10-report.txt",
    "output/test-diff-filter/test10-report.txt",
  },
  {
    "data/test-diff-filter/test11-v0.o",
    "data/test-diff-filter/test11-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test11-report.txt",
    "output/test-diff-filter/test11-report.txt",
  },
  {
    "data/test-diff-filter/test12-v0.o",
    "data/test-diff-filter/test12-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test12-report.txt",
    "output/test-diff-filter/test12-report.txt",
  },
  {
    "data/test-diff-filter/test13-v0.o",
    "data/test-diff-filter/test13-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test13-report.txt",
    "output/test-diff-filter/test13-report.txt",
  },
  {
    "data/test-diff-filter/test14-v0.o",
    "data/test-diff-filter/test14-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test14-0-report.txt",
    "output/test-diff-filter/test14-0-report.txt",
  },
  {
    "data/test-diff-filter/test14-v0.o",
    "data/test-diff-filter/test14-v1.o",
    "--redundant",
    "data/test-diff-filter/test14-1-report.txt",
    "output/test-diff-filter/test14-1-report.txt",
  },
  {
    "data/test-diff-filter/test15-v0.o",
    "data/test-diff-filter/test15-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test15-0-report.txt",
    "output/test-diff-filter/test15-0-report.txt",
  },
  {
    "data/test-diff-filter/test15-v0.o",
    "data/test-diff-filter/test15-v1.o",
    "--redundant",
    "data/test-diff-filter/test15-1-report.txt",
    "output/test-diff-filter/test15-1-report.txt",
  },
  {
    "data/test-diff-filter/test16-v0.o",
    "data/test-diff-filter/test16-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test16-report.txt",
    "output/test-diff-filter/test16-report.txt",
  },
  {
    "data/test-diff-filter/test16-v0.o",
    "data/test-diff-filter/test16-v1.o",
    "--redundant",
    "data/test-diff-filter/test16-report-2.txt",
    "output/test-diff-filter/test16-report-2.txt",
  },
  {
    "data/test-diff-filter/test17-v0.o",
    "data/test-diff-filter/test17-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test17-0-report.txt",
    "output/test-diff-filter/test17-0-report.txt",
  },
  {
    "data/test-diff-filter/test17-v0.o",
    "data/test-diff-filter/test17-v1.o",
    "--redundant",
    "data/test-diff-filter/test17-1-report.txt",
    "output/test-diff-filter/test17-1-report.txt",
  },
  {
    "data/test-diff-filter/test18-v0.o",
    "data/test-diff-filter/test18-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test18-report.txt",
    "output/test-diff-filter/test18-report.txt",
  },
  {
    "data/test-diff-filter/test19-enum-v0.o",
    "data/test-diff-filter/test19-enum-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test19-enum-report-0.txt",
    "output/test-diff-filter/test19-enum-report-0.txt",
  },
  {
    "data/test-diff-filter/test19-enum-v0.o",
    "data/test-diff-filter/test19-enum-v1.o",
    "--harmless",
    "data/test-diff-filter/test19-enum-report-1.txt",
    "output/test-diff-filter/test19-enum-report-1.txt",
  },
  {
    "data/test-diff-filter/test20-inline-v0.o",
    "data/test-diff-filter/test20-inline-v1.o",
    "--no-redundant",
    "data/test-diff-filter/test20-inline-report-0.txt",
    "output/test-diff-filter/test20-inline-report-0.txt",
  },
  {
    "data/test-diff-filter/test20-inline-v0.o",
    "data/test-diff-filter/test20-inline-v1.o",
    "--harmless",
    "data/test-diff-filter/test20-inline-report-1.txt",
    "output/test-diff-filter/test20-inline-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest21-compatible-vars-v0.so",
    "data/test-diff-filter/libtest21-compatible-vars-v1.so",
    "--harmless",
    "data/test-diff-filter/test21-compatible-vars-report-0.txt",
    "output/test-diff-filter/test21-compatible-vars-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest21-compatible-vars-v0.so",
    "data/test-diff-filter/libtest21-compatible-vars-v1.so",
    "--no-redundant",
    "data/test-diff-filter/test21-compatible-vars-report-1.txt",
    "output/test-diff-filter/test21-compatible-vars-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest22-compatible-fns-v0.so",
    "data/test-diff-filter/libtest22-compatible-fns-v1.so",
    "--harmless",
    "data/test-diff-filter/test22-compatible-fns-report-0.txt",
    "output/test-diff-filter/test22-compatible-fns-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest22-compatible-fns-v0.so",
    "data/test-diff-filter/libtest22-compatible-fns-v1.so",
    "--no-redundant",
    "data/test-diff-filter/test22-compatible-fns-report-1.txt",
    "output/test-diff-filter/test22-compatible-fns-report-1.txt",
  },
  {
    "data/test-diff-filter/libtest23-redundant-fn-parm-change-v0.so",
    "data/test-diff-filter/libtest23-redundant-fn-parm-change-v1.so",
    "",
    "data/test-diff-filter/test23-redundant-fn-parm-change-report-0.txt ",
    "output/test-diff-filter/test23-redundant-fn-parm-change-report-0.txt ",
  },
  {
    "data/test-diff-filter/libtest24-compatible-vars-v0.so",
    "data/test-diff-filter/libtest24-compatible-vars-v1.so",
    "",
    "data/test-diff-filter/test24-compatible-vars-report-0.txt ",
    "output/test-diff-filter/test24-compatible-vars-report-0.txt ",
  },
  {
    "data/test-diff-filter/libtest24-compatible-vars-v0.so",
    "data/test-diff-filter/libtest24-compatible-vars-v1.so",
    "--harmless",
    "data/test-diff-filter/test24-compatible-vars-report-1.txt ",
    "output/test-diff-filter/test24-compatible-vars-report-1.txt ",
  },
  {
    "data/test-diff-filter/libtest25-cyclic-type-v0.so",
    "data/test-diff-filter/libtest25-cyclic-type-v1.so",
    "",
    "data/test-diff-filter/test25-cyclic-type-report-0.txt ",
    "output/test-diff-filter/test25-cyclic-type-report-0.txt "
  },
  {
    "data/test-diff-filter/libtest25-cyclic-type-v0.so",
    "data/test-diff-filter/libtest25-cyclic-type-v1.so",
    "--redundant",
    "data/test-diff-filter/test25-cyclic-type-report-1.txt ",
    "output/test-diff-filter/test25-cyclic-type-report-1.txt "
  },
  {
    "data/test-diff-filter/libtest26-qualified-redundant-node-v0.so",
    "data/test-diff-filter/libtest26-qualified-redundant-node-v1.so",
    "",
    "data/test-diff-filter/test26-qualified-redundant-node-report-0.txt",
    "output/test-diff-filter/test26-qualified-redundant-node-report-0.txt"
  },
  {
    "data/test-diff-filter/libtest26-qualified-redundant-node-v0.so",
    "data/test-diff-filter/libtest26-qualified-redundant-node-v1.so",
    "--redundant",
    "data/test-diff-filter/test26-qualified-redundant-node-report-1.txt",
    "output/test-diff-filter/test26-qualified-redundant-node-report-1.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-0.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-0.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-linkage-name --redundant",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-1.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-1.txt"
  },
  {
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest27-redundant-and-filtered-children-nodes-v1.so",
    "--no-linkage-name --redundant --harmless",
    "data/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-2.txt",
    "output/test-diff-filter/test27-redundant-and-filtered-children-nodes-report-2.txt"
  },
  {
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v1.so",
    "--no-linkage-name --no-redundant",
   "data/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-0.txt",
    "output/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-0.txt",
  },
  {
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v0.so",
    "data/test-diff-filter/libtest28-redundant-and-filtered-children-nodes-v1.so",
    "--no-linkage-name --redundant --harmless",
   "data/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-1.txt",
    "output/test-diff-filter/test28-redundant-and-filtered-children-nodes-report-1.txt",
  },
  {
    "data/test-diff-filter/test29-finer-redundancy-marking-v0.o",
    "data/test-diff-filter/test29-finer-redundancy-marking-v1.o",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test29-finer-redundancy-marking-report-0.txt",
    "output/test-diff-filter/test29-finer-redundancy-marking-report-0.txt",
  },
  {
    "data/test-diff-filter/test30-pr18904-rvalueref-liba.so",
    "data/test-diff-filter/test30-pr18904-rvalueref-libb.so",
    "--no-linkage-name --no-redundant",
    "data/test-diff-filter/test30-pr18904-rvalueref-report0.txt",
    "output/test-diff-filter/test30-pr18904-rvalueref-report0.txt",
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::abidiff_status;

  bool is_ok = true;
  string in_elfv0_path, in_elfv1_path,
    abidiff_options, abidiff, cmd,
    ref_diff_report_path, out_diff_report_path;

    for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
      {
	in_elfv0_path = get_src_dir() + "/tests/" + s->in_elfv0_path;
	in_elfv1_path = get_src_dir() + "/tests/" + s->in_elfv1_path;
	abidiff_options = s->abidiff_options;
	ref_diff_report_path = get_src_dir() + "/tests/" + s->in_report_path;
	out_diff_report_path = get_build_dir() + "/tests/" + s->out_report_path;

	if (!ensure_parent_dir_created(out_diff_report_path))
	  {
	    cerr << "could not create parent directory for "
		 << out_diff_report_path;
	    is_ok = false;
	    continue;
	  }

	abidiff = get_build_dir() + "/tools/abidiff";
	abidiff += " " + abidiff_options;

	cmd = abidiff + " " + in_elfv0_path + " " + in_elfv1_path;
	cmd += " > " + out_diff_report_path;

	bool abidiff_ok = true;
	abidiff_status status =
	  static_cast<abidiff_status>(system(cmd.c_str()) & 255);
	if (abigail::tools_utils::abidiff_status_has_error(status))
	  abidiff_ok = false;

	if (abidiff_ok)
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
