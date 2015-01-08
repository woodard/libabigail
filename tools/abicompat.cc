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
//
// Author: Dodji Seketeli

/// @file
///
/// This program reads a program A, one library L in version V which A
/// links against, and the same library L in a different version, V+P.
/// The program then checks that the A is still ABI compatible with
/// L in version V+P.

#include <unistd.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <tr1/memory>
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-comparison.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using std::vector;
using std::tr1::shared_ptr;

struct options
{
  string		unknow_option;
  string		app_path;
  string		lib1_path;
  string		lib2_path;
  shared_ptr<char>	app_di_root_path;
  shared_ptr<char>	lib1_di_root_path;
  shared_ptr<char>	lib2_di_root_path;
  vector<string>	suppression_paths;
  bool			display_help;
  bool			list_undefined_symbols_only;
  bool			show_base_names;
  bool			show_redundant;

  options()
    :display_help(),
     list_undefined_symbols_only(),
     show_base_names(),
     show_redundant(true)
  {}
}; // end struct options

static void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name
      << " [options] [application-path] [path-lib-version-1 path-lib-version-2]"
      << "\n"
      << " where options can be: \n"
      << "  --help|-h  display this help message\n"
      << "  --list-undefined-symbols|-u  display the list of "
         "undefined symbols of the application\n"
      << "  --show-base-names|b  in the report, only show the base names "
         " of the files; not the full paths\n"
      << "  --app-debug-info-dir <path-to-app-debug-info>  set the path "
         "to the debug information directory for the application\n"
      << "  --lib-debug-info-dir1 <path-to-lib-debug-info1>  set the path "
         "to the debug information directory for the first library\n"
      << "  --lib-debug-info-dir2 <path-to-lib-debug-info2>  set the path "
         "to the debug information directory for the second library\n"
      <<  "--suppressions <path> specify a suppression file\n"
      << "--no-redundant  do not display redundant changes\n"
      << "--redundant  display redundant changes (this is the default)\n"
    ;
}

static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.app_path.empty())
	    opts.app_path = argv[i];
	  else if (opts.lib1_path.empty())
	    opts.lib1_path = argv[i];
	  else if (opts.lib2_path.empty())
	    opts.lib2_path = argv[i];
	  else
	    return false;
	}
      else if (!strcmp(argv[i], "--list-undefined-symbols")
	       || !strcmp(argv[i], "-u"))
	opts.list_undefined_symbols_only = true;
      else if (!strcmp(argv[i], "--show-base-names")
	       || !strcmp(argv[i], "-b"))
	opts.show_base_names = true;
      else if (!strcmp(argv[i], "--app-debug-info-dir"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.app_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--lib-debug-info-dir1"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.lib1_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--lib-debug-info-dir2"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.lib2_di_root_path =
	    abigail::tools_utils::make_path_absolute(argv[i + 1]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--suppressions"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.suppression_paths.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--redundant"))
	opts.show_redundant = true;
      else if (!strcmp(argv[i], "--no-redundant"))
	opts.show_redundant = false;
      else if (!strcmp(argv[i], "--help")
	       || !strcmp(argv[i], "-h"))
	{
	  opts.display_help = true;
	  return true;
	}
      else
	{
	  opts.unknow_option = argv[i];
	  return false;
	}
    }

  if (!opts.list_undefined_symbols_only)
    {
      if (opts.app_path.empty()
	  || opts.lib1_path.empty()
	  || opts.lib2_path.empty())
	return false;
    }

  return true;
}

