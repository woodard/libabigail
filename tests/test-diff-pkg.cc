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

// Author: Sinny Kumari

/// @file
///
/// This test harness program fetch ABI diff between ELF binaries present inside
/// input packages with optional debuginfo packages.
/// Resulting ABI diff report is compared with reference one.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include "test-utils.h"
#include "abg-tools-utils.h"

using std::string;
using std::cerr;

struct InOutSpec
{
  const char* first_in_package_path;
  const char* second_in_package_path;
  const char* first_in_debug_package_path;
  const char* second_in_debug_package_path;
  const char* ref_report_path;
  const char* out_report_path;
};// end struct InOutSpec

static InOutSpec in_out_specs[] =
{
  // Two RPM packages with debuginfo available and have ABI changes
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "data/test-diff-pkg/test-rpm-report-0.txt",
    "output/test-diff-pkg/test-rpm-report-0.txt"
  },
  // Two RPM packages with 2nd package debuginfo missing
  {
  "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
  "",
  "data/test-diff-pkg/test-rpm-report-1.txt",
  "output/test-diff-pkg/test-rpm-report-1.txt"
  },
  // Two RPM packages with first package debuginfo missing
  {
  "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
  "",
  "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
  "data/test-diff-pkg/test-rpm-report-2.txt",
  "output/test-diff-pkg/test-rpm-report-2.txt"
  },
  // Two RPM packages with missing debuginfo
  {
  "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
  "",
  "",
  "data/test-diff-pkg/test-rpm-report-3.txt",
  "output/test-diff-pkg/test-rpm-report-3.txt"
  },
  // Two RPM packages with no ABI change
  {
  "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
  "data/test-diff-pkg/test-rpm-report-4.txt",
  "output/test-diff-pkg/test-rpm-report-4.txt"
  },
  // This should be the last entry.
  {0, 0, 0, 0, 0, 0}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;

  bool is_ok = true;
  string first_in_package_path, second_in_package_path,
    ref_abi_diff_report_path, out_abi_diff_report_path, cmd, abipkgdiff,
    first_in_debug_package_path, second_in_debug_package_path;
  for (InOutSpec *s = in_out_specs; s->first_in_package_path; ++s)
    {
      first_in_package_path =
        get_src_dir() + "/tests/" + s->first_in_package_path;
      second_in_package_path =
        get_src_dir() + "/tests/" + s->second_in_package_path;
      if (s->first_in_debug_package_path
          && strcmp(s->first_in_debug_package_path, ""))
        first_in_debug_package_path =
          get_src_dir() + "/tests/" + s->first_in_debug_package_path;
      else
        first_in_debug_package_path.clear();

      if (s->second_in_debug_package_path
          && strcmp(s->second_in_debug_package_path, ""))
        second_in_debug_package_path =
          get_src_dir() + "/tests/" + s->second_in_debug_package_path;
      else
        second_in_debug_package_path.clear();
      ref_abi_diff_report_path = get_src_dir() + "/tests/" + s->ref_report_path;
      out_abi_diff_report_path =
        get_build_dir() + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_abi_diff_report_path))
        {
          cerr << "could not create parent directory for "
               << out_abi_diff_report_path;
          is_ok = false;
          continue;
        }

      abipkgdiff = get_build_dir() + "/tools/abipkgdiff";
      if (!first_in_debug_package_path.empty())
        abipkgdiff += " --d1 " + first_in_debug_package_path;
      if (!second_in_debug_package_path.empty())
        abipkgdiff += " --d2 " + second_in_debug_package_path;

      cmd =
        abipkgdiff + " " + first_in_package_path + " " + second_in_package_path;
      cmd += " > " + out_abi_diff_report_path;

      bool abipkgdiff_ok = true;
      if (system(cmd.c_str()) & 255)
        abipkgdiff_ok = false;

      if (abipkgdiff_ok)
        {
          cmd = "diff -u " + ref_abi_diff_report_path + " "
            + out_abi_diff_report_path;
          if (system(cmd.c_str()))
            is_ok = false;
        }
      else
        is_ok = false;

    }
    return !is_ok;
}
