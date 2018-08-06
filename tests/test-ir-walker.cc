// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2018 Red Hat, Inc.
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

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-dwarf-reader.h"
#include "test-utils.h"

using std::string;
using std::ofstream;
using std::cerr;
using std::cout;

///@file
///
/// This example shows how to walk the Internal Representation (IR)
/// graph of the ABI of a binary (called an ABI Corpus) and perform
/// actions on each node of the graph.
///
/// Basically, one has to define a "visitor" which carries member
/// functions that are called during the traversal of the graph.
///
/// On the visitor, there is potentially one member function pair per
/// type of node traversed.  Each time a given node is visited, the
/// corresponding member function pair is called by the traversal
/// machinery.  In other words, the visitor is notified each time a
/// node is traversed.
///
/// To define a visitor, one has to create a type which implements
/// (inherits) the abigail::ir_node_visitor interface.  The visitor
/// must have a pair of node_begin() and node_end() function per type
/// of node that we wish to be notified for.
///
///  Once the visitor is defined, we can load an elf file and build an
///  ABI corpus out of it by using the
///  libabigail::dwarf_reader::read_corpus_from_elf() function, for
///  instance.
///
///  Then we enumerate the translation units comprised in
///  that ABI corpus and we invoke their "traverse()" method, using
///  and instance of the visitor that we just defined.
///
///  Enjoy!

struct name_printing_visitor : public abigail::ir_node_visitor
{
  unsigned level_;

  name_printing_visitor()
    : level_()
  {}

  void
  build_level_prefix(string& str)
  {
    str.clear();
    for (unsigned i = 0; i < level_; ++i)
      str += ' ';
  }

  string
  build_level_prefix()
  {
    string prefix;
    build_level_prefix(prefix);
    return prefix;
  }

  bool
  visit_begin(abigail::namespace_decl* ns)
  {
    string prefix = build_level_prefix();

    cout << prefix << ns->get_pretty_representation() << "\n"
	 << prefix << "{\n";
    ++level_;
    return true;
  }

  bool
  visit_end(abigail::namespace_decl*)
  {
    string prefix = build_level_prefix();
    cout << prefix << "}\n";
    --level_;
    return true;
  }

  bool
  visit_begin(abigail::class_decl* klass)
  {
    string prefix = build_level_prefix();

    cout << prefix << klass->get_pretty_representation() << "\n"
	 << prefix << "{\n";
    ++level_;
    return true;
  }

  bool
  visit_end(abigail::class_decl*)
  {
    string prefix = build_level_prefix();
    cout << prefix << "}\n";
    --level_;
    return true;
  }

  bool
  visit_begin(abigail::function_decl* f)
  {
    string prefix = build_level_prefix();
    cout << prefix << f->get_pretty_representation() << "\n";
    ++level_;
    return true;
  }

  bool
  visit_end(abigail::function_decl*)
  {
    --level_;
    return true;
  }

  bool
  visit_begin(abigail::var_decl* v)
  {
    string prefix = build_level_prefix();
    cout << prefix << v->get_pretty_representation() << "\n";
    ++level_;
    return true;
  }

  bool
  visit_end(abigail::var_decl*)
  {
    --level_;
    return true;
  }
};

int
main(int argc, char **argv)
{
  if (argc < 2)
    return 0;

  string file_name = argv[1];

  abigail::ir::environment_sptr env(new abigail::ir::environment);
  abigail::corpus_sptr c;
  abigail::dwarf_reader::status status = abigail::dwarf_reader::STATUS_OK;
  if (!(c = abigail::dwarf_reader::read_corpus_from_elf(file_name,
							/*debug_info_path=*/0,
							env.get(),
							/*load_all_type=*/false,
							status)))
    {
      cerr << "failed to read " << file_name << "\n";
      return 1;
    }

  name_printing_visitor v;
  // Now traverse each translation unit of the corpus using our
  // instance of name_printing_visitor
  for (abigail::ir::translation_units::const_iterator tu_iterator =
	 c->get_translation_units().begin();
       tu_iterator != c->get_translation_units().end();
       ++tu_iterator)
    (*tu_iterator)->traverse(v);
}
