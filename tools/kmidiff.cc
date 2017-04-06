// -*- Mode: C++ -*-
//
// Copyright (C) 2017 Red Hat, Inc.
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
//
// Author: Dodji Seketeli

/// @file
///
/// The source code of the Kernel Module Interface Diff tool.

#include <sys/types.h>
#include <dirent.h>
#include <fts.h>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>

#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-comparison.h"

using std::string;
using std::vector;
using std::ostream;
using std::cout;
using std::cerr;

using namespace abigail::tools_utils;
using namespace abigail::dwarf_reader;
using namespace abigail::ir;

using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::corpus_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::tools_utils::gen_suppr_spec_from_kernel_abi_whitelist;

/// The options of this program.
struct options
{
  bool		display_usage;
  bool		display_version;
  bool		verbose;
  bool		missing_operand;
  string	wrong_option;
  string	kernel_dist_root1;
  string	kernel_dist_root2;
  vector<string> kabi_whitelist_paths;
  vector<string> suppression_paths;
  suppressions_type read_time_supprs;
  suppressions_type diff_time_supprs;

  options()
    : display_usage(),
      display_version(),
      verbose(),
      missing_operand()
  {}
}; // end struct options.

/// Display the usage of the program.
///
/// @param prog_name the name of this program.
///
/// @param out the output stream the usage stream is sent to.
static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] kernel-package1 kernel-package2\n"
    << " where options can be:\n"
    << " --help|-h  display this message\n"
    << " --version|-v  display program version information and exit\n"
    << " --verbose  display verbose messages\n"
    << " --suppressions|--suppr <path>  specify a suppression file\n"
    << " --kmi-whitelist|-w <path>  path to a kernel module interface "
    "whitelist\n";
}

/// Parse the command line of the program.
///
/// @param argc the number of arguments on the command line, including
/// the program name.
///
/// @param argv the arguments on the command line, including the
/// program name.
///
/// @param opts the options resulting from the command line parsing.
///
/// @return true iff the command line parsing went fine.
bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.kernel_dist_root1.empty())
	    opts.kernel_dist_root1 = argv[i];
	  else if (opts.kernel_dist_root2.empty())
	    opts.kernel_dist_root2 = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--verbose"))
	  opts.verbose = true;
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.display_usage = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--kmi-whitelist")
	       || !strcmp(argv[i], "-w"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return false;
	    }
	  opts.kabi_whitelist_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return false;
	    }
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
    }

  return true;
}

/// Check that the suppression specification files supplied are
/// present.  If not, emit an error on stderr.
///
/// @param opts the options instance to use.
///
/// @return true if all suppression specification files are present,
/// false otherwise.
static bool
maybe_check_suppression_files(const options& opts)
{
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    if (!check_file(*i, cerr, "abidiff"))
      return false;

  for (vector<string>::const_iterator i =
	 opts.kabi_whitelist_paths.begin();
       i != opts.kabi_whitelist_paths.end();
       ++i)
    if (!check_file(*i, cerr, "abidiff"))
      return false;

  return true;
}

/// Setup the diff context from the program's options.
///
/// @param ctxt the diff context to consider.
///
/// @param opts the options to set the context.
static void
set_diff_context(diff_context_sptr ctxt, const options& opts)
{
  ctxt->default_output_stream(&cout);
  ctxt->error_output_stream(&cerr);
  ctxt->show_relative_offset_changes(true);
  ctxt->show_redundant_changes(false);
  ctxt->show_locs(true);
  ctxt->show_linkage_names(false);
  ctxt->show_added_fns(false);
  ctxt->show_added_vars(false);
  ctxt->show_added_symbols_unreferenced_by_debug_info
    (false);
  ctxt->show_symbols_unreferenced_by_debug_info
    (true);

  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
     | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
     | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY);

  if (!opts.diff_time_supprs.empty())
    ctxt->add_suppressions(opts.diff_time_supprs);
}

/// Print information about the kernel (and modules) binaries found
/// under a given directory.
///
/// Note that this function actually look for the modules iff the
/// --verbose option was provided.
///
/// @param root the directory to consider.
///
/// @param opts the options to use during the search.
static void
print_kernel_dist_binary_paths_under(const string& root, const options &opts)
{
  string vmlinux;
  vector<string> modules;

  if (opts.verbose)
    if (get_binary_paths_from_kernel_dist(root, vmlinux, modules))
       {
	 cout << "Found kernel binaries under: '" << root << "'\n";
	 if (!vmlinux.empty())
	   cout << "[linux kernel binary]\n"
		<< "        '" << vmlinux << "'\n";
	 if (!modules.empty())
	   {
	     cout << "[linux kernel module binaries]\n";
	     for (vector<string>::const_iterator p = modules.begin();
		  p != modules.end();
		  ++p)
	       cout << "        '" << *p << "' \n";
	   }
	 cout << "\n";
       }
}

int
main(int argc, char* argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      emit_prefix(argv[0], cerr)
	<< "unrecognized option: "
	<< opts.wrong_option << "\n"
	<< "try the --help option for more information\n";
      return 1;
    }

  if (opts.missing_operand)
    {
      emit_prefix(argv[0], cerr)
	<< "missing operand to option: " << opts.wrong_option <<"\n"
	<< "try the --help option for more information\n";
      return 1;
    }

  if (!maybe_check_suppression_files(opts))
    return 1;

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return 1;
    }

  if (opts.display_version)
    {
      string major, minor, revision;
      abigail::abigail_get_library_version(major, minor, revision);
      emit_prefix(argv[0], cout)
	<< major << "." << minor << "." << revision << "\n";
      return 0;
    }

  environment_sptr env(new environment);

  corpus_group_sptr group1, group2;
  if (!opts.kernel_dist_root1.empty())
    {
      group1 =
	build_corpus_group_from_kernel_dist_under(opts.kernel_dist_root1,
						  opts.suppression_paths,
						  opts.kabi_whitelist_paths,
						  opts.read_time_supprs,
						  opts.verbose,
						  env);
      print_kernel_dist_binary_paths_under(opts.kernel_dist_root1, opts);
    }

  if (!opts.kernel_dist_root2.empty())
    {
      group2 =
	build_corpus_group_from_kernel_dist_under(opts.kernel_dist_root2,
						  opts.suppression_paths,
						  opts.kabi_whitelist_paths,
						  opts.read_time_supprs,
						  opts.verbose,
						  env);
      print_kernel_dist_binary_paths_under(opts.kernel_dist_root2, opts);
    }

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  if (group1 && group2)
    {
      diff_context_sptr diff_ctxt(new diff_context);
      set_diff_context(diff_ctxt, opts);

      corpus_diff_sptr diff= compute_diff(group1, group2, diff_ctxt);

      if (diff->has_net_changes())
	status = abigail::tools_utils::ABIDIFF_ABI_CHANGE;

      if (diff->has_incompatible_changes())
	status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

      if (diff->has_changes())
	diff->report(cout);
    }
  else
    status = abigail::tools_utils::ABIDIFF_ERROR;

  return status;
}
