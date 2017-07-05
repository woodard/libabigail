// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2017 Red Hat, Inc.
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
/// This test harness program computes the ABI changes between ELF
/// binaries present inside input packages.  Some of the input
/// packages have debuginfo, some don't.  The resulting ABI change
/// report is then compared with a reference one.
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution, which means they must be
/// referenced in tests/data/Makefile.am by the EXTRA_DIST variable.

// For package configuration macros.
#include "config.h"
#include <sys/wait.h>
#include <cassert>
#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include "abg-workers.h"
#include "test-utils.h"
#include "abg-tools-utils.h"

using std::string;
using std::cerr;
using abigail::tests::get_src_dir;

struct InOutSpec
{
  const char* first_in_package_path;
  const char* second_in_package_path;
  const char* prog_options;
  const char* suppression_path;
  const char* first_in_debug_package_path;
  const char* second_in_debug_package_path;
  const char* first_in_devel_package_path;
  const char* second_in_devel_package_path;
  const char* ref_report_path;
  const char* out_report_path;
};// end struct InOutSpec

static InOutSpec in_out_specs[] =
{
  // dir1 contains a suppr spec - it should be ignored.
  {
    "data/test-diff-pkg/dirpkg-0-dir1",
    "data/test-diff-pkg/dirpkg-0-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-0-report-0.txt",
    "output/test-diff-pkg/dirpkg-0-report-0.txt"
  },
  // dir2 contains a suppr spec - it should be recognized.
  {
    "data/test-diff-pkg/dirpkg-1-dir1",
    "data/test-diff-pkg/dirpkg-1-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-1-report-0.txt",
    "output/test-diff-pkg/dirpkg-1-report-0.txt"
  },
  // dir2 contains a suppr spec but --no-abignore is specified,
  // the file should be ignored.
  {
    "data/test-diff-pkg/dirpkg-1-dir1",
    "data/test-diff-pkg/dirpkg-1-dir2",
    "--no-default-suppression --no-abignore --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-1-report-1.txt",
    "output/test-diff-pkg/dirpkg-1-report-1.txt"
  },
  // dir2 contains several suppr spec files, ".abignore" and
  // "dir.abignore", so the specs should be merged.
  {
    "data/test-diff-pkg/dirpkg-2-dir1",
    "data/test-diff-pkg/dirpkg-2-dir2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-2-report-0.txt",
    "output/test-diff-pkg/dirpkg-2-report-0.txt"
  },
  // dir2 contains a suppr spec file, ".abignore" and
  // an additional suppr file is specified on the command line,
  // so the specs should be merged.
  {
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-show-locs",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-0.txt",
    "output/test-diff-pkg/dirpkg-3-report-0.txt"
  },
  // dir2 contains a suppr spec file, ".abignore", which should
  // be ignored because of the program options  and
  // an additional suppr file is specified on the command line,
  // which should be recognized.
  {
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-show-locs --no-abignore",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-1.txt",
    "output/test-diff-pkg/dirpkg-3-report-1.txt"
  },
  { // Just like the previous tests, but loc info is emitted.
    "data/test-diff-pkg/dirpkg-3-dir1",
    "data/test-diff-pkg/dirpkg-3-dir2",
    "--no-default-suppression --no-abignore",
    "data/test-diff-pkg/dirpkg-3.suppr",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dirpkg-3-report-2.txt",
    "output/test-diff-pkg/dirpkg-3-report-2.txt"
  },
  {
    "data/test-diff-pkg/symlink-dir-test1/dir1/symlinks",
    "data/test-diff-pkg/symlink-dir-test1/dir2/symlinks",
    "--no-default-suppression ",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/symlink-dir-test1-report0.txt ",
    "output/test-diff-pkg/symlink-dir-test1-report0.txt "
  },
#if WITH_TAR
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar",
    "data/test-diff-pkg/tarpkg-0-dir2.tar",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-0.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.ta",
    "data/test-diff-pkg/tarpkg-0-dir2.ta",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-01.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar.gz",
    "data/test-diff-pkg/tarpkg-0-dir2.tar.gz",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-02.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-0-dir1.tar.bz2",
    "data/test-diff-pkg/tarpkg-0-dir2.tar.bz2",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-0-report-0.txt",
    "output/test-diff-pkg/tarpkg-0-report-03.txt"
  },
  {
    "data/test-diff-pkg/tarpkg-1-dir1.tar.gz",
    "data/test-diff-pkg/tarpkg-1-dir2.tar.gz",
    "--no-default-suppression --dso-only",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/tarpkg-1-report-0.txt",
    "output/test-diff-pkg/tarpkg-1-report-0.txt"
  },
