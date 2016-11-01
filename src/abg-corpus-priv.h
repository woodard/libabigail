// -*- Mode: C++ -*-
//
// Copyright (C) 2016 Red Hat, Inc.
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
///
/// The private data and functions of the @ref abigail::ir::corpus type.
///
/// Interfaces declared/defined in this file are to be used by parts
/// of libabigail but *NOT* by clients of libabigail.
///

#ifndef __ABG_CORPUS_PRIV_H__
#define __ABG_CORPUS_PRIV_H__

#include "abg-sptr-utils.h"

namespace abigail
{

namespace sptr_utils
{
}// end namespace sptr_utils

namespace ir
{

using sptr_utils::regex_t_sptr;

/// A convenience typedef for std::vector<regex_t_sptr>.
typedef vector<regex_t_sptr> regex_t_sptrs_type;

// <corpus::exported_decls_builder>

/// Convenience typedef for a hash map which key is a string and which
/// data is a vector of abigail::ir::function_decl*
typedef unordered_map<string, vector<function_decl*> > str_fn_ptrs_map_type;
/// Convenience typedef for a hash map which key is a string and
/// which data is an abigail::ir::var_decl*.
typedef unordered_map<string, var_decl*> str_var_ptr_map_type;

/// The type of the private data of @ref
/// corpus::exported_decls_builder type.
class corpus::exported_decls_builder::priv
{
  friend class corpus::exported_decls_builder;
  friend class corpus;

  priv();

  functions&		fns_;
  variables&		vars_;
  // A map that associates a function ID (function symbol and its
  // version) to to a vector of functions with that ID.  Normally, one
  // would think that in the corpus, there must only one function for
  // a given ID.  Actually, in c++, there can be two function template
  // instantiations that produce the same function ID because the
  // template parameters of the second instantiation are just typedefs
  // of the first instantiation, for instance.  So there can be cases
  // where one ID appertains to more than one function.
  str_fn_ptrs_map_type	id_fns_map_;
  str_var_ptr_map_type	id_var_map_;
  strings_type&	fns_suppress_regexps_;
  regex_t_sptrs_type	compiled_fns_suppress_regexp_;
  strings_type&	vars_suppress_regexps_;
  regex_t_sptrs_type	compiled_vars_suppress_regexp_;
  strings_type&	fns_keep_regexps_;
  regex_t_sptrs_type	compiled_fns_keep_regexps_;
  strings_type&	vars_keep_regexps_;
  regex_t_sptrs_type	compiled_vars_keep_regexps_;
  strings_type&	sym_id_of_fns_to_keep_;
  strings_type&	sym_id_of_vars_to_keep_;

public:

  priv(functions& fns,
       variables& vars,
       strings_type& fns_suppress_regexps,
       strings_type& vars_suppress_regexps,
       strings_type& fns_keep_regexps,
       strings_type& vars_keep_regexps,
       strings_type& sym_id_of_fns_to_keep,
       strings_type& sym_id_of_vars_to_keep)
    : fns_(fns),
      vars_(vars),
      fns_suppress_regexps_(fns_suppress_regexps),
      vars_suppress_regexps_(vars_suppress_regexps),
      fns_keep_regexps_(fns_keep_regexps),
      vars_keep_regexps_(vars_keep_regexps),
    sym_id_of_fns_to_keep_(sym_id_of_fns_to_keep),
    sym_id_of_vars_to_keep_(sym_id_of_vars_to_keep)
  {}

  /// Getter for the compiled regular expressions that designate the
  /// functions to suppress from the set of exported functions.
  ///
  /// @return a vector of the compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_fns_suppress()
  {
    if (compiled_fns_suppress_regexp_.empty())
      {
	for (vector<string>::const_iterator i =
	       fns_suppress_regexps_.begin();
	     i != fns_suppress_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      compiled_fns_suppress_regexp_.push_back(r);
	  }
      }
    return compiled_fns_suppress_regexp_;
  }

  /// Getter for the compiled regular expressions that designates the
  /// functions to keep in the set of exported functions.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_fns_keep()
  {
    if (compiled_fns_keep_regexps_.empty())
      {
	for (vector<string>::const_iterator i =
	       fns_keep_regexps_.begin();
	     i != fns_keep_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      compiled_fns_keep_regexps_.push_back(r);
	  }
      }
    return compiled_fns_keep_regexps_;
  }

