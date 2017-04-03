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

/// If the user specified suppression specification files, this
/// function parses those, set them to the options of the program and
/// set the read context with the suppression specification.
///
/// @param read_ctxt the read context to consider.
///
/// @param opts the options to consider.
static void
set_suppressions(read_context &read_ctxt, options& opts)
{
  if (opts.read_time_supprs.empty())
    {
      for (vector<string>::const_iterator i = opts.suppression_paths.begin();
	   i != opts.suppression_paths.end();
	   ++i)
	read_suppressions(*i, opts.read_time_supprs);

      for (vector<string>::const_iterator i =
	     opts.kabi_whitelist_paths.begin();
	   i != opts.kabi_whitelist_paths.end();
	   ++i)
	gen_suppr_spec_from_kernel_abi_whitelist(*i, opts.read_time_supprs);
    }

  abigail::dwarf_reader::add_read_context_suppressions(read_ctxt,
						       opts.read_time_supprs);
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

/// Test if an FTSENT pointer (resulting from fts_read) represents the
/// vmlinux binary.
///
/// @param entry the FTSENT to consider.
///
/// @return true iff @p entry is for a vmlinux binary.
static bool
is_vmlinux(const FTSENT *entry)
{
  if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return false;

  string fname = entry->fts_name;

  if (fname == "vmlinux")
    {
      string dirname;
      dir_name(entry->fts_path, dirname);
      if (string_ends_with(dirname, "compressed"))
	return false;

      return true;
    }

  return false;
}

/// Test if an FTSENT pointer (resulting from fts_read) represents a a
/// linux kernel module binary.
///
/// @param entry the FTSENT to consider.
///
/// @return true iff @p entry is for a linux kernel module binary.
static bool
is_kernel_module(const FTSENT *entry)
{
    if (entry == NULL
      || (entry->fts_info != FTS_F && entry->fts_info != FTS_SL)
      || entry->fts_info == FTS_ERR
      || entry->fts_info == FTS_NS)
    return false;

  string fname = entry->fts_name;
  if (string_ends_with(fname, ".ko")
      || string_ends_with(fname, ".ko.xz")
      || string_ends_with(fname, ".ko.gz"))
    return true;

  return false;
}

/// Find a vmlinux and its kernel modules in a given directory tree.
///
/// @param from the directory tree to start looking from.
///
/// @param vmlinux_path output parameter.  This is set to the path
/// where the vmlinux binary is found.  This is set iff the returns
/// true.
///
/// @param module_paths output parameter.  This is set to the paths of
/// the linux kernel module binaries.
///
/// @return true iff at least the vmlinux binary was found.
static bool
find_vmlinux_and_module_paths(const string&	from,
			      string		&vmlinux_path,
			      vector<string>	&module_paths)
{
  char* path[] = {const_cast<char*>(from.c_str()), 0};

  FTS *file_hierarchy = fts_open(path, FTS_PHYSICAL|FTS_NOCHDIR|FTS_XDEV, 0);
  if (!file_hierarchy)
    return false;

  bool found_vmlinux = false;
  FTSENT *entry;
  while ((entry = fts_read(file_hierarchy)))
    {
      // Skip descendents of symbolic links.
      if (entry->fts_info == FTS_SL || entry->fts_info == FTS_SLNONE)
	{
	  fts_set(file_hierarchy, entry, FTS_SKIP);
	  continue;
	}

      if (!found_vmlinux && is_vmlinux(entry))
	{
	  vmlinux_path = entry->fts_path;
	  found_vmlinux = true;
	}
      else if (is_kernel_module(entry))
	module_paths.push_back(entry->fts_path);
    }

  fts_close(file_hierarchy);

  return found_vmlinux;
}

/// Get the paths of the vmlinux and kernel module binaries under
/// given directory.
///
/// @param dist_root the directory under which to look for.
///
/// @param vmlinux_path output parameter.  The path of the vmlinux
/// binary that was found.
///
/// @param module_paths output parameter.  The paths of the kernel
/// module binaries that were found.
///
/// @param debug_info_root_path output parameter. If a debug info
/// sub-directory is found under @p dist_root, it's set to this
/// parameter.
///
/// @return true if at least the path to the vmlinux binary was found.
static bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  string&		vmlinux_path,
				  vector<string>&	module_paths,
				  string&		debug_info_root_path)
{
  if (!dir_exists(dist_root))
    return false;

  // For now, we assume either an Enterprise Linux or a Fedora kernel
  // distribution directory.
  //
  // We also take into account split debug info package for these.  In
  // this case, the content split debug info package is installed
  // under the 'dist_root' directory as well, and its content is
  // accessible from <dist_root>/usr/lib/debug directory.

  string kernel_modules_root;
  string debug_info_root;
  if (dir_exists(dist_root + "/lib/modules"))
    {
      dist_root + "/lib/modules";
      debug_info_root = dist_root + "/usr/lib/debug";
    }

  if (dir_is_empty(debug_info_root))
    debug_info_root.clear();

  bool found = false;
  string from = !debug_info_root.empty() ? debug_info_root : dist_root;
  if (find_vmlinux_and_module_paths(from, vmlinux_path, module_paths))
    found = true;

  debug_info_root_path = debug_info_root;

  return found;
}

