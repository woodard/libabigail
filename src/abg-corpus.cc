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

#include <cstdio>
#include <cstring>
#include <cassert>
#include <ext/stdio_filebuf.h>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-writer.h"
#include "abg-libzip-utils.h"

namespace abigail
{

using std::ostringstream;
using std::list;
using std::vector;
using zip_utils::zip_sptr;
using zip_utils::zip_file_sptr;
using zip_utils::open_archive;
using zip_utils::open_file_in_archive;

template<typename T>
struct array_deleter
{
  void
  operator()(T* a)
  {
    delete [] a;
  }
};//end array_deleter

struct corpus::priv
{
  string			path;
  translation_units		members;
  vector<function_decl*>	fns;
  vector<var_decl*>		vars;
  bool				is_symbol_table_built;

private:
  priv();

public:
  priv(const string &p)
    : path(p),
      is_symbol_table_built(false)
  {}

  void
  build_symbol_table();
};

/// A visitor type to be used while traversing functions and variables
/// of the translations units of the corpus.  The goal of this visitor
/// is to build a symbol table containing all the public functions and
/// global variables of the all the translation units of the the
/// corpus.
class symtab_build_visitor_type : public ir_node_visitor
{
  vector<function_decl*>&	functions;
  vector<var_decl*>&		variables;
  list<function_decl*>		wip_fns;
  int				wip_fns_size;
  list<var_decl*>		wip_vars;
  int				wip_vars_size;

  symtab_build_visitor_type();

  friend class corpus::priv;

public:
  symtab_build_visitor_type(vector<function_decl*>&fns,
			    vector<var_decl*>&vars)
    : functions(fns),
      variables(vars),
      wip_fns_size(0),
      wip_vars_size(0)
  {}

  /// This function is called while visiting a @ref function_decl IR
  /// node of a translation unit.
  ///
  /// Add the function to the symbol table being constructed (WIP
  /// meaning work in progress).
  ///
  /// @param fn the function being visited.
  void
  visit(function_decl* fn)
  {
    wip_fns.push_back(fn);
    ++wip_fns_size;
  }

  /// This function is called while visiting a
  /// class_decl::function_decl IR node of a translation unit.
  ///
  /// Add the member function to the symbol table being constructed
  /// (WIP meaning work in progress).
  ///
  /// @param fn the member function being visited.
  void
  visit(class_decl::member_function* fn)
  {
    wip_fns.push_back(fn);
    ++wip_fns_size;
  }

  /// This function is called while visiting a @ref var_decl IR node
  /// of a translation unit.
  ///
  /// Add the variable to the symbol table being constructed (WIP
  /// meaning work in progress).
  ///
  /// @param var the variable being visited.
  void
  visit(var_decl* var)
  {
    wip_vars.push_back(var);
    ++wip_vars_size;
  }