  /// Getter of the compiled regular expressions that designate the
  /// variables to suppress from the set of exported variables.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_vars_suppress()
  {
    if (compiled_vars_suppress_regexp_.empty())
      {
	for (vector<string>::const_iterator i =
	       vars_suppress_regexps_.begin();
	     i != vars_suppress_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      compiled_vars_suppress_regexp_.push_back(r);
	  }
      }
    return compiled_vars_suppress_regexp_;
  }

  /// Getter for the compiled regular expressions that designate the
  /// variables to keep in the set of exported variables.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_vars_keep()
  {
    if (compiled_vars_keep_regexps_.empty())
      {
	for (vector<string>::const_iterator i =
	       vars_keep_regexps_.begin();
	     i != vars_keep_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      compiled_vars_keep_regexps_.push_back(r);
	  }
      }
    return compiled_vars_keep_regexps_;
  }

  /// Getter for a map of the IDs of the functions that are present in
  /// the set of exported functions.
  ///
  /// This map is useful during the construction of the set of
  /// exported functions, at least to ensure that every function is
  /// present only once in that set.  Actually, for each symbol ID,
  /// there can be several functions, given that each of those have
  /// different declaration names; this can happen with function
  /// template instantiations which decl names differ because the type
  /// parameters of the templates are typedefs of each other.
  ///
  /// @return a map which key is a string and which data is a pointer
  /// to a function.
  const str_fn_ptrs_map_type&
  id_fns_map() const
  {return id_fns_map_;}

  /// Getter for a map of the IDs of the functions that are present in
  /// the set of exported functions.
  ///
  /// This map is useful during the construction of the set of
  /// exported functions, at least to ensure that every function is
  /// present only once in that set.
  ///
  /// @return a map which key is a string and which data is a pointer
  /// to a function.
  str_fn_ptrs_map_type&
  id_fns_map()
  {return id_fns_map_;}

  /// Getter for a map of the IDs of the variables that are present in
  /// the set of exported variables.
  ///
  /// This map is useful during the construction of the set of
  /// exported variables, at least to ensure that every function is
  /// present only once in that set.
  ///
  /// @return a map which key is a string and which data is a pointer
  /// to a function.
  const str_var_ptr_map_type&
  id_var_map() const
  {return id_var_map_;}

  /// Getter for a map of the IDs of the variables that are present in
  /// the set of exported variables.
  ///
  /// This map is useful during the construction of the set of
  /// exported variables, at least to ensure that every function is
  /// present only once in that set.
  ///
  /// @return a map which key is a string and which data is a pointer
  /// to a function.
  str_var_ptr_map_type&
  id_var_map()
  {return id_var_map_;}

  /// Returns an ID for a given function.
  ///
  /// @param fn the function to calculate the ID for.
  ///
  /// @return a reference to a string representing the function ID.
  interned_string
  get_id(const function_decl& fn)
  {return fn.get_id();}

  /// Returns an ID for a given variable.
  ///
  /// @param var the variable to calculate the ID for.
  ///
  /// @return a reference to a string representing the variable ID.
  interned_string
  get_id(const var_decl& var)
  {return var.get_id();}

  /// Test if a given function ID is in the id-functions map.
  ///
  /// If it is, then return a pointer to the vector of functions with
  /// that ID.  If not, just return nil.
  ///
  /// @param fn_id the ID to consider.
  ///
  /// @return the pointer to the vector of functions with ID @p fn_id,
  /// or nil if no function with that ID exists.
  vector<function_decl*>*
  fn_id_is_in_id_fns_map(const string& fn_id)
  {
    str_fn_ptrs_map_type& m = id_fns_map();
    str_fn_ptrs_map_type::iterator i = m.find(fn_id);
    if (i == m.end())
      return 0;
    return &i->second;
  }

  /// Test if a a function if the same ID as a given function is
  /// present in the id-functions map.
  ///
  /// @param fn the function to consider.
  ///
  /// @return a pointer to the vector of functions with the same ID as
  /// @p fn, that are present in the id-functions map, or nil if no
  /// function with the same ID as @p fn is present in the
  /// id-functions map.
  vector<function_decl*>*
  fn_id_is_in_id_fns_map(const function_decl* fn)
  {
    string fn_id = fn->get_id();
    return fn_id_is_in_id_fns_map(fn_id);
  }

