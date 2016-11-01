// -*- mode: C++ -*-
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

/// @file

#include "config.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <ext/stdio_filebuf.h>
#include <stdexcept>
#include <algorithm>
#include <tr1/unordered_map>

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-sptr-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-writer.h"
#include "abg-tools-utils.h"

#if WITH_ZIP_ARCHIVE
#include "abg-libzip-utils.h"
#endif

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

#include "abg-corpus-priv.h"

namespace abigail
{

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

/// Constructor of @ref corpus::exported_decls_builder.
///
/// @param fns a reference to the vector of exported functions.
///
/// @param vars a reference to the vector of exported variables.
///
/// @param fns_suppress_regexps the regular expressions that designate
/// the functions to suppress from the exported functions set.
///
/// @param vars_suppress_regexps the regular expressions that designate
/// the variables to suppress from the exported variables set.
///
/// @param fns_keep_regexps the regular expressions that designate the
/// functions to keep in the exported functions set.
///
/// @param fns_keep_regexps the regular expressions that designate the
/// functions to keep in the exported functions set.
///
/// @param vars_keep_regexps the regular expressions that designate
/// the variables to keep in the exported variables set.
///
/// @param sym_id_of_fns_to_keep the IDs of the functions to keep in
/// the exported functions set.
///
/// @param sym_id_of_vars_to_keep the IDs of the variables to keep in
/// the exported variables set.
corpus::exported_decls_builder
::exported_decls_builder(functions&	fns,
			 variables&	vars,
			 strings_type&	fns_suppress_regexps,
			 strings_type&	vars_suppress_regexps,
			 strings_type&	fns_keep_regexps,
			 strings_type&	vars_keep_regexps,
			 strings_type&	sym_id_of_fns_to_keep,
			 strings_type&	sym_id_of_vars_to_keep)
  : priv_(new priv(fns, vars,
		   fns_suppress_regexps,
		   vars_suppress_regexps,
		   fns_keep_regexps,
		   vars_keep_regexps,
		   sym_id_of_fns_to_keep,
		   sym_id_of_vars_to_keep))
{
}

/// Getter for the reference to the vector of exported functions.
/// This vector is shared with with the @ref corpus.  It's where the
/// set of exported function is ultimately stored.
///
/// @return a reference to the vector of exported functions.
const corpus::functions&
corpus::exported_decls_builder::exported_functions() const
{return priv_->fns_;}

/// Getter for the reference to the vector of exported functions.
/// This vector is shared with with the @ref corpus.  It's where the
/// set of exported function is ultimately stored.
///
/// @return a reference to the vector of exported functions.
corpus::functions&
corpus::exported_decls_builder::exported_functions()
{return priv_->fns_;}

/// Getter for the reference to the vector of exported variables.
/// This vector is shared with with the @ref corpus.  It's where the
/// set of exported variable is ultimately stored.
///
/// @return a reference to the vector of exported variables.
const corpus::variables&
corpus::exported_decls_builder::exported_variables() const
{return priv_->vars_;}

/// Getter for the reference to the vector of exported variables.
/// This vector is shared with with the @ref corpus.  It's where the
/// set of exported variable is ultimately stored.
///
/// @return a reference to the vector of exported variables.
corpus::variables&
corpus::exported_decls_builder::exported_variables()
{return priv_->vars_;}

/// Consider at all the tunables that control wether a function should
/// be added to the set of exported function and if it fits in, add
/// the function to that set.
///
/// @param fn the function to add the set of exported functions.
void
corpus::exported_decls_builder::maybe_add_fn_to_exported_fns(function_decl* fn)
{
  if (!fn->get_is_in_public_symbol_table())
    return;

  const string& fn_id = priv_->get_id(*fn);
  assert(!fn_id.empty());

  if (priv_->fn_is_in_id_fns_map(fn))
    return;

  if (priv_->keep_wrt_id_of_fns_to_keep(fn)
      && priv_->keep_wrt_regex_of_fns_to_suppress(fn)
      && priv_->keep_wrt_regex_of_fns_to_keep(fn))
    priv_->add_fn_to_exported(fn);
}

/// Consider at all the tunables that control wether a variable should
/// be added to the set of exported variable and if it fits in, add
/// the variable to that set.
///
/// @param fn the variable to add the set of exported variables.
void
corpus::exported_decls_builder::maybe_add_var_to_exported_vars(var_decl* var)
{
  if (!var->get_is_in_public_symbol_table())
    return;

  const string& var_id = priv_->get_id(*var);
  assert(!var_id.empty());

  if (priv_->var_id_is_in_id_var_map(var_id))
    return;

  if (priv_->keep_wrt_id_of_vars_to_keep(var)
      && priv_->keep_wrt_regex_of_vars_to_suppress(var)
      && priv_->keep_wrt_regex_of_vars_to_keep(var))
    priv_->add_var_to_exported(var);
}

// </corpus::exported_decls_builder>

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


/// A comparison functor to compare elf_symbols for the purpose of
/// sorting.
struct comp_elf_symbols_functor
{
  bool
  operator()(const elf_symbol& l,
	     const elf_symbol& r) const
  {return l.get_id_string() < r.get_id_string();}

