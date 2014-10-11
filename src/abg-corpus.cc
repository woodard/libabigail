// -*- mode: C++ -*-
//
// Copyright (C) 2013-2014 Red Hat, Inc.
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

#include "config.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <ext/stdio_filebuf.h>
#include <stdexcept>
#include <algorithm>
#include <tr1/unordered_map>
#include "abg-sptr-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-writer.h"

#if WITH_ZIP_ARCHIVE
#include "abg-libzip-utils.h"
#endif

namespace abigail
{

namespace sptr_utils
{

/// A delete functor for a shared_ptr of regex_t.
struct regex_t_deleter
{
  /// The operator called to de-allocate the pointer to regex_t
  /// embedded in a shared_ptr<regex_t>
  ///
  /// @param r the pointer to regex_t to de-allocate.
  void
  operator()(::regex_t* r)
  {
    regfree(r);
    delete r;
  }
};//end struct regex_deleter

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex_t_sptr
build_sptr<regex_t>(regex_t *p)
{return regex_t_sptr(p, regex_t_deleter());}

}// end namespace sptr_utils

namespace ir
{

using std::ostringstream;
using std::tr1::unordered_map;
using std::list;
using std::vector;

#if WITH_ZIP_ARCHIVE
using zip_utils::zip_sptr;
using zip_utils::zip_file_sptr;
using zip_utils::open_archive;
using zip_utils::open_file_in_archive;
#endif // WITH_ZIP_ARCHIVE

using sptr_utils::regex_t_sptr;

struct corpus::priv
{
  origin			origin_;
  bool				is_public_decl_table_built;
  vector<string>		regex_patterns_fns_to_suppress;
  vector<string>		regex_patterns_vars_to_suppress;
  vector<string>		regex_patterns_fns_to_keep;
  vector<string>		regex_patterns_vars_to_keep;
  string			path;
  translation_units		members;
  vector<function_decl*>	fns;
  vector<var_decl*>		vars;
  string_elf_symbols_map_sptr	var_symbol_map;
  string_elf_symbols_map_sptr	fun_symbol_map;

private:
  priv();

public:
  priv(const string &p)
    : origin_(ARTIFICIAL_ORIGIN),
      is_public_decl_table_built(false),
      path(p)
  {}

  void
  build_public_decl_table();
};

/// Convenience typedef for a hash map of pointer to function_decl and
/// boolean.
typedef unordered_map<const function_decl*,
		      bool,
		      function_decl::hash,
		      function_decl::ptr_equal> fn_ptr_map_type;

/// Convenience typedef for a hash map of string and pointer to
/// function_decl.
typedef unordered_map<string, const function_decl*> str_fn_ptr_map_type;

/// Convenience typedef for a hash map of pointer to var_decl and boolean.
typedef unordered_map<const var_decl*,
		      bool,
		      var_decl::hash,
		      var_decl::ptr_equal> var_ptr_map_type;

/// Convenience typedef for a hash map of string and pointer to var_decl.
typedef unordered_map<string, const var_decl*> str_var_ptr_map_type;

/// A visitor type to be used while traversing functions and variables
/// of the translations units of the corpus.  The goal of this visitor
/// is to build a public decl table containing all the public functions and
/// global variables of the all the translation units of the the
/// corpus.
class symtab_build_visitor_type : public ir_node_visitor
{
  vector<function_decl*>&	functions;
  vector<var_decl*>&		variables;
  vector<string>&		regex_patterns_fns_to_suppress;
  vector<string>&		regex_patterns_vars_to_suppress;
  vector<string>&		regex_patterns_fns_to_keep;
  vector<string>&		regex_patterns_vars_to_keep;
  vector<regex_t_sptr>		r_fns_suppress;
  vector<regex_t_sptr>		r_vars_suppress;
  vector<regex_t_sptr>		r_fns_keep;
  vector<regex_t_sptr>		r_vars_keep;
  str_fn_ptr_map_type		functions_map;
  str_var_ptr_map_type		variables_map;
  list<function_decl*>		wip_fns;
  int				wip_fns_size;
  list<var_decl*>		wip_vars;
  int				wip_vars_size;

  symtab_build_visitor_type();

