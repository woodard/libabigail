// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program runs abidiff between input files and checks that
/// the exit code of the abidiff is the one we expect.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

/// This is an aggregate that specifies where a test shall get its
/// input from and where it shall write its ouput to.

#include <sys/wait.h>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using abigail::tools_utils::abidiff_status;
using abigail::tests::emit_test_status_and_update_counters;
using abigail::tests::emit_test_summary;

struct InOutSpec
{
  const char*	in_elfv0_path;
  const char*	in_elfv1_path;
  const char*	in_suppr_path;
  const char*   in_elfv0_headers_dirs;
  const char*   in_elfv1_headers_dirs;
  const char*   in_elfv0_debug_dir;
  const char*   in_elfv1_debug_dir;
  const char*	abidiff_options;
  abidiff_status status;
  const char*	in_report_path;
  const char*	out_report_path;
};// end struct InOutSpec;

InOutSpec in_out_specs[] =
{
  {
    "data/test-abidiff-exit/test1-voffset-change-v0.o",
    "data/test-abidiff-exit/test1-voffset-change-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test1-voffset-change-report0.txt",
    "output/test-abidiff-exit/test1-voffset-change-report0.txt"
  },
  {
    "data/test-abidiff-exit/test1-voffset-change-v0.o",
    "data/test-abidiff-exit/test1-voffset-change-v1.o",
    "data/test-abidiff-exit/test1-voffset-change.abignore",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test1-voffset-change-report1.txt",
    "output/test-abidiff-exit/test1-voffset-change-report1.txt"
  },
  {
    "data/test-abidiff-exit/test2-filtered-removed-fns-v0.o",
    "data/test-abidiff-exit/test2-filtered-removed-fns-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test2-filtered-removed-fns-report0.txt",
    "output/test-abidiff-exit/test2-filtered-removed-fns-report0.txt"
  },
  {
    "data/test-abidiff-exit/test2-filtered-removed-fns-v0.o",
    "data/test-abidiff-exit/test2-filtered-removed-fns-v1.o",
    "data/test-abidiff-exit/test2-filtered-removed-fns.abignore",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test2-filtered-removed-fns-report1.txt",
    "output/test-abidiff-exit/test2-filtered-removed-fns-report1.txt"
  },
  {
    "data/test-abidiff-exit/test-loc-v0.bi",
    "data/test-abidiff-exit/test-loc-v1.bi",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-loc-with-locs-report.txt",
    "output/test-abidiff-exit/test-loc-with-locs-report.txt"
  },
  {
    "data/test-abidiff-exit/test-loc-v0.bi",
    "data/test-abidiff-exit/test-loc-v1.bi",
    "",
    "",
    "",
    "",
    "",
    "--no-show-locs",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-loc-without-locs-report.txt",
    "output/test-abidiff-exit/test-loc-without-locs-report.txt"
  },
  {
    "data/test-abidiff-exit/test-no-stray-comma-v0.o",
    "data/test-abidiff-exit/test-no-stray-comma-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-no-stray-comma-report.txt",
    "output/test-abidiff-exit/test-no-stray-comma-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-stats-v0.o",
    "data/test-abidiff-exit/test-leaf-stats-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-show-locs --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-leaf-stats-report.txt",
    "output/test-abidiff-exit/test-leaf-stats-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-more-v0.o",
    "data/test-abidiff-exit/test-leaf-more-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-show-locs --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test-leaf-more-report.txt",
    "output/test-abidiff-exit/test-leaf-more-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-fun-type-v0.o",
    "data/test-abidiff-exit/test-leaf-fun-type-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-show-locs --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-leaf-fun-type-report.txt",
    "output/test-abidiff-exit/test-leaf-fun-type-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-redundant-v0.o",
    "data/test-abidiff-exit/test-leaf-redundant-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-leaf-redundant-report.txt",
    "output/test-abidiff-exit/test-leaf-redundant-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-peeling-v0.o",
    "data/test-abidiff-exit/test-leaf-peeling-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-leaf-peeling-report.txt",
    "output/test-abidiff-exit/test-leaf-peeling-report.txt"
  },
  {
    "data/test-abidiff-exit/test-leaf-cxx-members-v0.o",
    "data/test-abidiff-exit/test-leaf-cxx-members-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test-leaf-cxx-members-report.txt",
    "output/test-abidiff-exit/test-leaf-cxx-members-report.txt"
  },
  {
    "data/test-abidiff-exit/test-member-size-v0.o",
    "data/test-abidiff-exit/test-member-size-v1.o",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-member-size-report0.txt",
    "output/test-abidiff-exit/test-member-size-report0.txt"
  },
  {
    "data/test-abidiff-exit/test-member-size-v0.o",
    "data/test-abidiff-exit/test-member-size-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-member-size-report1.txt",
    "output/test-abidiff-exit/test-member-size-report1.txt"
  },
  {
    "data/test-abidiff-exit/test-decl-struct-v0.o",
    "data/test-abidiff-exit/test-decl-struct-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-decl-struct-report.txt",
    "output/test-abidiff-exit/test-decl-struct-report.txt"
  },
  {
    "data/test-abidiff-exit/test-fun-param-v0.abi",
    "data/test-abidiff-exit/test-fun-param-v1.abi",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-fun-param-report.txt",
    "output/test-abidiff-exit/test-fun-param-report.txt"
  },
  {
    "data/test-abidiff-exit/test-decl-enum-v0.o",
    "data/test-abidiff-exit/test-decl-enum-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-decl-enum-report.txt",
    "output/test-abidiff-exit/test-decl-enum-report.txt"
  },
  {
    "data/test-abidiff-exit/test-decl-enum-v0.o",
    "data/test-abidiff-exit/test-decl-enum-v1.o",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-decl-enum-report-2.txt",
    "output/test-abidiff-exit/test-decl-enum-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-decl-enum-v0.o",
    "data/test-abidiff-exit/test-decl-enum-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-decl-enum-report-3.txt",
    "output/test-abidiff-exit/test-decl-enum-report-3.txt"
  },
  {
    "data/test-abidiff-exit/test-net-change-v0.o",
    "data/test-abidiff-exit/test-net-change-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test-net-change-report0.txt",
    "output/test-abidiff-exit/test-net-change-report0.txt"
  },
  {
    "data/test-abidiff-exit/test-net-change-v0.o",
    "data/test-abidiff-exit/test-net-change-v1.o",
    "data/test-abidiff-exit/test-net-change.abignore",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-net-change-report1.txt",
    "output/test-abidiff-exit/test-net-change-report1.txt"
  },
  {
    "data/test-abidiff-exit/test-net-change-v0.o",
    "data/test-abidiff-exit/test-net-change-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abidiff-exit/test-net-change-report2.txt",
    "output/test-abidiff-exit/test-net-change-report2.txt"
  },
  {
    "data/test-abidiff-exit/test-net-change-v0.o",
    "data/test-abidiff-exit/test-net-change-v1.o",
    "data/test-abidiff-exit/test-net-change.abignore",
    "",
    "",
    "",
    "",
    "--no-default-suppression --no-show-locs --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-net-change-report3.txt",
    "output/test-abidiff-exit/test-net-change-report3.txt"
  },
  {
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-v0.o",
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-v1.o",
    "",
    "data/test-abidiff-exit/test-headers-dirs/headers-a",
    "data/test-abidiff-exit/test-headers-dirs/headers-a",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-report-1.txt",
    "output/test-abidiff-exit/test-headers-dirs/test-headers-dir-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-v0.o",
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-v1.o",
    "",
    "data/test-abidiff-exit/test-headers-dirs/headers-a, "
    "data/test-abidiff-exit/test-headers-dirs/headers-b",
    "data/test-abidiff-exit/test-headers-dirs/headers-a, "
    "data/test-abidiff-exit/test-headers-dirs/headers-b",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-headers-dirs/test-headers-dir-report-2.txt",
    "output/test-abidiff-exit/test-headers-dirs/test-headers-dir-report-2.txt"
  },
  {
    "data/test-abidiff-exit/qualifier-typedef-array-v0.o",
    "data/test-abidiff-exit/qualifier-typedef-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/qualifier-typedef-array-report-0.txt",
    "output/test-abidiff-exit/qualifier-typedef-array-report-0.txt"
  },
  {
    "data/test-abidiff-exit/qualifier-typedef-array-v0.o",
    "data/test-abidiff-exit/qualifier-typedef-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/qualifier-typedef-array-report-1.txt",
    "output/test-abidiff-exit/qualifier-typedef-array-report-1.txt"
  },
  {
    "data/test-abidiff-exit/qualifier-typedef-array-v0.o",
    "data/test-abidiff-exit/qualifier-typedef-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/qualifier-typedef-array-report-2.txt",
    "output/test-abidiff-exit/qualifier-typedef-array-report-2.txt"
  },
  {
    "data/test-abidiff-exit/qualifier-typedef-array-v0.o",
    "data/test-abidiff-exit/qualifier-typedef-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--harmless --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/qualifier-typedef-array-report-3.txt",
    "output/test-abidiff-exit/qualifier-typedef-array-report-3.txt"
  },
  {
    "data/test-abidiff-exit/test-non-leaf-array-v0.o",
    "data/test-abidiff-exit/test-non-leaf-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-non-leaf-array-report.txt",
    "output/test-abidiff-exit/test-non-leaf-array-report.txt"
  },
  {
    "data/test-abidiff-exit/test-crc-v0.abi",
    "data/test-abidiff-exit/test-crc-v1.abi",
    "",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-crc-report.txt",
    "output/test-abidiff-exit/test-crc-report.txt"
  },
  {
    "data/test-abidiff-exit/test-missing-alias.abi",
    "data/test-abidiff-exit/test-missing-alias.abi",
    "data/test-abidiff-exit/test-missing-alias.suppr",
    "",
    "",
    "",
    "",
    "",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-missing-alias-report.txt",
    "output/test-abidiff-exit/test-missing-alias-report.txt"
  },
  {
    "data/test-abidiff-exit/test-PR28316-v0.o",
    "data/test-abidiff-exit/test-PR28316-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-PR28316-report.txt",
    "output/test-abidiff-exit/test-PR28316-report.txt"
  },
  {
    "data/test-abidiff-exit/test-PR29144-v0.o",
    "data/test-abidiff-exit/test-PR29144-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-PR29144-report.txt",
    "output/test-abidiff-exit/test-PR29144-report.txt"
  },
  {
    "data/test-abidiff-exit/test-PR29144-v0.o",
    "data/test-abidiff-exit/test-PR29144-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--leaf-changes-only --no-default-suppression --harmless",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-PR29144-report-2.txt",
    "output/test-abidiff-exit/test-PR29144-report-2.txt"
  },
  {
    "data/test-abidiff-exit/ld-2.28-210.so",
    "data/test-abidiff-exit/ld-2.28-211.so",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-ld-2.28-210.so--ld-2.28-211.so.txt",
    "output/test-abidiff-exit/test-ld-2.28-210.so--ld-2.28-211.so.txt"
  },
  {
    "data/test-abidiff-exit/test-rhbz2114909-v0.o",
    "data/test-abidiff-exit/test-rhbz2114909-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-rhbz2114909-report-1.txt",
    "output/test-abidiff-exit/test-rhbz2114909-report-1.txt"
  },
  {
    "data/test-abidiff-exit/PR30048-test-v0.o",
    "data/test-abidiff-exit/PR30048-test-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/PR30048-test-report-0.txt",
    "output/test-abidiff-exit/PR30048-test-report-0.txt"
  },
  {
    "data/test-abidiff-exit/PR30048-test-2-v0.o",
    "data/test-abidiff-exit/PR30048-test-2-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/PR30048-test-2-report-1.txt",
    "output/test-abidiff-exit/PR30048-test-2-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-array-v0--v1-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v1-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v1.o",
    "data/test-abidiff-exit/test-allow-type-array-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-allow-type-array-v0--v1-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v1-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v2.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-array-v0--v2-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v2-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v2.o",
    "data/test-abidiff-exit/test-allow-type-array-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-array-v0--v2-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v2-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v3.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-array-v0--v3-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v3-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-array-v0.o",
    "data/test-abidiff-exit/test-allow-type-array-v3.o",
    "data/test-abidiff-exit/test-allow-type-array-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-allow-type-array-v0--v3-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-array-v0--v3-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v1-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v1-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v1.o",
    "data/test-abidiff-exit/test-allow-type-region-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-allow-type-region-v0--v1-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v1-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v2.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v2-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v2-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v2.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v2-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v2-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v2.o",
    "data/test-abidiff-exit/test-allow-type-region-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v2-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v2-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v3.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v3-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v3-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v3.o",
    "data/test-abidiff-exit/test-allow-type-region-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v3-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v3-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v4.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v4-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v4-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v4.o",
    "data/test-abidiff-exit/test-allow-type-region-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abidiff-exit/test-allow-type-region-v0--v4-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v4-report-2.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v5.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v5-report-1.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v5-report-1.txt"
  },
  {
    "data/test-abidiff-exit/test-allow-type-region-v0.o",
    "data/test-abidiff-exit/test-allow-type-region-v5.o",
    "data/test-abidiff-exit/test-allow-type-region-suppr.txt",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/test-allow-type-region-v0--v5-report-2.txt",
    "output/test-abidiff-exit/test-allow-type-region-v0--v5-report-2.txt"
  },
  {
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/v0/test1.o",
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/v1/test1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/test1-ada-subrange-report-1.txt",
    "output/test-abidiff-exit/ada-subrange/test1-ada-subrange/test1-ada-subrange-report-1.txt"
  },
  {
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/v0/test1.o",
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/v1/test1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/ada-subrange/test1-ada-subrange/test1-ada-subrange-report-2.txt",
    "output/test-abidiff-exit/ada-subrange/test1-ada-subrange/test1-ada-subrange-report-2.txt"
  },
  {
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/v0/test.o",
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/v1/test.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/test2-ada-subrange-redundant-report-1.txt",
    "output/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/test2-ada-subrange-redundant-report-1.txt"
  },
  {
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/v0/test.o",
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/v1/test.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --leaf-changes-only",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/test2-ada-subrange-redundant-report-2.txt",
    "output/test-abidiff-exit/ada-subrange/test2-ada-subrange-redundant/test2-ada-subrange-redundant-report-2.txt"
  },
  {
    "data/test-abidiff-exit/PR30329/old-image/usr/lib/x86_64-linux-gnu/libsqlite3.so.0.8.6",
    "data/test-abidiff-exit/PR30329/new-image/usr/lib/x86_64-linux-gnu/libsqlite3.so.0.8.6",
    "",
    "",
    "",
    "data/test-abidiff-exit/PR30329/old-image/usr/lib/debug",
    "data/test-abidiff-exit/PR30329/new-image/usr/lib/debug",
    "--no-default-suppression",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/PR30329/PR30329-report-1.txt",
    "output/test-abidiff-exit/PR30329/PR30329-report-1.txt"
  },
#ifdef WITH_BTF
  {
    "data/test-abidiff-exit/btf/test0-v0.o",
    "data/test-abidiff-exit/btf/test0-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --btf",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/btf/test0-report-1.txt",
    "output/test-abidiff-exit/btf/test0-report-1.txt"
  },
  {
    "data/test-abidiff-exit/btf/test0-v0.o",
    "data/test-abidiff-exit/btf/test0-v1.o",
    "",
    "",
    "",
    "",
    "",
    "--no-default-suppression --harmless --btf",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abidiff-exit/btf/test0-report-2.txt",
    "output/test-abidiff-exit/btf/test0-report-2.txt"
  },
#endif
  {0, 0, 0 ,0, 0, 0, 0, 0, abigail::tools_utils::ABIDIFF_OK, 0, 0}
};