  bool
  operator()(const elf_symbol_sptr l,
	     const elf_symbol_sptr r) const
  {return operator()(*l, *r);}
}; // end struct comp_elf_symbols_functor


// <corpus stuff>


/// Build the tables of symbols that are not referenced by any
/// function or variables of corpus::get_functions() or
/// corpus::get_variables().
///
/// Note that this function considers the list of function and
/// variable symbols to keep, that is provided by
/// corpus::get_sym_ids_of_fns_to_keep() and
/// corpus::get_sym_ids_of_vars_to_keep().  If a given unreferenced
/// function or variable symbol is not in the list of variable and
/// function symbols to keep, then that symbol is dropped and will not
/// be part of the resulting table of unreferenced symbol that is
/// built.
///
/// The built tables are accessible from
/// corpus::get_unreferenced_function_symbols() and
/// corpus::get_unreferenced_variable_symbols().
void
corpus::priv::build_unreferenced_symbols_tables()
{
  unordered_map<string, bool> refed_funs, refed_vars;
  elf_symbol_sptr sym;

  for (vector<function_decl*>::const_iterator f = fns.begin();
       f != fns.end();
       ++f)
    if (sym = (*f)->get_symbol())
      {
	refed_funs[sym->get_id_string()] = true;
	for (elf_symbol_sptr a = sym->get_next_alias();
	     a && !a->is_main_symbol();
	     a = a->get_next_alias())
	  refed_funs[a->get_id_string()] = true;
      }

  for (vector<var_decl*>::const_iterator v = vars.begin();
       v != vars.end();
       ++v)
    if (sym = (*v)->get_symbol())
      {
	refed_vars[sym->get_id_string()] = true;
	for (elf_symbol_sptr a = sym->get_next_alias();
	     a && !a->is_main_symbol();
	     a = a->get_next_alias())
	  refed_vars[a->get_id_string()] = true;
      }

  if (fun_symbol_map)
    {
      // Let's assume that the size of the unreferenced symbols vector
      // is roughly smaller than the size of the symbol table.
      unrefed_fun_symbols.reserve(fun_symbol_map->size());
      for (string_elf_symbols_map_type::const_iterator i
	     = fun_symbol_map->begin();
	   i != fun_symbol_map->end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end();
	     ++s)
	  {
	    string sym_id = (*s)->get_id_string();
	    if (refed_funs.find(sym_id) == refed_funs.end())
	      {
		bool keep = sym_id_fns_to_keep.empty() ? true : false;
		for (vector<string>::const_iterator i =
		       sym_id_fns_to_keep.begin();
		     i != sym_id_fns_to_keep.end();
		     ++i)
		  {
		    if (*i == sym_id)
		      {
			keep = true;
			break;
		      }
		  }
		if (keep)
		  unrefed_fun_symbols.push_back(*s);
	      }
	  }

      comp_elf_symbols_functor comp;
      std::sort(unrefed_fun_symbols.begin(),
		unrefed_fun_symbols.end(),
		comp);
    }

  if (var_symbol_map)
    {
      // Let's assume that the size of the unreferenced symbols vector
      // is roughly smaller than the size of the symbol table.
      unrefed_var_symbols.reserve(var_symbol_map->size());
      for (string_elf_symbols_map_type::const_iterator i
	     = var_symbol_map->begin();
	   i != var_symbol_map->end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end();
	     ++s)
	  {
	    string sym_id = (*s)->get_id_string();
	    if (refed_vars.find(sym_id) == refed_vars.end())
	      {
		bool keep = sym_id_vars_to_keep.empty() ? true : false;;
		for (vector<string>::const_iterator i =
		       sym_id_vars_to_keep.begin();
		     i != sym_id_vars_to_keep.end();
		     ++i)
		  {
		    if (*i == sym_id)
		      {
			keep = true;
			break;
		      }
		  }
		if (keep)
		  unrefed_var_symbols.push_back(*s);
	      }
	  }

      comp_elf_symbols_functor comp;
      std::sort(unrefed_var_symbols.begin(),
		unrefed_var_symbols.end(),
		comp);
    }
}

/// Record a canonical type which has been computed for the current
/// corpus.
///
/// This function associates the name of the canonical type to the
/// canonical type, so that further lookup of this canonical using
/// corpus::lookup_canonical_type succeeds in a faster manner.
///
/// @param t a canonical type for a type present in the current
/// corpus.  This type can later be retrieved from the current corpus
/// using corpus::lookup_canonical_type().
void
corpus::record_canonical_type(const type_base_sptr& t) const
{
  string n = get_pretty_representation(t, /*internal=*/true);
  priv_->canonical_types_[n] = t;
}

/// Retrieve a canonical type present in the current corpus, from its
/// name.
///
/// Note that the type must have been cached inside the corpus by an
/// invocation to corpus::record_canonical_type().
///
/// @param qualified_name the name of the canonical type to retrieve.
/// It must have been computed using the function
/// get_pretty_representation(type_base_sptr, true).
///
/// @return the canonical type which name is @p qualified_name and
/// which has been previously cached inside the corpus by an
/// invocation to corpus::record_canonical_type().
type_base_sptr
corpus::lookup_canonical_type(const string& qualified_name) const
{
  unordered_map<string, type_base_sptr>::const_iterator i =
    priv_->canonical_types_.find(qualified_name);
  if (i == priv_->canonical_types_.end())
    return type_base_sptr();
  return i->second;
}

/// Constructor of the @ref corpus type.
///
/// @param env the environment of the corpus.
///
/// @param path the path to the file containing the ABI corpus.
corpus::corpus(ir::environment* env, const string& path)
{priv_.reset(new priv(path, env));}

/// Getter of the enviroment of the corpus.
///
/// @return the environment of this corpus.
const environment*
corpus::get_environment() const
{return priv_->env;}

/// Getter of the enviroment of the corpus.
///
/// @return the environment of this corpus.
environment*
corpus::get_environment()
{return priv_->env;}

/// Setter of the environment of this corpus.
///
/// @param e the new environment.
void
corpus::set_environment(environment* e) const
{priv_->env = e;}

/// Add a translation unit to the current ABI Corpus. Next time
/// corpus::save is called, all the translation unit that got added to
/// the corpus are going to be serialized on disk in the file
/// associated to the current corpus.
///
/// @param tu the new translation unit to add.
void
corpus::add(const translation_unit_sptr tu)
{
  if (!tu->get_environment())
    tu->set_environment(get_environment());

  assert(tu->get_environment() == get_environment());

  priv_->members.push_back(tu);

  tu->set_corpus(this);
}

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
///
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

/// Getter of the needed property of the corpus.
///
/// This property is meaningful for, e.g, corpora built from ELF
/// shared library files.  In that case, this is a vector of names of
/// dependencies of the ELF shared library file.
///
/// @return the vector of dependencies needed by this corpus.
const vector<string>&
corpus::get_needed() const
{return priv_->needed;}

/// Setter of the needed property of the corpus.
///
/// This property is meaningful for, e.g, corpora built from ELF
/// shared library files.  In that case, this is a vector of names of
/// dependencies of the ELF shared library file.
///
/// @param needed the new vector of dependencies needed by this
/// corpus.
void
corpus::set_needed(const vector<string>& needed)
{priv_->needed = needed;}

/// Getter for the soname property of the corpus.
///
/// This property is meaningful for, e.g, corpora built from ELF
/// shared library files.  In that case, this is the shared object
/// name exported by the shared library.
///
/// @return the soname property of the corpus.
const string&
corpus::get_soname()
{return priv_->soname;}

/// Setter for the soname property of the corpus.
///
/// This property is meaningful for, e.g, corpora built from ELF
/// shared library files.  In that case, this is the shared object
/// name exported by the shared library.
///
/// @param soname the new soname property of the corpus.
void
corpus::set_soname(const string& soname)
{priv_->soname = soname;}

/// Getter for the architecture name of the corpus.
///
/// This property is meaningful for e.g, corpora built from ELF shared
/// library files.  In that case, this is a string representation of
/// the Elf{32,64}_Ehdr::e_machine field.
///
/// @return the architecture name string.
const string&
corpus::get_architecture_name()
{return priv_->architecture_name;}

/// Setter for the architecture name of the corpus.
///
/// This property is meaningful for e.g, corpora built from ELF shared
/// library files.  In that case, this is a string representation of
/// the Elf{32,64}_Ehdr::e_machine field.
///
/// @param arch the architecture name string.
void
corpus::set_architecture_name(const string& arch)
{priv_->architecture_name = arch;}

/// Tests if the corpus contains no translation unit.
///
/// @return true if the corpus contains no translation unit.
bool
corpus::is_empty() const
{
  return (priv_->members.empty()
	  && priv_->fun_symbol_map
	  && priv_->fun_symbol_map->empty()
	  && priv_->var_symbol_map
	  && priv_->var_symbol_map->empty()
	  && priv_->soname.empty()
	  && priv_->needed.empty());
}

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

/// Setter for the map of function symbols that are undefined in this
/// corpus.
///
/// @param map a new map for function symbols not defined in this
/// corpus.  The key of the map is the name of the function symbol.
/// The value is a vector of all the function symbols that have the
/// same name.
void
corpus::set_undefined_fun_symbol_map(string_elf_symbols_map_sptr map)
{priv_->undefined_fun_symbol_map = map;}

/// Setter of the variable symbols map.
///
/// @param map a shared pointer to the new variable symbols map.
void
corpus::set_var_symbol_map(string_elf_symbols_map_sptr map)
{priv_->var_symbol_map = map;}

/// Setter for the map of variable symbols that are undefined in this
/// corpus.
///
/// @param map a new map for variable symbols not defined in this
/// corpus.  The key of the map is the name of the variable symbol.
/// The value is a vector of all the variable symbols that have the
/// same name.
void
corpus::set_undefined_var_symbol_map(string_elf_symbols_map_sptr map)
{priv_->undefined_var_symbol_map = map;}

/// Getter for the function symbols map.
///
/// @return a shared pointer to the function symbols map.
const string_elf_symbols_map_sptr
corpus::get_fun_symbol_map_sptr() const
{
  if (!priv_->fun_symbol_map)
    priv_->fun_symbol_map.reset(new string_elf_symbols_map_type);
  return priv_->fun_symbol_map;
}

/// Getter for the function symbols map.
///
/// @return a reference to the function symbols map.
const string_elf_symbols_map_type&
corpus::get_fun_symbol_map() const
{return *get_fun_symbol_map_sptr();}

/// Getter for the map of function symbols that are undefined in this
/// corpus.
///
/// @return the map of function symbols not defined in this corpus.
/// The key of the map is the name of the function symbol.  The value
/// is a vector of all the function symbols that have the same name.
const string_elf_symbols_map_sptr
corpus::get_undefined_fun_symbol_map_sptr() const
{return priv_->undefined_fun_symbol_map;}

/// Getter for the map of function symbols that are undefined in this
/// corpus.
///
/// @return the map of function symbols not defined in this corpus.
/// The key of the map is the name of the function symbol.  The value
/// is a vector of all the function symbols that have the same name.
const string_elf_symbols_map_type&
corpus::get_undefined_fun_symbol_map() const
{return *get_undefined_fun_symbol_map_sptr();}

/// Functor to sort instances of @ref elf_symbol.
struct elf_symbol_comp_functor
{