  /// Test if a given function is present in a vector of functions.
  ///
  /// The function compares the ID and the qualified name of
  /// functions.
  ///
  /// @param fn the function to consider.
  ///
  /// @parm fns the vector of functions to consider.
  static bool
  fn_is_in_fns(const function_decl* fn, const vector<function_decl*>& fns)
  {
    if (fns.empty())
      return false;

    const string fn_id = fn->get_id();
    for (vector<function_decl*>::const_iterator i = fns.begin();
	 i != fns.end();
	 ++i)
      if ((*i)->get_id() == fn_id
	  && (*i)->get_qualified_name() == fn->get_qualified_name())
	return true;

    return false;
  }

  ///  Test if a function is in the id-functions map.
  ///
  ///  @param fn the function to consider.
  ///
  ///  @return true iff the function is in the id-functions map.
  bool
  fn_is_in_id_fns_map(const function_decl* fn)
  {
    vector<function_decl*>* fns = fn_id_is_in_id_fns_map(fn);
    if (fns && fn_is_in_fns(fn, *fns))
      return true;
    return false;
  }

  /// Add a given function to the map of functions that are present in
  /// the set of exported functions.
  ///
  /// @param fn the function to add to the map.
  void
  add_fn_to_id_fns_map(function_decl* fn)
  {
    if (!fn)
      return;

    // First associate the function id to the function.
    string fn_id = fn->get_id();
    vector<function_decl*>* fns = fn_id_is_in_id_fns_map(fn_id);
    if (!fns)
      fns = &(id_fns_map()[fn_id] = vector<function_decl*>());
    fns->push_back(fn);

    // Now associate all aliases of the underlying symbol to the
    // function too.
    elf_symbol_sptr sym = fn->get_symbol();
    assert(sym);
    string sym_id;
    do
      {
	sym_id = sym->get_id_string();
	if (sym_id == fn_id)
	  goto loop;
	fns = fn_id_is_in_id_fns_map(fn_id);
	if (!fns)
	  fns = &(id_fns_map()[fn_id] = vector<function_decl*>());
	fns->push_back(fn);
      loop:
	sym = sym->get_next_alias();
      }
    while (sym && !sym->is_main_symbol());
  }

  /// Test if a given (ID of a) varialble is present in the variable
  /// map.  In other words, it tests if a given variable is present in
  /// the set of exported variables.
  ///
  /// @param fn_id the ID of the variable to consider.
  ///
  /// @return true iff the variable designated by @p fn_id is present
  /// in the set of exported variables.
  bool
  var_id_is_in_id_var_map(const string& var_id) const
  {
    const str_var_ptr_map_type& m = id_var_map();
    str_var_ptr_map_type::const_iterator i = m.find(var_id);
    return i != m.end();
  }

  /// Add a given variable to the map of functions that are present in
  /// the set of exported functions.
  ///
  /// @param id the variable to add to the map.
  void
  add_var_to_map(var_decl* var)
  {
    if (var)
      {
	const string& var_id = get_id(*var);
	id_var_map()[var_id] = var;
      }
  }

  /// Add a function to the set of exported functions.
  ///
  /// @param fn the function to add to the set of exported functions.
  void
  add_fn_to_exported(function_decl* fn)
  {
    if (!fn_is_in_id_fns_map(fn))
      {
	fns_.push_back(fn);
	add_fn_to_id_fns_map(fn);
      }
  }

  /// Add a variable to the set of exported variables.
  ///
  /// @param fn the variable to add to the set of exported variables.
  void
  add_var_to_exported(var_decl* var)
  {
    const string& id = get_id(*var);
    if (!var_id_is_in_id_var_map(id))
      {
	vars_.push_back(var);
	add_var_to_map(var);
      }
  }

  /// Getter for the set of ids of functions to keep in the set of
  /// exported functions.
  ///
  /// @return the set of ids of functions to keep in the set of
  /// exported functions.
  const strings_type&
  sym_id_of_fns_to_keep() const
  {return sym_id_of_fns_to_keep_;}

  /// Getter for the set of ids of variables to keep in the set of
  /// exported variables.
  ///
  /// @return the set of ids of variables to keep in the set of
  /// exported variables.
  const strings_type&
  sym_id_of_vars_to_keep() const
  {return sym_id_of_vars_to_keep_;}

