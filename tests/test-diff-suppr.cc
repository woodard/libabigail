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

/// @file
///
/// This test harness program runs a diff between input ELF files
/// containing DWARF debugging information, exercising the
/// suppressions features of the "bidiff" command line program.
///
/// So it runs the diff diff between the two input files, using a
/// suppression file and compares the resulting diff with a reference
/// one.

#include <sys/wait.h>
#include <cstring>
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
  const char* in_suppr_path;
  const char* bidiff_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    NULL,
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-0.txt",
    "output/test-diff-suppr/test0-type-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "data/test-diff-suppr/test0-type-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-1.txt",
    "output/test-diff-suppr/test0-type-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "data/test-diff-suppr/test0-type-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-2.txt",
    "output/test-diff-suppr/test0-type-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "data/test-diff-suppr/test0-type-suppr-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-3.txt",
    "output/test-diff-suppr/test0-type-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-0.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "data/test-diff-suppr/test1-typedef-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-1.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "data/test-diff-suppr/test1-typedef-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-2.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/test2-struct-suppr-v0.o",
    "data/test-diff-suppr/test2-struct-suppr-v1.o",
    "data/test-diff-suppr/test2-struct-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test2-struct-suppr-report-0.txt",
    "output/test-diff-suppr/test2-struct-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test2-struct-suppr-v0.o",
    "data/test-diff-suppr/test2-struct-suppr-v1.o",
    "data/test-diff-suppr/test2-struct-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test2-struct-suppr-report-1.txt",
    "output/test-diff-suppr/test2-struct-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    NULL,
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-0.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    "data/test-diff-suppr/test3-struct-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-1.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    "data/test-diff-suppr/test3-struct-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-2.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest4-local-suppr-v0.so",
    "data/test-diff-suppr/libtest4-local-suppr-v1.so",
    "data/test-diff-suppr/test4-local-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test4-local-suppr-report-1.txt",
    "output/test-diff-suppr/test4-local-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest4-local-suppr-v0.so",
    "data/test-diff-suppr/libtest4-local-suppr-v1.so",
    "",
    "--no-show-locs",
    "data/test-diff-suppr/test4-local-suppr-report-0.txt",
    "output/test-diff-suppr/test4-local-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-0.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "data/test-diff-suppr/test5-fn-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-1.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "data/test-diff-suppr/test5-fn-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-2.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "data/test-diff-suppr/test5-fn-suppr-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-3.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "data/test-diff-suppr/test5-fn-suppr-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-4.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-4.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "data/test-diff-suppr/test5-fn-suppr-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-5.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-5.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-0.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-0.txt",
  },
  { // Just like the previous test, but loc info is emitted.
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "--no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-0-1.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-0-1.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "data/test-diff-suppr/test6-fn-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-1.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "data/test-diff-suppr/test6-fn-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-2.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "data/test-diff-suppr/test6-fn-suppr-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-3.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-0.txt",
    "output/test-diff-suppr/test7-var-suppr-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-1.txt",
    "output/test-diff-suppr/test7-var-suppr-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-2.txt",
    "output/test-diff-suppr/test7-var-suppr-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-3.txt",
    "output/test-diff-suppr/test7-var-suppr-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-4.txt",
    "output/test-diff-suppr/test7-var-suppr-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-5.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-5.txt",
    "output/test-diff-suppr/test7-var-suppr-report-5.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-6.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-6.txt",
    "output/test-diff-suppr/test7-var-suppr-report-6.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-7.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-7.txt",
    "output/test-diff-suppr/test7-var-suppr-report-7.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "data/test-diff-suppr/test7-var-suppr-8.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-8.txt",
    "output/test-diff-suppr/test7-var-suppr-report-8.txt"
  },
  {
    "data/test-diff-suppr/libtest8-redundant-fn-v0.so",
    "data/test-diff-suppr/libtest8-redundant-fn-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test8-redundant-fn-report-0.txt",
    "output/test-diff-suppr/test8-redundant-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest8-redundant-fn-v0.so",
    "data/test-diff-suppr/libtest8-redundant-fn-v1.so",
    "",
    "--no-show-locs --redundant",
    "data/test-diff-suppr/test8-redundant-fn-report-1.txt",
    "output/test-diff-suppr/test8-redundant-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest9-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest9-changed-parm-c-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test9-changed-parm-c-report-0.txt",
    "output/test-diff-suppr/est9-changed-parm-c-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest9-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest9-changed-parm-c-v1.so",
    "",
    "--no-show-locs --redundant",
    "data/test-diff-suppr/test9-changed-parm-c-report-1.txt",
    "output/test-diff-suppr/est9-changed-parm-c-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest10-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest10-changed-parm-c-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test10-changed-parm-c-report-0.txt",
    "output/test-diff-suppr/test10-changed-parm-c-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-0.txt",
    "output/test-diff-suppr/test11-add-data-member-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "data/test-diff-suppr/test11-add-data-member-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "data/test-diff-suppr/test11-add-data-member-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "data/test-diff-suppr/test11-add-data-member-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "data/test-diff-suppr/test11-add-data-member-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "data/test-diff-suppr/test11-add-data-member-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-0.txt",
    "output/test-diff-suppr/test12-add-data-member-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "data/test-diff-suppr/test12-add-data-member-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-1.txt",
    "output/test-diff-suppr/test12-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "data/test-diff-suppr/test12-add-data-member-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-2.txt",
    "output/test-diff-suppr/test12-add-data-member-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v0.so",
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test13-suppr-through-pointer-report-0.txt",
    "output/test-diff-suppr/test13-suppr-through-pointer-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v0.so",
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v1.so",
    "data/test-diff-suppr/test13-suppr-through-pointer-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test13-suppr-through-pointer-report-1.txt",
    "output/test-diff-suppr/test13-suppr-through-pointer-report-1.txt"
  },
  {
    "data/test-diff-suppr/test14-suppr-non-redundant-v0.o",
    "data/test-diff-suppr/test14-suppr-non-redundant-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test14-suppr-non-redundant-report-0.txt",
    "output/test-diff-suppr/test14-suppr-non-redundant-report-0.txt"
  },
  {
    "data/test-diff-suppr/test14-suppr-non-redundant-v0.o",
    "data/test-diff-suppr/test14-suppr-non-redundant-v1.o",
    "data/test-diff-suppr/test14-suppr-non-redundant-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test14-suppr-non-redundant-report-1.txt",
    "output/test-diff-suppr/test14-suppr-non-redundant-report-1.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-0.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "data/test-diff-suppr/test15-suppr-added-fn-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-1.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "data/test-diff-suppr/test15-suppr-added-fn-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-2.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-2.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "data/test-diff-suppr/test15-suppr-added-fn-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-3.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-3.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "data/test-diff-suppr/test15-suppr-added-fn-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-4.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-4.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "data/test-diff-suppr/test15-suppr-added-fn-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-5.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-5.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-0.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-1.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-2.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-2.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-3.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-3.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-4.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-4.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-0.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-0.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "data/test-diff-suppr/test17-suppr-added-var-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-1.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-1.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "data/test-diff-suppr/test17-suppr-added-var-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-2.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-2.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "data/test-diff-suppr/test17-suppr-added-var-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-3.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-3.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "data/test-diff-suppr/test17-suppr-added-var-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-4.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-4.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "data/test-diff-suppr/test17-suppr-added-var-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-5.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-5.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-0.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-0.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "data/test-diff-suppr/test18-suppr-removed-var-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-1.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-1.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "data/test-diff-suppr/test18-suppr-removed-var-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-2.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-2.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "data/test-diff-suppr/test18-suppr-removed-var-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-3.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-3.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "data/test-diff-suppr/test18-suppr-removed-var-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-4.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-4.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "data/test-diff-suppr/test18-suppr-removed-var-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-5.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-5.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-0.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-1.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-2.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-3.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-4.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-5.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-5.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-0.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-1.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-2.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-3.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-4.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-5.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-0.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-1.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-2.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-3.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-3.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-4.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-0.txt",
    "output/test-diff-suppr/test23-alias-filter-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "data/test-diff-suppr/test23-alias-filter-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-1.txt",
    "output/test-diff-suppr/test23-alias-filter-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "data/test-diff-suppr/test23-alias-filter-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-2.txt",
    "output/test-diff-suppr/test23-alias-filter-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "data/test-diff-suppr/test23-alias-filter-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-3.txt",
    "output/test-diff-suppr/test23-alias-filter-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "data/test-diff-suppr/test23-alias-filter-4.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-5.txt",
    "output/test-diff-suppr/test23-alias-filter-report-5.txt"
  },
{
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "data/test-diff-suppr/test24-soname-suppr-0.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-0.txt",
    "output/test-diff-suppr/test24-soname-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "data/test-diff-suppr/test24-soname-suppr-1.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-1.txt",
    "output/test-diff-suppr/test24-soname-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "data/test-diff-suppr/test24-soname-suppr-2.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-2.txt",
    "output/test-diff-suppr/test24-soname-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "data/test-diff-suppr/test24-soname-suppr-3.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-3.txt",
    "output/test-diff-suppr/test24-soname-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "data/test-diff-suppr/test24-soname-suppr-4.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-4.txt",
    "output/test-diff-suppr/test24-soname-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest25-typedef-v0.so",
    "data/test-diff-suppr/libtest25-typedef-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test25-typedef-report-0.txt",
    "output/test-diff-suppr/test25-typedef-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest25-typedef-v0.so",
    "data/test-diff-suppr/libtest25-typedef-v1.so",
    "data/test-diff-suppr/test25-typedef-suppr-0.txt",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test25-typedef-report-1.txt",
    "output/test-diff-suppr/test25-typedef-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-0.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "data/test-diff-suppr/test26-loc-suppr-0.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-1.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "data/test-diff-suppr/test26-loc-suppr-1.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-2.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "data/test-diff-suppr/test26-loc-suppr-2.suppr",
    "--no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-3.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-3.txt"
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL, NULL, NULL}
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
    in_suppression_path, bidiff_options, bidiff, cmd,
    ref_diff_report_path, out_diff_report_path;

    for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
      {
	in_elfv0_path = string(get_src_dir()) + "/tests/" + s->in_elfv0_path;
	in_elfv1_path = string(get_src_dir()) + "/tests/" + s->in_elfv1_path;
	if (s->in_suppr_path && strcmp(s->in_suppr_path, ""))
	  in_suppression_path =
	    string(get_src_dir()) + "/tests/" + s->in_suppr_path;
	else
	  in_suppression_path.clear();

	bidiff_options = s->bidiff_options;
	ref_diff_report_path =
	  string(get_src_dir()) + "/tests/" + s->in_report_path;
	out_diff_report_path =
	  string(get_build_dir()) + "/tests/" + s->out_report_path;

	if (!ensure_parent_dir_created(out_diff_report_path))
	  {
	    cerr << "could not create parent directory for "
		 << out_diff_report_path;
	    is_ok = false;
	    continue;
	  }

	bidiff = string(get_build_dir()) + "/tools/abidiff";
	bidiff += " " + bidiff_options;

	if (!in_suppression_path.empty())
	  bidiff += " --suppressions " + in_suppression_path;

	cmd = bidiff + " " + in_elfv0_path + " " + in_elfv1_path;
	cmd += " > " + out_diff_report_path;

	bool bidiff_ok = true;
	int code = system(cmd.c_str());
	if (!WIFEXITED(code))
	  bidiff_ok = false;
	else
	  {
	    abigail::tools_utils::abidiff_status status =
	      static_cast<abidiff_status>(WEXITSTATUS(code));
	    if (abigail::tools_utils::abidiff_status_has_error(status))
	      bidiff_ok = false;
	  }

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