  /// Return true if the first argument is less than the second one.
  ///
  /// @param l the first parameter to consider.
  ///
  /// @param r the second parameter to consider.
  ///
  /// @return true if @p l is less than @p r
  bool
  operator()(elf_symbol& l, elf_symbol& r)
  {return (l.get_id_string() < r.get_id_string());}

  /// Return true if the first argument is less than the second one.
  ///
  /// @param l the first parameter to consider.
  ///
  /// @param r the second parameter to consider.
  ///
  /// @return true if @p l is less than @p r
  bool
  operator()(elf_symbol* l, elf_symbol* r)
  {return operator()(*l, *r);}

  /// Return true if the first argument is less than the second one.
  ///
  /// @param l the first parameter to consider.
  ///
  /// @param r the second parameter to consider.
  ///
  /// @return true if @p l is less than @p r
  bool
  operator()(elf_symbol_sptr l, elf_symbol_sptr r)
  {return operator()(*l, *r);}
}; // end struct elf_symbol_comp_functor

/// Return a sorted vector of function symbols for this corpus.
///
/// Note that the first time this function is called, the symbols are
/// sorted and cached.  Subsequent invocations of this function return
/// the cached vector that was built previously.
///
/// @return the sorted list of function symbols.
const elf_symbols&
corpus::get_sorted_fun_symbols() const
{
  if (priv_->sorted_fun_symbols.empty()
      && !get_fun_symbol_map().empty())
    {
      priv_->sorted_fun_symbols.reserve(get_fun_symbol_map().size());
      for (string_elf_symbols_map_type::const_iterator i =
	     get_fun_symbol_map().begin();
	   i != get_fun_symbol_map().end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end();
	     ++s)
	  priv_->sorted_fun_symbols.push_back(*s);

      elf_symbol_comp_functor comp;
      std::sort(priv_->sorted_fun_symbols.begin(),
		priv_->sorted_fun_symbols.end(),
		comp);
    }
  return priv_->sorted_fun_symbols;
}

/// Getter for a sorted vector of the function symbols undefined in
/// this corpus.
///
/// @return a vector of the function symbols undefined in this corpus,
/// sorted by name and then version.
const elf_symbols&
corpus::get_sorted_undefined_fun_symbols() const
{
  if (priv_->sorted_undefined_fun_symbols.empty()
      && !get_undefined_fun_symbol_map().empty())
    {
      priv_->sorted_undefined_fun_symbols.reserve
	(get_undefined_fun_symbol_map().size());
      for (string_elf_symbols_map_type::const_iterator i =
	     get_undefined_fun_symbol_map().begin();
	   i != get_undefined_fun_symbol_map().end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end();
	     ++s)
	  priv_->sorted_undefined_fun_symbols.push_back(*s);

      elf_symbol_comp_functor comp;
      std::sort(priv_->sorted_undefined_fun_symbols.begin(),
		priv_->sorted_undefined_fun_symbols.end(),
		comp);
    }
  return priv_->sorted_undefined_fun_symbols;
}

/// Getter for the variable symbols map.
///
/// @return a shared pointer to the variable symbols map.
const string_elf_symbols_map_sptr
corpus::get_var_symbol_map_sptr() const
{
  if (!priv_->var_symbol_map)
    priv_->var_symbol_map.reset(new string_elf_symbols_map_type);
  return priv_->var_symbol_map;
}

/// Getter for the variable symbols map.
///
/// @return a reference to the variabl symbols map.
const string_elf_symbols_map_type&
corpus::get_var_symbol_map() const
{return *get_var_symbol_map_sptr();}

/// Getter for the map of variable symbols that are undefined in this
/// corpus.
///
/// @return the map of variable symbols not defined in this corpus.
/// The key of the map is the name of the variable symbol.  The value
/// is a vector of all the variable symbols that have the same name.
const string_elf_symbols_map_sptr
corpus::get_undefined_var_symbol_map_sptr() const
{return priv_->undefined_var_symbol_map;}

/// Getter for the map of variable symbols that are undefined in this
/// corpus.
///
/// @return the map of variable symbols not defined in this corpus.
/// The key of the map is the name of the variable symbol.  The value
/// is a vector of all the variable symbols that have the same name.
const string_elf_symbols_map_type&
corpus::get_undefined_var_symbol_map() const
{return *get_undefined_var_symbol_map_sptr();}

/// Getter for the sorted vector of variable symbols for this corpus.
///
/// Note that the first time this function is called, it computes the
/// sorted vector, caches the result and returns it.  Subsequent
/// invocations of this function just return the cached vector.
///
/// @return the sorted vector of variable symbols for this corpus.
const elf_symbols&
corpus::get_sorted_var_symbols() const
{
  if (priv_->sorted_var_symbols.empty()
      && !get_var_symbol_map().empty())
    {
      priv_->sorted_var_symbols.reserve(get_var_symbol_map().size());
      for (string_elf_symbols_map_type::const_iterator i =
	     get_var_symbol_map().begin();
	   i != get_var_symbol_map().end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end(); ++s)
	  priv_->sorted_var_symbols.push_back(*s);

      elf_symbol_comp_functor comp;
      std::sort(priv_->sorted_var_symbols.begin(),
		priv_->sorted_var_symbols.end(),
		comp);
    }
  return priv_->sorted_var_symbols;
}

/// Getter for a sorted vector of the variable symbols undefined in
/// this corpus.
///
/// @return a vector of the variable symbols undefined in this corpus,
/// sorted by name and then version.
const elf_symbols&
corpus::get_sorted_undefined_var_symbols() const
{
  if (priv_->sorted_undefined_var_symbols.empty()
      && !get_undefined_var_symbol_map().empty())
    {
      priv_->sorted_undefined_var_symbols.reserve
	(get_undefined_var_symbol_map().size());
      for (string_elf_symbols_map_type::const_iterator i =
	     get_undefined_var_symbol_map().begin();
	   i != get_undefined_var_symbol_map().end();
	   ++i)
	for (elf_symbols::const_iterator s = i->second.begin();
	     s != i->second.end(); ++s)
	  priv_->sorted_undefined_var_symbols.push_back(*s);

      elf_symbol_comp_functor comp;
      std::sort(priv_->sorted_undefined_var_symbols.begin(),
		priv_->sorted_undefined_var_symbols.end(),
		comp);
    }
  return priv_->sorted_undefined_var_symbols;
}

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

/// Look into a set of symbols and look for a symbol that has a given
/// version.
///
/// This is a sub-routine for corpus::lookup_function_symbol() and
/// corpus::lookup_variable_symbol().
///
/// @param version the version of the symbol to look for.
///
/// @param symbols the set of symbols to consider.
///
/// @return the symbol found, or nil if none was found.
static const elf_symbol_sptr
find_symbol_by_version(const elf_symbol::version& version,
		       const vector<elf_symbol_sptr>& symbols)
{
  if (version.is_empty())
    {
      // We are looing for a symbol with no version.

      // So first look for possible aliases with no version
      for (elf_symbols::const_iterator s = symbols.begin();
	   s != symbols.end();
	   ++s)
	if ((*s)->get_version().is_empty())
	  return *s;

      // Or, look for a version that is a default one!
      for (elf_symbols::const_iterator s = symbols.begin();
	   s != symbols.end();
	   ++s)
	if ((*s)->get_version().is_default())
	  return *s;
    }
  else
    // We are looking for a symbol with a particular defined version.
    for (elf_symbols::const_iterator s = symbols.begin();
	 s != symbols.end();
	 ++s)
      if ((*s)->get_version().str() == version.str())
	return *s;

  return elf_symbol_sptr();
}

/// Look in the function symbols map for a symbol with a given name.
///
/// @param symbol_name the name of the symbol to look for.
///
/// @param version the version of the symbol to look for.
///
/// return the symbol with name @p symbol_name and with version @p
/// version, or nil if no symbol has been found with that name and
/// version.
const elf_symbol_sptr
corpus::lookup_function_symbol(const string& symbol_name,
			       const elf_symbol::version& version) const
{
  if (!get_fun_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_fun_symbol_map_sptr()->find(symbol_name);
  if ( it == get_fun_symbol_map_sptr()->end())
    return elf_symbol_sptr();

  return find_symbol_by_version(version, it->second);
}

/// Look in the function symbols map for a symbol with the same name
/// and version as a given symbol.
///
/// @param symbol the symbol to look for.
///
/// return the symbol with the same name and version as @p symbol.
const elf_symbol_sptr
corpus::lookup_function_symbol(const elf_symbol& symbol) const
{return lookup_function_symbol(symbol.get_name(), symbol.get_version());}

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
/// return the first symbol with the name @p symbol_name and with
/// version @p version.
const elf_symbol_sptr
corpus::lookup_variable_symbol(const string& symbol_name,
			       const elf_symbol::version& version) const
{
    if (!get_var_symbol_map_sptr())
    return elf_symbol_sptr();

  string_elf_symbols_map_type::const_iterator it =
    get_var_symbol_map_sptr()->find(symbol_name);
  if ( it == get_var_symbol_map_sptr()->end())
    return elf_symbol_sptr();

  return find_symbol_by_version(version, it->second);
}

/// Look in the variable symbols map for a symbol with the same name
/// and version as a given symbol.
///
/// @param symbol the symbol to look for.
///
/// return the symbol with the same name and version as @p symbol.
const elf_symbol_sptr
corpus::lookup_variable_symbol(const elf_symbol& symbol) const
{return lookup_variable_symbol(symbol.get_name(), symbol.get_version());}

/// Return the functions public decl table of the current corpus.
///
/// The function public decl tables is a vector of all the functions
/// and member functions found in the current corpus.
///
/// Note that the caller can suppress some functions from the vector
/// supplying regular expressions describing the set of functions she
/// want to see removed from the public decl table by populating the
/// vector of regular expressions returned by
/// corpus::get_regex_patterns_of_fns_to_suppress().
///
/// @return the vector of functions of the public decl table.  The
/// functions are sorted using their mangled name or name if they
/// don't have mangle names.
const corpus::functions&
corpus::get_functions() const
{return priv_->fns;}

/// Lookup the function which has a given function ID.
///
/// Note that there can have been several functions with the same ID.
/// This is because debug info can declare the same function in
/// several different translation units.  Normally, all these function
/// should be equal.  But still, this function returns all these
/// functions.
///
/// @param id the ID of the function to lookup.  This ID must be
/// either the result of invoking function::get_id() of
/// elf_symbol::get_id_string().
///
/// @return the vector functions which ID is @p id, or nil if no
/// function with that ID was found.
const vector<function_decl*>*
corpus::lookup_functions(const string& id) const
{
  exported_decls_builder_sptr b = get_exported_decls_builder();
  str_fn_ptrs_map_type::const_iterator i =
    b->priv_->id_fns_map_.find(id);
  if (i == b->priv_->id_fns_map_.end())
    return 0;
  return &i->second;
}

/// Sort the set of functions exported by this corpus.
///
/// Normally, you shouldn't be calling this as the code that creates
/// the corpus for you should do it for you too.
void
corpus::sort_functions()
{
  func_comp fc;
  std::sort(priv_->fns.begin(), priv_->fns.end(), fc);
}

/// Return the public decl table of the global variables of the
/// current corpus.
///
/// The variable public decls table is a vector of all the public
/// global variables and static member variables found in the current
/// corpus.
///
/// Note that the caller can suppress some variables from the vector
/// supplying regular expressions describing the set of variables she
/// wants to see removed from the public decl table by populating the
/// vector of regular expressions returned by
/// corpus::get_regex_patterns_of_fns_to_suppress().
///
/// @return the vector of variables of the public decl table.  The
/// variables are sorted using their name.
const corpus::variables&
corpus::get_variables() const
{return priv_->vars;}

/// Sort the set of variables exported by this corpus.
///
/// Normally, you shouldn't be calling this as the code that creates
/// the corpus for you should do it for you too.
void
corpus::sort_variables()
{
  var_comp vc;
  std::sort(priv_->vars.begin(), priv_->vars.end(), vc);
}

/// Getter of the set of function symbols that are not referenced by
/// any function exported by the current corpus.
///
/// When the corpus has been created from an ELF library or program,
/// this function returns the set of function symbols not referenced
/// by any debug information.
///
/// @return the vector of function symbols not referenced by any
/// function exported by the current corpus.
const elf_symbols&
corpus::get_unreferenced_function_symbols() const
{
  if (priv_->unrefed_fun_symbols.empty()
      && priv_->unrefed_var_symbols.empty())
    priv_->build_unreferenced_symbols_tables();
  return priv_->unrefed_fun_symbols;
}

/// Getter of the set of variable symbols that are not referenced by
/// any variable exported by the current corpus.
///
/// When the corpus has been created from an ELF library or program,
/// this function returns the set of variable symbols not referenced
/// by any debug information.
///
/// @return the vector of variable symbols not referenced by any
/// variable exported by the current corpus.
const elf_symbols&
corpus::get_unreferenced_variable_symbols() const
{
    if (priv_->unrefed_fun_symbols.empty()
      && priv_->unrefed_var_symbols.empty())
    priv_->build_unreferenced_symbols_tables();
  return priv_->unrefed_var_symbols;
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

/// Getter for the vector of function symbol IDs to keep.
///
/// A symbol ID is a string made of the name of the symbol and its
/// version, separated by one or two '@'.
///
/// @return a vector of IDs of function symbols to keep.
vector<string>&
corpus::get_sym_ids_of_fns_to_keep()
{return priv_->sym_id_fns_to_keep;}

/// Getter for the vector of function symbol IDs to keep.
///
/// A symbol ID is a string made of the name of the symbol and its
/// version, separated by one or two '@'.
///
/// @return a vector of IDs of function symbols to keep.
const vector<string>&
corpus::get_sym_ids_of_fns_to_keep() const
{return priv_->sym_id_fns_to_keep;}

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

/// Getter for the vector of variable symbol IDs to keep.
///
/// A symbol ID is a string made of the name of the symbol and its
/// version, separated by one or two '@'.
///
/// @return a vector of IDs of variable symbols to keep.
vector<string>&
corpus::get_sym_ids_of_vars_to_keep()
{return priv_->sym_id_vars_to_keep;}

/// Getter for the vector of variable symbol IDs to keep.
///
/// A symbol ID is a string made of the name of the symbol and its
/// version, separated by one or two '@'.
///
/// @return a vector of IDs of variable symbols to keep.
const vector<string>&
corpus::get_sym_ids_of_vars_to_keep() const
{return priv_->sym_id_vars_to_keep;}

/// After the set of exported functions and variables have been built,
/// consider all the tunables that control that set and see if some
/// functions need to be removed from that set; if so, remove them.
void
corpus::maybe_drop_some_exported_decls()
{
  string sym_name, sym_version;

  vector<function_decl*> fns_to_keep;
  exported_decls_builder* b = get_exported_decls_builder().get();
  for (vector<function_decl*>::iterator f = priv_->fns.begin();
       f != priv_->fns.end();
       ++f)
    {
      if (b->priv_->keep_wrt_id_of_fns_to_keep(*f)
	  && b->priv_->keep_wrt_regex_of_fns_to_suppress(*f)
	  && b->priv_->keep_wrt_regex_of_fns_to_keep(*f))
	fns_to_keep.push_back(*f);
    }
  priv_->fns = fns_to_keep;

  vector<var_decl*> vars_to_keep;
  for (vector<var_decl*>::iterator v = priv_->vars.begin();
       v != priv_->vars.end();
       ++v)
    {
      if (b->priv_->keep_wrt_id_of_vars_to_keep(*v)
	  && b->priv_->keep_wrt_regex_of_vars_to_suppress(*v)
	  && b->priv_->keep_wrt_regex_of_vars_to_keep(*v))
	vars_to_keep.push_back(*v);
    }
  priv_->vars = vars_to_keep;
}

///  Getter for the object that is responsible for determining what
///  decls ought to be in the set of exported decls.
///
///  The object does have methods to add the decls to the set of
///  exported decls, right at the place where the corpus expects it,
///  so that there is no unnecessary copying involved.
///
///  @return a (smart) pointer to the instance of @ref
///  corpus::exported_decls_builder that is responsible for determine
///  what decls ought to be in the set of exported decls.
corpus::exported_decls_builder_sptr
corpus::get_exported_decls_builder() const
{
  if (!priv_->exported_decls_builder)
    {
      priv_->exported_decls_builder.reset
	(new exported_decls_builder(priv_->fns,
				    priv_->vars,
				    priv_->regex_patterns_fns_to_suppress,
				    priv_->regex_patterns_vars_to_suppress,
				    priv_->regex_patterns_fns_to_keep,
				    priv_->regex_patterns_vars_to_keep,
				    priv_->sym_id_fns_to_keep,
				    priv_->sym_id_vars_to_keep));
    }
  return priv_->exported_decls_builder;
}

/// Lookup a type definition in all the translation units of a given
/// ABI corpus.
///
/// @param @param qn the fully qualified name of the type to lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
const decl_base_sptr
lookup_type_in_corpus(const string& qn, const corpus& abi_corpus)
{
  decl_base_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_type_in_translation_unit(qn, **tu)))
      break;

  return result;
}

/// Lookup a class type definition in all the translation units of a
/// given ABI corpus.
///
/// @param @param qn the fully qualified name of the class type to lookup.
///
/// @param abi_corpus the ABI corpus which to look the type up in.
///
/// @return the type definition if any was found, or a NULL pointer.
const class_decl_sptr
lookup_class_type_in_corpus(const string& qn, const corpus& abi_corpus)
{
  class_decl_sptr result;

  for (translation_units::const_iterator tu =
	 abi_corpus.get_translation_units().begin();
       tu != abi_corpus.get_translation_units().end();
       ++tu)
    if ((result = lookup_class_type_in_translation_unit(qn, **tu)))
      break;

  return result;
}

/// Lookup a type in an ABI corpus.
///
/// @param type the type to lookup.
///
/// @param corpus the ABI corpus to consider for the lookup.
///
/// @param the type found in the corpus or NULL if no such type was
/// found.
type_base_sptr
lookup_type_in_corpus(const type_base_sptr& type, corpus& corpus)
{
  assert(type);

  type_base_sptr result;
  for (translation_units::const_iterator i =
	 corpus.get_translation_units().begin();
       i != corpus.get_translation_units().end();
       ++i)
    if ((result = lookup_type_in_translation_unit(type, **i)))
      break;

  return result;
}

/// Look into an ABI corpus for a function type.
///
/// @param fn_type the function type to be looked for in the ABI
/// corpus.
///
/// @param corpus the ABI corpus into which to look for the function
/// type.
///
/// @return the function type found in the corpus.
function_type_sptr
lookup_function_type_in_corpus(const function_type_sptr& fn_type,
			       corpus& corpus)
{
  assert(fn_type);

  function_type_sptr result;

  for (translation_units::const_iterator i =
	 corpus.get_translation_units().begin();
       i != corpus.get_translation_units().end();
       ++i)
    if ((result = lookup_function_type_in_translation_unit(fn_type, **i)))
      return result;

  if (!result)
    for (translation_units::const_iterator i =
	   corpus.get_translation_units().begin();
	 i != corpus.get_translation_units().end();
	 ++i)
      if ((result = synthesize_function_type_from_translation_unit(*fn_type,
								   **i)))
	return result;

  return result;
}

/// Update the map that associates a fully qualified name of a given
/// type to that type.
///
/// @param scope the scope of the type we are considering.
///
/// @param type the type we are considering.
///
/// @param types_map the map to update.  It's a map that assciates a
/// fully qualified name of a type to the type itself.
template<typename TypeKind>
void
maybe_update_types_lookup_map(scope_decl *scope,
			      const shared_ptr<TypeKind>& type,
			      istring_type_base_wptr_map_type& types_map)
{
  corpus *type_corpus = scope->get_corpus();
  assert(type_corpus);

  interned_string s = get_type_name(type);
  if (types_map.find(s) == types_map.end())
    types_map[s]= type;
}

/// This is the specialization for type @ref class_decl of the
/// function template:
///
///    maybe_update_types_lookup_map<T>(scope_decl*,
///					const shared_ptr<T>&,
///					istring_type_base_wptr_map_type&)
///
/// @param scope the scope of the type to consider.
///
/// @param class_type the type to consider.
///
/// @param types_map the type map to update.
template<>
void
maybe_update_types_lookup_map<class_decl>(scope_decl *scope,
					  const class_decl_sptr& class_type,
					  istring_type_base_wptr_map_type& map)
{
  class_decl_sptr type = class_type;

  bool update_qname_map = true;
  if (type->get_is_declaration_only())
    {
      if (class_decl_sptr def = class_type->get_definition_of_declaration())
	type = def;
      else
	update_qname_map = false;
    }

  if (!update_qname_map)
    return;

  corpus *type_corpus = scope->get_corpus();
  assert(type_corpus);

  string qname = type->get_qualified_name();
  interned_string s = type_corpus->get_environment()->intern(qname);
  if (map.find(s) == map.end())
    map[s]= type;
}

/// This is the specialization for type @ref function_type of the
/// function template:
///
///    maybe_update_types_lookup_map<T>(scope_decl*,
///					const shared_ptr<T>&,
///					istring_type_base_wptr_map_type&)
///
/// @param scope the scope of the type to consider.
///
/// @param class_type the type to consider.
///
/// @param types_map the type map to update.
template<>
void
maybe_update_types_lookup_map<function_type>
(scope_decl *scope,
 const function_type_sptr& type,
 istring_type_base_wptr_map_type& types_map)
{
  corpus *type_corpus = scope->get_corpus();
  assert(type_corpus);

  string str = get_pretty_representation(type, /*internal=*/true);
  interned_string s = type->get_environment()->intern(str);

  if (types_map.find(s) == types_map.end())
    types_map[s]= type;
}

/// Update the map that associates a fully qualified name of a given
/// type to that type.
///
/// @param type the type we are considering.
///
/// @param types_map the map to update.  It's a map that assciates a
/// fully qualified name of a type to the type itself.
template<typename TypeKind>
void
maybe_update_types_lookup_map(const shared_ptr<TypeKind>& type,
			      istring_type_base_wptr_map_type& types_map)
{
  scope_decl *scope = get_type_scope(type);
  if (scope)
    maybe_update_types_lookup_map(scope, type, types_map);
  else
    {
      // This type doesn't have a scope; this is maybe b/c it's a
      // function type.  Those beasts are usually put in the global
      // scope of the current translation unit by
      // translation_unit::bind_function_type_life_time.  For now,
      // let's say that this function should not be called on those
      // function types.
      //
      // XXXX: But if we really want function types to have a scope,
      // we should prolly add a get_scope member function to
      // function_type and make get_type_scope() to call that one.
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Update the map that associates the fully qualified name of a basic
/// type with the type itself.
///
/// @param basic_type the basic type to consider.
void
maybe_update_types_lookup_map(const type_decl_sptr& basic_type)
{
  if (corpus *type_corpus = basic_type->get_corpus())
    return maybe_update_types_lookup_map<type_decl>
      (basic_type, type_corpus->priv_->get_basic_types());
}

/// Update the map that associates the fully qualified name of a class
/// type with the type itself.
///
/// @param class_type the class type to consider.
void
maybe_update_types_lookup_map(const class_decl_sptr& class_type)
{
  if (corpus *type_corpus = class_type->get_corpus())
    maybe_update_types_lookup_map<class_decl>
      (class_type, type_corpus->priv_->get_class_types());
}

/// Update the map that associates the fully qualified name of a union
/// type with the type itself.
///
/// @param union_type the union type to consider.
void
maybe_update_types_lookup_map(const union_decl_sptr& union_type)
{
  if (corpus *type_corpus = union_type->get_corpus())
    maybe_update_types_lookup_map<union_decl>
      (union_type, type_corpus->priv_->get_union_types());
}

/// Update the map that associates the fully qualified name of an enum
/// type with the type itself.
///
/// @param enum_type the type to consider.
void
maybe_update_types_lookup_map(const enum_type_decl_sptr& enum_type)

{
  if (corpus *type_corpus = enum_type->get_corpus())
    maybe_update_types_lookup_map<enum_type_decl>
      (enum_type, type_corpus->priv_->get_enum_types());
}

/// Update the map that associates the fully qualified name of a
/// typedef type with the type itself.
///
/// @param typedef_type the type to consider.
void
maybe_update_types_lookup_map(const typedef_decl_sptr& typedef_type)

{
  if (corpus *type_corpus = typedef_type->get_corpus())
    maybe_update_types_lookup_map<typedef_decl>
      (typedef_type, type_corpus->priv_->get_typedef_types());
}

/// Update the map that associates the fully qualified name of a
/// qualified type with the type itself.
///
/// @param qualified_type the type to consider.
void
maybe_update_types_lookup_map(const qualified_type_def_sptr& qualified_type)
{
  if (corpus *type_corpus = qualified_type->get_corpus())
    maybe_update_types_lookup_map<qualified_type_def>
      (qualified_type, type_corpus->priv_->get_qualified_types());
}

/// Update the map that associates the fully qualified name of a
/// pointer type with the type itself.
///
/// @param pointer_type the type to consider.
void
maybe_update_types_lookup_map(const pointer_type_def_sptr& pointer_type)
{
  if (corpus *type_corpus = pointer_type->get_corpus())
    maybe_update_types_lookup_map<pointer_type_def>
      (pointer_type, type_corpus->priv_->get_pointer_types());
}

/// Update the map that associates the fully qualified name of a
/// reference type with the type itself.
///
/// @param reference_type the type to consider.
void
maybe_update_types_lookup_map(const reference_type_def_sptr& reference_type)
{
  if (corpus *type_corpus = reference_type->get_corpus())
    maybe_update_types_lookup_map<reference_type_def>
      (reference_type, type_corpus->priv_->get_reference_types());
}

/// Update the map that associates the fully qualified name of a type
/// with the type itself.
///
/// @param array_type the type to consider.
void
maybe_update_types_lookup_map(const array_type_def_sptr& array_type)
{
  if (corpus *type_corpus = array_type->get_corpus())
    maybe_update_types_lookup_map<array_type_def>
      (array_type, type_corpus->priv_->get_array_types());
}

/// Update the map that associates the fully qualified name of a
/// function type with the type itself.
///
/// @param scope the scope of the function type.
/// @param fn_type the type to consider.
void
maybe_update_types_lookup_map(scope_decl *scope,
			      const function_type_sptr& fn_type)
{
  if (corpus *type_corpus = scope->get_corpus())
    maybe_update_types_lookup_map<function_type>
      (scope, fn_type, type_corpus->priv_->get_function_types());
}

/// Update the map that associates the fully qualified name of a type
/// declaration with the type itself.
///
/// @param decl the declaration of the type to consider.
void
maybe_update_types_lookup_map(const decl_base_sptr& decl)
{
  if (!is_type(decl))
    return;

  if (type_decl_sptr basic_type = is_type_decl(decl))
    maybe_update_types_lookup_map(basic_type);
  else if (class_decl_sptr class_type = is_class_type(decl))
    maybe_update_types_lookup_map(class_type);
  else if (union_decl_sptr union_type = is_union_type(decl))
    maybe_update_types_lookup_map(union_type);
  else if (enum_type_decl_sptr enum_type = is_enum_type(decl))
    maybe_update_types_lookup_map(enum_type);
  else if (typedef_decl_sptr typedef_type = is_typedef(decl))
    maybe_update_types_lookup_map(typedef_type);
  else if (qualified_type_def_sptr qualified_type = is_qualified_type(decl))
    maybe_update_types_lookup_map(qualified_type);
  else if (pointer_type_def_sptr pointer_type = is_pointer_type(decl))
    maybe_update_types_lookup_map(pointer_type);
  else if (reference_type_def_sptr reference_type = is_reference_type(decl))
    maybe_update_types_lookup_map(reference_type);
  else if (array_type_def_sptr array_type = is_array_type(decl))
    maybe_update_types_lookup_map(array_type);
  else
    ABG_ASSERT_NOT_REACHED;
}

/// Update the map that associates the fully qualified name of a type
/// with the type itself.
///
/// @param type the type to consider.
void
maybe_update_types_lookup_map(const type_base_sptr& type)
{
  if (decl_base_sptr decl = get_type_declaration(type))
    maybe_update_types_lookup_map(decl);
  else
    ABG_ASSERT_NOT_REACHED;
}

/// Look into a given corpus to find a type which has the same
/// qualified name as a giventype.
///
/// @param t the type which has the same qualified name as the type we
/// are looking for.
///
/// @param corp the ABI corpus to look into for the type.
type_decl_sptr
lookup_basic_type(const type_decl& t, corpus& corp)
{return lookup_basic_type(t.get_name(), corp);}

/// Look into a given corpus to find a basic type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the basic type to look
/// for.
///
/// @param corp the corpus to look into.
type_decl_sptr
lookup_basic_type(const string &qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& basic_types = corp.priv_->get_basic_types();
  istring_type_base_wptr_map_type::iterator i =
    basic_types.find(corp.get_environment()->intern(qualified_name));
  if (i == basic_types.end())
    return type_decl_sptr();
  return is_type_decl(type_base_sptr(i->second));
}

/// Look into a given corpus to find a class type which has the same
/// qualified name as a given type.
///
/// @param t the class decl type which has the same qualified name as
/// the type we are looking for.
///
/// @param corp the corpus to look into.
class_decl_sptr
lookup_class_type(const class_decl& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal*/true);
  return lookup_class_type(s, corp);
}

/// Look into a given corpus to find a type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
class_decl_sptr
lookup_class_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& class_types = corp.priv_->get_class_types();
  istring_type_base_wptr_map_type::iterator i =
    class_types.find(corp.get_environment()->intern(qualified_name));
  if (i == class_types.end())
    return class_decl_sptr();
  return is_class_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find a enum type which has the same
/// qualified name as a given enum type.
///
/// @param t the enum type which has the same qualified name as the
/// type we are looking for.
///
/// @param corp the corpus to look into.
enum_type_decl_sptr
lookup_enum_type(const enum_type_decl& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_enum_type(s, corp);
}

/// Look into a given corpus to find an enum type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the enum type to look
/// for.
///
/// @param corp the corpus to look into.
enum_type_decl_sptr
lookup_enum_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& enum_types = corp.priv_->get_enum_types();
  istring_type_base_wptr_map_type::iterator i =
    enum_types.find(corp.get_environment()->intern(qualified_name));
  if (i == enum_types.end())
    return enum_type_decl_sptr();
  return is_enum_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find a typedef type which has the
/// same qualified name as a given typedef type.
///
/// @param t the typedef type which has the same qualified name as the
/// typedef type we are looking for.
///
/// @param corp the corpus to look into.
typedef_decl_sptr
lookup_typedef_type(const typedef_decl& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_typedef_type(s, corp);
}

/// Look into a given corpus to find a typedef type which has a
/// given qualified name.
///
/// @param qualified_name the qualified name of the typedef type to
/// look for.
///
/// @param corp the corpus to look into.
typedef_decl_sptr
lookup_typedef_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& typedef_types =
    corp.priv_->get_typedef_types();
  istring_type_base_wptr_map_type::iterator i =
    typedef_types.find(corp.get_environment()->intern(qualified_name));
  if (i == typedef_types.end())
    return typedef_decl_sptr();
  return is_typedef(type_base_sptr(i->second));
}

/// Look into a corpus to find a class or typedef type which has a
/// given qualified name.
///
/// @param qualified_name the name of the type to find.
///
/// @param corp the corpus to look into.
///
/// @return the typedef or class type found.
type_base_sptr
lookup_class_or_typedef_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& class_types =
    corp.priv_->get_class_types();
  istring_type_base_wptr_map_type& typedef_types =
    corp.priv_->get_typedef_types();