  /// Look at the set of functions to keep and tell if if a given
  /// function is to be kept, according to that set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_id_of_fns_to_keep(const function_decl* fn)
  {
    if (!fn)
      return false;

    bool keep = true;

    if (elf_symbol_sptr sym = fn->get_symbol())
      {
	if (!sym_id_of_fns_to_keep().empty())
	  keep = false;
	if (!keep)
	  {
	    for (vector<string>::const_iterator i =
		   sym_id_of_fns_to_keep().begin();
		 i != sym_id_of_fns_to_keep().end();
		 ++i)
	      {
		string sym_name, sym_version;
		assert(elf_symbol::get_name_and_version_from_id(*i,
								sym_name,
								sym_version));
		if (sym_name == sym->get_name()
		    && sym_version == sym->get_version().str())
		  {
		    keep = true;
		    break;
		  }
	      }
	  }
      }
    else
      keep = false;

    return keep;
  }

  /// Look at the set of functions to suppress from the exported
  /// functions set and tell if if a given function is to be kept,
  /// according to that set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_regex_of_fns_to_suppress(const function_decl *fn)
  {
    if (!fn)
      return false;

    string frep = fn->get_qualified_name();
    bool keep = true;

    for (regex_t_sptrs_type::const_iterator i =
	   compiled_regex_fns_suppress().begin();
	 i != compiled_regex_fns_suppress().end();
	 ++i)
      if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == 0)
	{
	  keep = false;
	  break;
	}

    return keep;
  }

  /// Look at the regular expressions of the functions to keep and
  /// tell if if a given function is to be kept, according to that
  /// set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_regex_of_fns_to_keep(const function_decl *fn)
  {
    if (!fn)
      return false;

    string frep = fn->get_qualified_name();
    bool keep = true;

    if (!compiled_regex_fns_keep().empty())
      keep = false;

    if (!keep)
      for (regex_t_sptrs_type::const_iterator i =
	     compiled_regex_fns_keep().begin();
	   i != compiled_regex_fns_keep().end();
	   ++i)
	if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == 0)
	  {
	    keep = true;
	    break;
	  }

    return keep;
  }

  /// Look at the regular expressions of the variables to keep and
  /// tell if if a given variable is to be kept, according to that
  /// set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_id_of_vars_to_keep(const var_decl* var)
  {
    if (!var)
      return false;

    bool keep = true;

    if (elf_symbol_sptr sym = var->get_symbol())
      {
	if (!sym_id_of_vars_to_keep().empty())
	  keep = false;
	if (!keep)
	  {
	    for (vector<string>::const_iterator i =
		   sym_id_of_vars_to_keep().begin();
		 i != sym_id_of_vars_to_keep().end();
		 ++i)
	      {
		string sym_name, sym_version;
		assert(elf_symbol::get_name_and_version_from_id(*i,
								sym_name,
								sym_version));
		if (sym_name == sym->get_name()
		    && sym_version == sym->get_version().str())
		  {
		    keep = true;
		    break;
		  }
	      }
	  }
      }
    else
      keep = false;

    return keep;
  }

  /// Look at the set of variables to suppress from the exported
  /// variables set and tell if if a given variable is to be kept,
  /// according to that set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_regex_of_vars_to_suppress(const var_decl *var)
  {
    if (!var)
      return false;

    string frep = var->get_qualified_name();
    bool keep = true;

    for (regex_t_sptrs_type::const_iterator i =
	   compiled_regex_vars_suppress().begin();
	 i != compiled_regex_vars_suppress().end();
	 ++i)
      if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == 0)
	{
	  keep = false;
	  break;
	}

    return keep;
  }

  /// Look at the regular expressions of the variables to keep and
  /// tell if if a given variable is to be kept, according to that
  /// set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_regex_of_vars_to_keep(const var_decl *var)
  {
    if (!var)
      return false;

    string frep = var->get_qualified_name();
    bool keep = true;

    if (!compiled_regex_vars_keep().empty())
      keep = false;

    if (!keep)
      {
	for (regex_t_sptrs_type::const_iterator i =
	       compiled_regex_vars_keep().begin();
	     i != compiled_regex_vars_keep().end();
	     ++i)
	  if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == 0)
	    {
	      keep = true;
	      break;
	    }
      }

    return keep;
  }
}; // end struct corpus::exported_decls_builder::priv