/// Prefix the strings in a vector of string.
///
/// @param strings the strings to prefix.
///
/// @param prefix the prefix to use.
static void
do_prefix_strings(std::vector<std::string> &strings,
		  const std::string& prefix)
{
  for (std::vector<std::string>::size_type i = 0; i < strings.size(); ++i)
    strings[i] = prefix + strings[i];
}

int
main()
{
  using std::string;
  using std::vector;
  using std::cerr;
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::split_string;
  using abigail::tools_utils::abidiff_status;

  unsigned int total_count = 0, passed_count = 0, failed_count = 0;

  string in_elfv0_path, in_elfv1_path,
    in_suppression_path, abidiff_options, abidiff, cmd, diff_cmd,
    ref_diff_report_path, out_diff_report_path, in_elfv0_debug_dir,
    in_elfv1_debug_dir;
  vector<string> in_elfv0_headers_dirs, in_elfv1_headers_dirs;
  string source_dir_prefix = string(get_src_dir()) + "/tests/";
  string build_dir_prefix = string(get_build_dir()) + "/tests/";

    for (InOutSpec* s = in_out_specs; s->in_elfv0_path; ++s)
      {
	bool is_ok = true;
	in_elfv0_path = source_dir_prefix + s->in_elfv0_path;
	in_elfv1_path = source_dir_prefix + s->in_elfv1_path;
	in_elfv0_debug_dir = source_dir_prefix + s->in_elfv0_debug_dir;
	in_elfv1_debug_dir = source_dir_prefix + s->in_elfv1_debug_dir;
	in_elfv0_headers_dirs.clear();
	in_elfv1_headers_dirs.clear();
	if (s->in_elfv0_headers_dirs && strcmp(s->in_elfv0_headers_dirs, ""))
	  {
	    split_string(s->in_elfv0_headers_dirs, ",", in_elfv0_headers_dirs);
	    do_prefix_strings(in_elfv0_headers_dirs, source_dir_prefix);
	  }

	if (s->in_elfv1_headers_dirs && strcmp(s->in_elfv1_headers_dirs, ""))
	  {
	    split_string(s->in_elfv1_headers_dirs, ",", in_elfv1_headers_dirs);
	    do_prefix_strings(in_elfv1_headers_dirs, source_dir_prefix);
	  }

	if (s->in_suppr_path && strcmp(s->in_suppr_path, ""))
	  in_suppression_path = source_dir_prefix + s->in_suppr_path;
	else
	  in_suppression_path.clear();

	abidiff_options = s->abidiff_options;
	ref_diff_report_path = source_dir_prefix + s->in_report_path;
	out_diff_report_path = build_dir_prefix + s->out_report_path;

	if (!ensure_parent_dir_created(out_diff_report_path))
	  {
	    cerr << "could not create parent directory for "
		 << out_diff_report_path
		 << "\n";
	    is_ok = false;
	    continue;
	  }

	abidiff = string(get_build_dir()) + "/tools/abidiff";
	if (!abidiff_options.empty())
	  abidiff += " " + abidiff_options;

	if (!in_elfv0_debug_dir.empty())
	  abidiff += " --debug-info-dir1 " + in_elfv0_debug_dir;

	if (!in_elfv1_debug_dir.empty())
	  abidiff += " --debug-info-dir2 " + in_elfv1_debug_dir;

	if (!in_elfv0_headers_dirs.empty())
	  for (vector<string>::const_iterator s = in_elfv0_headers_dirs.begin();
	       s != in_elfv0_headers_dirs.end();
	       ++s)
	    abidiff += " --headers-dir1 " + *s;

	if (!in_elfv1_headers_dirs.empty())
	  for (vector<string>::const_iterator s = in_elfv1_headers_dirs.begin();
	       s != in_elfv1_headers_dirs.end();
	       ++s)
	    abidiff += " --headers-dir2 " + *s;

	if (!in_suppression_path.empty())
	  abidiff += " --suppressions " + in_suppression_path;

	cmd = abidiff + " " + in_elfv0_path + " " + in_elfv1_path;
	cmd += " > " + out_diff_report_path;

	bool abidiff_ok = true;
	int code = system(cmd.c_str());
	if (!WIFEXITED(code))
	  abidiff_ok = false;
	else
	  {
	    abigail::tools_utils::abidiff_status status =
	      static_cast<abidiff_status>(WEXITSTATUS(code));
	    if (status != s->status)
	      {
		cerr << "for command '"
		     << cmd
		     << "', expected abidiff status to be " << s->status
		     << " but instead, got " << status << "\n";
		abidiff_ok = false;
	      }
	  }

	if (abidiff_ok)
	  {
	    diff_cmd = "diff -u " + ref_diff_report_path
	      + " " + out_diff_report_path;
	    if (system(diff_cmd.c_str()))
	      is_ok = false;
	  }
	else
	  is_ok = false;

	emit_test_status_and_update_counters(is_ok,
					     cmd,
					     passed_count,
					     failed_count,
					     total_count);
      }

    emit_test_summary(total_count, passed_count, failed_count);


    return failed_count;
}