#endif //WITH_TAR

#ifdef WITH_RPM
  // Two RPM packages with debuginfo available and have ABI changes
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-0.txt",
    "output/test-diff-pkg/test-rpm-report-0.txt"
  },
  // Two RPM packages with 2nd package debuginfo missing
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-1.txt",
    "output/test-diff-pkg/test-rpm-report-1.txt"
  },

  // Two RPM packages with first package debuginfo missing
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-2.txt",
    "output/test-diff-pkg/test-rpm-report-2.txt"
  },

  // Two RPM packages with missing debuginfo
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-3.txt",
    "output/test-diff-pkg/test-rpm-report-3.txt"
  },

  // Two RPM packages with no ABI change
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "--no-default-suppression --no-show-locs",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-4.txt",
    "output/test-diff-pkg/test-rpm-report-4.txt"
  },
  // Two RPM packages with debuginfo available and we don't want to
  // see added symbols.
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "--no-default-suppression --no-show-locs --no-added-syms",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.104-3.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-rpm-report-5.txt",
    "output/test-diff-pkg/test-rpm-report-5.txt"
  },
  {
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64.rpm",
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-20.el7.ppc64.rpm",
    "--no-default-suppression --no-show-locs --no-added-syms",
    "",
    "data/test-diff-pkg/qemu-kvm-rhev-debuginfo-2.3.0-7.el7.ppc64.rpm",
    "data/test-diff-pkg/qemu-kvm-rhev-debuginfo-2.3.0-20.el7.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64--qemu-img-rhev-2.3.0-20.el7.ppc64-report-0.txt",
    "output/test-diff-pkg/qemu-img-rhev-2.3.0-7.el7.ppc64--qemu-img-rhev-2.3.0-20.el7.ppc64-report-0.txt"
  },
  {
    "data/test-diff-pkg/empty-pkg-libvirt-0.9.11.3-1.el7.ppc64.rpm",
    "data/test-diff-pkg/empty-pkg-libvirt-1.2.17-13.el7_2.2.ppc64.rpm",
    "",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/empty-pkg-report-0.txt",
    "output/test-diff-pkg/empty-pkg-report-0.txt"
  },
  {
    "data/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64.rpm",
    "data/test-diff-pkg/gmp-4.3.1-10.el6.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/gmp-debuginfo-4.3.1-7.el6_2.2.ppc64.rpm",
    "data/test-diff-pkg/gmp-debuginfo-4.3.1-10.el6.ppc64.rpm",
    "",
    "",
    "data/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64--gmp-4.3.1-10.el6.ppc64-report-0.txt",
    "output/test-diff-pkg/gmp-4.3.1-7.el6_2.2.ppc64--gmp-4.3.1-10.el6.ppc64-report-0.txt"
  },
  {
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.3-3.20141204.fc23.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-4.3-3.20141204.fc23.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-0.txt",
    "output/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.3-3.20141204.fc23.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-4.3-3.20141204.fc23.x86_64.rpm",
    "data/test-diff-pkg/tbb-devel-4.1-9.20130314.fc22.x86_64.rpm",
    "data/test-diff-pkg/tbb-devel-4.3-3.20141204.fc23.x86_64.rpm",
    "data/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-1.txt",
    "output/test-diff-pkg/tbb-4.1-9.20130314.fc22.x86_64--tbb-4.3-3.20141204.fc23.x86_64-report-1.txt"
  },
  {
    "data/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64.rpm",
    "data/test-diff-pkg/tbb-2017-9.20170118.fc27.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/tbb-debuginfo-2017-8.20161128.fc26.x86_64.rpm",
    "data/test-diff-pkg/tbb-debuginfo-2017-9.20170118.fc27.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64--tbb-2017-9.20170118.fc27.x86_64.txt",
    "output/test-diff-pkg/tbb-2017-8.20161128.fc26.x86_64--tbb-2017-9.20170118.fc27.x86_64.txt"
  },
  {
    "data/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm",
    "data/test-diff-pkg/libICE-1.0.9-2.el7.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/libICE-debuginfo-1.0.6-1.el6.x86_64.rpm",
    "data/test-diff-pkg/libICE-debuginfo-1.0.9-2.el7.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm--libICE-1.0.9-2.el7.x86_64.rpm-report-0.txt",
    "output/test-diff-pkg/libICE-1.0.6-1.el6.x86_64.rpm--libICE-1.0.9-2.el7.x86_64.rpm-report-0.txt"
  },
  {
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686.rpm",
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.28-8.el7.i686.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/gtk2-debuginfo-2.24.22-5.el7.i686.rpm",
    "data/test-diff-pkg/gtk2-debuginfo-2.24.28-8.el7.i686.rpm",
    "",
    "",
    "data/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686--gtk2-immodule-xim-2.24.28-8.el7.i686-report-0.txt",
    "output/test-diff-pkg/gtk2-immodule-xim-2.24.22-5.el7.i686--gtk2-immodule-xim-2.24.28-8.el7.i686-report-0.txt"
  },
  {
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.80-3.fc12.x86_64.rpm",
    "--no-default-suppression --show-identical-binaries",
    "",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-debuginfo-0.80-3.fc12.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/test-dbus-glib-0.80-3.fc12.x86_64-report-0.txt",
    "output/test-diff-pkg/test-dbus-glib-0.80-3.fc12.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64.rpm",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.armv7hl.rpm",
    "--no-default-suppression",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64--dbus-glib-0.104-3.fc23.armv7hl-report-0.txt",
    "output/test-diff-pkg/dbus-glib-0.104-3.fc23.x86_64--dbus-glib-0.104-3.fc23.armv7hl-report-0.txt"
  },
  {
    "data/test-diff-pkg/nonexistent-0.rpm",
    "data/test-diff-pkg/nonexistent-1.rpm",
    "--no-default-suppression",
    "",
    "",
    "",
    "",
    "",
    "data/test-diff-pkg/test-nonexistent-report-0.txt",
    "output/test-diff-pkg/test-nonexistent-report-0.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-0.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-0.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-devel-0.12.8-1.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-1.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-1.txt"
  },
  {
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-server-0.12.8-1.el7.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/spice-debuginfo-0.12.4-19.el7.x86_64.rpm",
    "data/test-diff-pkg/spice-debuginfo-0.12.8-1.el7.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-2.txt",
    "output/test-diff-pkg/spice-server-0.12.4-19.el7.x86_64-0.12.8-1.el7.x86_64-report-2.txt"
  },
  {
    "data/test-diff-pkg/libcdio-0.94-1.fc26.x86_64.rpm",
    "data/test-diff-pkg/libcdio-0.94-2.fc26.x86_64.rpm",
    "--no-default-suppression --redundant",
    "",
    "data/test-diff-pkg/libcdio-debuginfo-0.94-1.fc26.x86_64.rpm",
    "data/test-diff-pkg/libcdio-debuginfo-0.94-2.fc26.x86_64.rpm",
    "",
    "",
    "data/test-diff-pkg/libcdio-0.94-1.fc26.x86_64--libcdio-0.94-2.fc26.x86_64-report.1.txt",
    "output/test-diff-pkg/libcdio-0.94-1.fc26.x86_64--libcdio-0.94-2.fc26.x86_64-report.1.txt"
  },