/// The private data of the @ref corpus type.
struct corpus::priv
{
  mutable unordered_map<string, type_base_sptr> canonical_types_;
  environment*					env;
  corpus::exported_decls_builder_sptr		exported_decls_builder;
  origin					origin_;
  vector<string>				regex_patterns_fns_to_suppress;
  vector<string>				regex_patterns_vars_to_suppress;
  vector<string>				regex_patterns_fns_to_keep;
  vector<string>				regex_patterns_vars_to_keep;
  vector<string>				sym_id_fns_to_keep;
  vector<string>				sym_id_vars_to_keep;
  string					path;
  vector<string>				needed;
  string					soname;
  string					architecture_name;
  translation_units				members;
  vector<function_decl*>			fns;
  vector<var_decl*>				vars;
  string_elf_symbols_map_sptr			var_symbol_map;
  string_elf_symbols_map_sptr			undefined_var_symbol_map;
  elf_symbols					sorted_var_symbols;
  elf_symbols					sorted_undefined_var_symbols;
  string_elf_symbols_map_sptr			fun_symbol_map;
  string_elf_symbols_map_sptr			undefined_fun_symbol_map;
  elf_symbols					sorted_fun_symbols;
  elf_symbols					sorted_undefined_fun_symbols;
  elf_symbols					unrefed_fun_symbols;
  elf_symbols					unrefed_var_symbols;
  // A scope (namespace of class) is shared among all translation
  // units of a given corpus.
  //mutable istring_scopes_sptr_map_type		scopes_;
  mutable istring_type_base_wptr_map_type	basic_types_;
  mutable istring_type_base_wptr_map_type	class_types_;
  mutable istring_type_base_wptr_map_type	union_types_;
  mutable istring_type_base_wptr_map_type	enum_types_;
  mutable istring_type_base_wptr_map_type	typedef_types_;
  mutable istring_type_base_wptr_map_type	qualified_types_;
  mutable istring_type_base_wptr_map_type	pointer_types_;
  mutable istring_type_base_wptr_map_type	reference_types_;
  mutable istring_type_base_wptr_map_type	array_types_;
  mutable istring_type_base_wptr_map_type	function_types_;

private:
  priv();

public:
  priv(const string &	p,
       environment*	e)
    : env(e),
      origin_(ARTIFICIAL_ORIGIN),
      path(p)
  {}

  void
  build_unreferenced_symbols_tables();

  // istring_scope_sptr_map_type&
  // get_scopes()
  // {return scopes_;}

  // istring_scope_sptr_map_type&
  // get_scopes() const
  // {return const_cast<priv*>(this)->get_scopes();}

  /// Getter for the map that associates the name of a basic type to
  /// the @ref type_decl_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_basic_types()
  {return basic_types_;}

  /// Getter for the map that associates the name of a basic type to
  /// the @ref type_decl_sptr that represents that type
  const istring_type_base_wptr_map_type&
  get_basic_types() const
  {return const_cast<priv*>(this)->get_basic_types();}

  /// Getter for the map that associates the name of a class type to
  /// the @ref class_decl_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_class_types()
  {return class_types_;}

  /// Getter for the map that associates the name of a class type to
  /// the @ref class_decl_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_class_types() const
  {return const_cast<priv*>(this)->get_class_types();}

  /// Getter for the map that associates the name of a union type to
  /// the @ref union_decl_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_union_types()
  {return union_types_;}

  /// Getter for the map that associates the name of a union type to
  /// the @ref union_decl_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_union_types() const
  {return const_cast<priv*>(this)->get_union_types();}

  /// Getter for the map that associates the name of an enum type to
  /// the @ref enum_type_decl_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_enum_types()
  {return enum_types_;}

  /// Getter for the map that associates the name of an enum type to
  /// the @ref enum_type_decl_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_enum_types() const
  {return const_cast<priv*>(this)->get_enum_types();}

  /// Getter for the map that associates the name of a typedef to the
  /// @ref typedef_decl_sptr that represents tha type.
  istring_type_base_wptr_map_type&
  get_typedef_types()
  {return typedef_types_;}

  /// Getter for the map that associates the name of a typedef to the
  /// @ref typedef_decl_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_typedef_types() const
  {return const_cast<priv*>(this)->get_typedef_types();}

  /// Getter for the map that associates the name of a qualified type
  /// to the @ref qualified_type_def_sptr.
  istring_type_base_wptr_map_type&
  get_qualified_types()
  {return qualified_types_;}

  /// Getter for the map that associates the name of a qualified type
  /// to the @ref qualified_type_def_sptr.
  const istring_type_base_wptr_map_type&
  get_qualified_types() const
  {return const_cast<priv*>(this)->get_qualified_types();}

