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
//
// Author: Dodji Seketeli

/// @file

#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include "abg-config.h"
#include "abg-comp-filter.h"
#include "abg-suppression.h"
#include "abg-tools-utils.h"
#include "abg-reader.h"
#include "abg-dwarf-reader.h"

using std::vector;
using std::string;
using std::ostream;
using std::cout;
using std::cerr;
using std::tr1::shared_ptr;
using abigail::ir::environment;
using abigail::ir::environment_sptr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::corpus_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::corpus_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using namespace abigail::dwarf_reader;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::check_file;
using abigail::tools_utils::guess_file_type;
using abigail::tools_utils::gen_suppr_spec_from_headers;
using abigail::tools_utils::load_default_system_suppressions;
using abigail::tools_utils::load_default_user_suppressions;
using abigail::tools_utils::abidiff_status;

struct options
{
  bool display_usage;
  bool display_version;
  bool missing_operand;
  string		wrong_option;
  string		file1;
  string		file2;
  vector<string>	suppression_paths;
  vector<string>	drop_fn_regex_patterns;
  vector<string>	drop_var_regex_patterns;
  vector<string>	keep_fn_regex_patterns;
  vector<string>	keep_var_regex_patterns;
  string		headers_dir1;
  string		headers_dir2;
  bool			drop_private_types;
  bool			no_default_supprs;
  bool			no_arch;
  bool			show_stats_only;
  bool			show_symtabs;
  bool			show_deleted_fns;
  bool			show_changed_fns;
  bool			show_added_fns;
  bool			show_all_fns;
  bool			show_deleted_vars;
  bool			show_changed_vars;
  bool			show_added_vars;
  bool			show_all_vars;
  bool			show_linkage_names;
  bool			show_locs;
  bool			show_harmful_changes;
  bool			show_harmless_changes;
  bool			show_redundant_changes;
  bool			show_symbols_not_referenced_by_debug_info;
  bool			dump_diff_tree;
  bool			show_stats;
  bool			do_log;
  shared_ptr<char>	di_root_path1;
  shared_ptr<char>	di_root_path2;

  options()
    : display_usage(),
      display_version(),
      missing_operand(),
      drop_private_types(true),
      no_default_supprs(),
      no_arch(),
      show_stats_only(),
      show_symtabs(),
      show_deleted_fns(),
      show_changed_fns(),
      show_added_fns(),
      show_all_fns(true),
      show_deleted_vars(),
      show_changed_vars(),
      show_added_vars(),
      show_all_vars(true),
      show_linkage_names(true),
      show_locs(true),
      show_harmful_changes(true),
      show_harmless_changes(),
      show_redundant_changes(),
      show_symbols_not_referenced_by_debug_info(true),
      dump_diff_tree(),
      show_stats(),
      do_log()
  {}
};//end struct options;

static void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << " [options] [<file1> <file2>]\n"
    << " where options can be:\n"
    << " --help|-h  display this message\n "
    << " --version|-v  display program version information and exit\n"
    << " --debug-info-dir1|--d1 <path> the root for the debug info of file1\n"
    << " --debug-info-dir2|--d2 <path> the root for the debug info of file2\n"
    << " --headers-dir1|--hd1 <path>  the path to headers of file1\n"
    << " --headers-dir2|--hd2 <path>  the path to headers of file2\n"
    << "  --dont-drop-private-types\n  keep private types in "
    "internal representation\n"
    << " --stat  only display the diff stats\n"
    << " --symtabs  only display the symbol tables of the corpora\n"
    << " --no-default-suppression  don't load any "
       "default suppression specification\n"
    << " --no-architecture  do not take architecture in account\n"
    << " --deleted-fns  display deleted public functions\n"
    << " --changed-fns  display changed public functions\n"
    << " --added-fns  display added public functions\n"
    << " --deleted-vars  display deleted global public variables\n"
    << " --changed-vars  display changed global public variables\n"
    << " --added-vars  display added global public variables\n"
    << " --no-linkage-name  do not display linkage names of "
    "added/removed/changed\n"
    << " --no-unreferenced-symbols  do not display changes "
    "about symbols not referenced by debug info\n"
    << " --no-show-locs  do now show location information\n"
    << " --suppressions|--suppr <path> specify a suppression file\n"
    << " --drop <regex>  drop functions and variables matching a regexp\n"
    << " --drop-fn <regex> drop functions matching a regexp\n"
    << " --drop-var <regex> drop variables matching a regexp\n"
    << " --keep <regex>  keep only functions and variables matching a regex\n"
    << " --keep-fn <regex>  keep only functions matching a regex\n"
    << " --keep-var  <regex>  keep only variables matching a regex\n"
    << " --harmless  display the harmless changes\n"
    << " --no-harmful  do not display the harmful changes\n"
    << " --redundant  display redundant changes\n"
    << " --no-redundant  do not display redundant changes "
    "(this is the default)\n"
    << " --dump-diff-tree  emit a debug dump of the internal diff tree to "
    "the error output stream\n"
    <<  " --stats  show statistics about various internal stuff\n"
    << " --verbose show verbose messages about internal stuff\n";
}