#endif //WITH_RPM

#ifdef WITH_DEB
  // Two debian packages.
  {
    "data/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64.deb",
    "data/test-diff-pkg/libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64.deb",
    "--no-default-suppression --no-show-locs --fail-no-dbg",
    "",
    "data/test-diff-pkg/libsigc++-2.0-0c2a-dbgsym_2.4.0-1_amd64.ddeb",
    "data/test-diff-pkg/libsigc++-2.0-0v5-dbgsym_2.4.1-1ubuntu2_amd64.ddeb",
    "",
    "",
    "data/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64--libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64-report-0.txt",
    "output/test-diff-pkg/libsigc++-2.0-0c2a_2.4.0-1_amd64--libsigc++-2.0-0v5_2.4.1-1ubuntu2_amd64-report-0.txt"
  },
#endif // WITH_DEB
  // This should be the last entry.
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/// A task which launches abipkgdiff on the binaries passed to the
/// constructor of the task.  The test also launches gnu diff on the
/// result of abipkdiff to compare it against a reference abipkgdiff
/// result.
struct test_task : public abigail::workers::task
{
  InOutSpec spec;
  bool is_ok;
  string diff_cmd;
  string error_message;

  test_task(const InOutSpec& s)
    : spec(s),
      is_ok(true)
  {}