  friend class corpus::priv;

public:
  symtab_build_visitor_type(vector<function_decl*>&	fns,
			    vector<var_decl*>&		vars,
			    vector<string>&		fns_suppress_regexps,
			    vector<string>&		vars_suppress_regexps,
			    vector<string>&		fns_keep_regexps,
			    vector<string>&		vars_keep_regexps)
    : functions(fns),
      variables(vars),
      regex_patterns_fns_to_suppress(fns_suppress_regexps),
      regex_patterns_vars_to_suppress(vars_suppress_regexps),
      regex_patterns_fns_to_keep(fns_keep_regexps),
      regex_patterns_vars_to_keep(vars_keep_regexps),
      wip_fns_size(0),
      wip_vars_size(0)
  {}

  /// Getter for the map of string and function pointers.
  ///
  /// @return the map of string and function pointers.
  const str_fn_ptr_map_type&
  fns_map() const
  {return functions_map;}

  /// Getter for the map of string and function pointers.
  ///
  /// @return the map of string and function pointers.
  str_fn_ptr_map_type&
  fns_map()
  {return functions_map;}

  /// Build a string that uniquely identifies a function_decl inside
  /// one corpus.
  ///
  /// That ID is either the name of the symbol of the function,
  /// concatenated with its version name, if the symbol exists.
  /// Otherwise, it's the linkage name of the function, if it exists.
  /// Otherwise it's the pretty representation ofthe function.
  ///
  /// @param fn the function to build the id for.
  ///
  /// @return the unique ID.
  string
  build_id(const function_decl& fn)
  {return fn.get_id();}

  /// Build a string that uniquely identifies a var_decl inside
  /// one corpus.
  ///
  /// That ID is either the name of the symbol of the variable,
  /// concatenated with its version name, if the symbol exists.
  /// Otherwise, it's the linkage name of the varible, if it exists.
  /// Otherwise it's the pretty representation ofthe variable.
  ///
  /// @param var the function to build the id for.
  ///
  /// @return the unique ID.
  string
  build_id(const var_decl& var)
  {return var.get_id();}

  /// Build a string that uniquely identifies a function_decl inside
  /// one corpus.
  ///
  /// That ID is either the name of the symbol of the function,
  /// concatenated with its version name, if the symbol exists.
  /// Otherwise, it's the linkage name of the function, if it exists.
  /// Otherwise it's the pretty representation ofthe function.
  ///
  /// @param fn the function to build the id for.
  ///
  /// @return the unique ID.
  string
  build_id(const function_decl* fn)
  {return build_id(*fn);}

  /// Build a string that uniquely identifies a var_decl inside
  /// one corpus.
  ///
  /// That ID is either the name of the symbol of the variable,
  /// concatenated with its version name, if the symbol exists.
  /// Otherwise, it's the linkage name of the varible, if it exists.
  /// Otherwise it's the pretty representation ofthe variable.
  ///
  /// @param var the function to build the id for.
  ///
  /// @return the unique ID.
  string
  build_id(const var_decl* fn)
  {return build_id(*fn);}

  /// Test if a given function name is in the map of strings and
  /// function pointer.
  ///
  /// @param fn_name the function name to test for.
  ///
  /// @return true if @p fn_name is in the map, false otherwise.
  bool
  fn_is_in_map(const string& fn_name) const
  {return fns_map().find(fn_name) != fns_map().end();}

  /// Add a pair function name / function_decl to the map of string
  /// and function_decl.
  ///
  /// @param fn the name of the function to add.
  ///
  /// @param v the function to add.
  void
  add_fn_to_map(const string& fn, const function_decl* v)
  {fns_map()[fn] = v;}

  /// Getter for the map of string and poitner to var_decl.
  ///
  /// @return the map of string and poitner to var_decl.
  const str_var_ptr_map_type&
  vars_map() const
  {return variables_map;}

  /// Getter for the map of string and pointer to var_decl.
  ///
  /// @return the map of string and poitner to var_decl.
  str_var_ptr_map_type&
  vars_map()
  {return variables_map;}

  /// Tests if a variable name is in the map of string and pointer to var_decl.
  ///
  /// @param v the string to test.
  ///
  /// @return true if @v is in the map.
  bool
  var_is_in_map(const string& v) const
  {return vars_map().find(v) != vars_map().end();}

  /// @return the vector of regex_t* describing the set of functions
  /// to suppress from the function public decl table.
  vector<regex_t_sptr>&
  regex_fns_suppress()
  {
    if (r_fns_suppress.empty())
      {
	for (vector<string>::const_iterator i =
	       regex_patterns_fns_to_suppress.begin();
	     i != regex_patterns_fns_to_suppress.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      r_fns_suppress.push_back(r);
	  }
      }
    return r_fns_suppress;

  }