/// Parse the command line and set the options accordingly.
///
/// @param argc the number of words on the command line
///
/// @param argv the command line, which is an array of words.
///
/// @param opts the options data structure.  This is set by the
/// function iff it returns true.
///
/// @return true if the command line could be parsed and opts filed,
/// false otherwise.
bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.file1.empty())
	    opts.file1 = argv[i];
	  else if (opts.file2.empty())
	    opts.file2 = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--version")
	       || !strcmp(argv[i], "-v"))
	{
	  opts.display_version = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--debug-info-dir1")
	       || !strcmp(argv[i], "--d1"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.di_root_path1 =
	    abigail::tools_utils::make_path_absolute(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--debug-info-dir2")
	       || !strcmp(argv[i], "--d2"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.di_root_path2 =
	    abigail::tools_utils::make_path_absolute(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--headers-dir1")
	       || !strcmp(argv[i], "--hd1"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.headers_dir1 = argv[j];
	  ++i;
	}
      else if (!strcmp(argv[i], "--headers-dir2")
	       || !strcmp(argv[i], "--hd2"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.headers_dir2 = argv[j];
	  ++i;
	}
      else if (!strcmp(argv[i], "--stat"))
	opts.show_stats_only = true;
      else if (!strcmp(argv[i], "--symtabs"))
	opts.show_symtabs = true;
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.display_usage = true;
	  return true;
	}
      else if (!strcmp(argv[i], "--dont-drop-private-types"))
	opts.drop_private_types = false;
      else if (!strcmp(argv[i], "--no-default-suppression"))
	opts.no_default_supprs = true;
      else if (!strcmp(argv[i], "--no-architecture"))
	opts.no_arch = true;
      else if (!strcmp(argv[i], "--deleted-fns"))
	{
	  opts.show_deleted_fns = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--changed-fns"))
	{
	  opts.show_changed_fns = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--added-fns"))
	{
	  opts.show_added_fns = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--deleted-vars"))
	{
	  opts.show_deleted_vars = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--changed-vars"))
	{
	  opts.show_changed_vars = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--added-vars"))
	{
	  opts.show_added_vars = true;
	  opts.show_all_fns = false;
	  opts.show_all_vars = false;
	}
      else if (!strcmp(argv[i], "--no-linkage-name"))
	opts.show_linkage_names = false;
      else if (!strcmp(argv[i], "--no-unreferenced-symbols"))
	opts.show_symbols_not_referenced_by_debug_info = false;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--suppressions")
	       || !strcmp(argv[i], "--suppr"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--drop"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.drop_fn_regex_patterns.push_back(argv[j]);
	  opts.drop_var_regex_patterns.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--drop-fn"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.drop_fn_regex_patterns.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--drop-var"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.drop_var_regex_patterns.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--keep"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.keep_fn_regex_patterns.push_back(argv[j]);
	  opts.keep_var_regex_patterns.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--keep-fn"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.keep_fn_regex_patterns.push_back(argv[j]);
	}
      else if (!strcmp(argv[i], "--keep-var"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    {
	      opts.missing_operand = true;
	      opts.wrong_option = argv[i];
	      return true;
	    }
	  opts.keep_var_regex_patterns.push_back(argv[j]);
	}
      else if (!strcmp(argv[i], "--harmless"))
	opts.show_harmless_changes = true;
      else if (!strcmp(argv[i], "--no-harmful"))
	opts.show_harmful_changes = false;
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant_changes = true;
      else if (!strcmp(argv[i], "--no-redundant"))
	opts.show_redundant_changes = false;
      else if (!strcmp(argv[i], "--dump-diff-tree"))
	opts.dump_diff_tree = true;
      else if (!strcmp(argv[i], "--stats"))
	opts.show_stats = true;
      else if (!strcmp(argv[i], "--verbose"))
	opts.do_log = true;
      else
	{
	  if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	    opts.wrong_option = argv[i];
	  return false;
	}
    }

  return true;
}

/// Display the function symbol tables for the two corpora.
///
/// @param c1 the first corpus to display the symbol table for.
///
/// @param c2 the second corpus to display the symbol table for.
///
/// @param o the output stream to emit the symbol tables to.
static void
display_symtabs(const corpus_sptr c1, const corpus_sptr c2, ostream& o)
{
  o << "size of the functions symtabs: "
    << c1->get_functions().size()
    << " and "
    << c2->get_functions().size()
    << "\n\n";

  if (c1->get_functions().size())
    o << "First functions symbol table\n\n";
  for (abigail::corpus::functions::const_iterator i =
	 c1->get_functions().begin();
       i != c1->get_functions().end();
       ++i)
    o << (*i)->get_pretty_representation() << std::endl;

  if (c1->get_functions().size() != 0)
    o << "\n";

  if (c2->get_functions().size())
    o << "Second functions symbol table\n\n";
  for (abigail::corpus::functions::const_iterator i =
	 c2->get_functions().begin();
       i != c2->get_functions().end();
       ++i)
    o << (*i)->get_pretty_representation() << std::endl;
}

using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;

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

  return true;
}

/// Update the diff context from the @ref options data structure.
///
/// @param ctxt the diff context to update.
///
/// @param opts the instance of @ref options to consider.
static void
set_diff_context_from_opts(diff_context_sptr ctxt,
			   options& opts)
{
  ctxt->default_output_stream(&cout);
  ctxt->error_output_stream(&cerr);
  ctxt->show_stats_only(opts.show_stats_only);
  ctxt->show_deleted_fns(opts.show_all_fns || opts.show_deleted_fns);
  ctxt->show_changed_fns(opts.show_all_fns || opts.show_changed_fns);
  ctxt->show_added_fns(opts.show_all_fns || opts.show_added_fns);
  ctxt->show_deleted_vars(opts.show_all_vars || opts.show_deleted_vars);
  ctxt->show_changed_vars(opts.show_all_vars || opts.show_changed_vars);
  ctxt->show_added_vars(opts.show_all_vars || opts.show_added_vars);
  ctxt->show_linkage_names(opts.show_linkage_names);
  ctxt->show_locs(opts.show_locs);
  ctxt->show_redundant_changes(opts.show_redundant_changes);
  ctxt->show_symbols_unreferenced_by_debug_info
    (opts.show_symbols_not_referenced_by_debug_info);

  if (!opts.show_harmless_changes)
      ctxt->switch_categories_off
	(abigail::comparison::ACCESS_CHANGE_CATEGORY
	 | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
	 | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
	 | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
	 | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
	 | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
	 | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY);
  if (!opts.show_harmful_changes)
    ctxt->switch_categories_off
      (abigail::comparison::SIZE_OR_OFFSET_CHANGE_CATEGORY
       | abigail::comparison::VIRTUAL_MEMBER_CHANGE_CATEGORY);

  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);
  ctxt->add_suppressions(supprs);

  if (!opts.no_default_supprs && opts.suppression_paths.empty())
    {
      // Load the default system and user suppressions.
      suppressions_type& supprs = ctxt->suppressions();

      load_default_system_suppressions(supprs);
      load_default_user_suppressions(supprs);
    }

  if (!opts.headers_dir1.empty())
    {
      // Generate suppression specification to avoid showing ABI
      // changes on types that are not defined in public headers.
      suppression_sptr suppr =
	gen_suppr_spec_from_headers(opts.headers_dir1);
      if (suppr)
	ctxt->add_suppression(suppr);
    }

    if (!opts.headers_dir2.empty())
    {
      // Generate suppression specification to avoid showing ABI
      // changes on types that are not defined in public headers.
      suppression_sptr suppr =
	gen_suppr_spec_from_headers(opts.headers_dir2);
      if (suppr)
	ctxt->add_suppression(suppr);
    }

  ctxt->dump_diff_tree(opts.dump_diff_tree);
}

/// Set suppression specifications to the @p read_context used to load
/// the ABI corpus from the ELF/DWARF file.
///
/// These suppression specifications are going to be applied to drop
/// some ABI artifacts on the floor (while reading the ELF/DWARF file
/// or the native XML ABI file) and thus minimize the size of the
/// resulting ABI corpus.
///
/// @param read_ctxt the read context to apply the suppression
/// specifications to.  Note that the type of this parameter is
/// generic (class template) because in practise, it can be either an
/// abigail::dwarf_reader::read_context type or an
/// abigail::xml_reader::read_context type.
///
/// @param opts the options where to get the suppression
/// specifications from.
template<class ReadContextType>
static void
set_suppressions(ReadContextType& read_ctxt, const options& opts)
{
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);

  if (opts.drop_private_types)
    {
      if (!opts.headers_dir1.empty())
	{
	  // Generate suppression specification to avoid showing ABI
	  // changes on types that are not defined in public headers.
	  //
	  // As these suppression specifications are applied during the
	  // corpus loading, they are going to be dropped from the
	  // internal representation altogether.
	  suppression_sptr suppr =
	    gen_suppr_spec_from_headers(opts.headers_dir1);
	  if (suppr)
	    supprs.push_back(suppr);
	}

      if (!opts.headers_dir2.empty())
	{
	  // Generate suppression specification to avoid showing ABI
	  // changes on types that are not defined in public headers.
	  //
	  // As these suppression specifications are applied during the
	  // corpus loading, they are going to be dropped from the
	  // internal representation altogether.
	  suppression_sptr suppr =
	    gen_suppr_spec_from_headers(opts.headers_dir2);
	  if (suppr)
	    supprs.push_back(suppr);
	}
    }

    add_read_context_suppressions(read_ctxt, supprs);
}

/// Set the regex patterns describing the functions to drop from the
/// symbol table of a given corpus.
///
/// @param opts the options to the regex patterns from.
///
/// @param c the corpus to set the regex patterns into.
static void
set_corpus_keep_drop_regex_patterns(options& opts, corpus_sptr c)
{
  if (!opts.drop_fn_regex_patterns.empty())
    {
      vector<string>& v = opts.drop_fn_regex_patterns;
      vector<string>& p = c->get_regex_patterns_of_fns_to_suppress();
      p.assign(v.begin(), v.end());
    }

  if (!opts.keep_fn_regex_patterns.empty())
    {
      vector<string>& v = opts.keep_fn_regex_patterns;
      vector<string>& p = c->get_regex_patterns_of_fns_to_keep();
      p.assign(v.begin(), v.end());
    }

  if (!opts.drop_var_regex_patterns.empty())
    {
      vector<string>& v = opts.drop_var_regex_patterns;
      vector<string>& p = c->get_regex_patterns_of_vars_to_suppress();
      p.assign(v.begin(), v.end());
    }

 if (!opts.keep_var_regex_patterns.empty())
    {
      vector<string>& v = opts.keep_var_regex_patterns;
      vector<string>& p = c->get_regex_patterns_of_vars_to_keep();
      p.assign(v.begin(), v.end());
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
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.missing_operand)
    {
      emit_prefix(argv[0], cerr)
	<< "missing operand to option: " << opts.wrong_option <<"\n"
	<< "try the --help option for more information\n";
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_usage)
    {
      display_usage(argv[0], cout);
      return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	      | abigail::tools_utils::ABIDIFF_ERROR);
    }

  if (opts.display_version)
    {
      string major, minor, revision;
      abigail::abigail_get_library_version(major, minor, revision);
      emit_prefix(argv[0], cout)
	<< major << "." << minor << "." << revision << "\n";
      return 0;
    }

  if (!maybe_check_suppression_files(opts))
    return (abigail::tools_utils::ABIDIFF_USAGE_ERROR
	    | abigail::tools_utils::ABIDIFF_ERROR);

  abidiff_status status = abigail::tools_utils::ABIDIFF_OK;
  if (!opts.file1.empty() && !opts.file2.empty())
    {
      if (!check_file(opts.file1, cerr))
	return abigail::tools_utils::ABIDIFF_ERROR;

      if (!check_file(opts.file2, cerr))
	return abigail::tools_utils::ABIDIFF_ERROR;

      abigail::tools_utils::file_type t1_type, t2_type;

      t1_type = guess_file_type(opts.file1);
      if (t1_type == abigail::tools_utils::FILE_TYPE_UNKNOWN)
	{
	  emit_prefix(argv[0], cerr)
	    << "Unknown content type for file " << opts.file1 << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      t2_type = guess_file_type(opts.file2);
      if (t2_type == abigail::tools_utils::FILE_TYPE_UNKNOWN)
	{
	  emit_prefix(argv[0], cerr)
	    << "Unknown content type for file " << opts.file2 << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      environment_sptr env(new environment);
      translation_unit_sptr t1, t2;
      abigail::dwarf_reader::status c1_status =
	abigail::dwarf_reader::STATUS_OK,
	c2_status = abigail::dwarf_reader::STATUS_OK;
      corpus_sptr c1, c2;
      char *di_dir1 = 0, *di_dir2 = 0;
      bool files_suppressed = false;

      diff_context_sptr ctxt(new diff_context);
      set_diff_context_from_opts(ctxt, opts);
      suppressions_type& supprs = ctxt->suppressions();
      files_suppressed = (file_is_suppressed(opts.file1, supprs)
			  || file_is_suppressed(opts.file2, supprs));

      if (files_suppressed)
	// We don't have to compare anything because a user
	// suppression specification file instructs us to avoid
	// loading either one of the input files.
	return abigail::tools_utils::ABIDIFF_OK;

      switch (t1_type)
	{
	case abigail::tools_utils::FILE_TYPE_UNKNOWN:
	  emit_prefix(argv[0], cerr)
	    << "Unknown content type for file " << opts.file1 << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	  break;
	case abigail::tools_utils::FILE_TYPE_NATIVE_BI:
	  t1 = abigail::xml_reader::read_translation_unit_from_file(opts.file1,
								    env.get());
	  break;
	case abigail::tools_utils::FILE_TYPE_ELF:
	case abigail::tools_utils::FILE_TYPE_AR:
	  {
	    di_dir1 = opts.di_root_path1.get();
	    abigail::dwarf_reader::read_context_sptr ctxt =
	      abigail::dwarf_reader::create_read_context(opts.file1,
							 &di_dir1, env.get(),
							 /*read_all_types=*/false);
	    assert(ctxt);

	    abigail::dwarf_reader::set_show_stats(*ctxt, opts.show_stats);
	    set_suppressions(*ctxt, opts);
	    abigail::dwarf_reader::set_do_log(*ctxt, opts.do_log);
	    c1 = abigail::dwarf_reader::read_corpus_from_elf(*ctxt, c1_status);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_XML_CORPUS:
	  {
	    abigail::xml_reader::read_context_sptr ctxt =
	      abigail::xml_reader::create_native_xml_read_context(opts.file1,
								  env.get());
	    assert(ctxt);
	    set_suppressions(*ctxt, opts);
	    c1 = abigail::xml_reader::read_corpus_from_input(*ctxt);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_ZIP_CORPUS:
#ifdef WITH_ZIP_ARCHIVE
	  c1 = abigail::xml_reader::read_corpus_from_file(opts.file1);
#endif //WITH_ZIP_ARCHIVE
          break;
        case abigail::tools_utils::FILE_TYPE_RPM:
        case abigail::tools_utils::FILE_TYPE_SRPM:
        case abigail::tools_utils::FILE_TYPE_DEB:
        case abigail::tools_utils::FILE_TYPE_DIR:
        case abigail::tools_utils::FILE_TYPE_TAR:
          break;
        }

      switch (t2_type)
	{
	case abigail::tools_utils::FILE_TYPE_UNKNOWN:
	  emit_prefix(argv[0], cerr)
	    << "Unknown content type for file " << opts.file2 << "\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	  break;
	case abigail::tools_utils::FILE_TYPE_NATIVE_BI:
	  t2 = abigail::xml_reader::read_translation_unit_from_file(opts.file2,
								    env.get());
	  break;
	case abigail::tools_utils::FILE_TYPE_ELF:
	case abigail::tools_utils::FILE_TYPE_AR:
	  {
	    di_dir2 = opts.di_root_path2.get();
	    abigail::dwarf_reader::read_context_sptr ctxt =
	      abigail::dwarf_reader::create_read_context(opts.file2,
							 &di_dir2, env.get(),
							 /*read_all_types=*/false);
	    assert(ctxt);
	    abigail::dwarf_reader::set_show_stats(*ctxt, opts.show_stats);
	    abigail::dwarf_reader::set_do_log(*ctxt, opts.do_log);
	    set_suppressions(*ctxt, opts);

	    c2 = abigail::dwarf_reader::read_corpus_from_elf(*ctxt, c2_status);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_XML_CORPUS:
	  {
	    abigail::xml_reader::read_context_sptr ctxt =
	      abigail::xml_reader::create_native_xml_read_context(opts.file2,
								  env.get());
	    assert(ctxt);
	    set_suppressions(*ctxt, opts);
	    c2 = abigail::xml_reader::read_corpus_from_input(*ctxt);
	  }
	  break;
	case abigail::tools_utils::FILE_TYPE_ZIP_CORPUS:
#ifdef WITH_ZIP_ARCHIVE
	  c2 = abigail::xml_reader::read_corpus_from_file(opts.file2);
#endif //WITH_ZIP_ARCHIVE
          break;
        case abigail::tools_utils::FILE_TYPE_RPM:
        case abigail::tools_utils::FILE_TYPE_SRPM:
        case abigail::tools_utils::FILE_TYPE_DEB:
        case abigail::tools_utils::FILE_TYPE_DIR:
        case abigail::tools_utils::FILE_TYPE_TAR:
          break;
        }

      if (!t1 && !c1)
	{
	  emit_prefix(argv[0], cerr)
	    << "failed to read input file " << opts.file1 << "\n";
	  if (!(c1_status & abigail::dwarf_reader::STATUS_OK))
	    {
	      if (c1_status
		  & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
		{
		  emit_prefix(argv[0], cerr) << "could not find the debug info";
		  if (di_dir1 == 0)
		    emit_prefix(argv[0], cerr)
		      << " Maybe you should consider using the "
		      "--debug-info-dir1 option to tell me about the "
		      "root directory of the debuginfo? "
		      "(e.g, --debug-info-dir1 /usr/lib/debug)\n";
		  else
		    emit_prefix(argv[0], cerr)
		      << "Maybe the root path to the debug information '"
		      << di_dir1 << "' is wrong?\n";
		}
	      if (c1_status
		  & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
		emit_prefix(argv[0], cerr)
		  << "could not find the ELF symbols in the file '"
		  << opts.file1
		  << "'\n";
	      return abigail::tools_utils::ABIDIFF_ERROR;
	    }
	}

      if (!t2 && !c2)
	{
	  emit_prefix(argv[0],  cerr)
	    << "failed to read input file " << opts.file2 << "\n";
	  if (!(c2_status & abigail::dwarf_reader::STATUS_OK))
	    {
	      if (c2_status
		  & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
		{
		  emit_prefix(argv[0], cerr) << "could not find the debug info";
		  if (di_dir2 == 0)
		    emit_prefix(argv[0], cerr)
		      << " Maybe you should consider using the "
		      "--debug-info-dir1 option to tell me about the "
		      "root directory of the debuginfo? "
		      "(e.g, --debug-info-dir1 /usr/lib/debug)\n";
		  else
		    emit_prefix(argv[0], cerr)
		      << "Maybe the root path to the debug information '"
		      << di_dir2 << "' is wrong?\n";
		}
	      if (c2_status
		  & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
		emit_prefix(argv[0], cerr)
		  << "could not find the ELF symbols in the file '"
		  << opts.file2
		  << "'\n";
	      return abigail::tools_utils::ABIDIFF_ERROR;
	    }
	}

      if (!!c1 != !!c2
	  || !!t1 != !!t2)
	{
	  emit_prefix(argv[0], cerr)
	    << "the two input should be of the same kind\n";
	  return abigail::tools_utils::ABIDIFF_ERROR;
	}

      if (opts.no_arch)
	{
	  if (c1)
	    c1->set_architecture_name("");
	  if (c2)
	    c2->set_architecture_name("");
	}

      if (t1)
	{
	  translation_unit_diff_sptr diff = compute_diff(t1, t2);
	  if (diff->has_changes())
	    diff->report(cout);
	}
      else if (c1)
	{
	  if (opts.show_symtabs)
	    {
	      display_symtabs(c1, c2, cout);
	      return abigail::tools_utils::ABIDIFF_OK;
	    }

	  set_corpus_keep_drop_regex_patterns(opts, c1);
	  set_corpus_keep_drop_regex_patterns(opts, c2);

	  corpus_diff_sptr diff = compute_diff(c1, c2, ctxt);

	  if (diff->has_net_changes())
	    status = abigail::tools_utils::ABIDIFF_ABI_CHANGE;

	  if (diff->has_incompatible_changes())
	    status |= abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE;

	  if (diff->has_changes())
	    diff->report(cout);
	}
      else
	status = abigail::tools_utils::ABIDIFF_ERROR;
    }

  return status;
}

#ifdef __ABIGAIL_IN_THE_DEBUGGER__

/// Emit a textual representation of a given @ref corpus_diff tree to
/// stdout.
///
/// This is useful when debugging this program.
///
/// @param diff_tree the diff tree to emit a textual representation
/// for.
void
print_diff_tree(abigail::comparison::corpus_diff* diff_tree)
{
  print_diff_tree(diff_tree, std::cout);
}

/// Emit a textual representation of a given @ref corpus_diff tree to
/// stdout.
///
/// This is useful when debugging this program.
///
/// @param diff_tree the diff tree to emit a textual representation
/// for.
void
print_diff_tree(abigail::comparison::corpus_diff_sptr diff_tree)
{
  print_diff_tree(diff_tree, std::cout);
}

/// Emit a textual representation of a given @ref corpus_diff tree to
/// stdout.
///
/// This is useful when debugging this program.
///
/// @param diff_tree the diff tree to emit a textual representation
/// for.
void
print_diff_tree(abigail::comparison::diff_sptr diff_tree)
{
  print_diff_tree(diff_tree.get(), std::cout);
}

/// Emit a textual representation of a given @ref diff tree to
/// stdout.
///
/// This is useful when debugging this program.
///
/// @param diff_tree the diff tree to emit a textual representation
/// for.
void
print_diff_tree(abigail::comparison::diff* diff_tree)
{
  print_diff_tree(diff_tree, std::cout);
}
#endif // __ABIGAIL_IN_THE_DEBUGGER__