  /// This virtual function overload actually performs the job of the
  /// task.
  ///
  /// It actually launches abipkgdiff on the binaries passed to the
  /// constructor of the task.  It also launches gnu diff on the
  /// result of the abidiff to compare it against a reference abidiff
  /// result.
  virtual void
  perform()
  {
    using abigail::tests::get_build_dir;
    using abigail::tools_utils::ensure_parent_dir_created;

    string first_in_package_path, second_in_package_path,
      prog_options,
      ref_abi_diff_report_path, out_abi_diff_report_path, cmd, abipkgdiff,
      first_in_debug_package_path, second_in_debug_package_path,
      first_in_devel_package_path, second_in_devel_package_path,
      suppression_path;

    first_in_package_path =
      string(get_src_dir()) + "/tests/" + spec.first_in_package_path;
    second_in_package_path =
      string(get_src_dir()) + "/tests/" + spec.second_in_package_path;

    prog_options = spec.prog_options;

    if (spec.first_in_debug_package_path
	&& strcmp(spec.first_in_debug_package_path, ""))
      first_in_debug_package_path =
	string(get_src_dir()) + "/tests/" + spec.first_in_debug_package_path;
    else
      first_in_debug_package_path.clear();

    if (spec.second_in_debug_package_path
	&& strcmp(spec.second_in_debug_package_path, ""))
      second_in_debug_package_path =
	string(get_src_dir()) + "/tests/" + spec.second_in_debug_package_path;
    else
      second_in_debug_package_path.clear();

    if (spec.first_in_devel_package_path
	&& strcmp(spec.first_in_devel_package_path, ""))
      first_in_devel_package_path =
	string(get_src_dir()) + "/tests/" + spec.first_in_devel_package_path;

    if (spec.second_in_devel_package_path
	&& strcmp(spec.second_in_devel_package_path, ""))
      second_in_devel_package_path =
	string(get_src_dir()) + "/tests/" + spec.second_in_devel_package_path;

    if (spec.suppression_path
	&& strcmp(spec.suppression_path, ""))
      suppression_path =
	string(get_src_dir()) + "/tests/" + spec.suppression_path;
    else
      suppression_path.clear();

    ref_abi_diff_report_path =
      string(get_src_dir()) + "/tests/" + spec.ref_report_path;
    out_abi_diff_report_path =
      string(get_build_dir()) + "/tests/" + spec.out_report_path;

    if (!ensure_parent_dir_created(out_abi_diff_report_path))
      {
	cerr << "could not create parent directory for "
	     << out_abi_diff_report_path;
	is_ok = false;
	return;
      }

    abipkgdiff = string(get_build_dir()) + "/tools/abipkgdiff";

    if (!prog_options.empty())
      abipkgdiff +=  " " + prog_options;

    if (!first_in_debug_package_path.empty())
      abipkgdiff += " --d1 " + first_in_debug_package_path;
    if (!second_in_debug_package_path.empty())
      abipkgdiff += " --d2 " + second_in_debug_package_path;

    if (!first_in_devel_package_path.empty())
      abipkgdiff += " --devel1 " + first_in_devel_package_path;

    if (!second_in_devel_package_path.empty())
      abipkgdiff += " --devel2 " + second_in_devel_package_path;

    if (!suppression_path.empty())
      abipkgdiff += " --suppressions " + suppression_path;

    cmd =
      abipkgdiff + " " + first_in_package_path + " " + second_in_package_path;
    cmd += " > " + out_abi_diff_report_path + " 2>&1";

    bool abipkgdiff_ok = true;
    int code = system(cmd.c_str());
    if (!WIFEXITED(code))
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
}; // end struct test_task

/// A convenience typedef for shared
typedef shared_ptr<test_task> test_task_sptr;

int
main()
{
  using std::vector;
  using std::tr1::dynamic_pointer_cast;
  using abigail::workers::queue;
  using abigail::workers::task;
  using abigail::workers::task_sptr;
  using abigail::workers::get_number_of_threads;

  /// Create a task queue.  The max number of worker threads of the
  /// queue is the number of the concurrent threads supported by the
  /// processor of the machine this code runs on.
  const size_t num_tests = sizeof(in_out_specs) / sizeof (InOutSpec) - 1;
  size_t num_workers = std::min(get_number_of_threads(), num_tests);
  queue task_queue(num_workers);

  bool is_ok = true;

  for (InOutSpec *s = in_out_specs; s->first_in_package_path; ++s)
    {
      test_task_sptr t(new test_task(*s));
      assert(task_queue.schedule_task(t));
    }

  /// Wait for all worker threads to finish their job, and wind down.
  task_queue.wait_for_workers_to_complete();

  // Now walk the results and print whatever error messages need to be
  // printed.

  const vector<task_sptr>& completed_tasks =
    task_queue.get_completed_tasks();
  assert(completed_tasks.size() == num_tests);

  for (vector<task_sptr>::const_iterator ti = completed_tasks.begin();
       ti != completed_tasks.end();
       ++ti)
    {
      test_task_sptr t = dynamic_pointer_cast<test_task>(*ti);
      if (!t->is_ok)
	{
	  is_ok = false;
	  if (!t->diff_cmd.empty())
	    system(t->diff_cmd.c_str());
	  if (!t->error_message.empty())
	    cerr << t->error_message << '\n';
	}
    }

    return !is_ok;
}