  /// @return the vector of regex_t* describing the set of variables
  /// to suppress from the variable public decl table.
  vector<regex_t_sptr>&
  regex_vars_suppress()
  {
    if (r_vars_suppress.empty())
      {
	for (vector<string>::const_iterator i =
	       regex_patterns_vars_to_suppress.begin();
	     i != regex_patterns_vars_to_suppress.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      r_vars_suppress.push_back(r);
	  }
      }
    return r_vars_suppress;
  }

  /// @return the vector of regex_t* describing the set of functions
  /// to keep into the function public decl table.  All the other functions
  /// not described by these regular expressions are not dropped from
  /// the public decl table.
  vector<regex_t_sptr>&
  regex_fns_keep()
  {
    if (r_fns_keep.empty())
      {
	for (vector<string>::const_iterator i =
	       regex_patterns_fns_to_keep.begin();
	     i != regex_patterns_fns_to_keep.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      r_fns_keep.push_back(r);
	  }
      }
    return r_fns_keep;
  }

  /// @return the vector of regex_t* describing the set of variables
  /// to keep into the variable public decl table.  All the other variabled
  /// not described by these regular expressions are not dropped from
  /// the public decl table.
  vector<regex_t_sptr>&
  regex_vars_keep()
  {
    if (r_vars_keep.empty())
      {
	for (vector<string>::const_iterator i =
	       regex_patterns_vars_to_keep.begin();
	     i != regex_patterns_vars_to_keep.end();
	     ++i)
	  {
	    regex_t_sptr r = sptr_utils::build_sptr(new regex_t);
	    if (regcomp(r.get(), i->c_str(), REG_EXTENDED) == 0)
	      r_vars_keep.push_back(r);
	  }
      }
      return r_vars_keep;
  }

  /// Add a pair variable name / pointer var_decl to the map of string
  /// and pointer to var_decl.
  ///
  /// @param vn the name of the variable to add.
  ///
  /// @param v the variable to add.
  void
  add_var_to_map(const string& vn, const var_decl* v)
  {vars_map()[vn] = v;}

  /// Add a given function to the list of functions that are supposed
  /// to end up in the function public decl table.
  ///
  /// Note that this function applies regular expressions supposed to
  /// describe the set of functions to be dropped from the public decl
  /// table and then drop the functions matched.  It then applies
  /// regular expressions supposed to describe the set of functions to
  /// be kept into the public decl table and then keeps the functions that
  /// match and drops the functions that don't.
  ///
  /// @param fn the function to add to the public decl table, if it
  /// complies with the regular expressions that might have been
  /// specified by the client code.
  void
  add_fn_to_wip_fns(function_decl* fn)
  {
    string frep = fn->get_qualified_name();
    bool keep = true;
    for (vector<regex_t_sptr>::const_iterator i =
	   regex_fns_suppress().begin();
	 i != regex_fns_suppress().end();
	 ++i)
      if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == 0)
	{
	  keep = false;
	  break;
	}

    if (keep)
      {
	for (vector<regex_t_sptr>::const_iterator i =
	       regex_fns_keep().begin();
	     i != regex_fns_keep().end();
	     ++i)
	  if (regexec(i->get(), frep.c_str(), 0, NULL, 0) == REG_NOMATCH)
	    {
	      keep = false;
	      break;
	    }
      }

    if (keep)
      {
	wip_fns.push_back(fn);
	++wip_fns_size;
      }
  }

  /// Add a given variable to the list of variables that are supposed
  /// to end up in the variable public decl table.
  ///
  /// Note that this variable applies regular expressions supposed to
  /// describe the set of variables to be dropped from the public decl
  /// table and then drop the variables matched.  It then applies
  /// regular expressions supposed to describe the set of variables to
  /// be kept into the public decl table and then keeps the variables that
  /// match and drops the variables that don't.
  ///
  /// @param var the var to add to the public decl table, if it complies
  /// with the regular expressions that might have been specified by
  /// the client code.
  void
  add_var_to_wip_vars(var_decl* var)
  {
    string vrep = var->get_qualified_name();
    bool keep = true;
    for (vector<regex_t_sptr>::const_iterator i =
	   regex_vars_suppress().begin();
	 i != regex_vars_suppress().end();
	 ++i)
      if (regexec(i->get(), vrep.c_str(), 0, NULL, 0) == 0)
	{
	  keep = false;
	  break;
	}

    if (keep)
      {
	for (vector<regex_t_sptr>::const_iterator i =
	       regex_vars_keep().begin();
	     i != regex_vars_keep().end();
	     ++i)
	  if (regexec(i->get(), vrep.c_str(), 0, NULL, 0) == REG_NOMATCH)
	    {
	      keep = false;
	      break;
	    }
      }

    if (keep)
      {
	wip_vars.push_back(var);
	++wip_vars_size;
      }
  }

