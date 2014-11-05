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
/// This program runs a diff between input files and compares the
/// resulting report with a reference report.  If the resulting report
/// is different from the reference report, the test has failed.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "abg-reader.h"
#include "test-utils.h"
#include "abg-comparison.h"

using std::string;
using std::ofstream;
using std::cerr;

struct InOutSpec
{
  const char* first_in_path;
  const char* second_in_path;
  const char* ref_diff_path;
  const char* out_path;
};// end struct InOutSpec

static InOutSpec specs[] =
{
  {
    "data/test-abidiff/test-enum0-v0.cc.bi",
    "data/test-abidiff/test-enum0-v1.cc.bi",
    "data/test-abidiff/test-enum0-report.txt",
    "output/test-abidiff/test-enum0-report.txt"
  },
  {
    "data/test-abidiff/test-enum1-v0.cc.bi",
    "data/test-abidiff/test-enum1-v1.cc.bi",
    "data/test-abidiff/test-enum1-report.txt",
    "output/test-abidiff/test-enum1-report.txt"
  },
  {
    "data/test-abidiff/test-qual-type0-v0.cc.bi",
    "data/test-abidiff/test-qual-type0-v1.cc.bi",
    "data/test-abidiff/test-qual-type0-report.txt",
    "output/test-abidiff/test-qual-type0-report.txt"
  },
  {
    "data/test-abidiff/test-struct0-v0.cc.bi",
    "data/test-abidiff/test-struct0-v1.cc.bi",
    "data/test-abidiff/test-struct0-report.txt",
    "output/test-abidiff/test-struct0-report.txt"
  },
  {
    "data/test-abidiff/test-struct1-v0.cc.bi",
    "data/test-abidiff/test-struct1-v1.cc.bi",
    "data/test-abidiff/test-struct1-report.txt",
    "output/test-abidiff/test-struct1-report.txt"
  },
  {
    "data/test-abidiff/test-var0-v0.cc.bi",
    "data/test-abidiff/test-var0-v1.cc.bi",
    "data/test-abidiff/test-var0-report.txt",
    "output/test-abidiff/test-var0-report.txt"
  },
  // This should be the last entry.
  {0, 0, 0, 0}
};

#define  NUM_SPEC_ELEMS \
  ((sizeof(specs) / sizeof(InOutSpec)) - 1)

using std::string;
using std::cerr;
using std::ofstream;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::compute_diff;

int
main(int, char*[])
{
  bool is_ok = true;

  string out_path =
    abigail::tests::get_build_dir() + "/tests/" + specs->out_path;
  if (!abigail::tools::ensure_parent_dir_created(out_path))
    {
      cerr << "Could not create parent director for " << out_path;
      return 1;
    }

  string first_in_path, second_in_path, ref_diff_path;
  for (InOutSpec *s = specs; s->first_in_path; ++s)
    {
      first_in_path =
	abigail::tests::get_src_dir() + "/tests/" + s->first_in_path;
      second_in_path =
	abigail::tests::get_src_dir() + "/tests/" + s->second_in_path;
      ref_diff_path =
	abigail::tests::get_src_dir() + "/tests/" + s->ref_diff_path;
      out_path =
	abigail::tests::get_build_dir() + "/tests/" + s->out_path;

      if (!abigail::tools::ensure_parent_dir_created(out_path))
	{
	  cerr << "Could not create parent directory for " << out_path;
	  continue;
	}

      translation_unit_sptr tu1 =
	abigail::xml_reader::read_translation_unit_from_file(first_in_path);
      if (!tu1)
	{
	  cerr << "failed to read " << tu1->get_path() << "\n";
	  is_ok = false;
	  continue;
	}

      translation_unit_sptr tu2 =
	abigail::xml_reader::read_translation_unit_from_file(second_in_path);
      if (!tu2)
	{
	  cerr << "failed to read " << tu1->get_path() << "\n";
	  is_ok = false;
	  continue;
	}

      translation_unit_diff_sptr d = compute_diff(tu1, tu2);
      ofstream of(out_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "failed to read " << s->out_path << "\n";
	  is_ok = false;
	  continue;
	}

      d->report(of);
      of.close();

      string cmd = "diff -u " + ref_diff_path + " " + out_path;
      if (system(cmd.c_str()))
	is_ok = false;
    }

  return !is_ok;
}
