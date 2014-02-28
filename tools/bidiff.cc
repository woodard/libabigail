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
//
// Author: Dodji Seketeli

/// @file

#include <cstring>
#include <string>
#include <iostream>
#include "abg-comparison.h"
#include "abg-tools-utils.h"
#include "abg-reader.h"
#include "abg-dwarf-reader.h"

using std::string;
using std::ostream;
using std::cout;
using std::cerr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::corpus_sptr;
using abigail::comparison::translation_unit_diff_sptr;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::tools::check_file;
using abigail::tools::guess_file_type;

struct options
{
  string file1;
  string file2;
  bool show_stats_only;
  bool show_symtabs;
  bool show_deleted_fns;
  bool show_changed_fns;
  bool show_added_fns;
  bool show_all_fns;
  bool show_deleted_vars;
  bool show_changed_vars;
  bool show_added_vars;
  bool show_all_vars;

  options()
    : show_stats_only(false),
      show_symtabs(false),
      show_deleted_fns(false),
      show_changed_fns(false),
      show_added_fns(false),
      show_all_fns(true),
      show_deleted_vars(false),
      show_changed_vars(false),
      show_added_vars(false),
      show_all_vars(true)
  {}
};//end struct options;

void
display_usage(const string prog_name, ostream& out)
{
  out << "usage: " << prog_name << "[options] [<bi-file1> <bi-file2>]\n"
      << " where options can be:\n"
      << " --stat  only display the diff stats\n"
      << " --symtabs  only display the symbol tables of the corpora\n"
      << " --deleted-fns  display deleted public functions\n"
      << " --changed-fns  display changed public functions\n"
      << " --added-fns  display added public functions\n"
      << " --deleted-vars  display deleted global public variables\n"
      << " --changed-vars  display changed global public variables\n"
      << " --added-vars  display added global public variables\n"
      << " --help  display this message\n";
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
      else if (!strcmp(argv[i], "--stat"))
	opts.show_stats_only = true;
      else if (!strcmp(argv[i], "--symtabs"))
	opts.show_symtabs = true;
      else if (!strcmp(argv[i], "--help"))
	return false;
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
      else
	return false;
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

/// Update the diff context from the @ref options data structure.
///
/// @param ctxt the diff context to update.
///
/// @param opts the instance of @ref options to consider.
static void
set_diff_context_from_opts(diff_context_sptr ctxt,
			   options& opts)
{
  ctxt->show_stats_only(opts.show_stats_only);
  ctxt->show_deleted_fns(opts.show_all_fns || opts.show_deleted_fns);
  ctxt->show_changed_fns(opts.show_all_fns || opts.show_changed_fns);
  ctxt->show_added_fns(opts.show_all_fns || opts.show_added_fns);
  ctxt->show_deleted_vars(opts.show_all_vars || opts.show_deleted_vars);
  ctxt->show_changed_vars(opts.show_all_vars || opts.show_changed_vars);
  ctxt->show_added_vars(opts.show_all_vars || opts.show_added_vars);
}

}

int
main(int argc, char* argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      display_usage(argv[0], cerr);
      return true;
    }

  if (!opts.file1.empty() && !opts.file2.empty())
    {
      if (!check_file(opts.file1, cerr))
	return true;

      if (!check_file(opts.file2, cerr))
	return true;

      abigail::tools::file_type t1_type, t2_type;

      t1_type = guess_file_type(opts.file1);
      if (t1_type == abigail::tools::FILE_TYPE_UNKNOWN)
	{
	  cerr << "Unknown content type for file " << opts.file1 << "\n";
	  return true;
	}

      t2_type = guess_file_type(opts.file2);
      if (t2_type == abigail::tools::FILE_TYPE_UNKNOWN)
	{
	  cerr << "Unknown content type for file " << opts.file2 << "\n";
	  return true;
	}

      translation_unit_sptr t1, t2;
      corpus_sptr c1, c2;

      switch (t1_type)
	{
	case abigail::tools::FILE_TYPE_UNKNOWN:
	  cerr << "Unknown content type for file " << opts.file1 << "\n";
	  return true;
	  break;
	case abigail::tools::FILE_TYPE_NATIVE_BI:
	  t1 = abigail::xml_reader::read_translation_unit_from_file(opts.file1);
	  break;
	case abigail::tools::FILE_TYPE_ELF:
	  c1 = abigail::dwarf_reader::read_corpus_from_elf(opts.file1);
	  break;
	case abigail::tools::FILE_TYPE_XML_CORPUS:
	  c1 =
	    abigail::xml_reader::read_corpus_from_native_xml_file(opts.file1);
	  break;
	case abigail::tools::FILE_TYPE_ZIP_CORPUS:
	  c1 = abigail::xml_reader::read_corpus_from_file(opts.file1);
	  break;
	}

      switch (t2_type)
	{
	case abigail::tools::FILE_TYPE_UNKNOWN:
	  cerr << "Unknown content type for file " << opts.file2 << "\n";
	  return true;
	  break;
	case abigail::tools::FILE_TYPE_NATIVE_BI:
	  t2 = abigail::xml_reader::read_translation_unit_from_file(opts.file2);
	  break;
	case abigail::tools::FILE_TYPE_ELF:
	  c2 = abigail::dwarf_reader::read_corpus_from_elf(opts.file2);
	  break;
	case abigail::tools::FILE_TYPE_XML_CORPUS:
	  c2 =
	    abigail::xml_reader::read_corpus_from_native_xml_file(opts.file2);
	  break;
	case abigail::tools::FILE_TYPE_ZIP_CORPUS:
	  c2 = abigail::xml_reader::read_corpus_from_file(opts.file2);
	  break;
	}

      if (!t1 && !c1)
	{
	  cerr << "failed to read input file " << opts.file1 << "\n";
	  return true;
	}

      if (!t2 && !c2)
	{
	  cerr << "failed to read input file" << opts.file2 << "\n";
	  return true;
	}

      if (!!c1 != !!c2
	  || !!t1 != !!t2)
	{
	  cerr << "the two input should be of the same kind\n";
	  return true;
	}

      if (t1)
	{
	  translation_unit_diff_sptr changes = compute_diff(t1, t2);
	  changes->report(cout);
	}
      else if (c1)
	{
	  if (opts.show_symtabs)
	    {
	      display_symtabs(c1, c2, cout);
	      return false;
	    }

	  diff_context_sptr ctxt(new diff_context);
	  set_diff_context_from_opts(ctxt, opts);
	  corpus_diff_sptr changes = compute_diff(c1, c2, ctxt);
	  changes->report(cout);
	}

      return false;
    }

  return true;
}
