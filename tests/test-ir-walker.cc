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

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-ir.h"
#include "abg-reader.h"
#include "test-utils.h"

using std::string;
using std::ofstream;
using std::cerr;
using std::cout;

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

  abigail::translation_unit tu(file_name);
  if (!abigail::xml_reader::read_translation_unit_from_file(tu))
    {
      cerr << "failed to read " << file_name << "\n";
      return 1;
    }

  name_printing_visitor v;
  tu.traverse(v);
}