  interned_string name = corp.get_environment()->intern(qualified_name);

  istring_type_base_wptr_map_type::iterator i = class_types.find(name);
  if (i == class_types.end())
    {
      i = typedef_types.find(name);
      if (i == typedef_types.end())
	return type_base_sptr();
    }
  return type_base_sptr(i->second);
}

/// Look into a corpus to find a class, typedef or enum type which has
/// a given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the typedef, class or enum type found.
type_base_sptr
lookup_class_typedef_or_enum_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& enum_types = corp.priv_->get_class_types();
  type_base_sptr result = lookup_class_or_typedef_type(qualified_name, corp);

  if (!result)
    {
      interned_string name = corp.get_environment()->intern(qualified_name);
      istring_type_base_wptr_map_type::iterator i = enum_types.find(name);
      if (i != enum_types.end())
	result = type_base_sptr(i->second);
    }

  return result;
}

/// Look into a given corpus to find a qualified type which has the
/// same qualified name as a given type.
///
/// @param t the type which has the same qualified name as the
/// qualified type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the qualified type found.
qualified_type_def_sptr
lookup_qualified_type(const qualified_type_def& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_qualified_type(s, corp);
}

/// Look into a given corpus to find a qualified type which has a
/// given qualified name.
///
/// @param qualified_name the qualified name of the type to look for.
///
/// @param corp the corpus to look into.
///
/// @return the type found.
qualified_type_def_sptr
lookup_qualified_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& qualified_types =
    corp.priv_->get_qualified_types();
  istring_type_base_wptr_map_type::iterator i =
    qualified_types.find(corp.get_environment()->intern(qualified_name));
  if (i == qualified_types.end())
    return qualified_type_def_sptr();
  return is_qualified_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find a pointer type which has the same
