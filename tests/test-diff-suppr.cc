// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.
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
  const char* headers_dir1;
  const char* headers_dir2;
  const char* in_suppr_path;
  const char* abidiff_options;
  const char* in_report_path;
  const char* out_report_path;
}; // end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    NULL,
    NULL,
    NULL,
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-0.txt",
    "output/test-diff-suppr/test0-type-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-1.txt",
    "output/test-diff-suppr/test0-type-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-2.txt",
    "output/test-diff-suppr/test0-type-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-3.txt",
    "output/test-diff-suppr/test0-type-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-4.txt",
    "output/test-diff-suppr/test0-type-suppr-report-4.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-5.txt",
    "output/test-diff-suppr/test0-type-suppr-report-5.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-5.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-6.txt",
    "output/test-diff-suppr/test0-type-suppr-report-6.txt",
  },
  {
    "data/test-diff-suppr/test0-type-suppr-v0.o",
    "data/test-diff-suppr/test0-type-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test0-type-suppr-6.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test0-type-suppr-report-7.txt",
    "output/test-diff-suppr/test0-type-suppr-report-7.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-0.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test1-typedef-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-1.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test1-typedef-suppr-v0.o",
    "data/test-diff-suppr/test1-typedef-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test1-typedef-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test1-typedef-suppr-report-2.txt",
    "output/test-diff-suppr/test1-typedef-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/test2-struct-suppr-v0.o",
    "data/test-diff-suppr/test2-struct-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test2-struct-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test2-struct-suppr-report-0.txt",
    "output/test-diff-suppr/test2-struct-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test2-struct-suppr-v0.o",
    "data/test-diff-suppr/test2-struct-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test2-struct-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test2-struct-suppr-report-1.txt",
    "output/test-diff-suppr/test2-struct-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    "",
    "",
    NULL,
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-0.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test3-struct-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-1.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/test3-struct-suppr-v0.o",
    "data/test-diff-suppr/test3-struct-suppr-v1.o",
    "",
    "",
    "data/test-diff-suppr/test3-struct-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test3-struct-suppr-report-2.txt",
    "output/test-diff-suppr/test3-struct-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest4-local-suppr-v0.so",
    "data/test-diff-suppr/libtest4-local-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test4-local-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test4-local-suppr-report-1.txt",
    "output/test-diff-suppr/test4-local-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest4-local-suppr-v0.so",
    "data/test-diff-suppr/libtest4-local-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-suppr/test4-local-suppr-report-0.txt",
    "output/test-diff-suppr/test4-local-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-0.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-0.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test5-fn-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-1.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test5-fn-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-2.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test5-fn-suppr-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-3.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test5-fn-suppr-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-4.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-4.txt",
  },
  {
    "data/test-diff-suppr/libtest5-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest5-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test5-fn-suppr-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test5-fn-suppr-report-5.txt",
    "output/test-diff-suppr/test5-fn-suppr-report-5.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-0.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-0.txt",
  },
  { // Just like the previous test, but loc info is emitted.
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-0-1.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-0-1.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test6-fn-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-1.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-1.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test6-fn-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-2.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-2.txt",
  },
  {
    "data/test-diff-suppr/libtest6-fn-suppr-v0.so",
    "data/test-diff-suppr/libtest6-fn-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test6-fn-suppr-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test6-fn-suppr-report-3.txt",
    "output/test-diff-suppr/test6-fn-suppr-report-3.txt",
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-0.txt",
    "output/test-diff-suppr/test7-var-suppr-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-1.txt",
    "output/test-diff-suppr/test7-var-suppr-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-2.txt",
    "output/test-diff-suppr/test7-var-suppr-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-3.txt",
    "output/test-diff-suppr/test7-var-suppr-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-4.txt",
    "output/test-diff-suppr/test7-var-suppr-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-5.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-5.txt",
    "output/test-diff-suppr/test7-var-suppr-report-5.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-6.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-6.txt",
    "output/test-diff-suppr/test7-var-suppr-report-6.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-7.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-7.txt",
    "output/test-diff-suppr/test7-var-suppr-report-7.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-8.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-8.txt",
    "output/test-diff-suppr/test7-var-suppr-report-8.txt"
  },
  {
    "data/test-diff-suppr/libtest7-var-suppr-v0.so",
    "data/test-diff-suppr/libtest7-var-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test7-var-suppr-9.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test7-var-suppr-report-9.txt",
    "output/test-diff-suppr/test7-var-suppr-report-9.txt"
  },
  {
    "data/test-diff-suppr/libtest8-redundant-fn-v0.so",
    "data/test-diff-suppr/libtest8-redundant-fn-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test8-redundant-fn-report-0.txt",
    "output/test-diff-suppr/test8-redundant-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest8-redundant-fn-v0.so",
    "data/test-diff-suppr/libtest8-redundant-fn-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-suppr/test8-redundant-fn-report-1.txt",
    "output/test-diff-suppr/test8-redundant-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest9-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest9-changed-parm-c-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test9-changed-parm-c-report-0.txt",
    "output/test-diff-suppr/est9-changed-parm-c-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest9-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest9-changed-parm-c-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --redundant",
    "data/test-diff-suppr/test9-changed-parm-c-report-1.txt",
    "output/test-diff-suppr/est9-changed-parm-c-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest10-changed-parm-c-v0.so",
    "data/test-diff-suppr/libtest10-changed-parm-c-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test10-changed-parm-c-report-0.txt",
    "output/test-diff-suppr/test10-changed-parm-c-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-0.txt",
    "output/test-diff-suppr/test11-add-data-member-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test11-add-data-member-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test11-add-data-member-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test11-add-data-member-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test11-add-data-member-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest11-add-data-member-v0.so",
    "data/test-diff-suppr/libtest11-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test11-add-data-member-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test11-add-data-member-report-1.txt",
    "output/test-diff-suppr/test11-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-0.txt",
    "output/test-diff-suppr/test12-add-data-member-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test12-add-data-member-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-1.txt",
    "output/test-diff-suppr/test12-add-data-member-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest12-add-data-member-v0.so",
    "data/test-diff-suppr/libtest12-add-data-member-v1.so",
    "",
    "",
    "data/test-diff-suppr/test12-add-data-member-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test12-add-data-member-report-2.txt",
    "output/test-diff-suppr/test12-add-data-member-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v0.so",
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test13-suppr-through-pointer-report-0.txt",
    "output/test-diff-suppr/test13-suppr-through-pointer-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v0.so",
    "data/test-diff-suppr/libtest13-suppr-through-pointer-v1.so",
    "",
    "",
    "data/test-diff-suppr/test13-suppr-through-pointer-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test13-suppr-through-pointer-report-1.txt",
    "output/test-diff-suppr/test13-suppr-through-pointer-report-1.txt"
  },
  {
    "data/test-diff-suppr/test14-suppr-non-redundant-v0.o",
    "data/test-diff-suppr/test14-suppr-non-redundant-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test14-suppr-non-redundant-report-0.txt",
    "output/test-diff-suppr/test14-suppr-non-redundant-report-0.txt"
  },
  {
    "data/test-diff-suppr/test14-suppr-non-redundant-v0.o",
    "data/test-diff-suppr/test14-suppr-non-redundant-v1.o",
    "",
    "",
    "data/test-diff-suppr/test14-suppr-non-redundant-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test14-suppr-non-redundant-report-1.txt",
    "output/test-diff-suppr/test14-suppr-non-redundant-report-1.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-0.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test15-suppr-added-fn-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-1.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test15-suppr-added-fn-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-2.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-2.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test15-suppr-added-fn-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-3.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-3.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test15-suppr-added-fn-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-4.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-4.txt"
  },
  {
    "data/test-diff-suppr/test15-suppr-added-fn-v0.o",
    "data/test-diff-suppr/test15-suppr-added-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test15-suppr-added-fn-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test15-suppr-added-fn-report-5.txt",
    "output/test-diff-suppr/test15-suppr-added-fn-report-5.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-0.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-0.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test16-suppr-removed-fn-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-1.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-1.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test16-suppr-removed-fn-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-2.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-2.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test16-suppr-removed-fn-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-3.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-3.txt"
  },
  {
    "data/test-diff-suppr/test16-suppr-removed-fn-v0.o",
    "data/test-diff-suppr/test16-suppr-removed-fn-v1.o",
    "",
    "",
    "data/test-diff-suppr/test16-suppr-removed-fn-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test16-suppr-removed-fn-report-4.txt",
    "output/test-diff-suppr/test16-suppr-removed-fn-report-4.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-0.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-0.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test17-suppr-added-var-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-1.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-1.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test17-suppr-added-var-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-2.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-2.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test17-suppr-added-var-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-3.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-3.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test17-suppr-added-var-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-4.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-4.txt"
  },
  {
    "data/test-diff-suppr/test17-suppr-added-var-v0.o",
    "data/test-diff-suppr/test17-suppr-added-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test17-suppr-added-var-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test17-suppr-added-var-report-5.txt",
    "output/test-diff-suppr/test17-suppr-added-var-report-5.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-0.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-0.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test18-suppr-removed-var-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-1.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-1.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test18-suppr-removed-var-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-2.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-2.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test18-suppr-removed-var-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-3.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-3.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test18-suppr-removed-var-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-4.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-4.txt"
  },
  {
    "data/test-diff-suppr/test18-suppr-removed-var-v0.o",
    "data/test-diff-suppr/test18-suppr-removed-var-v1.o",
    "",
    "",
    "data/test-diff-suppr/test18-suppr-removed-var-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test18-suppr-removed-var-report-5.txt",
    "output/test-diff-suppr/test18-suppr-removed-var-report-5.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-0.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-1.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-2.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-3.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-4.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v0.o",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test19-suppr-added-fn-sym-report-5.txt",
    "output/test-diff-suppr/test19-suppr-added-fn-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-0.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-1.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-2.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-3.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v0.o",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-4.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test20-suppr-removed-fn-sym-report-5.txt",
    "output/test-diff-suppr/test20-suppr-removed-fn-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-0.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test21-suppr-added-var-sym-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-1.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test21-suppr-added-var-sym-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-2.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test21-suppr-added-var-sym-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-3.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test21-suppr-added-var-sym-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-4.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/test21-suppr-added-var-sym-v0.o",
    "data/test-diff-suppr/test21-suppr-added-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test21-suppr-added-var-sym-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test21-suppr-added-var-sym-report-5.txt",
    "output/test-diff-suppr/test21-suppr-added-var-sym-report-5.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-0.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-0.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-1.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-1.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-2.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-2.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-3.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-3.txt"
  },
  {
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v0.o",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-v1.o",
    "",
    "",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test22-suppr-removed-var-sym-report-4.txt",
    "output/test-diff-suppr/test22-suppr-removed-var-sym-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-0.txt",
    "output/test-diff-suppr/test23-alias-filter-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "",
    "data/test-diff-suppr/test23-alias-filter-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-1.txt",
    "output/test-diff-suppr/test23-alias-filter-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "",
    "data/test-diff-suppr/test23-alias-filter-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-2.txt",
    "output/test-diff-suppr/test23-alias-filter-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "",
    "data/test-diff-suppr/test23-alias-filter-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-3.txt",
    "output/test-diff-suppr/test23-alias-filter-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest23-alias-filter-v0.so",
    "data/test-diff-suppr/libtest23-alias-filter-v1.so ",
    "",
    "",
    "data/test-diff-suppr/test23-alias-filter-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test23-alias-filter-report-5.txt",
    "output/test-diff-suppr/test23-alias-filter-report-5.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-0.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-0.txt",
    "output/test-diff-suppr/test24-soname-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-1.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-1.txt",
    "output/test-diff-suppr/test24-soname-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-2.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-2.txt",
    "output/test-diff-suppr/test24-soname-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-3.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-3.txt",
    "output/test-diff-suppr/test24-soname-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-4.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-4.txt",
    "output/test-diff-suppr/test24-soname-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-5.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-5.txt",
    "output/test-diff-suppr/test24-soname-report-5.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-6.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-6.txt",
    "output/test-diff-suppr/test24-soname-report-6.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-7.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-7.txt",
    "output/test-diff-suppr/test24-soname-report-7.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-8.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-8.txt",
    "output/test-diff-suppr/test24-soname-report-8.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-9.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-9.txt",
    "output/test-diff-suppr/test24-soname-report-9.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-10.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-10.txt",
    "output/test-diff-suppr/test24-soname-report-10.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-11.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-11.txt",
    "output/test-diff-suppr/test24-soname-report-11.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-12.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-12.txt",
    "output/test-diff-suppr/test24-soname-report-12.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-13.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-13.txt",
    "output/test-diff-suppr/test24-soname-report-13.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-13.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-13.txt",
    "output/test-diff-suppr/test24-soname-report-13.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-14.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-14.txt",
    "output/test-diff-suppr/test24-soname-report-14.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-15.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-15.txt",
    "output/test-diff-suppr/test24-soname-report-15.txt"
  },
  {
    "data/test-diff-suppr/libtest24-soname-v0.so",
    "data/test-diff-suppr/libtest24-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test24-soname-suppr-16.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test24-soname-report-16.txt",
    "output/test-diff-suppr/test24-soname-report-16.txt"
  },
  {
    "data/test-diff-suppr/libtest25-typedef-v0.so",
    "data/test-diff-suppr/libtest25-typedef-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test25-typedef-report-0.txt",
    "output/test-diff-suppr/test25-typedef-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest25-typedef-v0.so",
    "data/test-diff-suppr/libtest25-typedef-v1.so",
    "",
    "",
    "data/test-diff-suppr/test25-typedef-suppr-0.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test25-typedef-report-1.txt",
    "output/test-diff-suppr/test25-typedef-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-0.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test26-loc-suppr-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-1.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test26-loc-suppr-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-2.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest26-loc-suppr-v0.so",
    "data/test-diff-suppr/libtest26-loc-suppr-v1.so",
    "",
    "",
    "data/test-diff-suppr/test26-loc-suppr-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test26-loc-suppr-report-3.txt",
    "output/test-diff-suppr/test26-loc-suppr-report-3.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-0.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-0.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test27-add-aliased-function-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-1.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-1.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test27-add-aliased-function-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-2.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-2.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test27-add-aliased-function-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-3.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-3.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test27-add-aliased-function-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-4.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-4.txt"
  },
  {
    "data/test-diff-suppr/test27-add-aliased-function-v0.o",
    "data/test-diff-suppr/test27-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test27-add-aliased-function-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test27-add-aliased-function-report-5.txt",
    "output/test-diff-suppr/test27-add-aliased-function-report-5.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-0.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-0.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-1.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-1.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-1.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-2.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-2.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-2.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-3.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-3.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-3.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-4.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-4.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-4.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-5.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-5.txt"
  },
  {
    "data/test-diff-suppr/test28-add-aliased-function-v0.o",
    "data/test-diff-suppr/test28-add-aliased-function-v1.o",
    "",
    "",
    "data/test-diff-suppr/test28-add-aliased-function-5.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test28-add-aliased-function-report-6.txt",
    "output/test-diff-suppr/test28-add-aliased-function-report-6.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-0.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-0.txt",
    "output/test-diff-suppr/test29-soname-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-1.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-1.txt",
    "output/test-diff-suppr/test29-soname-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-2.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-2.txt",
    "output/test-diff-suppr/test29-soname-report-2.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-3.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-3.txt",
    "output/test-diff-suppr/test29-soname-report-3.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-4.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-4.txt",
    "output/test-diff-suppr/test29-soname-report-4.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-5.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-5.txt",
    "output/test-diff-suppr/test29-soname-report-5.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-6.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-6.txt",
    "output/test-diff-suppr/test29-soname-report-6.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-7.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-7.txt",
    "output/test-diff-suppr/test29-soname-report-7.txt"
  },
  {
    "data/test-diff-suppr/libtest29-soname-v0.so",
    "data/test-diff-suppr/libtest29-soname-v1.so",
    "",
    "",
    "data/test-diff-suppr/test29-suppr-8.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test29-soname-report-8.txt",
    "output/test-diff-suppr/test29-soname-report-8.txt"
  },
  {
    "data/test-diff-suppr/test30-pub-lib-v0.so",
    "data/test-diff-suppr/test30-pub-lib-v1.so",
    "",
    "",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test30-report-0.txt",
    "output/test-diff-suppr/test30-report-0.txt"
  },
  {
    "data/test-diff-suppr/test30-pub-lib-v0.so",
    "data/test-diff-suppr/test30-pub-lib-v1.so",
    "data/test-diff-suppr/test30-include-dir-v0",
    "data/test-diff-suppr/test30-include-dir-v1",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test30-report-1.txt",
    "output/test-diff-suppr/test30-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest31-v0.so",
    "data/test-diff-suppr/libtest31-v1.so",
    "",
    "",
    "data/test-diff-suppr/libtest31.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test31-report-0.txt",
    "output/test-diff-suppr/test31-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest31-v0.so",
    "data/test-diff-suppr/libtest31-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test31-report-1.txt",
    "output/test-diff-suppr/test31-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest32-v0.so",
    "data/test-diff-suppr/libtest32-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test32-report-0.txt",
    "output/test-diff-suppr/test32-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest32-v0.so",
    "data/test-diff-suppr/libtest32-v1.so",
    "",
    "",
    "data/test-diff-suppr/libtest32-0.suppr",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test32-report-1.txt",
    "output/test-diff-suppr/test32-report-1.txt"
  },
  {
    "data/test-diff-suppr/libtest33-v0.so",
    "data/test-diff-suppr/libtest33-v1.so",
    "",
    "",
    "data/test-diff-suppr/test33-suppr-1.txt",
    "--no-default-suppression --no-show-locs --no-redundant",
    "data/test-diff-suppr/test33-report-0.txt",
    "output/test-diff-suppr/test33-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest34-v0.so",
    "data/test-diff-suppr/libtest34-v1.so",
    "data/test-diff-suppr/test34-pub-include-dir-v0",
    "data/test-diff-suppr/test34-pub-include-dir-v1",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test34-report-0.txt",
    "output/test-diff-suppr/test34-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest35-leaf-v0.so",
    "data/test-diff-suppr/libtest35-leaf-v1.so",
    "",
    "",
    "data/test-diff-suppr/test35-leaf.suppr",
    "--no-default-suppression --leaf-changes-only --impacted-interfaces",
    "data/test-diff-suppr/test35-leaf-report-0.txt",
    "output/test-diff-suppr/test35-leaf-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest36-leaf-v0.so",
    "data/test-diff-suppr/libtest36-leaf-v1.so",
    "",
    "",
    "",
    "--no-default-suppression --leaf-changes-only --impacted-interfaces",
    "data/test-diff-suppr/test36-leaf-report-0.txt",
    "output/test-diff-suppr/test36-leaf-report-0.txt"
  },
  {
    "data/test-diff-suppr/test37-opaque-type-v0.o",
    "data/test-diff-suppr/test37-opaque-type-v1.o",
    "data/test-diff-suppr/test37-opaque-type-header-dir",
    "data/test-diff-suppr/test37-opaque-type-header-dir",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test37-opaque-type-report-0.txt",
    "output/test-diff-suppr/test37-opaque-type-report-0.txt"
  },
  {
   "data/test-diff-suppr/test38-char-class-in-ini-v0.o",
   "data/test-diff-suppr/test38-char-class-in-ini-v1.o",
   "",
   "",
   "data/test-diff-suppr/test38-char-class-in-ini.abignore",
   "--no-default-suppression",
   "data/test-diff-suppr/test38-char-class-in-ini-report-0.txt",
   "output/test-diff-suppr/test38-char-class-in-ini-report-0.txt"
  },
  {
    "data/test-diff-suppr/test39-opaque-type-v0.o",
    "data/test-diff-suppr/test39-opaque-type-v1.o",
    "data/test-diff-suppr/test39-public-headers-dir",
    "data/test-diff-suppr/test39-public-headers-dir",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test39-opaque-type-report-0.txt",
    "output/test-diff-suppr/test39-opaque-type-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest40-enumerator-changes-v0.so",
    "data/test-diff-suppr/libtest40-enumerator-changes-v1.so",
    "",
    "",
    "data/test-diff-suppr/test40-enumerator-changes-0.suppr",
    "--no-default-suppression",
    "data/test-diff-suppr/test40-enumerator-changes-report-0.txt",
    "output/test-diff-suppr/test40-enumerator-changes-report-0.txt"
  },
  {
    "data/test-diff-suppr/libtest41-enumerator-changes-v0.so",
    "data/test-diff-suppr/libtest41-enumerator-changes-v1.so",
    "",
    "",
    "data/test-diff-suppr/test41-enumerator-changes-0.suppr",
    "--no-default-suppression",
    "data/test-diff-suppr/test41-enumerator-changes-report-0.txt",
    "output/test-diff-suppr/test41-enumerator-changes-report-0.txt"
  },
  {
    "data/test-diff-suppr/test42-negative-suppr-type-v0.o",
    "data/test-diff-suppr/test42-negative-suppr-type-v1.o",
    "",
    "",
    "data/test-diff-suppr/test42-negative-suppr-type-suppr-1.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test42-negative-suppr-type-report-0.txt",
    "output/test-diff-suppr/test42-negative-suppr-type-report-0.txt"
  },
  {
    "data/test-diff-suppr/test42-negative-suppr-type-v0.o",
    "data/test-diff-suppr/test42-negative-suppr-type-v1.o",
    "",
    "",
    "data/test-diff-suppr/test42-negative-suppr-type-suppr-2.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test42-negative-suppr-type-report-1.txt",
    "output/test-diff-suppr/test42-negative-suppr-type-report-1.txt"
  },
  {
    "data/test-diff-suppr/test43-suppr-direct-fn-subtype-v0.o",
    "data/test-diff-suppr/test43-suppr-direct-fn-subtype-v1.o",
    "",
    "",
    "data/test-diff-suppr/test43-suppr-direct-fn-subtype-suppr-1.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test43-suppr-direct-fn-subtype-report-1.txt",
    "output/test-diff-suppr/test43-suppr-direct-fn-subtype-report-1.txt"
  },
  {
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v0.o",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v1.o",
    "",
    "",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-1.txt",
    "output/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-1.txt"
  },
  {
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v0.o",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v1.o",
    "",
    "",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp.suppr.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-2.txt",
    "output/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-2.txt"
  },
  {
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v0.o.abi",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v1.o.abi",
    "",
    "",
    "",
    "--no-default-suppression",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-1.txt",
    "output/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-1.txt"
  },
  {
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v0.o.abi",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-v1.o.abi",
    "",
    "",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp.suppr.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-2.txt",
    "output/test-diff-suppr/test44-suppr-sym-name-not-regexp-report-2.txt"
  },
  {
    "data/test-diff-suppr/test45-abi.xml",
    "data/test-diff-suppr/test45-abi-wl.xml",
    "",
    "",
    "data/test-diff-suppr/test45-abi.suppr.txt",
    "--no-default-suppression",
    "data/test-diff-suppr/test45-abi-report-1.txt",
    "output/test-diff-suppr/test45-abi-report-1.txt"
  },
  {
    "data/test-diff-suppr/test46-PR25128-base.xml",
    "data/test-diff-suppr/test46-PR25128-new.xml",
    "",
    "",
    "",
    "--no-default-suppression --leaf-changes-only",
    "data/test-diff-suppr/test46-PR25128-report-1.txt",
    "output/test-diff-suppr/test46-PR25128-report-1.txt"
  },
  // This should be the last entry
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::abidiff_status;

  bool is_ok = true;
  string in_elfv0_path, in_elfv1_path, headers_dir1, headers_dir2,
    in_suppression_path, abidiff_options, abidiff, cmd,
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

	abidiff_options = s->abidiff_options;

	ref_diff_report_path =
	  string(get_src_dir()) + "/tests/" + s->in_report_path;
	out_diff_report_path =
	  string(get_build_dir()) + "/tests/" + s->out_report_path;

	if (s->headers_dir1 && strcmp(s->headers_dir1, ""))
	  headers_dir1 = s->headers_dir1;
	else
	  headers_dir1.clear();

	if (s->headers_dir2 && strcmp(s->headers_dir2, ""))
	  headers_dir2 = s->headers_dir2;
	else
	  headers_dir2.clear();

	if (!ensure_parent_dir_created(out_diff_report_path))
	  {
	    cerr << "could not create parent directory for "
		 << out_diff_report_path;
	    is_ok = false;
	    continue;
	  }

	abidiff = string(get_build_dir()) + "/tools/abidiff";
	abidiff += " " + abidiff_options;

	if (!in_suppression_path.empty())
	  abidiff += " --suppressions " + in_suppression_path;

	if (!headers_dir1.empty())
	  abidiff +=
	    " --hd1 " + string(get_src_dir()) + "/tests/" + headers_dir1;

	if (!headers_dir2.empty())
	  abidiff +=
	    " --hd2 " + string(get_src_dir()) + "/tests/" + headers_dir2;

	cmd = abidiff + " " + in_elfv0_path + " " + in_elfv1_path;
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