/// Get the paths of the vmlinux and kernel module binaries under
/// given directory.
///
/// @param dist_root the directory under which to look for.
///
/// @param vmlinux_path output parameter.  The path of the vmlinux
/// binary that was found.
///
/// @param module_paths output parameter.  The paths of the kernel
/// module binaries that were found.
///
/// @return true if at least the path to the vmlinux binary was found.
static bool
get_binary_paths_from_kernel_dist(const string&	dist_root,
				  string&		vmlinux_path,
				  vector<string>&	module_paths)
{
  string debug_info_root_path;
  return get_binary_paths_from_kernel_dist(dist_root,
					   vmlinux_path,
					   module_paths,
					   debug_info_root_path);
}

/// Walk a given directory and build an instance of @ref corpus_group
/// from the vmlinux kernel binary and the linux kernel modules found
/// under that directory and under its sub-directories, recursively.
///
/// The main corpus of the @ref corpus_group is made of the vmlinux
/// binary.  The other corpora are made of the linux kernel binaries.
///
/// @param root the path of the directory under which vmlinux and its
/// kernel modules are to be found.
///
/// @param opts the options to use during the search.
///
/// @param env the environment to create the corpus_group in.
static corpus_group_sptr
build_corpus_group_from_kernel_dist_under(const string&	root,
					  options&		opts,
					  environment_sptr&	env)
{
  corpus_group_sptr result;
  string vmlinux;
  vector<string> modules;
  string debug_info_root_path;

  if (opts.verbose)
    cout << "Analysing kernel dist root '" << root << "' ... " << std::flush;

  bool got_binary_paths =
    get_binary_paths_from_kernel_dist(root, vmlinux, modules,
				      debug_info_root_path);

  if (opts.verbose)
    cout << "DONE\n";

  if (got_binary_paths)
    {
      shared_ptr<char> di_root =
	make_path_absolute(debug_info_root_path.c_str());
      char *di_root_ptr = di_root.get();
      abigail::dwarf_reader::status status = abigail::dwarf_reader::STATUS_OK;
      corpus_group_sptr group;
      if (!vmlinux.empty())
	{
	  read_context_sptr ctxt =
	    create_read_context(vmlinux, &di_root_ptr, env.get(),
				/*read_all_types=*/false,
				/*linux_kernel_mode=*/true);

	  set_suppressions(*ctxt, opts);

	  // If we have been given a whitelist of functions and
	  // variable symbols to look at, then we can avoid loading
	  // and analyzing the ELF symbol table.
	  bool do_ignore_symbol_table = !opts.read_time_supprs.empty();
	  set_ignore_symbol_table(*ctxt, do_ignore_symbol_table);

	  group.reset(new corpus_group(env.get(), root));

	  set_read_context_corpus_group(*ctxt, group);

	  if (opts.verbose)
	    cout << "reading kernel binary '"
		 << vmlinux << "' ... " << std::flush;

	  // Read the vmlinux corpus and add it to the group.
	  read_and_add_corpus_to_group_from_elf(*ctxt, *group, status);

	    if (opts.verbose)
	      cout << " DONE\n";
	}

      if (!group->is_empty())
	{
	  // Now add the corpora of the modules to the corpus group.
	  int total_nb_modules = modules.size();
	  int cur_module_index = 1;
	  for (vector<string>::const_iterator m = modules.begin();
	       m != modules.end();
	       ++m, ++cur_module_index)
	    {
	      if (opts.verbose)
		cout << "reading module '"
		     << *m << "' ("
		     << cur_module_index
		     << "/" << total_nb_modules
		     << ") ... " << std::flush;

	      read_context_sptr module_ctxt =
		create_read_context(*m, &di_root_ptr, env.get(),
				    /*read_all_types=*/false,
				    /*linux_kernel_mode=*/true);

	      // If we have been given a whitelist of functions and
	      // variable symbols to look at, then we can avoid loading
	      // and analyzing the ELF symbol table.
	      bool do_ignore_symbol_table = !opts.read_time_supprs.empty();

	      set_ignore_symbol_table(*module_ctxt, do_ignore_symbol_table);

	      set_suppressions(*module_ctxt, opts);

	      set_read_context_corpus_group(*module_ctxt, group);

	      read_and_add_corpus_to_group_from_elf(*module_ctxt,
						    *group, status);
	      if (opts.verbose)
		cout << " DONE\n";
	    }

	  result = group;
	}
    }

  return result;
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
						  opts, env);
      print_kernel_dist_binary_paths_under(opts.kernel_dist_root1, opts);
    }

  if (!opts.kernel_dist_root2.empty())
    {
      group2 =
	build_corpus_group_from_kernel_dist_under(opts.kernel_dist_root2,
						  opts, env);
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