/// qualified name as a given pointer type.
///
/// @param t the pointer type which has the same qualified name as the
/// type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the pointer type found.
pointer_type_def_sptr
lookup_pointer_type(const pointer_type_def& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_pointer_type(s, corp);
}

/// Look into a given corpus to find a pointer type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the pointer type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the pointer type found.
pointer_type_def_sptr
lookup_pointer_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& pointer_types =
    corp.priv_->get_pointer_types();
  istring_type_base_wptr_map_type::iterator i =
    pointer_types.find(corp.get_environment()->intern(qualified_name));
  if (i == pointer_types.end())
    return pointer_type_def_sptr();
  return is_pointer_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find a reference type which has the
/// same qualified name as a given reference type.
///
/// @param t the reference type which has the same qualified name as
/// the reference type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the reference type found.
reference_type_def_sptr
lookup_reference_type(const reference_type_def& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_reference_type(s, corp);
}

/// Look into a given corpus to find a reference type which has a
/// given qualified name.
///
/// @param qualified_name the qualified name of the reference type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the reference type found.
reference_type_def_sptr
lookup_reference_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& reference_types =
    corp.priv_->get_reference_types();
  istring_type_base_wptr_map_type::iterator i =
    reference_types.find(corp.get_environment()->intern(qualified_name));
  if (i == reference_types.end())
    return reference_type_def_sptr();
  return is_reference_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find an array type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the array type to look