  /// This function is called while visiting a @ref var_decl IR node
  /// of a translation unit.
  ///
  /// Add the variable to the symbol table being constructed (WIP
  /// meaning work in progress).
  ///
  /// @param var the variable being visited.
  void
  visit(class_decl::data_member* var)
  {
    if (var->is_static())
      {
	wip_vars.push_back(var);
	++wip_vars_size;
      }
  }

};// end struct symtab_build_visitor_type

/// This is a comparison functor for comparing pointers to @ref
/// function_decl.
struct func_comp
{
  /// The comparisong operator for pointers to @ref function_decl.  It
  /// performs a string comparison of the mangled names of the
  /// functions.  If the functions don't have mangled names, it
  /// compares their names instead.
  ///
  /// @param first the first function to consider in the comparison.
  ///
  /// @param second the second function to consider in the comparison.
  ///
  /// @return true if the (mangled) name of the first function is less
  /// than the (mangled)name of the second one, false otherwise.
  bool
  operator()(const function_decl* first,
	     const function_decl* second) const
  {
    assert(first != 0 && second != 0);

    string first_name, second_name;
    first_name = first->get_mangled_name();
    if (first_name.empty())
      first_name = first->get_name();
    assert(!first_name.empty());

    second_name = second->get_mangled_name();
    if (second_name.empty())
      second_name = second->get_name();
    assert(!second_name.empty());

    return first_name < second_name;
  }
};

/// This is a comparison functor for comparing pointers to @ref
/// var_decl.
struct var_comp
{
  /// The comparison operator for pointers to @ref var_decl.
  ///
  /// It perform a string comparison on the names of the variables.
  ///
  /// @param first the first variable to consider for the comparison.
  ///
  /// @param second the second variable to consider for the comparison.
  ///
  /// @return true if first is less than second, false otherwise.
  bool
  operator()(const var_decl* first,
	     const var_decl* second) const
  {
    assert(first != 0 && second != 0);

    string first_name, second_name;
    first_name = first->get_qualified_name();
    assert(!first_name.empty());
    second_name = second->get_qualified_name();
    assert(!second_name.empty());

    return first_name < second_name;
  }
};

/// Build the symbol tables for the corpus.  That is, walk all the
/// functions of all translation units of the corpus, stuff them in a
/// vector and sort the vector.  Likewise for the variables.
void
corpus::priv::build_symbol_table()
{
  symtab_build_visitor_type v(fns, vars);

  for (translation_units::iterator i = members.begin();
       i != members.end();
       ++i)
      (*i)->traverse(v);

  fns.reserve(v.wip_fns_size);
  for (list<function_decl*>::iterator i = v.wip_fns.begin();
       i != v.wip_fns.end();
       ++i)
      fns.push_back(*i);
  v.wip_fns.clear();
  v.wip_fns_size = 0;

  vars.reserve(v.wip_vars_size);
  for (list<var_decl*>::iterator i = v.wip_vars.begin();
       i != v.wip_vars.end();
       ++i)
      vars.push_back(*i);
  v.wip_vars.clear();
  v.wip_vars_size = 0;

  func_comp fc;
  std::sort(fns.begin(), fns.end(), fc);

  var_comp vc;
  std::sort(vars.begin(), vars.end(), vc);

  is_symbol_table_built = true;
}

/// @param path the path to the file containing the ABI corpus.
corpus::corpus(const string& path)
{priv_.reset(new priv(path));}

/// Add a translation unit to the current ABI Corpus.	Next time
/// corpus::save is called, all the translation unit that got added
/// to the corpus are going to be serialized on disk in the file
/// associated to the current corpus.
///
/// @param tu the new translation unit to add.
void
corpus::add(const translation_unit_sptr tu)
{priv_->members.push_back(tu);}

/// Return the list of translation units of the current corpus.
///
/// @return the list of translation units of the current corpus.
const translation_units&
corpus::get_translation_units() const
{return priv_->members;}

/// Erase the translation units contained in this in-memory object.
///
/// Note that the on-disk archive file that contains the serialized
/// representation of this object is not modified.
void
corpus::drop_translation_units()
{priv_->members.clear();}

/// Get the file path associated to the corpus file.
///
/// A subsequent call to corpus::read will deserialize the content of
/// the abi file expected at this path; likewise, a call to
/// corpus::write will serialize the translation units contained in
/// the corpus object into the on-disk file at this path.

/// @return the file path associated to the current corpus.
string&
corpus::get_path() const
{return priv_->path;}

/// Set the file path associated to the corpus file.
///
/// A subsequent call to corpus::read will deserialize the content of
/// the abi file expected at this path; likewise, a call to
/// corpus::write will serialize the translation units contained in
/// the corpus object into the on-disk file at this path.
/// @param the new file path to assciate to the current corpus.
void
corpus::set_path(const string& path)
{priv_->path = path;}

/// Tests if the corpus contains no translation unit.
///
/// @return true if the corpus contains no translation unit.
bool
corpus::is_empty() const
{return priv_->members.empty();}

/// Compare the current @ref corpus against another one.
///
/// @param other the other corpus to compare against.
///
/// @return true if the two corpus are equal, false otherwise.
bool
corpus::operator==(const corpus& other) const
{
  translation_units::const_iterator i, j;
  for (i = get_translation_units().begin(),
	 j = other.get_translation_units().begin();
       (i != get_translation_units().end()
	&& j != other.get_translation_units().end());
       ++i, ++j)
    if ((**i) != (**j))
      return false;

  return (i == get_translation_units().end()
	  && j == other.get_translation_units().end());
}

/// @return the vector of functions of the symbol table.  The
/// functions are sorted using their mangled name or name if they
/// don't have mangle names.
const corpus::functions&
corpus::get_functions() const
{
  if (!priv_->is_symbol_table_built)
    priv_->build_symbol_table();
  assert(priv_->is_symbol_table_built);

  return priv_->fns;
}

/// @return the vector of variables of the symbol table.  The
/// variables are sorted using their name.
const corpus::variables&
corpus::get_variables() const
{
  if (!priv_->is_symbol_table_built)
    priv_->build_symbol_table();
  assert(priv_->is_symbol_table_built);

  return priv_->vars;
}

}// end namespace abigail
