// -*- mode: C++ -*-
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

/// @file

#ifndef __ABG_CORPUS_H__
#define __ABG_CORPUS_H__

#include <abg-ir.h>

namespace abigail
{

namespace ir
{

class corpus;
/// A convenience typedef for shared pointer to @ref corpus.
typedef shared_ptr<corpus> corpus_sptr;

/// This is the abstraction of a set of translation units (themselves
/// seen as bundles of unitary abi artefacts like types and decls)
/// bundled together as a corpus.  A corpus is thus the Application
/// binary interface of a program, a library or just a set of modules
/// put together.
class corpus
{
public:
  struct				priv;
  typedef shared_ptr<priv>		priv_sptr;
  typedef std::string			string;
  typedef vector<function_decl*>	functions;
  typedef vector<var_decl*>		variables;

  /// This abstracts where the corpus comes from.  That is, either it
  /// has been read from the native xml format, from DWARF or built
  /// artificially using the library's API.
  enum origin
  {
    ARTIFICIAL_ORIGIN = 0,
    NATIVE_XML_ORIGIN,
    DWARF_ORIGIN
  };

private:
  shared_ptr<priv> priv_;

  corpus();

public:

  corpus(const string&);

  void
  add(const translation_unit_sptr);

  const translation_units&
  get_translation_units() const;

  void
  drop_translation_units();

  origin
  get_origin() const;

  void
  set_origin(origin);

  string&
  get_path() const;

  void
  set_path(const string&);

  bool
  is_empty() const;

  bool
  operator==(const corpus&) const;

  void
  set_fun_symbol_map(string_elf_symbols_map_sptr);

  void
  set_var_symbol_map(string_elf_symbols_map_sptr);

  const string_elf_symbols_map_sptr
  get_fun_symbol_map_sptr() const;

  const string_elf_symbols_map_type&
  get_fun_symbol_map() const;

  const string_elf_symbols_map_sptr
  get_var_symbol_map_sptr() const;

  const string_elf_symbols_map_type&
  get_var_symbol_map() const;

  const elf_symbol_sptr
  lookup_function_symbol(const string& n) const;

  const elf_symbol_sptr
  lookup_function_symbol(const string& symbol_name,
			 const string& symbol_version) const;

  const elf_symbol_sptr
  lookup_variable_symbol(const string& n) const;

  const elf_symbol_sptr
  lookup_variable_symbol(const string& symbol_name,
			 const string& symbol_version) const;

  const functions&
  get_functions() const;

  const variables&
  get_variables() const;

  const elf_symbols&
  get_unreferenced_function_symbols() const;

  const elf_symbols&
  get_unreferenced_variable_symbols() const;

  vector<string>&
  get_regex_patterns_of_fns_to_suppress();

  const vector<string>&
  get_regex_patterns_of_fns_to_suppress() const;

  vector<string>&
  get_regex_patterns_of_vars_to_suppress();

  const vector<string>&
  get_regex_patterns_of_vars_to_suppress() const;

  vector<string>&
  get_regex_patterns_of_fns_to_keep();

  const vector<string>&
  get_regex_patterns_of_fns_to_keep() const;

  vector<string>&
  get_regex_patterns_of_vars_to_keep();

  const vector<string>&
  get_regex_patterns_of_vars_to_keep() const;

};// end class corpus.

}// end namespace ir
}//end namespace abigail
#endif //__ABG_CORPUS_H__
