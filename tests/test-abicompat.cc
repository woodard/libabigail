// -*- Mode: C++ -*-
//
// Copyright (C) 2014 Red Hat, Inc.
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
/// Given a program P that links against a library L of version V
/// denoted L(V), this program checks if P is still ABI compatible
/// with a subsequent version of L denoted L(V+N), N being a positive
/// integer.  The result of the check is a report that is compared
/// against a reference report.  This program actually performs these
/// checks for a variety of tuple {P, L(V), L(V+N)}
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::cerr;

struct InOutSpec
{
  const char* in_app_path;
  const char* in_lib1_path;
  const char* in_lib2_path;
  const char* suppressions;
  const char* options;
  const char* in_report_path;
  const char* out_report_path;
};

InOutSpec in_out_specs[] =
{
  {
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test0-fn-changed-report-0.txt",
    "output/test-abicompat/test0-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "data/test-abicompat/test0-fn-changed-0.suppr",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test0-fn-changed-report-1.txt",
    "output/test-abicompat/test0-fn-changed-report-1.txt",
  },
  {
    "data/test-abicompat/test1-fn-removed-app",
    "data/test-abicompat/libtest1-fn-removed-v0.so",
    "data/test-abicompat/libtest1-fn-removed-v1.so",
    "",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test1-fn-removed-report-0.txt",
    "output/test-abicompat/test1-fn-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test2-var-removed-app",
    "data/test-abicompat/libtest2-var-removed-v0.so",
    "data/test-abicompat/libtest2-var-removed-v1.so",
    "",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test2-var-removed-report-0.txt",
    "output/test-abicompat/test2-var-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test3-fn-removed-app",
    "data/test-abicompat/libtest3-fn-removed-v0.so",
    "data/test-abicompat/libtest3-fn-removed-v1.so",
    "",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test3-fn-removed-report-0.txt",
    "output/test-abicompat/test3-fn-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test4-soname-changed-app",
    "data/test-abicompat/libtest4-soname-changed-v0.so",
    "data/test-abicompat/libtest4-soname-changed-v1.so",
    "",
    "--show-base-names --no-redundant",
    "data/test-abicompat/test4-soname-changed-report-0.txt",
    "output/test-abicompat/test4-soname-changed-report-0.txt",
  },
  // This entry must be the last one.
  {0, 0, 0, 0, 0, 0, 0}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools::ensure_parent_dir_created;

  bool is_ok = true;
  string in_app_path, in_lib1_path, in_lib2_path, suppression_path,
    abicompat_options, ref_report_path, out_report_path, abicompat, cmd;

  for (InOutSpec* s = in_out_specs; s->in_app_path; ++s)
    {
      in_app_path = get_src_dir() + "/tests/" + s->in_app_path;
      in_lib1_path = get_src_dir() + "/tests/" + s->in_lib1_path;
      in_lib2_path = get_src_dir() + "/tests/" + s->in_lib2_path;
      if (s->suppressions == 0 || !strcmp(s->suppressions, ""))
	suppression_path.clear();
      else
	suppression_path = get_src_dir() + "/tests/" + s->suppressions;
      abicompat_options = s->options;
      ref_report_path = get_src_dir() + "/tests/" + s->in_report_path;
      out_report_path = get_build_dir() + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_report_path;
	  is_ok = false;
	  continue;
	}

      abicompat = get_build_dir() + "/tools/abicompat";
      if (!suppression_path.empty())
	abicompat += " --suppressions " + suppression_path;
      abicompat += " " + abicompat_options;

      cmd = abicompat + " "
	+ in_app_path + " " + in_lib1_path + " " + in_lib2_path;

      cmd += " > " + out_report_path;

      bool abicompat_ok = true;
      if (system(cmd.c_str()))
	abicompat_ok = false;

      if (abicompat_ok)
	{
	  cmd = "diff -u " + ref_report_path + " " + out_report_path;
	  if (system(cmd.c_str()))
	    is_ok = false;
	}
      else
	is_ok = false;
    }

  return !is_ok;
}