  /// Getter for the map that associates the name of a pointer type to
  /// the @ref pointer_type_def_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_pointer_types()
  {return pointer_types_;}

  /// Getter for the map that associates the name of a pointer type to
  /// the @ref pointer_type_def_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_pointer_types() const
  {return const_cast<priv*>(this)->get_pointer_types();}

  /// Getter for the map that associates the name of a pointer type to
  /// the @ref reference_type_def_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_reference_types()
  {return reference_types_;}

  /// Getter for the map that associates the name of a pointer type to
  /// the @ref reference_type_def_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_reference_types() const
  {return const_cast<priv*>(this)->get_reference_types();}

  /// Getter for the map that associates the name of an array type to
  /// the @ref array_type_def_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_array_types()
  {return array_types_;}

  /// Getter for the map that associates the name of an array type to
  /// the @ref array_type_def_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_array_types() const
  {return const_cast<priv*>(this)->get_array_types();}

  /// Getter for the map that associates the name of a function type
  /// to the @ref function_type_sptr that represents that type.
  istring_type_base_wptr_map_type&
  get_function_types()
  {return function_types_;}

  /// Getter for the map that associates the name of a function type
  /// to the @ref function_type_sptr that represents that type.
  const istring_type_base_wptr_map_type&
  get_function_types() const
  {return const_cast<priv*>(this)->get_function_types();}
}; // end struct corpus::priv

void
maybe_update_scope_lookup_map(const scope_decl_sptr& member_scope);

void
maybe_update_scope_lookup_map(const decl_base_sptr& member_scope);

void
maybe_update_types_lookup_map(const type_decl_sptr& basic_type);

void
maybe_update_types_lookup_map(const class_decl_sptr& class_type);

void
maybe_update_types_lookup_map(const union_decl_sptr& union_type);

void
maybe_update_types_lookup_map(const enum_type_decl_sptr& enum_type);

void
maybe_update_types_lookup_map(const typedef_decl_sptr& typedef_type);

void
maybe_update_types_lookup_map(const qualified_type_def_sptr& qualified_type);

void
maybe_update_types_lookup_map(const pointer_type_def_sptr& pointer_type);

void
maybe_update_types_lookup_map(const reference_type_def_sptr& reference_type);

void
maybe_update_types_lookup_map(const array_type_def_sptr& array_type);

void
maybe_update_types_lookup_map(scope_decl *scope,
			      const function_type_sptr& function_type);

void
maybe_update_types_lookup_map(const decl_base_sptr& decl);

void
maybe_update_types_lookup_map(const type_base_sptr& type);

type_decl_sptr
lookup_basic_type(const type_decl&, corpus&);

type_decl_sptr
lookup_basic_type(const string&, corpus&);

class_decl_sptr
lookup_class_type(const class_decl&, corpus&);

class_decl_sptr
lookup_class_type(const string&, corpus&);

enum_type_decl_sptr
lookup_enum_type(const enum_type_decl&, corpus&);

enum_type_decl_sptr
lookup_enum_type(const string&, corpus&);

typedef_decl_sptr
lookup_typedef_type(const typedef_decl&, corpus&);

typedef_decl_sptr
lookup_typedef_type(const string&, corpus&);

type_base_sptr
lookup_class_or_typedef_type(const string&, corpus&);

type_base_sptr
lookup_class_typedef_or_enum_type(const string&, corpus&);

qualified_type_def_sptr
lookup_qualified_type(const qualified_type_def&, corpus&);

qualified_type_def_sptr
lookup_qualified_type(const string&, corpus&);

pointer_type_def_sptr
lookup_pointer_type(const pointer_type_def&, corpus&);

pointer_type_def_sptr
lookup_pointer_type(const string&, corpus&);

reference_type_def_sptr
lookup_reference_type(const reference_type_def&, corpus&);

reference_type_def_sptr
lookup_reference_type(const string&, corpus&);

array_type_def_sptr
lookup_array_type(const array_type_def&, corpus&);

array_type_def_sptr
lookup_array_type(const string&, corpus&);

function_type_sptr
lookup_function_type(const function_type&, corpus&);

function_type_sptr
lookup_function_type(const string&, corpus&);

}// end namespace ir

}// end namespace abigail

#endif // __ABG_CORPUS_PRIV_H__