  /// This function is called while visiting a @ref function_decl IR
  /// node of a translation unit.
  ///
  /// Add the function to the public decl table being constructed (WIP
  /// meaning work in progress).
  ///
  /// @param fn the function being visited.
  ///
  /// @return true if the traversal of the tree should continue, false
  /// otherwise.
  ///
  /// @return true if the traversal of the tree should continue, false
  /// otherwise.
  bool
  visit(function_decl* fn)
  {
    add_fn_to_wip_fns(fn);
    return true;
  }

  /// This function is called while visiting a @ref var_decl IR node
  /// of a translation unit.
  ///
  /// Add the variable to the public decl table being constructed (WIP
  /// meaning work in progress).
  ///
  /// @param var the variable being visited.
  ///
  /// @return true if the traversal of the tree should continue, false
  /// otherwise.
  bool
  visit(var_decl* var)
  {
    if (!is_data_member(var)
	|| get_member_is_static(var))
      add_var_to_wip_vars(var);
    return true;
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
    first_name = first->get_linkage_name();
    if (first_name.empty())
      first_name = first->get_name();
    assert(!first_name.empty());

    second_name = second->get_linkage_name();
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
    first_name = first->get_linkage_name();
    if (first_name.empty())
      {
	first_name = first->get_pretty_representation();
	second_name = second->get_pretty_representation();
	assert(!second_name.empty());
      }
    assert(!first_name.empty());

    if (second_name.empty())
      second_name = second->get_linkage_name();

    if (second_name.empty())
      {
	second_name = second->get_pretty_representation();
	first_name = first->get_pretty_representation();
	assert(!first_name.empty());
      }
    assert(!second_name.empty());

    return first_name < second_name;
  }
};

/// Build the public decl tables for the corpus.  That is, walk all the
/// functions of all translation units of the corpus, stuff them in a
/// vector and sort the vector.  Likewise for the variables.
void
corpus::priv::build_public_decl_table()
{
  symtab_build_visitor_type v(fns, vars,
			      regex_patterns_fns_to_suppress,
			      regex_patterns_vars_to_suppress,
			      regex_patterns_fns_to_keep,
			      regex_patterns_vars_to_keep);

  for (translation_units::iterator i = members.begin();
       i != members.end();
       ++i)
      (*i)->traverse(v);

  fns.reserve(v.wip_fns_size);
  for (list<function_decl*>::iterator i = v.wip_fns.begin();
       i != v.wip_fns.end();
       ++i)
    {
      string n = v.build_id(*i);
      assert(!n.empty());

      if (origin_ == DWARF_ORIGIN
	  && !(*i)->get_is_in_public_symbol_table())
	// The symbol for this function is not exported so let's drop it.
	continue;

      if (!v.fn_is_in_map(n))
	{
	  assert((origin_ == DWARF_ORIGIN
		  && (*i)->get_is_in_public_symbol_table())
		 || origin_ != DWARF_ORIGIN);
	  fns.push_back(*i);
	  v.add_fn_to_map(n, *i);
	}
    }
  v.wip_fns.clear();
  v.wip_fns_size = 0;

  vars.reserve(v.wip_vars_size);
  for (list<var_decl*>::iterator i = v.wip_vars.begin();
       i != v.wip_vars.end();
       ++i)
    {
      string n = v.build_id(*i);
      assert(!n.empty());

      if (origin_ == DWARF_ORIGIN
	  && !(*i)->get_is_in_public_symbol_table())
	// The symbol for this variable is not exported so let's drop it.
	continue;

      if (!v.var_is_in_map(n))
	{
	  assert((origin_ == DWARF_ORIGIN
		  && (*i)->get_is_in_public_symbol_table())
		 || origin_ != DWARF_ORIGIN);
	  vars.push_back(*i);
	  v.add_var_to_map(n, *i);
	}
    }
  v.wip_vars.clear();
  v.wip_vars_size = 0;

  func_comp fc;
  std::sort(fns.begin(), fns.end(), fc);

  var_comp vc;
  std::sort(vars.begin(), vars.end(), vc);

  is_public_decl_table_built = true;
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

/// Getter for the origin of the corpus.
///
/// @return the origin of the corpus.
corpus::origin
corpus::get_origin() const
{return priv_->origin_;}

/// Setter for the origin of the corpus.
///
/// @param o the new origin for the corpus.
void
corpus::set_origin(origin o)
{priv_->origin_ = o;}

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
///
/// @param path the new file path to assciate to the current corpus.
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

/// Setter of the function symbols map.
///
/// @param map a shared pointer to the new function symbols map.
void
corpus::set_fun_symbol_map(string_elf_symbols_map_sptr map)
{priv_->fun_symbol_map = map;}

/// Setter of the variable symbols map.
///
/// @param map a shared pointer to the new variable symbols map.
void
corpus::set_var_symbol_map(string_elf_symbols_map_sptr map)
{priv_->var_symbol_map = map;}

/// Getter for the function symbols map.
///
/// @return a shared pointer to the function symbols map.
const string_elf_symbols_map_sptr
corpus::get_fun_symbol_map_sptr() const
{return priv_->fun_symbol_map;}

/// Getter for the function symbols map.
///
/// @return a reference to the function symbols map.
const string_elf_symbols_map_type&
corpus::get_fun_symbol_map() const
{return *get_fun_symbol_map_sptr();}

/// Getter for the variable symbols map.
///
/// @return a shared pointer to the variable symbols map.
const string_elf_symbols_map_sptr
corpus::get_var_symbol_map_sptr() const
{return priv_->var_symbol_map;}

/// Getter for the variable symbols map.
///
/// @return a reference to the variabl symbols map.
const string_elf_symbols_map_type&
corpus::get_var_symbol_map() const
{return *get_var_symbol_map_sptr();}

/// Look in the function symbols map for a symbol with a given name.
///
/// @param n the name of the symbol to look for.
///
/// return the first symbol with the name @p n.
const elf_symbol_sptr
corpus::lookup_function_symbol(const string& n) const
{
  if (!get_fun_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_fun_symbol_map_sptr()->find(n);
  if ( it == get_fun_symbol_map_sptr()->end())
    return elf_symbol_sptr();
  return it->second[0];
}

/// Look in the function symbols map for a symbol with a given name.
///
/// @param symbol_name the name of the symbol to look for.
///
/// @param symbol_version the version of the symbol to look for.
///
/// return the symbol with the name @p symbol_name and with the name
/// @p symbol_version, or nil if no symbol has been found with that
/// name and version.
const elf_symbol_sptr
corpus::lookup_function_symbol(const string& symbol_name,
			       const string& symbol_version) const
{
  if (!get_fun_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_fun_symbol_map_sptr()->find(symbol_name);
  if ( it == get_fun_symbol_map_sptr()->end())
    return elf_symbol_sptr();

  for (elf_symbols::const_iterator s = it->second.begin();
       s != it->second.end();
       ++s)
    if ((*s)->get_version().str() == symbol_version)
      return *s;
  return elf_symbol_sptr();
}

/// Look in the variable symbols map for a symbol with a given name.
///
/// @param n the name of the symbol to look for.
///
/// return the first symbol with the name @p n.
const elf_symbol_sptr
corpus::lookup_variable_symbol(const string& n) const
{
    if (!get_var_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_var_symbol_map_sptr()->find(n);
  if ( it == get_var_symbol_map_sptr()->end())
    return elf_symbol_sptr();
  return it->second[0];
}

/// Look in the variable symbols map for a symbol with a given name.
///
/// @param symbol_name the name of the symbol to look for.
///
/// @param symbol_version the version of the symbol to look for.
///
/// return the first symbol with the name @p n.
const elf_symbol_sptr
corpus::lookup_variable_symbol(const string& symbol_name,
			       const string& symbol_version) const
{
    if (!get_var_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_var_symbol_map_sptr()->find(symbol_name);
  if ( it == get_var_symbol_map_sptr()->end())
    return elf_symbol_sptr();

  for (elf_symbols::const_iterator s = it->second.begin();
       s != it->second.end();
       ++s)
    if ((*s)->get_version().str() == symbol_version)
      return *s;
  return elf_symbol_sptr();
}

/// Build and return the functions public decl table of the current corpus.
///
/// The function public decl tables is a vector of all the functions and
/// member functions found in the current corpus.  This vector is
/// built the first time this function is called.
///
/// Note that the caller can suppress some functions from the vector
/// supplying regular expressions describing the set of functions she
/// want to see removed from the public decl table by populating the vector
/// of regular expressions returned by
/// corpus::get_regex_patterns_of_fns_to_suppress().
///
/// @return the vector of functions of the public decl table.  The
/// functions are sorted using their mangled name or name if they
/// don't have mangle names.
const corpus::functions&
corpus::get_functions() const
{
  if (!priv_->is_public_decl_table_built)
    priv_->build_public_decl_table();
  assert(priv_->is_public_decl_table_built);

  return priv_->fns;
}

/// Build and return the public decl table of the global variables of the
/// current corpus.
///
/// The variable public decls table is a vector of all the public global
/// variables and static member variables found in the current corpus.
/// This vector is built the first time this function is called.
///
/// Note that the caller can suppress some variables from the vector
/// supplying regular expressions describing the set of variables she
/// wants to see removed from the public decl table by populating the vector
/// of regular expressions returned by
/// corpus::get_regex_patterns_of_fns_to_suppress().
///
/// @return the vector of variables of the public decl table.  The
/// variables are sorted using their name.
const corpus::variables&
corpus::get_variables() const
{
  if (!priv_->is_public_decl_table_built)
    priv_->build_public_decl_table();
  assert(priv_->is_public_decl_table_built);

  return priv_->vars;
}

/// Accessor for the regex patterns describing the functions to drop
/// from the public decl table.
///
/// @return the regex patterns describing the functions to drop from
/// the public decl table.
vector<string>&
corpus::get_regex_patterns_of_fns_to_suppress()
{return priv_->regex_patterns_fns_to_suppress;}

/// Accessor for the regex patterns describing the functions to drop
/// from the public decl table.
///
/// @return the regex patterns describing the functions to drop from
/// the public decl table.
const vector<string>&
corpus::get_regex_patterns_of_fns_to_suppress() const
{return priv_->regex_patterns_fns_to_suppress;}

/// Accessor for the regex patterns describing the variables to drop
/// from the public decl table.
///
/// @return the regex patterns describing the variables to drop from
/// the public decl table.
vector<string>&
corpus::get_regex_patterns_of_vars_to_suppress()
{return priv_->regex_patterns_vars_to_suppress;}

/// Accessor for the regex patterns describing the variables to drop
/// from the public decl table.
///
/// @return the regex patterns describing the variables to drop from
/// the public decl table.
const vector<string>&
corpus::get_regex_patterns_of_vars_to_suppress() const
{return priv_->regex_patterns_vars_to_suppress;}

/// Accessor for the regex patterns describing the functions to keep
/// into the public decl table.  The other functions not matches by these
/// regexes are dropped from the public decl table.
///
/// @return the regex patterns describing the functions to keep into
/// the public decl table.
vector<string>&
corpus::get_regex_patterns_of_fns_to_keep()
{return priv_->regex_patterns_fns_to_keep;}

/// Accessor for the regex patterns describing the functions to keep
/// into the public decl table.  The other functions not matches by these
/// regexes are dropped from the public decl table.
///
/// @return the regex patterns describing the functions to keep into
/// the public decl table.
const vector<string>&
corpus::get_regex_patterns_of_fns_to_keep() const
{return priv_->regex_patterns_fns_to_keep;}

/// Accessor for the regex patterns describing the variables to keep
/// into the public decl table.  The other variables not matches by these
/// regexes are dropped from the public decl table.
///
/// @return the regex patterns describing the variables to keep into
/// the public decl table.
vector<string>&
corpus::get_regex_patterns_of_vars_to_keep()
{return priv_->regex_patterns_vars_to_keep;}

/// Accessor for the regex patterns describing the variables to keep
/// into the public decl table.  The other variables not matches by these
/// regexes are dropped from the public decl table.
///
/// @return the regex patterns describing the variables to keep into
/// the public decl table.
const vector<string>&
corpus::get_regex_patterns_of_vars_to_keep() const
{return priv_->regex_patterns_vars_to_keep;}

} // end namespace ir
}// end namespace abigail