/// for.
///
/// @param corp the corpus to look into.
///
/// @return the array type found.
array_type_def_sptr
lookup_array_type(const array_type_def& t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal=*/true);
  return lookup_array_type(s, corp);
}

/// Look into a given corpus to find an array type which has the same
/// qualified name as a given array type.
///
/// @param t the type which has the same qualified name as the type we
/// are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the type found.
array_type_def_sptr
lookup_array_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& array_types =
    corp.priv_->get_array_types();
  istring_type_base_wptr_map_type::iterator i =
    array_types.find(corp.get_environment()->intern(qualified_name));
  if (i == array_types.end())
    return array_type_def_sptr();
  return is_array_type(type_base_sptr(i->second));
}

/// Look into a given corpus to find a function type which has the same
/// qualified name as a given function type.
///
/// @param t the function type which has the same qualified name as
/// the function type we are looking for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
function_type_sptr
lookup_function_type(const function_type&t, corpus& corp)
{
  string s = t.get_pretty_representation(/*internal*/true);
  return lookup_function_type(s, corp);
}

/// Look into a given corpus to find a function type which has a given
/// qualified name.
///
/// @param qualified_name the qualified name of the function type to
/// look for.
///
/// @param corp the corpus to look into.
///
/// @return the function type found.
function_type_sptr
lookup_function_type(const string& qualified_name, corpus& corp)
{
  istring_type_base_wptr_map_type& function_types =
    corp.priv_->get_function_types();
  istring_type_base_wptr_map_type::iterator i =
    function_types.find(corp.get_environment()->intern(qualified_name));
  if (i == function_types.end())
    return function_type_sptr();
  return is_function_type(type_base_sptr(i->second));
}

// </corpus stuff>
}// end namespace ir
}// end namespace abigail