int
main(int argc, char* argv[])
{
  options opts;

  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.unknow_option.empty())
	{
	  cerr << "unrecognized option: " << opts.unknow_option << "\n"
	       << "try the --help option for more information\n";
	  return false;
	}

      cerr << "wrong invocation\n"
	   << "try the --help option for more information\n";
      return false;
    }

  if (opts.display_help)
    {
      display_usage(argv[0], cout);
      return true;
    }

  assert(!opts.app_path.empty());
  if (!abigail::tools_utils::check_file(opts.app_path, cerr))
    return 1;

  abigail::tools_utils::file_type type =
    abigail::tools_utils::guess_file_type(opts.app_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF)
    {
      cerr << opts.app_path << " is not an ELF file\n";
      return 1;
    }

  using abigail::tools_utils::check_file;
  using abigail::tools_utils::base_name;
  using abigail::corpus;
  using abigail::corpus_sptr;
  using abigail::ir::elf_symbols;
  using abigail::ir::demangle_cplus_mangled_name;
  using abigail::dwarf_reader::status;
  using abigail::dwarf_reader::read_corpus_from_elf;
  using abigail::comparison::diff_context_sptr;
  using abigail::comparison::diff_context;
  using abigail::comparison::corpus_diff;
  using abigail::comparison::corpus_diff_sptr;
  using abigail::comparison::compute_diff;
  using abigail::comparison::suppression_sptr;
  using abigail::comparison::suppressions_type;
  using abigail::comparison::read_suppressions;

  // Read the application ELF file.
  corpus_sptr app_corpus;
  char * app_di_root = opts.app_di_root_path.get();
  status status = read_corpus_from_elf(opts.app_path,
				       &app_di_root,
				       app_corpus);

  if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
    {
      cerr << "could not read symbols from " << opts.app_path << "\n";
      return 1;
    }
  if (!(status & abigail::dwarf_reader::STATUS_OK))
    {
      cerr << "could not read file " << opts.app_path << "\n";
      return 1;
    }

  if (opts.list_undefined_symbols_only)
    {
      for (elf_symbols::const_iterator i =
	     app_corpus->get_sorted_undefined_fun_symbols().begin();
	   i != app_corpus->get_sorted_undefined_fun_symbols().end();
	   ++i)
	{
	  string id = (*i)->get_id_string();
	  string sym_name = (*i)->get_name();
	  string demangled_name = demangle_cplus_mangled_name(sym_name);
	  if (demangled_name != sym_name)
	    cout << demangled_name << "  {" << id << "}\n";
	  else
	    cout << id << "\n";
	}
      return 0;
    }

  // Read the first version of the library.
  assert(!opts.lib1_path.empty());
  if (!abigail::tools_utils::check_file(opts.lib1_path, cerr))
    return 1;
  type = abigail::tools_utils::guess_file_type(opts.lib1_path);
  if (type != abigail::tools_utils::FILE_TYPE_ELF)
    {
      cerr << opts.lib1_path << " is not an ELF file\n";
      return 1;
    }

  corpus_sptr lib1_corpus;
  char * lib1_di_root = opts.lib1_di_root_path.get();
  status = read_corpus_from_elf(opts.lib1_path,
				&lib1_di_root,
				lib1_corpus);
  if (status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
    cerr << "could not read debug info for " << opts.lib1_path << "\n";
  if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
    {
      cerr << "could not read symbols from " << opts.lib1_path << "\n";
      return 1;
    }
  if (!(status & abigail::dwarf_reader::STATUS_OK))
    {
      cerr << "could not read file " << opts.lib1_path << "\n";
      return 1;
    }

  // Read the second version of the library.
  assert(!opts.lib2_path.empty());
  corpus_sptr lib2_corpus;
  char * lib2_di_root = opts.lib2_di_root_path.get();
  status = read_corpus_from_elf(opts.lib2_path,
				&lib2_di_root,
				lib2_corpus);
  if (status & abigail::dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND)
    cerr << "could not read debug info for " << opts.lib2_path << "\n";
  if (status & abigail::dwarf_reader::STATUS_NO_SYMBOLS_FOUND)
    {
      cerr << "could not read symbols from " << opts.lib2_path << "\n";
      return 1;
    }
  if (!(status & abigail::dwarf_reader::STATUS_OK))
    {
      cerr << "could not read file " << opts.lib2_path << "\n";
      return 1;
    }

  assert(lib1_corpus);
  assert(lib2_corpus);
  assert(app_corpus);

  // compare lib1 and lib2 only by looking at the functions and
  // variables which symbols are those undefined in the app.

  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_fun_symbols().begin();
       i != app_corpus->get_sorted_undefined_fun_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib1_corpus->get_sym_ids_of_fns_to_keep().push_back(id);
      lib2_corpus->get_sym_ids_of_fns_to_keep().push_back(id);
    }
  for (elf_symbols::const_iterator i =
	 app_corpus->get_sorted_undefined_var_symbols().begin();
       i != app_corpus->get_sorted_undefined_var_symbols().end();
       ++i)
    {
      string id = (*i)->get_id_string();
      lib1_corpus->get_sym_ids_of_vars_to_keep().push_back(id);
      lib2_corpus->get_sym_ids_of_vars_to_keep().push_back(id);
    }

  diff_context_sptr ctxt(new diff_context());
  ctxt->show_added_fns(false);
  ctxt->show_added_vars(false);
  ctxt->show_added_symbols_unreferenced_by_debug_info(false);
  ctxt->show_linkage_names(true);
  ctxt->show_redundant_changes(opts.show_redundant);
  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY
     | abigail::comparison::NON_VIRT_MEM_FUN_CHANGE_CATEGORY
     | abigail::comparison::STATIC_DATA_MEMBER_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_ENUM_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY);

  // load the suppression specifications
  // before starting to diff the libraries.
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    if (check_file(*i, cerr))
      read_suppressions(*i, supprs);

  if (!supprs.empty())
    ctxt->add_suppressions(supprs);

  // Now really do the diffing.
  corpus_diff_sptr changes = compute_diff(lib1_corpus, lib2_corpus, ctxt);

  const corpus_diff::diff_stats& s =
    changes->apply_filters_and_suppressions_before_reporting();

  if (changes->soname_changed()
      || s.num_func_removed() != 0
      || s.num_vars_removed() != 0
      || s.num_func_syms_removed() != 0
      || s.num_var_syms_removed() != 0
      || s.net_num_func_changed() != 0
      || s.net_num_vars_changed() != 0)
    {
      string app_path = opts.app_path,
	lib1_path = opts.lib1_path,
	lib2_path = opts.lib2_path;

      if (opts.show_base_names)
	{
	  base_name(opts.app_path, app_path);
	  base_name(opts.lib1_path, lib1_path);
	  base_name(opts.lib2_path, lib2_path);
	}

      bool abi_break_for_sure = changes->soname_changed()
	|| s.num_vars_removed()
	|| s.num_func_removed()
	|| s.num_var_syms_removed()
	|| s.num_func_syms_removed();

      cout << "ELF file '" << app_path << "'";
      if (abi_break_for_sure)
	cout << " is not ";
      else
	cout << " might not be ";

      cout << "ABI compatible with '" << lib2_path
	   << "' due to differences with '" << lib1_path
	   << "' below:\n";
      changes->report(cout);
    }
  return 0;
}
