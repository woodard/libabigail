// -*- mode: C++ -*-
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
//
//Author: Dodji Seketeli

/// @file
///
/// Definitions for the Internal Representation artifacts of libabigail.

#include <cxxabi.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <sstream>
#include <tr1/memory>
#include <tr1/unordered_map>
#include "abg-sptr-utils.h"
#include "abg-ir.h"

namespace
{
/// This internal type is a tree walker that walks the sub-tree of a
/// type and sets the environment of the type (including its sub-type)
/// to a new environment.
class environment_setter : public abigail::ir::ir_node_visitor
{
  abigail::ir::type_or_decl_base* artifact_;
  abigail::ir::environment* env_;

public:
  environment_setter(abigail::ir::type_or_decl_base*	a,
		     abigail::ir::environment*		env)
    : artifact_(a),
      env_(env)
  {}

  /// This function is called on each sub-tree node that is a
  /// declaration.  Note that it's also called on some types because
  /// most types that have a declarations also inherit the type @ref
  /// decl_base.
  ///
  /// @param d the declaration being visited.
  bool
  visit_begin(abigail::ir::decl_base* d)
  {
    if (abigail::ir::environment* env = d->get_environment())
      {
	assert(env == env_);
	return false;
      }
    else
      d->set_environment(env_);

    return true;

  }

  /// This function is called on each sub-tree node that is a type.
  ///
  /// @param t the type being visited.
  bool
  visit_begin(abigail::ir::type_base* t)
  {
    if (abigail::ir::environment* env = t->get_environment())
      {
	assert(env == env_);
	return false;
      }
    else
      {
	assert(!t->get_environment());
	t->set_environment(env_);
      }
    return true;
  }
};

/// This internal type is a tree walking that is used to set the
/// qualified name of a tree of decls and types.  It used by the
/// function update_qualified_name().
class qualified_name_setter : public abigail::ir::ir_node_visitor
{
  abigail::ir::decl_base* node_;

public:
  qualified_name_setter(abigail::ir::decl_base* node)
    : node_(node)
  {}

  bool
  do_update(abigail::ir::decl_base* d);

  bool
  visit_begin(abigail::ir::decl_base* d);

  bool
  visit_begin(abigail::ir::type_base* d);
}; // end class qualified_name_setter

}// end anon namespace

namespace abigail
{

namespace ir
{

// Inject.
using std::string;
using std::list;
using std::vector;
using std::tr1::unordered_map;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;

/// @brief the location of a token represented in its simplest form.
/// Instances of this type are to be stored in a sorted vector, so the
/// type must have proper relational operators.
class expanded_location
{
  string	path_;
  unsigned	line_;
  unsigned	column_;

  expanded_location();

public:

  friend class location_manager;

  expanded_location(const string& path, unsigned line, unsigned column)
  : path_(path), line_(line), column_(column)
  {}

  bool
  operator==(const expanded_location& l) const
  {
    return (path_ == l.path_
	    && line_ == l.line_
	    && column_ && l.column_);
  }

  bool
  operator<(const expanded_location& l) const
  {
    if (path_ < l.path_)
      return true;
    else if (path_ > l.path_)
      return false;

    if (line_ < l.line_)
      return true;
    else if (line_ > l.line_)
      return false;

    return column_ < l.column_;
  }
};

/// Expand the location into a tripplet path, line and column number.
///
/// @param path the output parameter where this function sets the
/// expanded path.
///
/// @param line the output parameter where this function sets the
/// expanded line.
///
/// @param column the ouptut parameter where this function sets the
/// expanded column.
void
location::expand(std::string& path, unsigned& line, unsigned& column) const
{
  assert(get_location_manager());
  get_location_manager()->expand_location(*this, path, line, column);
}

struct location_manager::priv
{
  /// This sorted vector contains the expanded locations of the tokens
  /// coming from a given ABI Corpus.  The index of a given expanded
  /// location in the table gives us an integer that is used to build
  /// instance of location types.
  std::vector<expanded_location> locs;
};

location_manager::location_manager()
{priv_ = shared_ptr<location_manager::priv>(new location_manager::priv);}

/// Insert the triplet representing a source locus into our internal
/// vector of location triplet.  Return an instance of location type,
/// built from an integral type that represents the index of the
/// source locus triplet into our source locus table.
///
/// @param file_path the file path of the source locus
/// @param line the line number of the source location
/// @param col the column number of the source location
location
location_manager::create_new_location(const std::string&	file_path,
				      size_t			line,
				      size_t			col)
{
  expanded_location l(file_path, line, col);

  // Just append the new expanded location to the end of the vector
  // and return its index.  Note that indexes start at 1.
  priv_->locs.push_back(l);
  return location(priv_->locs.size(), this);
}

/// Given an instance of location type, return the triplet
/// {path,line,column} that represents the source locus.  Note that
/// the location must have been previously created from the function
/// location_manager::create_new_location, otherwise this function yields
/// unexpected results, including possibly a crash.
///
/// @param location the instance of location type to expand
/// @param path the resulting path of the source locus
/// @param line the resulting line of the source locus
/// @param column the resulting colum of the source locus
void
location_manager::expand_location(const location&	location,
				  std::string&		path,
				  unsigned&		line,
				  unsigned&		column) const
{
  if (location.value_ == 0)
    return;
  expanded_location &l = priv_->locs[location.value_ - 1];
  path = l.path_;
  line = l.line_;
  column = l.column_;
}

typedef unordered_map<function_type_sptr,
		      bool,
		      function_type::hash,
		      type_shared_ptr_equal> fn_type_ptr_map;

/// Private type to hold private members of @ref translation_unit
struct translation_unit::priv
{
  environment*					env_;
  const corpus*				corp;
  bool						is_constructed_;
  char						address_size_;
  language					language_;
  std::string					path_;
  location_manager				loc_mgr_;
  mutable global_scope_sptr			global_scope_;
  mutable function_types_type			function_types_;
  mutable vector<type_base_sptr>		synthesized_types_;
  mutable string_type_base_wptr_map_type	types_;

  priv(environment* env)
    : env_(env),
      corp(),
      is_constructed_(),
      address_size_(),
      language_(LANG_UNKNOWN)
  {}

  ~priv()
  {}
}; // end translation_unit::priv

// <translation_unit stuff>

/// Constructor of translation_unit.
///
/// @param path the location of the translation unit.
///
/// @param env the environment of this translation unit.  Please note
/// that the life time of the environment must be greater than the
/// life time of the translation unit because the translation uses
/// resources that are allocated in the environment.
///
/// @param address_size the size of addresses in the translation unit,
/// in bits.
translation_unit::translation_unit(const std::string&	path,
				   environment*	env,
				   char		address_size)
  : priv_(new priv(env))
{
  priv_->path_ = path;
  priv_->address_size_ = address_size;
}

/// Getter of the the global scope of the translation unit.
///
/// @return the global scope of the current translation unit.  If
/// there is not global scope allocated yet, this function creates one
/// and returns it.
const shared_ptr<global_scope>
translation_unit::get_global_scope() const
{
  if (!priv_->global_scope_)
    {
      priv_->global_scope_.reset
	(new global_scope(const_cast<translation_unit*>(this)));
      // The global scope must be out of the same environment as its
      // translation unit.
      priv_->global_scope_->
	set_environment(const_cast<environment*>(get_environment()));
      priv_->global_scope_->set_translation_unit(this);
    }
  return priv_->global_scope_;
}

/// Getter of the function types of the current @ref translation_unit.
///
/// @return the function types of the current translation unit.
const function_types_type
translation_unit::get_function_types() const
{ return priv_->function_types_; }

/// Getter of the types of the current @ref translation_unit.
///
/// @return a map of the types of the translation unit.  The key of
/// the map is the qualified name of the type, and value is the type.
const string_type_base_wptr_map_type&
translation_unit::get_types() const
{return priv_->types_;}

/// Getter of the types of the current @ref translation_unit.
///
/// @return a map of the types of the translation unit.  The key of
/// the map is the qualified name of the type, and value is the type.
string_type_base_wptr_map_type&
translation_unit::get_types()
{return priv_->types_;}

/// Getter of the environment of the current @ref translation_unit.
///
/// @return the translation unit of the current translation unit.
const environment*
translation_unit::get_environment() const
{return priv_->env_;}

/// Getter of the environment of the current @ref translation_unit.
///
/// @return the translation unit of the current translation unit.
environment*
translation_unit::get_environment()
{return priv_->env_;}

/// Setter of the environment of the current @ref translation_unit.
///
/// @param env the environment.
void
translation_unit::set_environment(environment* env)
{priv_->env_ = env;}

/// Getter of the language of the source code of the translation unit.
///
/// @return the language of the source code.
translation_unit::language
translation_unit::get_language() const
{return priv_->language_;}

/// Setter of the language of the source code of the translation unit.
///
/// @param l the new language.
void
translation_unit::set_language(language l)
{priv_->language_ = l;}

/// @return the path of the compilation unit associated to the current
/// instance of translation_unit.
const std::string&
translation_unit::get_path() const
{return priv_->path_;}

/// Set the path associated to the current instance of
/// translation_unit.
///
/// @param a_path the new path to set.
void
translation_unit::set_path(const string& a_path)
{priv_->path_ = a_path;}

/// Set the corpus this translation unit is a member of.
///
/// Note that adding a translation unit to a @ref corpus automatically
/// triggers a call to this member function.
///
/// @param corpus the corpus.
void
translation_unit::set_corpus(const corpus* c)
{priv_->corp = c;}

/// Get the corpus this translation unit is a member of.
///
/// @return the parent corpus, or nil if this doesn't belong to any
/// corpus yet.
const corpus*
translation_unit::get_corpus() const
{return priv_->corp;}

/// Getter of the location manager for the current translation unit.
///
/// @return a reference to the location manager for the current
/// translation unit.
location_manager&
translation_unit::get_loc_mgr()
{return priv_->loc_mgr_;}

/// const Getter of the location manager.
///
/// @return a const reference to the location manager for the current
/// translation unit.
const location_manager&
translation_unit::get_loc_mgr() const
{return priv_->loc_mgr_;}

/// Tests whether if the current translation unit contains ABI
/// artifacts or not.
///
/// @return true iff the current translation unit is empty.
bool
translation_unit::is_empty() const
{return get_global_scope()->is_empty();}

/// Getter of the address size in this translation unit.
///
/// @return the address size, in bits.
char
translation_unit::get_address_size() const
{return priv_->address_size_;}

/// Setter of the address size in this translation unit.
///
/// @param a the new address size in bits.
void
translation_unit::set_address_size(char a)
{priv_->address_size_= a;}

/// Getter of the 'is_constructed" flag.  It says if the translation
/// unit is fully constructed or not.
///
/// This flag is important for cases when comparison might depend on
/// if the translation unit is fully built or not.  For instance, when
/// reading types from DWARF, the virtual methods of a class are not
/// necessarily fully constructed until we have reached the end of the
/// translation unit.  In that case, before we've reached the end of
/// the translation unit, we might not take virtual functions into
/// account when comparing classes.
///
/// @return true if the translation unit is constructed.
bool
translation_unit::is_constructed() const
{return priv_->is_constructed_;}

/// Setter of the 'is_constructed" flag.  It says if the translation
/// unit is fully constructed or not.
///
/// This flag is important for cases when comparison might depend on
/// if the translation unit is fully built or not.  For instance, when
/// reading types from DWARF, the virtual methods of a class are not
/// necessarily fully constructed until we have reached the end of the
/// translation unit.  In that case, before we've reached the end of
/// the translation unit, we might not take virtual functions into
/// account when comparing classes.
///
/// @param f true if the translation unit is constructed.
void
translation_unit::set_is_constructed(bool f)
{priv_->is_constructed_ = f;}

/// Compare the current translation unit against another one.
///
/// @param other the other tu to compare against.
///
/// @return true if the two translation units are equal, false
/// otherwise.
bool
translation_unit::operator==(const translation_unit& other)const
{
  if (get_address_size() != other.get_address_size())
    return false;

  return *get_global_scope() == *other.get_global_scope();
}

/// Ensure that the life time of a function type is bound to the life
/// time of the current translation unit.
///
/// @param ftype the function time which life time to bind to the life
/// time of the current instance of @ref translation_unit.  That is,
/// it's onlyh when the translation unit is destroyed that the
/// function type can be destroyed to.
void
translation_unit::bind_function_type_life_time(function_type_sptr ftype) const
{
  priv_->function_types_.push_back(ftype);

  // The function type must be ouf of the same environment as its
  // translation unit.
  if (const environment* env = get_environment())
    {
      if (const environment* e = ftype->get_environment())
	assert(env == e);
      ftype->set_environment(const_cast<environment*>(env));
    }

  if (const translation_unit* existing_tu = ftype->get_translation_unit())
    assert(existing_tu == this);
  else
    ftype->set_translation_unit(this);
}

/// This implements the ir_traversable_base::traverse virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
///
/// @return true if the entire type IR tree got traversed, false
/// otherwise.
bool
translation_unit::traverse(ir_node_visitor& v)
{return get_global_scope()->traverse(v);}

translation_unit::~translation_unit()
{}

/// Converts a translation_unit::language enumerator into a string.
///
/// @param l the language enumerator to translate.
///
/// @return the resulting string.
string
translation_unit_language_to_string(translation_unit::language l)
{
  switch (l)
    {
    case translation_unit::LANG_UNKNOWN:
      return "LANG_UNKNOWN";
    case translation_unit::LANG_Cobol74:
      return "LANG_Cobol74";
    case translation_unit::LANG_Cobol85:
      return "LANG_Cobol85";
    case translation_unit::LANG_C89:
      return "LANG_C89";
    case translation_unit::LANG_C99:
      return "LANG_C99";
    case translation_unit::LANG_C11:
      return "LANG_C11";
    case translation_unit::LANG_C:
      return "LANG_C";
    case translation_unit::LANG_C_plus_plus_11:
      return "LANG_C_plus_plus_11";
    case translation_unit::LANG_C_plus_plus_14:
      return "LANG_C_plus_plus_14";
    case translation_unit::LANG_C_plus_plus:
      return "LANG_C_plus_plus";
    case translation_unit::LANG_ObjC:
      return "LANG_ObjC";
    case translation_unit::LANG_ObjC_plus_plus:
      return "LANG_ObjC_plus_plus";
    case translation_unit::LANG_Fortran77:
      return "LANG_Fortran77";
    case translation_unit::LANG_Fortran90:
      return "LANG_Fortran90";
    case translation_unit::LANG_Fortran95:
      return "LANG_Fortran95";
    case translation_unit::LANG_Ada83:
      return "LANG_Ada83";
    case translation_unit::LANG_Ada95:
      return "LANG_Ada95";
    case translation_unit::LANG_Pascal83:
      return "LANG_Pascal83";
    case translation_unit::LANG_Modula2:
      return "LANG_Modula2";
    case translation_unit::LANG_Java:
      return "LANG_Java";
    case translation_unit::LANG_PL1:
      return "LANG_PL1";
    case translation_unit::LANG_UPC:
      return "LANG_UPC";
    case translation_unit::LANG_D:
      return "LANG_D";
    case translation_unit::LANG_Python:
      return "LANG_Python";
    case translation_unit::LANG_Go:
      return "LANG_Go";
    case translation_unit::LANG_Mips_Assembler:
      return "LANG_Mips_Assembler";
    default:
      return "LANG_UNKNOWN";
    }

  return "LANG_UNKNOWN";
}

/// Parse a string representing a language into a
/// translation_unit::language enumerator into a string.
///
/// @param l the string representing the language.
///
/// @return the resulting translation_unit::language enumerator.
translation_unit::language
string_to_translation_unit_language(const string& l)
{
  if (l == "LANG_Cobol74")
    return translation_unit::LANG_Cobol74;
  else if (l == "LANG_Cobol85")
    return translation_unit::LANG_Cobol85;
  else if (l == "LANG_C89")
    return translation_unit::LANG_C89;
  else if (l == "LANG_C99")
    return translation_unit::LANG_C99;
  else if (l == "LANG_C11")
    return translation_unit::LANG_C11;
  else if (l == "LANG_C")
    return translation_unit::LANG_C;
  else if (l == "LANG_C_plus_plus_11")
    return translation_unit::LANG_C_plus_plus_11;
  else if (l == "LANG_C_plus_plus_14")
    return translation_unit::LANG_C_plus_plus_14;
  else if (l == "LANG_C_plus_plus")
    return translation_unit::LANG_C_plus_plus;
  else if (l == "LANG_ObjC")
    return translation_unit::LANG_ObjC;
  else if (l == "LANG_ObjC_plus_plus")
    return translation_unit::LANG_ObjC_plus_plus;
  else if (l == "LANG_Fortran77")
    return translation_unit::LANG_Fortran77;
  else if (l == "LANG_Fortran90")
    return translation_unit::LANG_Fortran90;
    else if (l == "LANG_Fortran95")
    return translation_unit::LANG_Fortran95;
  else if (l == "LANG_Ada83")
    return translation_unit::LANG_Ada83;
  else if (l == "LANG_Ada95")
    return translation_unit::LANG_Ada95;
  else if (l == "LANG_Pascal83")
    return translation_unit::LANG_Pascal83;
  else if (l == "LANG_Modula2")
    return translation_unit::LANG_Modula2;
  else if (l == "LANG_Java")
    return translation_unit::LANG_Java;
  else if (l == "LANG_PL1")
    return translation_unit::LANG_PL1;
  else if (l == "LANG_UPC")
    return translation_unit::LANG_UPC;
  else if (l == "LANG_D")
    return translation_unit::LANG_D;
  else if (l == "LANG_Python")
    return translation_unit::LANG_Python;
  else if (l == "LANG_Go")
    return translation_unit::LANG_Go;
  else if (l == "LANG_Mips_Assembler")
    return translation_unit::LANG_Mips_Assembler;

  return translation_unit::LANG_UNKNOWN;
}

/// Test if a language enumerator designates the C language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the C language.
bool
is_c_language(translation_unit::language l)
{
  return (l == translation_unit::LANG_C89
	  || l == translation_unit::LANG_C99
	  || l == translation_unit::LANG_C11
	  || l == translation_unit::LANG_C);
}

/// Test if a language enumerator designates the C++ language.
///
/// @param l the language enumerator to consider.
///
/// @return true iff @p l designates the C++ language.
bool
is_cplus_plus_language(translation_unit::language l)
{
  return (l == translation_unit::LANG_C_plus_plus_11
	  || l == translation_unit::LANG_C_plus_plus_14
	  || l == translation_unit::LANG_C_plus_plus);
}

/// A deep comparison operator for pointers to translation units.
///
/// @param l the first translation unit to consider for the comparison.
///
/// @param r the second translation unit to consider for the comparison.
///
/// @return true if the two translation units are equal, false otherwise.
bool
operator==(translation_unit_sptr l, translation_unit_sptr r)
{
  if (l.get() == r.get())
    return true;

  if (!!l != !!r)
    return false;

  return *l == *r;
}

// </translation_unit stuff>

// <elf_symbol stuff>
struct elf_symbol::priv
{
  size_t		index_;
  size_t		size_;
  string		name_;
  elf_symbol::type	type_;
  elf_symbol::binding	binding_;
  elf_symbol::version	version_;
  bool			is_defined_;
  elf_symbol_wptr	main_symbol_;
  elf_symbol_wptr	next_alias_;
  string		id_string_;

  priv()
    : index_(),
      size_(),
      type_(elf_symbol::NOTYPE_TYPE),
      binding_(elf_symbol::GLOBAL_BINDING),
      is_defined_(false)
  {}

  priv(size_t				i,
       size_t				s,
       const string&			n,
       elf_symbol::type		t,
       elf_symbol::binding		b,
       bool				d,
       const elf_symbol::version&	v)
    : index_(i),
      size_(s),
      name_(n),
      type_(t),
      binding_(b),
      version_(v),
      is_defined_(d)
  {}
}; // end struct elf_symbol::priv

/// Default constructor of the @ref elf_symbol type.
///
/// Note that this constructor is private, so client code cannot use
/// it to create instances of @ref elf_symbol.  Rather, client code
/// should use the @ref elf_symbol::create() function to create
/// instances of @ref elf_symbol instead.
elf_symbol::elf_symbol()
  : priv_(new priv)
{
}

/// Constructor of the @ref elf_symbol type.
///
/// Note that this constructor is private, so client code cannot use
/// it to create instances of @ref elf_symbol.  Rather, client code
/// should use the @ref elf_symbol::create() function to create
/// instances of @ref elf_symbol instead.
///
/// @param i the index of the symbol in the (ELF) symbol table.
///
/// @param s the size of the symbol.
///
/// @param n the name of the symbol.
///
/// @param t the type of the symbol.
///
/// @param b the binding of the symbol.
///
/// @param d true if the symbol is defined, false otherwise.
///
/// @param v the version of the symbol.
elf_symbol::elf_symbol(size_t		i,
		       size_t		s,
		       const string&	n,
		       type		t,
		       binding		b,
		       bool		d,
		       const version&	v)
  : priv_(new priv(i, s, n, t, b, d, v))
{}

/// Factory of instances of @ref elf_symbol.
///
/// This is the function to use to create instances of @ref elf_symbol.
///
/// @return a (smart) pointer to a newly created instance of @ref
/// elf_symbol.
elf_symbol_sptr
elf_symbol::create()
{
  elf_symbol_sptr e(new elf_symbol());
  e->priv_->main_symbol_ = e;
  return e;
}

/// Factory of instances of @ref elf_symbol.
///
/// This is the function to use to create instances of @ref elf_symbol.
///
/// @param i the index of the symbol in the (ELF) symbol table.
///
/// @param s the size of the symbol.
///
/// @param n the name of the symbol.
///
/// @param t the type of the symbol.
///
/// @param b the binding of the symbol.
///
/// @param d true if the symbol is defined, false otherwise.
///
/// @param v the version of the symbol.
///
/// @return a (smart) pointer to a newly created instance of @ref
/// elf_symbol.
elf_symbol_sptr
elf_symbol::create(size_t		i,
		   size_t		s,
		   const string&	n,
		   type		t,
		   binding		b,
		   bool		d,
		   const version&	v)
{
  elf_symbol_sptr e(new elf_symbol(i, s, n, t, b, d, v));
  e->priv_->main_symbol_ = e;
  return e;
}

/// Test textual equality between two symbols.
///
/// Textual equality means that the aliases of the compared symbols
/// are not taken into account.  Only the name, type, and version of
/// the symbols are compared.
///
/// @return true iff the two symbols are textually equal.
static bool
textually_equals(const elf_symbol&l,
		 const elf_symbol&r)
{
  bool equals = (l.get_name() == r.get_name()
		 && l.get_type() == r.get_type()
		 && l.is_public() == r.is_public()
		 && l.is_defined() == r.is_defined()
		 && l.get_version() == r.get_version());

  if (equals && l.is_variable())
    // These are variable symbols.  Let's compare their symbol size.
    // The symbol size in this case is the size taken by the storage
    // of the variable.  If that size changes, then it's an ABI
    // change.
    equals = l.get_size() == r.get_size();

  return equals;
}

/// Getter for the index
///
/// @return the index of the symbol.
size_t
elf_symbol::get_index() const
{return priv_->index_;}

/// Setter for the index.
///
/// @param s the new index.
void
elf_symbol::set_index(size_t s)
{priv_->index_ = s;}

/// Getter for the name of the @ref elf_symbol.
///
/// @return a reference to the name of the @ref symbol.
const string&
elf_symbol::get_name() const
{return priv_->name_;}

/// Setter for the name of the current intance of @ref elf_symbol.
///
/// @param n the new name.
void
elf_symbol::set_name(const string& n)
{
  priv_->name_ = n;
  priv_->id_string_.clear();
}

/// Getter for the type of the current instance of @ref elf_symbol.
///
/// @return the type of the elf symbol.
elf_symbol::type
elf_symbol::get_type() const
{return priv_->type_;}

/// Setter for the type of the current instance of @ref elf_symbol.
///
/// @param t the new symbol type.
void
elf_symbol::set_type(type t)
{priv_->type_ = t;}

/// Getter of the size of the symbol.
///
/// @return the size of the symbol, in bytes.
size_t
elf_symbol::get_size() const
{return priv_->size_;}

/// Setter of the size of the symbol.
///
/// @param size the new size of the symbol, in bytes.
void
elf_symbol::set_size(size_t size)
{priv_->size_ = size;}

/// Getter for the binding of the current instance of @ref elf_symbol.
///
/// @return the binding of the symbol.
elf_symbol::binding
elf_symbol::get_binding() const
{return priv_->binding_;}

/// Setter for the binding of the current instance of @ref elf_symbol.
///
/// @param b the new binding.
void
elf_symbol::set_binding(binding b)
{priv_->binding_ = b;}

/// Getter for the version of the current instanc of @ref elf_symbol.
///
/// @return the version of the elf symbol.
elf_symbol::version&
elf_symbol::get_version() const
{return priv_->version_;}

/// Setter for the version of the current instance of @ref elf_symbol.
///
/// @param v the new version of the elf symbol.
void
elf_symbol::set_version(const version& v)
{
  priv_->version_ = v;
  priv_->id_string_.clear();
}

/// Test if the current instance of @ref elf_symbol is defined or not.
///
/// @return true if the current instance of @ref elf_symbol is
/// defined, false otherwise.
bool
elf_symbol::is_defined() const
{return priv_->is_defined_;}

/// Sets a flag saying if the current instance of @ref elf_symbol is
/// defined
///
/// @param b the new value of the flag.
void
elf_symbol::is_defined(bool d)
{priv_->is_defined_ = d;}

/// Test if the current instance of @ref elf_symbol is public or not.
///
/// This tests if the symbol defined, and either
///		- has global binding
///		- has weak binding
///		- or has a GNU_UNIQUE binding.
///
/// return true if the current instance of @ref elf_symbol is public,
/// false otherwise.
bool
elf_symbol::is_public() const
{
  return (is_defined()
	  && (get_binding() == GLOBAL_BINDING
	      || get_binding() == WEAK_BINDING
	      || get_binding() == GNU_UNIQUE_BINDING));
}

/// Test if the current instance of @ref elf_symbol is a function
/// symbol or not.
///
/// @return true if the current instance of @ref elf_symbol is a
/// function symbol, false otherwise.
bool
elf_symbol::is_function() const
{return get_type() == FUNC_TYPE || get_type() == GNU_IFUNC_TYPE;}

/// Test if the current instance of @ref elf_symbol is a variable
/// symbol or not.
///
/// @return true if the current instance of @ref elf_symbol is a
/// variable symbol, false otherwise.
bool
elf_symbol::is_variable() const
{return get_type() == OBJECT_TYPE || get_type() == TLS_TYPE;}

/// @name Elf symbol aliases
///
/// An alias A for an elf symbol S is a symbol that is defined at the
/// same address as S.  S is chained to A through the
/// elf_symbol::get_next_alias() method.
///
/// When there are several aliases to a symbol, the main symbol is the
/// the first symbol found in the symbol table for a given address.
///
/// The alias chain is circular.  That means if S is the main symbol
/// and A is the alias, S is chained to A and A
/// is chained back to the main symbol S.  The last alias in an alias
///chain is always chained to the main symbol.
///
/// Thus, when looping over the aliases of an elf_symbol A, detecting
/// an alias that is equal to the main symbol should logically be a
/// loop exit condition.
///
/// Accessing and adding aliases for instances of elf_symbol is done
/// through the member functions below.

/// @{

/// Get the main symbol of an alias chain.
///
///@return the main symbol.
const elf_symbol_sptr
elf_symbol::get_main_symbol() const
{return elf_symbol_sptr(priv_->main_symbol_);}

/// Get the main symbol of an alias chain.
///
///@return the main symbol.
elf_symbol_sptr
elf_symbol::get_main_symbol()
{return elf_symbol_sptr(priv_->main_symbol_);}

/// Tests whether this symbol is the main symbol.
///
/// @return true iff this symbol is the main symbol.
bool
elf_symbol::is_main_symbol() const
{return get_main_symbol().get() == this;}

/// Get the next alias of the current symbol.
///
///@return the alias, or NULL if there is no alias.
elf_symbol_sptr
elf_symbol::get_next_alias() const
{
  if (priv_->next_alias_.expired())
    return elf_symbol_sptr();
  return elf_symbol_sptr(priv_->next_alias_);
}


/// Check if the current elf_symbol has an alias.
///
///@return true iff the current elf_symbol has an alias.
bool
elf_symbol::has_aliases() const
{return get_next_alias();}

/// Get the number of aliases to this elf symbol
///
/// @return the number of aliases to this elf symbol.
int
elf_symbol::get_number_of_aliases() const
{
  int result = 0;

  for (elf_symbol_sptr a = get_next_alias();
       a && a.get() != get_main_symbol().get();
       a = a->get_next_alias())
    ++result;

  return result;
}

/// Add an alias to the current elf symbol.
///
/// @param alias the new alias.  Note that this elf_symbol should *NOT*
/// have aliases prior to the invocation of this function.
void
elf_symbol::add_alias(elf_symbol_sptr alias)
{
  if (!alias)
    return;

  assert(!alias->has_aliases());
  assert(is_main_symbol());

  if (has_aliases())
    {
      elf_symbol_sptr last_alias;
      for (elf_symbol_sptr a = get_next_alias();
	   a && (a != get_main_symbol());
	   a = a->get_next_alias())
	{
	  if (a->get_next_alias() == get_main_symbol())
	    {
	      assert(last_alias == 0);
	      last_alias = a;
	    }
	}
      assert(last_alias);

      last_alias->priv_->next_alias_ = alias;
    }
  else
    priv_->next_alias_ = alias;

  alias->priv_->next_alias_ = get_main_symbol();
  alias->priv_->main_symbol_ = get_main_symbol();
}

/// Get a string that is representative of a given elf_symbol.
///
/// If the symbol has a version, then the ID string is the
/// concatenation of the name of the symbol, the '@' character, and
/// the version of the symbol.  If the version is the default version
/// of the symbol then the '@' character is replaced by a "@@" string.
///
/// Otherwise, if the symbol does not have any version, this function
/// returns the name of the symbol.
///
/// @return a the ID string.
const string&
elf_symbol::get_id_string() const
{
  if (priv_->id_string_.empty())
    {
      string s = get_name ();

      if (!get_version().is_empty())
	{
	  if (get_version().is_default())
	    s += "@@";
	  else
	    s += "@";
	  s += get_version().str();
	}
      priv_->id_string_ = s;
    }

  return priv_->id_string_;
}

/// From the aliases of the current symbol, lookup one with a given name.
///
/// @param name the name of symbol alias we are looking for.
///
/// @return the symbol alias that has the name @p name, or nil if none
/// has been found.
elf_symbol_sptr
elf_symbol::get_alias_from_name(const string& name) const
{
  if (name == get_name())
    return elf_symbol_sptr(priv_->main_symbol_);

   for (elf_symbol_sptr a = get_next_alias();
	a && a.get() != get_main_symbol().get();
	a = a->get_next_alias())
     if (a->get_name() == name)
       return a;

   return elf_symbol_sptr();
}

/// In the list of aliases of a given elf symbol, get the alias that
/// equals this current symbol.
///
/// @param other the elf symbol to get the potential aliases from.
///
/// @return the alias of @p other that texually equals the current
/// symbol, or nil if no alias textually equals the current symbol.
elf_symbol_sptr
elf_symbol::get_alias_which_equals(const elf_symbol& other) const
{
  for (elf_symbol_sptr a = other.get_next_alias();
       a && a.get() != a->get_main_symbol().get();
       a = a->get_next_alias())
    if (textually_equals(*this, *a))
      return a;
  return elf_symbol_sptr();
}

/// Return a comma separated list of the id of the current symbol as
/// well as the id string of its aliases.
///
/// @param syms a map of all the symbols of the corpus the current
/// symbol belongs to.
///
/// @param include_symbol_itself if set to true, then the name of the
/// current symbol is included in the list of alias names that is emitted.
///
/// @return the string.
string
elf_symbol::get_aliases_id_string(const string_elf_symbols_map_type& syms,
				  bool include_symbol_itself) const
{
  string result;

  if (include_symbol_itself)
      result = get_id_string();

  vector<elf_symbol_sptr> aliases;
  compute_aliases_for_elf_symbol(*this, syms, aliases);
  if (!aliases.empty() && include_symbol_itself)
    result += ", ";

  for (vector<elf_symbol_sptr>::const_iterator i = aliases.begin();
       i != aliases.end();
       ++i)
    {
      if (i != aliases.begin())
	result += ", ";
      result += (*i)->get_id_string();
    }
  return result;
}

/// Return a comma separated list of the id of the current symbol as
/// well as the id string of its aliases.
///
/// @param include_symbol_itself if set to true, then the name of the
/// current symbol is included in the list of alias names that is emitted.
///
/// @return the string.
string
elf_symbol::get_aliases_id_string(bool include_symbol_itself) const
{
  vector<elf_symbol_sptr> aliases;
  if (include_symbol_itself)
    aliases.push_back(get_main_symbol());

  for (elf_symbol_sptr a = get_next_alias();
       a && a.get() != get_main_symbol().get();
       a = a->get_next_alias())
    aliases.push_back(a);

  string result;
  for (vector<elf_symbol_sptr>::const_iterator i = aliases.begin();
       i != aliases.end();
       ++i)
    {
      if (i != aliases.begin())
	result += ", ";
      result += (*i)->get_id_string();
    }

  return result;
}

/// Given the ID of a symbol, get the name and the version of said
/// symbol.
///
/// @param id the symbol ID to consider.
///
/// @param name the symbol name extracted from the ID.  This is set
/// only if the function returned true.
///
/// @param ver the symbol version extracted from the ID.
bool
elf_symbol::get_name_and_version_from_id(const string&	id,
					 string&	name,
					 string&	ver)
{
  name.clear(), ver.clear();

  string::size_type i = id.find("@");
  if (i == string::npos)
    {
      name = id;
      return true;
    }

  name = id.substr(0, i);
  ++i;

  if (i >= id.size())
    return true;

  string::size_type j = id.find("@", i);
  if (j == string::npos)
    j = i;
  else
    ++j;

  if (j >= id.size())
    {
      ver = "";
      return true;
    }

  ver = id.substr(j);
  return true;
}

///@}

/// Test if two main symbols are textually equal, or, if they have
/// aliases that are textually equal.
///
/// @param other the symbol to compare against.
///
/// @return true iff the current instance of elf symbol equals the @p
/// other.
bool
elf_symbol::operator==(const elf_symbol& other) const
{
  bool are_equal = textually_equals(*this, other);
  return are_equal;
  if (!are_equal)
    are_equal = get_alias_which_equals(other);
    return are_equal;
}

/// Test if the current symbol aliases another one.
///
/// @param o the other symbol to test against.
///
/// @return true iff the current symbol aliases @p o.
bool
elf_symbol::does_alias(const elf_symbol& o) const
{
  if (*this == o)
    return true;

  if (get_main_symbol() == o.get_main_symbol())
    return true;

  for (elf_symbol_sptr a = get_next_alias();
       a && a!= get_main_symbol();
       a = a->get_next_alias())
    {
      if (o == *a)
	return true;
    }
  return false;
}

bool
operator==(const elf_symbol_sptr& lhs, const elf_symbol_sptr& rhs)
{
  if (!!lhs != !!rhs)
    return false;

  if (!lhs)
    return true;

  return *lhs == *rhs;
}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol& s1, const elf_symbol& s2)
{return s1.does_alias(s2) || s2.does_alias(s1);}

void
compute_aliases_for_elf_symbol(const elf_symbol& sym,
			       const string_elf_symbols_map_type& symtab,
			       vector<elf_symbol_sptr>& aliases)
{

  if (elf_symbol_sptr a = sym.get_next_alias())
    for (; a != sym.get_main_symbol(); a = a->get_next_alias())
      aliases.push_back(a);
  else
    for (string_elf_symbols_map_type::const_iterator i = symtab.begin();
	 i != symtab.end();
	 ++i)
      for (elf_symbols::const_iterator j = i->second.begin();
	   j != i->second.end();
	   ++j)
	{
	  if (**j == sym)
	    for (elf_symbol_sptr s = (*j)->get_next_alias();
		 s && s != (*j)->get_main_symbol();
		 s = s->get_next_alias())
	      aliases.push_back(s);
	  else
	    for (elf_symbol_sptr s = (*j)->get_next_alias();
		 s && s != (*j)->get_main_symbol();
		 s = s->get_next_alias())
	      if (*s == sym)
		aliases.push_back(*j);
	}
}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol* s1, const elf_symbol* s2)
{
  if (!!s1 != !!s2)
    return false;
  if (s1 == s2)
    return true;
  return elf_symbols_alias(*s1, *s2);
}

/// Test if two symbols alias.
///
/// @param s1 the first symbol to consider.
///
/// @param s2 the second symbol to consider.
///
/// @return true if @p s1 aliases @p s2.
bool
elf_symbols_alias(const elf_symbol_sptr s1, const elf_symbol_sptr s2)
{return elf_symbols_alias(s1.get(), s2.get());}

/// Serialize an instance of @ref symbol_type and stream it to a given
/// output stream.
///
/// @param o the output stream to serialize the symbole type to.
///
/// @param t the symbol type to serialize.
std::ostream&
operator<<(std::ostream& o, elf_symbol::type t)
{
  string repr;

  switch (t)
    {
    case elf_symbol::NOTYPE_TYPE:
      repr = "unspecified symbol type";
      break;
    case elf_symbol::OBJECT_TYPE:
      repr = "variable symbol type";
      break;
    case elf_symbol::FUNC_TYPE:
      repr = "function symbol type";
      break;
    case elf_symbol::SECTION_TYPE:
      repr = "section symbol type";
      break;
    case elf_symbol::FILE_TYPE:
      repr = "file symbol type";
      break;
    case elf_symbol::COMMON_TYPE:
      repr = "common data object symbol type";
      break;
    case elf_symbol::TLS_TYPE:
      repr = "thread local data object symbol type";
      break;
    case elf_symbol::GNU_IFUNC_TYPE:
      repr = "indirect function symbol type";
      break;
    default:
      {
	std::ostringstream s;
	s << "unknown symbol type (" << (char)t << ')';
	repr = s.str();
      }
      break;
    }

  o << repr;
  return o;
}

/// Serialize an instance of @ref symbol_binding and stream it to a
/// given output stream.
///
/// @param o the output stream to serialize the symbole type to.
///
/// @param t the symbol binding to serialize.
std::ostream&
operator<<(std::ostream& o, elf_symbol::binding b)
{
  string repr;

  switch (b)
    {
    case elf_symbol::LOCAL_BINDING:
      repr = "local binding";
      break;
    case elf_symbol::GLOBAL_BINDING:
      repr = "global binding";
      break;
    case elf_symbol::WEAK_BINDING:
      repr = "weak binding";
      break;
    case elf_symbol::GNU_UNIQUE_BINDING:
      repr = "GNU unique binding";
      break;
    default:
      {
	std::ostringstream s;
	s << "unknown binding (" << (unsigned char) b << ")";
	repr = s.str();
      }
      break;
    }

  o << repr;
  return o;
}

/// Convert a string representing a symbol type into an
/// elf_symbol::type.
///
///@param s the string to convert.
///
///@param t the resulting elf_symbol::type.
///
/// @return true iff the conversion completed successfully.
bool
string_to_elf_symbol_type(const string& s, elf_symbol::type& t)
{
  if (s == "no-type")
    t = elf_symbol::NOTYPE_TYPE;
  else if (s == "object-type")
    t = elf_symbol::OBJECT_TYPE;
  else if (s == "func-type")
    t = elf_symbol::FUNC_TYPE;
  else if (s == "section-type")
    t = elf_symbol::SECTION_TYPE;
  else if (s == "file-type")
    t = elf_symbol::FILE_TYPE;
  else if (s == "common-type")
    t = elf_symbol::COMMON_TYPE;
  else if (s == "tls-type")
    t = elf_symbol::TLS_TYPE;
  else if (s == "gnu-ifunc-type")
    t = elf_symbol::GNU_IFUNC_TYPE;
  else
    return false;

  return true;
}

/// Convert a string representing a an elf symbol binding into an
/// elf_symbol::binding.
///
/// @param s the string to convert.
///
/// @param b the resulting elf_symbol::binding.
///
/// @return true iff the conversion completed successfully.
bool
string_to_elf_symbol_binding(const string& s, elf_symbol::binding& b)
{
    if (s == "local-binding")
      b = elf_symbol::LOCAL_BINDING;
    else if (s == "global-binding")
      b = elf_symbol::GLOBAL_BINDING;
    else if (s == "weak-binding")
      b = elf_symbol::WEAK_BINDING;
    else if (s == "gnu-unique-binding")
      b = elf_symbol::GNU_UNIQUE_BINDING;
    else
      return false;

    return true;
}

// <elf_symbol::version stuff>

struct elf_symbol::version::priv
{
  string	version_;
  bool		is_default_;

  priv()
    : is_default_(false)
  {}

  priv(const string& v,
       bool d)
    : version_(v),
      is_default_(d)
  {}
}; // end struct elf_symbol::version::priv

elf_symbol::version::version()
  : priv_(new priv)
{}

/// @param v the name of the version.
///
/// @param is_default true if this is a default version.
elf_symbol::version::version(const string& v,
			     bool is_default)
  : priv_(new priv(v, is_default))
{}

elf_symbol::version::version(const elf_symbol::version& v)
  : priv_(new priv(v.str(), v.is_default()))
{
}

/// Cast the version_type into a string that is its name.
///
/// @return the name of the version.
elf_symbol::version::operator const string&() const
{return priv_->version_;}

/// Getter for the version name.
///
/// @return the version name.
const string&
elf_symbol::version::str() const
{return priv_->version_;}

/// Setter for the version name.
///
/// @param s the version name.
void
elf_symbol::version::str(const string& s)
{priv_->version_ = s;}

/// Getter for the 'is_default' property of the version.
///
/// @return true iff this is a default version.
bool
elf_symbol::version::is_default() const
{return priv_->is_default_;}

/// Setter for the 'is_default' property of the version.
///
/// @param f true if this is the default version.
void
elf_symbol::version::is_default(bool f)
{priv_->is_default_ = f;}

bool
elf_symbol::version::is_empty() const
{return str().empty();}

/// Compares the current version against another one.
///
/// @param o the other version to compare the current one to.
///
/// @return true iff the current version equals @p o.
bool
elf_symbol::version::operator==(const elf_symbol::version& o) const
{return str() == o.str();}

/// Assign a version to the current one.
///
/// @param o the other version to assign to this one.
///
/// @return a reference to the assigned version.
elf_symbol::version&
elf_symbol::version::operator=(const elf_symbol::version& o)
{
  str(o.str());
  is_default(o.is_default());
  return *this;
}

// </elf_symbol::version stuff>

// </elf_symbol stuff>

dm_context_rel::~dm_context_rel()
{}

// <environment stuff>

/// The private data of the @ref environment type.
struct environment::priv
{
  bool				canonicalization_is_done_;
  canonical_types_map_type	canonical_types_;
  type_decl_sptr		void_type_decl_;
  type_decl_sptr		variadic_marker_type_decl_;
  unordered_map<string, bool>	classes_being_compared_;
  vector<type_base_sptr>	extra_live_types_;

  priv()
    : canonicalization_is_done_()
  {}
};// end struct environment::priv

/// Default constructor of the @ref environment type.
environment::environment()
  :priv_(new priv)
{}

/// Destructor for the @ref environment type.
environment::~environment()
{}

/// Getter the map of canonical types.
///
/// @return the map of canonical types.  The key of the map is the
/// hash of the canonical type and its value if the canonical type.
environment::canonical_types_map_type&
environment::get_canonical_types_map()
{return priv_->canonical_types_;}

/// Get a @ref type_decl that represents a "void" type for the current
/// environment.
///
/// @return the @ref type_decl that represents a "void" type.
const type_decl_sptr&
environment::get_void_type_decl() const
{
  if (!priv_->void_type_decl_)
    {
      priv_->void_type_decl_.reset(new type_decl("void", 0, 0, location()));
      priv_->void_type_decl_->set_environment(const_cast<environment*>(this));
    }
  return priv_->void_type_decl_;
}

/// Get a @ref type_decl instance that represents a the type of a
/// variadic function parameter.
///
/// @return the Get a @ref type_decl instance that represents a the
/// type of a variadic function parameter.
const type_decl_sptr&
environment::get_variadic_parameter_type_decl() const
{
  if (!priv_->variadic_marker_type_decl_)
    {
      priv_->variadic_marker_type_decl_.
	reset(new type_decl("variadic parameter type",
			    0, 0, location()));
      priv_->variadic_marker_type_decl_->
	set_environment(const_cast<environment*>(this));
    }
  return priv_->variadic_marker_type_decl_;
}

/// Test if the canonicalization of types created out of the current
/// environment is done.
///
/// @return true iff the canonicalization of types created out of the current
/// environment is done.
bool
environment::canonicalization_is_done() const
{return priv_->canonicalization_is_done_;}

/// Set a flag saying if the canonicalization of types created out of
/// the current environment is done or not.
///
/// Note that this function must only be called by internal code of
/// the library that creates ABI artifacts (e.g, read an abi corpus
/// from elf or from our own xml format and creates representations of
/// types out of it) and thus needs to canonicalize types to speed-up
/// further type comparison.
///
/// @param f the new value of the flag.
void
environment::canonicalization_is_done(bool f)
{priv_->canonicalization_is_done_ = f;}

// </environment stuff>

// <type_or_decl_base stuff>

/// The private data of @ref type_or_decl_base.
struct type_or_decl_base::priv
{
  bool			hashing_started_;
  environment*		env_;
  const translation_unit* translation_unit_;

  priv()
    : hashing_started_(),
      env_(),
      translation_unit_()
  {}
}; // end struct type_or_decl_base

/// Default constructor of @ref type_or_decl_base.
type_or_decl_base::type_or_decl_base()
  :priv_(new priv)
{}

/// Copy constructor of @ref type_or_decl_base.
type_or_decl_base::type_or_decl_base(const type_or_decl_base& o)
{*priv_ = *o.priv_;}

/// The destructor of the @ref type_or_decl_base type.
type_or_decl_base::~type_or_decl_base()
{}

/// Getter for the 'hashing_started' property.
///
/// @return the 'hashing_started' property.
bool
type_or_decl_base::hashing_started() const
{return priv_->hashing_started_;}

/// Setter for the 'hashing_started' property.
///
/// @param b the value to set the 'hashing_property' to.
void
type_or_decl_base::hashing_started(bool b) const
{priv_->hashing_started_ = b;}

/// Setter of the environment of the current ABI artifact.
///
/// This just sets the environment artifact of the current ABI
/// artifact, not on its sub-trees.  If you want to set the
/// environment of an ABI artifact including its sub-tree, use the
/// abigail::ir::set_environment_for_artifact() function.
///
/// @param env the new environment.
void
type_or_decl_base::set_environment(environment* env)
{priv_->env_ = env;}

/// Getter of the environment of the current ABI artifact.
///
/// @return the environment of the artifact.
const environment*
type_or_decl_base::get_environment() const
{return priv_->env_;}

/// Getter of the environment of the current ABI artifact.
///
/// @return the environment of the artifact.
environment*
type_or_decl_base::get_environment()
{return priv_->env_;}

/// Get the @ref corpus this ABI artifact belongs to.
///
/// @return the corpus this ABI artifact belongs to, or nil if it
/// belongs to none for now.
const corpus*
type_or_decl_base::get_corpus() const
{
  const translation_unit* tu = get_translation_unit();
  if (!tu)
    return 0;
  return tu->get_corpus();
}

/// Set the @ref translation_unit this ABI artifact belongs to.
///
/// Note that adding an ABI artifact to a containining on should
/// invoke this member function.
void
type_or_decl_base::set_translation_unit(const translation_unit* tu)
{priv_->translation_unit_ = tu;}

/// Get the @ref translation_unit this ABI artifact belongs to.
///
/// @return the translation unit this ABI artifact belongs to, or nil
/// if belongs to none for now.
const translation_unit*
type_or_decl_base::get_translation_unit() const
{return priv_->translation_unit_;}

/// Traverse the the ABI artifact.
///
/// @param v the visitor used to traverse the sub-tree nodes of the
/// artifact.
bool
type_or_decl_base::traverse(ir_node_visitor&)
{return true;}

/// Set the environment of a given ABI artifact, including recursively
/// setting the environment on the sub-trees of the artifact.
///
/// @param artifact the artifact to set the environment for.
///
/// @param env the new environment.
void
set_environment_for_artifact(type_or_decl_base* artifact, environment* env)
{
  assert(artifact && env);

  ::environment_setter s(artifact, env);
  artifact->traverse(s);
}

/// Set the environment of a given ABI artifact, including recursively
/// setting the environment on the sub-trees of the artifact.
///
/// @param artifact the artifact to set the environment for.
///
/// @param env the new environment.
void
set_environment_for_artifact(type_or_decl_base_sptr artifact,
			     environment* env)
{set_environment_for_artifact(artifact.get(), env);}

/// Non-member equality operator for the @type_or_decl_base type.
///
/// @param lr the left-hand operand of the equality.
///
/// @param rr the right-hand operatnr of the equality.
///
/// @return true iff @p lr equals @p rr.
bool
operator==(const type_or_decl_base& lr, const type_or_decl_base& rr)
{
  const type_or_decl_base* l = &lr;
  const type_or_decl_base* r = &rr;

  const decl_base* dl = dynamic_cast<const decl_base*>(l),
    *dr = dynamic_cast<const decl_base*>(r);

  if (!!dl != !!dr)
    return false;

  if (dl && dr)
    return *dl == *dr;

  const type_base* tl = dynamic_cast<const type_base*>(l),
    *tr = dynamic_cast<const type_base*>(r);

  if (!!tl != !!tr)
    return false;

  if (tl && tr)
    return *tl == *tr;

  return false;
}

/// Non-member equality operator for the @type_or_decl_base type.
///
/// @param l the left-hand operand of the equality.
///
/// @param r the right-hand operatnr of the equality.
///
/// @return true iff @p l equals @p r.
bool
operator==(const type_or_decl_base_sptr& l, const type_or_decl_base_sptr& r)
{
  if (!! l != !!r)
    return false;

  if (!l)
    return true;

  return *r == *l;
}

// </type_or_decl_base stuff>

// <Decl definition>

struct decl_base::priv
{
  bool			in_pub_sym_tab_;
  bool			is_anonymous_;
  location		location_;
  context_rel_sptr	context_;
  std::string		name_;
  std::string		qualified_parent_name_;
  // This temporary qualified name is the cache used for the qualified
  // name before the type associated to this decl (if applicable) is
  // canonicalized.  Once the type is canonicalized, the cached use is
  // the data member qualified_parent_name_ above.
  std::string		temporary_qualified_name_;
  std::string		qualified_name_;
  std::string		linkage_name_;
  visibility		visibility_;

  priv()
    : in_pub_sym_tab_(false),
      is_anonymous_(true),
      visibility_(VISIBILITY_DEFAULT)
  {}

  priv(const std::string& name, const location& locus,
       const std::string& linkage_name, visibility vis)
    : in_pub_sym_tab_(false),
      location_(locus),
      name_(name),
      linkage_name_(linkage_name),
      visibility_(vis)
  {
    is_anonymous_ = name_.empty();
  }

  priv(location l)
    : in_pub_sym_tab_(false),
      is_anonymous_(true),
      location_(l),
      visibility_(VISIBILITY_DEFAULT)
  {}
};// end struct decl_base::priv

/// Constructor for the @ref decl_base type.
///
/// @param name the name of the declaration.
///
/// @param locus the location where to find the declaration in the
/// source code.
///
/// @pram linkage_name the linkage name of the declaration.
///
/// @param vis the visibility of the declaration.
decl_base::decl_base(const std::string&	name,
		     const location&		locus,
		     const std::string&	linkage_name,
		     visibility		vis)
  : priv_(new priv(name, locus, linkage_name, vis))
{}

/// Constructor for the @ref decl_base type.
///
/// @param l the location where to find the declaration in the source
/// code.
decl_base::decl_base(const location& l)
  : priv_(new priv(l))
{}

decl_base::decl_base(const decl_base& d)
  : type_or_decl_base(d)
{
  priv_->in_pub_sym_tab_ = d.priv_->in_pub_sym_tab_;
  priv_->location_ = d.priv_->location_;
  priv_->name_ = d.priv_->name_;
  priv_->qualified_parent_name_ = d.priv_->qualified_parent_name_;
  priv_->qualified_name_ = d.priv_->qualified_name_;
  priv_->linkage_name_ = d.priv_->linkage_name_;
  priv_->context_ = d.priv_->context_;
  priv_->visibility_ = d.priv_->visibility_;
}

/// Getter for the qualified name.
///
/// Unlike decl_base::get_qualified_name() this doesn't try to update
/// the qualified name.
///
/// @return the qualified name.
const string&
decl_base::peek_qualified_name() const
{return priv_->qualified_name_;}

/// Setter for the qualified name.
///
/// @param n the new qualified name.
void
decl_base::set_qualified_name(const string& n) const
{priv_->qualified_name_ = n;}

/// Getter of the temporary qualified name of the current declaration.
///
/// This temporary qualified name is used as a qualified name cache by
/// the type for which this is the declaration (when applicable)
/// before the type is canonicalized.  Once the type is canonicalized,
/// it's the result of decl_base::peek_qualified_name() that becomes
/// the qualified name cached.
///
/// @return the temporary qualified name.
const string&
decl_base::peek_temporary_qualified_name() const
{return priv_->temporary_qualified_name_;}

/// Setter for the temporary qualified name of the current
/// declaration.
///
///@param n the new temporary qualified name.
///
/// This temporary qualified name is used as a qualified name cache by
/// the type for which this is the declaration (when applicable)
/// before the type is canonicalized.  Once the type is canonicalized,
/// it's the result of decl_base::peek_qualified_name() that becomes
/// the qualified name cached.
void
decl_base::set_temporary_qualified_name(const string& n) const
{priv_->temporary_qualified_name_ = n;}

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
const context_rel*
decl_base::get_context_rel() const
{return priv_->context_.get();}

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
context_rel*
decl_base::get_context_rel()
{return priv_->context_.get();}

void
decl_base::set_context_rel(context_rel_sptr c)
{priv_->context_ = c;}

/// Get the hash of a decl.  If the hash hasn't been computed yet,
/// compute it ans store its value; otherwise, just return the hash.
///
/// @return the hash of the decl.
size_t
decl_base::get_hash() const
{
  size_t result = 0;

  if (const type_base* t = dynamic_cast<const type_base*>(this))
    {
      type_base::dynamic_hash hash;
      result = hash(t);
    }
  else
    // If we reach this point, it mean we are missing a virtual
    // overload for decl_base::get_hash.  Add it!
    abort();

  return result;
}

/// Test if the decl is defined in a ELF symbol table as a public
/// symbol.
///
/// @return true iff the decl is defined in a ELF symbol table as a
/// public symbol.
bool
decl_base::get_is_in_public_symbol_table() const
{return priv_->in_pub_sym_tab_;}

/// Set the flag saying if this decl is from a symbol that is in
/// a public symbols table, defined as public (global or weak).
///
/// @param f the new flag value.
void
decl_base::set_is_in_public_symbol_table(bool f)
{priv_->in_pub_sym_tab_ = f;}

/// Get the location of a given declaration.
///
/// The location is an abstraction for the tripplet {file path,
/// line, column} that defines where the declaration appeared in the
/// source code.
///
/// To get the value of the tripplet {file path, line, column} from
/// the @ref location, you need to use the
/// location_manager::expand_location() method.
///
/// The instance of @ref location_manager that you want is
/// accessible from the instance of @ref translation_unit that the
/// current instance of @ref decl_base belongs to, via a call to
/// translation_unit::get_loc_mgr().
///
/// @return the location of the current instance of @ref decl_base.
const location&
decl_base::get_location() const
{return priv_->location_;}

/// Set the location for a given declaration.
///
/// The location is an abstraction for the tripplet {file path,
/// line, column} that defines where the declaration appeared in the
/// source code.
///
/// To create a location from a tripplet {file path, line, column},
/// you need to use the method @ref
/// location_manager::create_new_location().
///
/// The instance of @ref location_manager that you want is
/// accessible from the instance of @ref translation_unit that the
/// current instance of @ref decl_base belongs to, via a call to
/// translation_unit::get_loc_mgr().
void
decl_base::set_location(const location& l)
{priv_->location_ = l;}

/// Setter for the name of the decl.
///
/// @param n the new name to set.
void
decl_base::set_name(const string& n)
{
  priv_->name_ = n;
  priv_->is_anonymous_ = priv_->name_.empty();
}

/// Test if the current declaration is anonymous.
///
/// Being anonymous means that the declaration was created without a
/// name.  This can usually happen for enum or struct types.
///
/// @return true iff the type is anonymous.
bool
decl_base::get_is_anonymous() const
{return priv_->is_anonymous_;}

/// Set the "is_anonymous" flag of the current declaration.
///
/// Being anonymous means that the declaration was created without a
/// name.  This can usually happen for enum or struct types.
///
/// @param f the new value of the flag.
void
decl_base::set_is_anonymous(bool f)
{priv_->is_anonymous_ = f;}

/// Getter for the mangled name.
///
/// @return the new mangled name.
const string&
decl_base::get_linkage_name() const
{return priv_->linkage_name_;}

/// Setter for the linkage name.
///
/// @param m the new linkage name.
void
decl_base::set_linkage_name(const std::string& m)
{priv_->linkage_name_ = m;}

/// Getter for the visibility of the decl.
///
/// @return the new visibility.
decl_base::visibility
decl_base::get_visibility() const
{return priv_->visibility_;}

/// Setter for the visibility of the decl.
///
/// @param v the new visibility.
void
decl_base::set_visibility(visibility v)
{priv_->visibility_ = v;}

/// Return the type containing the current decl, if any.
///
/// @return the type that contains the current decl, or NULL if there
/// is none.
scope_decl*
decl_base::get_scope() const
{
  if (priv_->context_)
    return priv_->context_->get_scope();
  return 0;
}

/// Return a copy of the qualified name of the parent of the current
/// decl.
///
/// @return the newly-built qualified name of the of the current decl.
const string&
decl_base::get_qualified_parent_name() const
{return priv_->qualified_parent_name_;}

/// Getter for the name of the current decl.
///
/// @return the name of the current decl.
const string&
decl_base::get_name() const
{return priv_->name_;}

/// Compute the qualified name of the decl.
///
/// @param qn the resulting qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
decl_base::get_qualified_name(string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Get the pretty representatin of the current declaration.
///
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the default pretty representation for a decl.  This is
/// basically the fully qualified name of the decl optionally prefixed
/// with a meaningful string to add context for the user.
string
decl_base::get_pretty_representation(bool internal) const
{return get_qualified_name(internal);}

/// Compute the qualified name of the decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const string&
decl_base::get_qualified_name(bool /*internal*/) const
{return priv_->qualified_name_.empty() ? get_name() : priv_->qualified_name_;}

change_kind
operator|(change_kind l, change_kind r)
{
  return static_cast<change_kind>(static_cast<unsigned>(l)
				  | static_cast<unsigned>(r));
}

change_kind
operator&(change_kind l, change_kind r)
{
  return static_cast<change_kind>(static_cast<unsigned>(l)
				  & static_cast<unsigned>(r));
}

change_kind&
operator|=(change_kind& l, change_kind r)
{
  l = l | r;
  return l;
}

change_kind&
operator&=(change_kind& l, change_kind r)
{
  l = l & r;
  return l;
}

/// Compares two instances of @ref decl_base.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff it's non-null and if the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const decl_base& l, const decl_base& r, change_kind* k)
{
  bool result = true;
  if (!l.get_linkage_name().empty()
      && !r.get_linkage_name().empty())
    {
      if (l.get_linkage_name() != r.get_linkage_name())
	{
	  // Linkage names are different.  That usually means the two
	  // decls are different, unless we are looking at two
	  // function declarations which have two different symbols
	  // that are aliases of each other.
	  const function_decl *f1 = is_function_decl(&l),
	    *f2 = is_function_decl(&r);
	  if (f1 && f2 && function_decls_alias(*f1, *f2))
	    ;// The two functions are aliases, so they are not different.
	  else
	    {
	      result = false;
	      if (k)
		*k |= LOCAL_CHANGE_KIND;
	      else
		return false;
	    }
	}
    }

  if (l.get_qualified_name() != r.get_qualified_name())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_decl(l) && is_member_decl(r))
    {
      context_rel* r1 = const_cast<context_rel*>(l.get_context_rel());
      context_rel *r2 = const_cast<context_rel*>(r.get_context_rel());

      access_specifier la = no_access, ra = no_access;
      bool member_types_or_functions =
	((is_type(l) && is_type(r))
	 || (is_function_decl(l) && is_function_decl(r)));

      if (member_types_or_functions)
	{
	  // Access specifiers on member types in DWARF is not
	  // reliable; in the same DSO, the same struct can be either
	  // a class or a struct, and the access specifiers of its
	  // member types are not necessarily given, so they
	  // effectively can be considered differently, again, in the
	  // same DSO.  So, here, let's avoid considering those!
	  // during comparison.
	  la = r1->get_access_specifier();
	  ra = r2->get_access_specifier();
	  r1->set_access_specifier(no_access);
	  r2->set_access_specifier(no_access);
	}

      bool rels_are_different = *r1 != *r2;

      if (member_types_or_functions)
	{
	  // restore the access specifiers.
	  r1->set_access_specifier(la);
	  r2->set_access_specifier(ra);
	}

      if (rels_are_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }

  return result;
}

/// Return true iff the two decls have the same name.
///
/// This function doesn't test if the scopes of the the two decls are
/// equal.
///
/// Note that this virtual function is to be implemented by classes
/// that extend the \p decl_base class.
bool
decl_base::operator==(const decl_base& other) const
{return equals(*this, other, 0);}

/// Destructor of the @ref decl_base type.
decl_base::~decl_base()
{delete priv_;}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
decl_base::traverse(ir_node_visitor&)
{
  // Do nothing in the base class.
  return true;
}

/// Setter of the scope of the current decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
void
decl_base::set_scope(scope_decl* scope)
{
  if (!priv_->context_)
    priv_->context_.reset(new context_rel(scope));
  else
    priv_->context_->set_scope(scope);
}

// </decl_base definition>

/// Streaming operator for the decl_base::visibility.
///
/// @param o the output stream to serialize the visibility to.
///
/// @param v the visibility to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, decl_base::visibility v)
{
  string r;
  switch (v)
    {
    case decl_base::VISIBILITY_NONE:
      r = "none";
      break;
    case decl_base::VISIBILITY_DEFAULT:
      r = "default";
      break;
    case decl_base::VISIBILITY_PROTECTED:
      r = "protected";
      break;
    case decl_base::VISIBILITY_HIDDEN:
      r = "hidden";
      break;
    case decl_base::VISIBILITY_INTERNAL:
      r = "internal";
      break;
    }
  return o;
}

/// Streaming operator for decl_base::binding.
///
/// @param o the output stream to serialize the visibility to.
///
/// @param b the binding to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, decl_base::binding b)
{
  string r;
  switch (b)
    {
    case decl_base::BINDING_NONE:
      r = "none";
      break;
    case decl_base::BINDING_LOCAL:
      r = "local";
      break;
    case decl_base::BINDING_GLOBAL:
      r = "global";
      break;
    case decl_base::BINDING_WEAK:
      r = "weak";
      break;
    }
  o << r;
  return o;
}

/// Turn equality of shared_ptr of decl_base into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of decl_base on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of decl_base on right-hand-side of the
/// equality.
///
/// @return true if the decl_base pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const decl_base_sptr& l, const decl_base_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Turn equality of shared_ptr of type_base into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of type_base on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of type_base on right-hand-side of the
/// equality.
///
/// @return true if the type_base pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const type_base_sptr& l, const type_base_sptr& r)
{
    if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Tests if a declaration has got a scope.
///
/// @param d the decalaration to consider.
///
/// @return true if the declaration has got a scope, false otherwise.
bool
has_scope(const decl_base& d)
{return (d.get_scope());}

/// Tests if a declaration has got a scope.
///
/// @param d the decalaration to consider.
///
/// @return true if the declaration has got a scope, false otherwise.
bool
has_scope(const decl_base_sptr d)
{return has_scope(*d.get());}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base_sptr d)
{return is_at_class_scope(d);}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base* d)
{return is_at_class_scope(d);}

/// Tests if a declaration is a class member.
///
/// @param d the declaration to consider.
///
/// @return true if @p d is a class member, false otherwise.
bool
is_member_decl(const decl_base& d)
{return is_at_class_scope(d);}

/// Test if a declaration is a @ref scope_decl.
///
/// @param d the declaration to take in account.
///
/// @return the a pointer to the @ref scope_decl sub-object of @p d,
/// if d is a @ref scope_decl.
scope_decl*
is_scope_decl(decl_base* d)
{return dynamic_cast<scope_decl*>(d);}

/// Tests if a type is a class member.
///
/// @param t the type to consider.
///
/// @return true if @p t is a class member type, false otherwise.
bool
is_member_type(const type_base_sptr t)
{
  decl_base_sptr d = get_type_declaration(t);
  return is_member_decl(d);
}

/// Gets the access specifier for a class member.
///
/// @param d the declaration of the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return the access specifier for the class member @p d.
access_specifier
get_member_access_specifier(const decl_base& d)
{
  assert(is_member_decl(d));

  const context_rel* c = d.get_context_rel();
  assert(c);

  return c->get_access_specifier();
}

/// Gets the access specifier for a class member.
///
/// @param d the declaration of the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return the access specifier for the class member @p d.
access_specifier
get_member_access_specifier(const decl_base_sptr& d)
{return get_member_access_specifier(*d);}

/// Sets the access specifier for a class member.
///
/// @param d the class member to set the access specifier for.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @param a the new access specifier to set the class member to.
void
set_member_access_specifier(decl_base& d,
			    access_specifier a)
{
  assert(is_member_decl(d));

  context_rel* c = d.get_context_rel();
  assert(c);

  c->set_access_specifier(a);
}

/// Sets the access specifier for a class member.
///
/// @param d the class member to set the access specifier for.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @param a the new access specifier to set the class member to.
void
set_member_access_specifier(const decl_base_sptr& d,
			    access_specifier a)
{set_member_access_specifier(*d, a);}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider. Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base&d)
{
  assert(is_member_decl(d));

  const context_rel* c = d.get_context_rel();
  assert(c);

  return c->get_is_static();
}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider. Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base* d)
{return get_member_is_static(*d);}

/// Gets a flag saying if a class member is static or not.
///
/// @param d the declaration for the class member to consider.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @return true if the class member @p d is static, false otherwise.
bool
get_member_is_static(const decl_base_sptr& d)
{return get_member_is_static(*d);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl& v)
{return is_at_class_scope(v);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl* v)
{return is_data_member(*v);}

/// Test if a var_decl is a data member.
///
/// @param v the var_decl to consider.
///
/// @return true if @p v is data member, false otherwise.
bool
is_data_member(const var_decl_sptr d)
{return is_at_class_scope(d);}

/// Test if a decl is a data member.
///
/// @param d the decl to consider.
///
/// @return a pointer to the data member iff @p d is a data member, or
/// a null pointer.
var_decl_sptr
is_data_member(const decl_base_sptr& d)
{
  if (var_decl_sptr v = is_var_decl(d))
    {
      if (is_data_member(v))
	return v;
    }
  return var_decl_sptr();
}

/// Set the offset of a data member into its containing class.
///
/// @param m the data member to consider.
///
/// @param o the offset, in bits.
void
set_data_member_offset(var_decl_sptr m, size_t o)
{
  assert(is_data_member(m));

  dm_context_rel* ctxt_rel =
    dynamic_cast<dm_context_rel*>(m->get_context_rel());
  assert(ctxt_rel);

  ctxt_rel->set_offset_in_bits(o);
}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
size_t
get_data_member_offset(const var_decl& m)
{
  assert(is_data_member(m));
  const dm_context_rel* ctxt_rel =
    dynamic_cast<const dm_context_rel*>(m.get_context_rel());
  assert(ctxt_rel);
  return ctxt_rel->get_offset_in_bits();
}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
size_t
get_data_member_offset(const var_decl_sptr m)
{return get_data_member_offset(*m);}

/// Get the offset of a data member.
///
/// @param m the data member to consider.
///
/// @return the offset (in bits) of @p m in its containing class.
size_t
get_data_member_offset(const decl_base_sptr d)
{return get_data_member_offset(dynamic_pointer_cast<var_decl>(d));}

/// Set a flag saying if a data member is laid out.
///
/// @param m the data member to consider.
///
/// @param l true if @p m is to be considered as laid out.
void
set_data_member_is_laid_out(var_decl_sptr m, bool l)
{
  assert(is_data_member(m));
  dm_context_rel* ctxt_rel =
    dynamic_cast<dm_context_rel*>(m->get_context_rel());
  ctxt_rel->set_is_laid_out(l);
}

/// Test whether a data member is laid out.
///
/// @param m the data member to consider.
///
/// @return true if @p m is laid out, false otherwise.
bool
get_data_member_is_laid_out(const var_decl& m)
{
  assert(is_data_member(m));
  const dm_context_rel* ctxt_rel =
    dynamic_cast<const dm_context_rel*>(m.get_context_rel());

  return ctxt_rel->get_is_laid_out();
}

/// Test whether a data member is laid out.
///
/// @param m the data member to consider.
///
/// @return true if @p m is laid out, false otherwise.
bool
get_data_member_is_laid_out(const var_decl_sptr m)
{return get_data_member_is_laid_out(*m);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl& f)
{return is_at_class_scope(f);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl* f)
{return is_member_function(*f);}

/// Test whether a function_decl is a member function.
///
/// @param f the function_decl to test.
///
/// @return true if @p f is a member function, false otherwise.
bool
is_member_function(const function_decl_sptr& f)
{return is_member_function(*f);}

/// Test whether a member function is a constructor.
///
/// @param f the member function to test.
///
/// @return true if @p f is a constructor, false otherwise.
bool
get_member_function_is_ctor(const function_decl& f)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_constructor();
}

/// Test whether a member function is a constructor.
///
/// @param f the member function to test.
///
/// @return true if @p f is a constructor, false otherwise.
bool
get_member_function_is_ctor(const function_decl_sptr& f)
{return get_member_function_is_ctor(*f);}


/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(function_decl& f, bool c)
{
  assert(is_member_function(f));

  class_decl::method_decl* m =
    dynamic_cast<class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_constructor(c);
}

/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(const function_decl_sptr& f, bool c)
{set_member_function_is_ctor(*f, c);}

/// Test whether a member function is a destructor.
///
/// @param f the function to test.
///
/// @return true if @p f is a destructor, false otherwise.
bool
get_member_function_is_dtor(const function_decl& f)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_destructor();
}

/// Test whether a member function is a destructor.
///
/// @param f the function to test.
///
/// @return true if @p f is a destructor, false otherwise.
bool
get_member_function_is_dtor(const function_decl_sptr& f)
{return get_member_function_is_dtor(*f);}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(function_decl& f, bool d)
{
    assert(is_member_function(f));

  class_decl::method_decl* m =
    dynamic_cast<class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_destructor(d);
}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(const function_decl_sptr& f, bool d)
{set_member_function_is_dtor(*f, d);}

/// Test whether a member function is const.
///
/// @param f the function to test.
///
/// @return true if @p f is const, false otherwise.
bool
get_member_function_is_const(const function_decl& f)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_const();
}

/// Test whether a member function is const.
///
/// @param f the function to test.
///
/// @return true if @p f is const, false otherwise.
bool
get_member_function_is_const(const function_decl_sptr& f)
{return get_member_function_is_const(*f);}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(function_decl& f, bool is_const)
{
  assert(is_member_function(f));

  class_decl::method_decl* m =
    dynamic_cast<class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_const(is_const);
}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(const function_decl_sptr& f, bool is_const)
{set_member_function_is_const(*f, is_const);}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @return the vtable offset of @p f.
size_t
get_member_function_vtable_offset(const function_decl& f)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->vtable_offset();
}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @return the vtable offset of @p f.
size_t
get_member_function_vtable_offset(const function_decl_sptr& f)
{return get_member_function_vtable_offset(*f);}

/// Set the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.
void
set_member_function_vtable_offset(function_decl& f, size_t s)
{
  assert(is_member_function(f));

  class_decl::method_decl* m =
    dynamic_cast<class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->vtable_offset(s);
}
/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.
void
set_member_function_vtable_offset(const function_decl_sptr& f, size_t s)
{return set_member_function_vtable_offset(*f, s);}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl& f)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  const mem_fn_context_rel* ctxt =
    dynamic_cast<const mem_fn_context_rel*>(m->get_context_rel());

  return ctxt->is_virtual();
}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl_sptr& mem_fn)
{return mem_fn ? get_member_function_is_virtual(*mem_fn) : false;}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl* mem_fn)
{return mem_fn ? get_member_function_is_virtual(*mem_fn) : false;}

/// Set the virtual-ness of a member function.
///
/// @param f the member function to consider.
///
/// @param is_virtual set to true if the function is virtual.
void
set_member_function_is_virtual(function_decl& f, bool is_virtual)
{
  assert(is_member_function(f));

  class_decl::method_decl* m =
    dynamic_cast<class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel* ctxt =
    dynamic_cast<mem_fn_context_rel*>(m->get_context_rel());

  ctxt->is_virtual(is_virtual);
}

/// Set the virtual-ness of a member function.
///
/// @param f the member function to consider.
///
/// @param is_virtual set to true if the function is virtual.
void
set_member_function_is_virtual(const function_decl_sptr& fn, bool is_virtual)
{
  if (fn)
    {
      set_member_function_is_virtual(*fn, is_virtual);
      fixup_virtual_member_function
	(dynamic_pointer_cast<class_decl::method_decl>(fn));
    }
}

/// Recursively returns the the underlying type of a typedef.  The
/// return type should not be a typedef of anything anymore.
///
///
/// Also recursively strip typedefs from the sub-types of the type
/// given in arguments.
///
/// Note that this function builds types in which typedefs are
/// stripped off.  Usually, types are held by their scope, so their
/// life time is bound to the life time of their scope.  But as this
/// function cannot really insert the built type into it's scope, it
/// must ensure that the newly built type stays live long enough.
///
/// So, if the newly built type has a canonical type, this function
/// returns the canonical type.  Otherwise, this function ensure that
/// the newly built type has a life time that is the same as the life
/// time of the entire libabigail library.
///
/// @param type the type to strip the typedefs from.
///
/// @return the resulting type stripped from its typedefs, or just
/// return @p type if it has no typedef in any of its sub-types.
type_base_sptr
strip_typedef(const type_base_sptr type)
{
  if (!type)
    return type;

  // If type is a class type then do not try to strip typedefs from it.
  // And if it has no canonical type (which can mean that it's a
  // declaration-only class), then, make sure its live for ever and
  // return it.
  if (class_decl_sptr cl = is_class_type(type))
    {
      if (!cl->get_canonical_type())
	keep_type_alive(type);
      return type;
    }

  environment* env = type->get_environment();
  assert(env);
  type_base_sptr t = type;

  if (const typedef_decl_sptr ty = is_typedef(t))
    t = strip_typedef(type_or_void(ty->get_underlying_type(), env));
  else if (const reference_type_def_sptr ty = is_reference_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_pointed_to_type(),
						    env));
      assert(p);
      t.reset(new reference_type_def(p,
				     ty->is_lvalue(),
				     ty->get_size_in_bits(),
				     ty->get_alignment_in_bits(),
				     ty->get_location()));
    }
  else if (const pointer_type_def_sptr ty = is_pointer_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_pointed_to_type(),
						    env));
      assert(p);
      t.reset(new pointer_type_def(p,
				   ty->get_size_in_bits(),
				   ty->get_alignment_in_bits(),
				   ty->get_location()));
    }
  else if (const qualified_type_def_sptr ty = is_qualified_type(t))
    {
      type_base_sptr p = strip_typedef(type_or_void(ty->get_underlying_type(),
						    env));
      assert(p);
      t.reset(new qualified_type_def(p,
				     ty->get_cv_quals(),
				     ty->get_location()));
    }
  else if (const array_type_def_sptr ty = is_array_type(t))
    {
      type_base_sptr p = strip_typedef(ty->get_element_type());
      assert(p);
      t.reset(new array_type_def(p, ty->get_subranges(), ty->get_location()));
    }
  else if (const method_type_sptr ty = is_method_type(t))
    {
      function_decl::parameters parm;
      for (function_decl::parameters::const_iterator i =
	     ty->get_parameters().begin();
	   i != ty->get_parameters().end();
	   ++i)
	{
	  function_decl::parameter_sptr p = *i;
	  type_base_sptr typ = strip_typedef(p->get_type());
	  assert(typ);
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(typ,
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_artificial()));
	  parm.push_back(stripped);
	}
      type_base_sptr p = strip_typedef(ty->get_return_type());
      assert(!!p == !!ty->get_return_type());
      t.reset(new method_type(p,
			      ty->get_class_type(),
			      parm,
			      ty->get_size_in_bits(),
			      ty->get_alignment_in_bits()));
    }
  else if (const function_type_sptr ty = is_function_type(t))
    {
      function_decl::parameters parm;
      for (function_decl::parameters::const_iterator i =
	     ty->get_parameters().begin();
	   i != ty->get_parameters().end();
	   ++i)
	{
	  function_decl::parameter_sptr p = *i;
	  type_base_sptr typ = strip_typedef(p->get_type());
	  assert(typ);
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(typ,
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_artificial()));
	  parm.push_back(stripped);
	}
      type_base_sptr p = strip_typedef(ty->get_return_type());
      assert(!!p == !!ty->get_return_type());
      t.reset(new function_type(p, parm,
				ty->get_size_in_bits(),
				ty->get_alignment_in_bits()));
    }

  if (!t->get_environment())
    set_environment_for_artifact(t, env);

  if (!(type->get_canonical_type() && canonicalize(t)))
    keep_type_alive(t);

  return t->get_canonical_type() ? t->get_canonical_type() : t;
}

/// Return the leaf underlying type node of a @ref typedef_decl node.
///
/// If the underlying type of a @ref typedef_decl node is itself a
/// @ref typedef_decl node, then recursively look at the underlying
/// type nodes to get the first one that is not a a @ref typedef_decl
/// node.  This is what a leaf underlying type node means.
///
/// Otherwise, if the underlying type node of @ref typedef_decl is
/// *NOT* a @ref typedef_decl node, then just return the underlying
/// type node.
///
/// And if the type node considered is not a @ref typedef_decl node,
/// then just return it.
///
/// @return the leaf underlying type node of a @p type.
type_base_sptr
peel_typedef_type(const type_base_sptr& type)
{
  typedef_decl_sptr t = is_typedef(type);
  if (!t)
    return type;

  if (is_typedef(t->get_underlying_type()))
    return peel_typedef_type(t->get_underlying_type());
  return t->get_underlying_type();
}

/// Return the leaf underlying type node of a @ref typedef_decl node.
///
/// If the underlying type of a @ref typedef_decl node is itself a
/// @ref typedef_decl node, then recursively look at the underlying
/// type nodes to get the first one that is not a a @ref typedef_decl
/// node.  This is what a leaf underlying type node means.
///
/// Otherwise, if the underlying type node of @ref typedef_decl is
/// *NOT* a @ref typedef_decl node, then just return the underlying
/// type node.
///
/// And if the type node considered is not a @ref typedef_decl node,
/// then just return it.
///
/// @return the leaf underlying type node of a @p type.
const type_base*
peel_typedef_type(const type_base* type)
{
  const typedef_decl* t = is_typedef(type);
  if (!t)
    return t;

  return peel_typedef_type(t->get_underlying_type()).get();
}

/// Return the leaf pointed-to type node of a @ref pointer_type_def
/// node.
///
/// If the pointed-to type of a @ref pointer_type_def node is itself a
/// @ref pointer_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// pointer_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref pointer_type_def is
/// *NOT* a @ref pointer_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref pointer_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
type_base_sptr
peel_pointer_type(const type_base_sptr& type)
{
  pointer_type_def_sptr t = is_pointer_type(type);
  if (!t)
    return type;

  if (is_pointer_type(t->get_pointed_to_type()))
    return peel_pointer_type(t->get_pointed_to_type());
  return t->get_pointed_to_type();
}

/// Return the leaf pointed-to type node of a @ref pointer_type_def
/// node.
///
/// If the pointed-to type of a @ref pointer_type_def node is itself a
/// @ref pointer_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// pointer_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref pointer_type_def is
/// *NOT* a @ref pointer_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref pointer_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
const type_base*
peel_pointer_type(const type_base* type)
{
  const pointer_type_def* t = is_pointer_type(type);
  if (!t)
    return type;

  return peel_pointer_type(t->get_pointed_to_type()).get();
}

/// Return the leaf pointed-to type node of a @ref reference_type_def
/// node.
///
/// If the pointed-to type of a @ref reference_type_def node is itself
/// a @ref reference_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// reference_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref reference_type_def
/// is *NOT* a @ref reference_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref reference_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
type_base_sptr
peel_reference_type(const type_base_sptr& type)
{
  reference_type_def_sptr t = is_reference_type(type);
  if (!t)
    return type;

  if (is_reference_type(t->get_pointed_to_type()))
    return peel_reference_type(t->get_pointed_to_type());
  return t->get_pointed_to_type();
}

/// Return the leaf pointed-to type node of a @ref reference_type_def
/// node.
///
/// If the pointed-to type of a @ref reference_type_def node is itself
/// a @ref reference_type_def node, then recursively look at the
/// pointed-to type nodes to get the first one that is not a a @ref
/// reference_type_def node.  This is what a leaf pointed-to type node
/// means.
///
/// Otherwise, if the pointed-to type node of @ref reference_type_def
/// is *NOT* a @ref reference_type_def node, then just return the
/// pointed-to type node.
///
/// And if the type node considered is not a @ref reference_type_def
/// node, then just return it.
///
/// @return the leaf pointed-to type node of a @p type.
const type_base*
peel_reference_type(const type_base* type)
{
  const reference_type_def* t = is_reference_type(type);
  if (!t)
    return type;

  return peel_reference_type(t->get_pointed_to_type()).get();
}

/// Return the leaf element type of an array.
///
/// If the element type is itself an array, then recursively return
/// the element type of that array itself.
///
/// @param type the array type to consider.  If this is not an array
/// type, this type is returned by the function.
///
/// @return the leaf element type of the array @p type, or, if it's
/// not an array type, then just return @p.
const type_base_sptr
peel_array_type(const type_base_sptr& type)
{
  const array_type_def_sptr t = is_array_type(type);
  if (!t)
    return type;

  return peel_array_type(t->get_element_type());
}

/// Return the leaf element type of an array.
///
/// If the element type is itself an array, then recursively return
/// the element type of that array itself.
///
/// @param type the array type to consider.  If this is not an array
/// type, this type is returned by the function.
///
/// @return the leaf element type of the array @p type, or, if it's
/// not an array type, then just return @p.
const type_base*
peel_array_type(const type_base* type)
{
  const array_type_def* t = is_array_type(type);
  if (!t)
    return type;

  return peel_array_type(t->get_element_type()).get();
}

/// Return the leaf underlying type of a qualified type.
///
/// If the underlying type is itself a qualified type, then
/// recursively return the first underlying type of that qualified
/// type to return the first underlying type that is not a qualified type.
///
/// If the underlying type is NOT a qualified type, then just return
/// that underlying type.
///
/// @param type the qualified type to consider.
///
/// @return the leaf underlying type.
const type_base*
peel_qualified_type(const type_base* type)
{
  const qualified_type_def* t = is_qualified_type(type);
  if (!t)
    return type;

  return peel_qualified_type(t->get_underlying_type().get());
}

/// Return the leaf underlying type of a qualified type.
///
/// If the underlying type is itself a qualified type, then
/// recursively return the first underlying type of that qualified
/// type to return the first underlying type that is not a qualified type.
///
/// If the underlying type is NOT a qualified type, then just return
/// that underlying type.
///
/// @param type the qualified type to consider.
///
/// @return the leaf underlying type.
const type_base_sptr
peel_qualified_type(const type_base_sptr& type)
{
  const qualified_type_def_sptr t = is_qualified_type(type);
  if (!t)
    return type;

  return peel_qualified_type(t->get_underlying_type());
}

/// Return the leaf underlying or pointed-to type node of a @ref
/// typedef_decl, @ref pointer_type_def, @ref reference_type_def or
/// @ref qualified_type_def node.
///
/// @return the leaf underlying or pointed-to type node of @p type.
type_base_sptr
peel_typedef_pointer_or_reference_type(const type_base_sptr type)
{
  type_base_sptr typ  = type;
  while (is_typedef(typ)
	 || is_pointer_type(typ)
	 || is_reference_type(typ)
	 || is_qualified_type(typ))
    {
      if (typedef_decl_sptr t = is_typedef(typ))
	typ = peel_typedef_type(t);

      if (pointer_type_def_sptr t = is_pointer_type(typ))
	typ = peel_pointer_type(t);

      if (reference_type_def_sptr t = is_reference_type(typ))
	typ = peel_reference_type(t);

      if (array_type_def_sptr t = is_array_type(typ))
	typ = peel_array_type(t);

      if (qualified_type_def_sptr t = is_qualified_type(typ))
	typ = peel_qualified_type(t);
    }

  return typ;
}

/// Return the leaf underlying or pointed-to type node of a @ref
/// typedef_decl, @ref pointer_type_def, @ref reference_type_def or
/// @ref qualified_type_def type node.
///
/// @return the leaf underlying or pointed-to type node of @p type.
type_base*
peel_typedef_pointer_or_reference_type(const type_base* type)
{
  while (is_typedef(type)
	 || is_pointer_type(type)
	 || is_reference_type(type)
	 || is_qualified_type(type))
    {
      if (const typedef_decl* t = is_typedef(type))
	type = peel_typedef_type(t);

      if (const pointer_type_def* t = is_pointer_type(type))
	type = peel_pointer_type(t);

      if (const reference_type_def* t = is_reference_type(type))
	type = peel_reference_type(t);

      if (const array_type_def* t = is_array_type(type))
	type = peel_array_type(t);

      if (const qualified_type_def* t = is_qualified_type(type))
	type = peel_qualified_type(t);
    }

  return const_cast<type_base*>(type);
}

/// Update the qualified name of a given sub-tree.
///
/// @param d the sub-tree for which to update the qualified name.
static void
update_qualified_name(decl_base * d)
{
  ::qualified_name_setter setter(d);
  d->traverse(setter);
}

/// Update the qualified name of a given sub-tree.
///
/// @param d the sub-tree for which to update the qualified name.
static void
update_qualified_name(decl_base_sptr d)
{return update_qualified_name(d.get());}

/// Update the map that is going to be used later for lookup of types
/// in a given scope declaration.
///
/// That is, add a new name -> type relationship in the map, for a
/// given type declaration.
///
/// @param member the declaration of the type to update the scope for.
/// This type declaration must be added to the scope, after or before
/// invoking this function.  If it appears that this @p member is not
/// a type, this function does nothing.  Also, if this member is a
/// declaration-only class, the function does nothing.
static void
maybe_update_types_lookup_map(scope_decl *scope,
			      decl_base_sptr member)
{
  string n = member->get_qualified_name();
  type_base_sptr t = is_type(member);
  bool update_qname_map = t;
  if (update_qname_map)
    {
      if (class_decl_sptr c = is_class_type(member))
	{
	  if (c->get_is_declaration_only())
	    {
	      if (class_decl_sptr def = c->get_definition_of_declaration())
		t = def;
	      else
		update_qname_map = false;
	    }
	}
    }
  if (update_qname_map)
    {
      translation_unit* tu = get_translation_unit(scope);
      if (tu)
	{
	  string qname = member->get_qualified_name();
	  string_type_base_wptr_map_type& types = tu->get_types();
	  string_type_base_wptr_map_type::iterator it = types.find(qname);
	  if (it == types.end())
	    types[qname] = t;
	}
    }
}

/// Add a member decl to this scope.  Note that user code should not
/// use this, but rather use add_decl_to_scope.
///
/// Note that this function updates the qualified name of the member
/// decl that is added.  It also sets the scope of the member.  Thus,
/// it asserts that member should not have its scope set, prior to
/// calling this function.
///
/// @param member the new member decl to add to this scope.
decl_base_sptr
scope_decl::add_member_decl(const decl_base_sptr member)
{
  assert(!has_scope(member));

  member->set_scope(this);
  members_.push_back(member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
    member_scopes_.push_back(m);

  update_qualified_name(member);

  if (environment* env = get_environment())
    set_environment_for_artifact(member, env);

  if (const translation_unit* tu = get_translation_unit())
    {
      if (const translation_unit* existing_tu = member->get_translation_unit())
	assert(tu == existing_tu);
      else
	member->set_translation_unit(tu);
    }

  maybe_update_types_lookup_map(this, member);

  return member;
}

/// Insert a member decl to this scope, right before an element
/// pointed to by a given iterator.  Note that user code should not
/// use this, but rather use insert_decl_into_scope.
///
/// Note that this function updates the qualified name of the inserted
/// member.
///
/// @param member the new member decl to add to this scope.
///
/// @param before an interator pointing to the element before which
/// the new member should be inserted.
decl_base_sptr
scope_decl::insert_member_decl(const decl_base_sptr member,
			       declarations::iterator before)
{
  assert(!member->get_scope());

  member->set_scope(this);
  members_.insert(before, member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
    member_scopes_.push_back(m);

  update_qualified_name(member);

  if (environment* env = get_environment())
    set_environment_for_artifact(member, env);

  if (const translation_unit* tu = get_translation_unit())
    {
      if (const translation_unit* existing_tu = member->get_translation_unit())
	assert(tu == existing_tu);
      else
	member->set_translation_unit(tu);
    }

  maybe_update_types_lookup_map(this, member);

  return member;
}

/// Remove a declaration from the current scope.
///
/// @param member the declaration to remove from the scope.
void
scope_decl::remove_member_decl(const decl_base_sptr member)
{
  for (declarations::iterator i = members_.begin();
       i != members_.end();
       ++i)
    {
      if (**i == *member)
	{
	  members_.erase(i);
	  // Do not access i after this point as it's invalided by the
	  // erase call.
	  break;
	}
    }

  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(member);
  if (scope)
    {
      for (scopes::iterator i = member_scopes_.begin();
	   i != member_scopes_.end();
	   ++i)
	{
	  if (**i == *member)
	    {
	      member_scopes_.erase(i);
	      break;
	    }
	}
    }
}

/// Return the hash value for the current instance of scope_decl.
///
/// This method can trigger the computing of the hash value, if need be.
///
/// @return the hash value.
size_t
scope_decl::get_hash() const
{
  scope_decl::hash hash_scope;
  return hash_scope(this);
}

/// Compares two instances of @ref scope_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const scope_decl& l, const scope_decl& r, change_kind* k)
{
  bool result = true;

  if (!l.decl_base::operator==(r))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  scope_decl::declarations::const_iterator i, j;
  for (i = l.get_member_decls().begin(), j = r.get_member_decls().begin();
       i != l.get_member_decls().end() && j != r.get_member_decls().end();
       ++i, ++j)
    {
      if (**i != **j)
	{
	  result = false;
	  if (k)
	    {
	      *k |= SUBTYPE_CHANGE_KIND;
	      break;
	    }
	  else
	    return false;
	}
    }

  if (i != l.get_member_decls().end() || j != r.get_member_decls().end())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Return true iff both scopes have the same names and have the same
/// member decls.
///
/// This function doesn't check for equality of the scopes of its
/// arguments.
bool
scope_decl::operator==(const decl_base& o) const
{
  const scope_decl* other = dynamic_cast<const scope_decl*>(&o);
  if (!other)
    return false;

  return equals(*this, *other, 0);
}

/// Equality operator for @ref scope_decl_sptr.
///
/// @param l the left hand side operand of the equality operator.
///
/// @pram r the right hand side operand of the equalify operator.
///
/// @return true iff @p l equals @p r.
bool
operator==(scope_decl_sptr l, scope_decl_sptr r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  return *l == *r;
}

/// Find a member of the current scope and return an iterator on it.
///
/// @param decl the scope member to find.
///
/// @param i the iterator to set to the member @p decl.  This is set
/// iff the function returns true.
///
/// @return true if the member decl was found, false otherwise.
bool
scope_decl::find_iterator_for_member(const decl_base* decl,
				     declarations::iterator& i)
{
  if (!decl)
    return false;

  if (get_member_decls().empty())
    {
      i = get_member_decls().end();
      return false;
    }

  for (declarations::iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    {
      if ((*it).get() == decl)
	{
	  i = it;
	  return true;
	}
    }

  return false;
}

/// Find a member of the current scope and return an iterator on it.
///
/// @param decl the scope member to find.
///
/// @param i the iterator to set to the member @p decl.  This is set
/// iff the function returns true.
///
/// @return true if the member decl was found, false otherwise.
bool
scope_decl::find_iterator_for_member(const decl_base_sptr decl,
				     declarations::iterator& i)
{return find_iterator_for_member(decl.get(), i);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance of scope_decl
/// and on its member nodes.
///
/// @return true if the traversal of the tree should continue, false
/// otherwise.
bool
scope_decl::traverse(ir_node_visitor &v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      for (scope_decl::declarations::const_iterator i =
	     get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	if (!(*i)->traverse(v))
	  break;
      visiting(false);
    }
  return v.visit_end(this);
}

scope_decl::~scope_decl()
{}

/// Appends a declaration to a given scope, if the declaration
/// doesn't already belong to one.
///
/// @param decl the declaration to add to the scope
///
/// @param scope the scope to append the declaration to
decl_base_sptr
add_decl_to_scope(decl_base_sptr decl, scope_decl* scope)
{
  assert(scope);

  if (scope && decl && !decl->get_scope())
    decl = scope->add_member_decl(decl);

  return decl;
}

/// Appends a declaration to a given scope, if the declaration doesn't
/// already belong to a scope.
///
/// @param decl the declaration to add append to the scope
///
/// @param scope the scope to append the decl to
decl_base_sptr
add_decl_to_scope(shared_ptr<decl_base> decl, shared_ptr<scope_decl> scope)
{return add_decl_to_scope(decl, scope.get());}

/// Remove a given decl from its scope
///
/// @param decl the decl to remove from its scope.
void
remove_decl_from_scope(decl_base_sptr decl)
{
  if (!decl)
    return;

  scope_decl* scope = decl->get_scope();
  scope->remove_member_decl(decl);
  decl->set_scope(0);
}

/// Inserts a declaration into a given scope, before a given IR child
/// node of the scope.
///
/// @param decl the declaration to insert into the scope.
///
/// @param before an iterator pointing to the child IR node before
/// which to insert the declaration.
///
/// @param scope the scope into which to insert the declaration.
decl_base_sptr
insert_decl_into_scope(decl_base_sptr decl,
		       scope_decl::declarations::iterator before,
		       scope_decl* scope)
{
  if (scope && decl && !decl->get_scope())
    {
      decl_base_sptr d = scope->insert_member_decl(decl, before);
      decl = d;
    }
  return decl;
}

/// Inserts a declaration into a given scope, before a given IR child
/// node of the scope.
///
/// @param decl the declaration to insert into the scope.
///
/// @param before an iterator pointing to the child IR node before
/// which to insert the declaration.
///
/// @param scope the scope into which to insert the declaration.
decl_base_sptr
insert_decl_into_scope(decl_base_sptr decl,
		       scope_decl::declarations::iterator before,
		       scope_decl_sptr scope)
{return insert_decl_into_scope(decl, before, scope.get());}

/// return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const decl_base& decl)
{
  if (const global_scope* s = dynamic_cast<const global_scope*>(&decl))
    return s;

  scope_decl* scope = decl.get_scope();
  while (scope && !dynamic_cast<global_scope*>(scope))
    scope = scope->get_scope();

  return scope ? dynamic_cast<global_scope*> (scope) : 0;
}

/// return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const decl_base* decl)
{return get_global_scope(*decl);}

/// Return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
const global_scope*
get_global_scope(const shared_ptr<decl_base> decl)
{return get_global_scope(decl.get());}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// Note that @p scope must come before @p decl in topological
/// order.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base* decl,
			 const scope_decl* scope)
{
  if (!decl)
    return 0;

  if (scope == 0)
    return get_global_scope(decl);

  // Handle the case where decl is a scope itself.
  const scope_decl* s = dynamic_cast<const scope_decl*>(decl);
  if (!s)
    s = decl->get_scope();

  if (is_global_scope(s))
    return scope;

  // Here, decl is in the scope 'scope', or decl and 'scope' are the
  // same.  The caller needs to be prepared to deal with this case.
  if (s == scope)
    return s;

  while (s && !is_global_scope(s) && s->get_scope() != scope)
    s = s->get_scope();

  if (!s || is_global_scope(s))
    // SCOPE must come before decl in topological order, but I don't
    // know how to ensure that ...
    return scope;
  assert(s);

  return s;
}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base_sptr decl,
			 const scope_decl* scope)
{return get_top_most_scope_under(decl.get(), scope);}

/// Return the a scope S containing a given declaration and that is
/// right under a given scope P.
///
/// @param decl the decl for which to find a scope.
///
/// @param scope the scope under which the resulting scope must be.
///
/// @return the resulting scope.
const scope_decl*
get_top_most_scope_under(const decl_base_sptr decl,
			 const scope_decl_sptr scope)
{return get_top_most_scope_under(decl, scope.get());}

/// Build and return a copy of the name of an ABI artifact that is
/// either a type of a decl.
///
/// @param tod the ABI artifact to get the name for.
///
/// @param qualified if yes, return the qualified name of @p tod;
/// otherwise, return the non-qualified name;
///
/// @return the name of @p tod.
string
get_name(const type_or_decl_base_sptr& tod, bool qualified)
{
  string result;

  if (type_base_sptr t = dynamic_pointer_cast<type_base>(tod))
    result = get_type_name(t, qualified);
  else if (decl_base_sptr d = dynamic_pointer_cast<decl_base>(tod))
    {
      if (qualified)
	result = d->get_qualified_name();
      else
	result = d->get_name();
    }
  else
    // We should never reach this point.
    abort();

  return result;
}

/// Get the scope of a given type.
///
/// @param t the type to consider.
///
/// @return the scope of type @p t or 0 if the type has no scope yet.
scope_decl*
get_type_scope(type_base* t)
{
  if (!t)
    return 0;

  decl_base* d = get_type_declaration(t);
  if (d)
    d->get_scope();
  return 0;
}

/// Get the scope of a given type.
///
/// @param t the type to consider.
///
/// @return the scope of type @p t or 0 if the type has no scope yet.
scope_decl*
get_type_scope(const type_base_sptr& t)
{return get_type_scope(t.get());}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
string
get_type_name(const type_base_sptr t, bool qualified, bool internal)
{return get_type_name(t.get(), qualified, internal);}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
string
get_type_name(const type_base* t, bool qualified, bool internal)
{
  const decl_base* d = dynamic_cast<const decl_base*>(t);
  if (!d)
    {
      const function_type* fn_type = is_function_type(t);
      assert(fn_type);
      return get_function_type_name(fn_type, internal);
    }
  if (qualified)
    return d->get_qualified_name(internal);
  return d->get_name();
}

/// Get the name of a given type and return a copy of it.
///
/// @param t the type to consider.
///
/// @param qualified if true then return the qualified name of the
/// type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
string
get_type_name(const type_base& t, bool qualified, bool internal)
{return get_type_name(&t, qualified, internal);}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_function_type_name(const function_type_sptr& fn_type,
		       bool internal)
{return get_function_type_name(fn_type.get(), internal);}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_function_type_name(const function_type* fn_type,
		       bool internal)
{
  if (!fn_type)
    return "";

  if (const method_type* method = is_method_type(fn_type))
    return get_method_type_name(method, internal);

  return get_function_type_name(*fn_type, internal);
}

/// Get the name of a given function type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_function_type_name(const function_type& fn_type,
		       bool internal)
{
  std::ostringstream o;
  type_base_sptr return_type= fn_type.get_return_type();

  o <<  get_pretty_representation(return_type, internal);

  o << " (";
  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      if (i != fn_type.get_parameters().begin())
	o << ", ";
      o << get_pretty_representation((*i)->get_type(), internal);
    }
  o <<")";
  return o.str();
}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_method_type_name(const method_type_sptr fn_type,
		     bool internal)
{return get_method_type_name(fn_type.get(), internal);}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_method_type_name(const method_type* fn_type,
		     bool internal)
{
  if (fn_type)
    get_method_type_name(*fn_type, internal);

  return "";
}

/// Get the name of a given method type and return a copy of it.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the function type name
string
get_method_type_name(const method_type& fn_type,
		     bool internal)
{
  std::ostringstream o;
  type_base_sptr return_type= fn_type.get_return_type();

  o <<  get_pretty_representation(return_type, internal);

  class_decl_sptr class_type = fn_type.get_class_type();
  assert(class_type);

  o << " (" << class_type->get_qualified_name(internal) << "::*)"
    << " (";

  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      if (i != fn_type.get_parameters().begin())
	o << ", ";
      o << get_pretty_representation((*i)->get_type(), internal);
    }
  o <<")";
  return o.str();
}

/// Build and return a copy of the pretty representation of an ABI
/// artifact that could be either a type of a decl.
///
/// param tod the ABI artifact to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of an ABI artifact
/// that could be either a type of a decl.
string
get_pretty_representation(const type_or_decl_base* tod, bool internal)
{
  string result;

  if (type_base* t =
      dynamic_cast<type_base*>(const_cast<type_or_decl_base*>(tod)))
    result = get_pretty_representation(t, internal);
  else if (decl_base* d =
	   dynamic_cast<decl_base*>(const_cast<type_or_decl_base*>(tod)))
    result =  get_pretty_representation(d, internal);
  else
    // We should never reach this point
    abort();

  return result;
}

/// Build and return a copy of the pretty representation of an ABI
/// artifact that could be either a type of a decl.
///
/// param tod the ABI artifact to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of an ABI artifact
/// that could be either a type of a decl.
string
get_pretty_representation(const type_or_decl_base_sptr& tod, bool internal)
{return get_pretty_representation(tod.get(), internal);}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base* d, bool internal)
{
  if (!d)
    return "";
  return d->get_pretty_representation(internal);
}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base* t, bool internal)
{
  if (!t)
    return "void";
  if (const function_type* fn_type = is_function_type(t))
    return get_pretty_representation(fn_type, internal);

  const decl_base* d = get_type_declaration(t);
  assert(d);
  return get_pretty_representation(d, internal);
}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base_sptr& d, bool internal)
{return get_pretty_representation(d.get(), internal);}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base_sptr& t, bool internal)
{return get_pretty_representation(t.get(), internal);}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type_sptr& fn_type,
			  bool internal)
{return get_pretty_representation(fn_type.get(), internal);}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type* fn_type, bool internal)
{
  if (!fn_type)
    return "void";

  if (const method_type* method = is_method_type(fn_type))
    return get_pretty_representation(method, internal);

  return get_pretty_representation(*fn_type, internal);
}

/// Get the pretty representation of a function type.
///
/// @param fn_type the function type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the function type.
string
get_pretty_representation(const function_type& fn_type, bool internal)
{
  std::ostringstream o;
  o << "function type " << get_function_type_name(fn_type, internal);
  return o.str();
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type& method, bool internal)
{
  std::ostringstream o;
  o << "method type " << get_method_type_name(method, internal);
  return o.str();
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type* method, bool internal)
{
  if (!method)
    return "void";
  return get_pretty_representation(*method, internal);
}

/// Get the pretty representation of a method type.
///
/// @param method the method type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the string represenation of the method type.
string
get_pretty_representation(const method_type_sptr method, bool internal)
{return get_pretty_representation(method.get(), internal);}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
const decl_base*
get_type_declaration(const type_base* t)
{return dynamic_cast<const decl_base*>(t);}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
decl_base*
get_type_declaration(type_base* t)
{return dynamic_cast<decl_base*>(t);}

/// Get the declaration for a given type.
///
/// @param t the type to consider.
///
/// @return the declaration for the type to return.
decl_base_sptr
get_type_declaration(const type_base_sptr t)
{return dynamic_pointer_cast<decl_base>(t);}

/// Test if two types are equal modulo a typedef.
///
/// Type A and B are compatible if
///
///	- A and B are equal
///	- or if one type is a typedef of the other one.
///
/// @param type1 the first type to consider.
///
/// @param type2 the second type to consider.
///
/// @return true iff @p type1 and @p type2 are compatible.
bool
types_are_compatible(const type_base_sptr type1,
		     const type_base_sptr type2)
{
  if (!type1 || !type2)
    return false;

  type_base_sptr t1 = strip_typedef(type1);
  type_base_sptr t2 = strip_typedef(type2);

  return t1 == t2;
}

/// Test if two types are equal modulo a typedef.
///
/// Type A and B are compatible if
///
///	- A and B are equal
///	- or if one type is a typedef of the other one.
///
/// @param type1 the declaration of the first type to consider.
///
/// @param type2 the declaration of the second type to consider.
///
/// @return true iff @p type1 and @p type2 are compatible.
bool
types_are_compatible(const decl_base_sptr d1,
		     const decl_base_sptr d2)
{return types_are_compatible(is_type(d1), is_type(d2));}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const decl_base& decl)
{return const_cast<translation_unit*>(decl.get_translation_unit());}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const decl_base* decl)
{return decl ? get_translation_unit(*decl) : 0;}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const shared_ptr<decl_base> decl)
{return get_translation_unit(decl.get());}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return true iff the current scope is the global one.
bool
is_global_scope(const scope_decl& scope)
{return !!dynamic_cast<const global_scope*>(&scope);}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return the @ref global_scope* representing the scope @p scope or
/// 0 if @p scope is not a global scope.
const global_scope*
is_global_scope(const scope_decl* scope)
{return dynamic_cast<const global_scope*>(scope);}

/// Tests whether if a given scope is the global scope.
///
/// @param scope the scope to consider.
///
/// @return true iff the current scope is the global one.
bool
is_global_scope(const shared_ptr<scope_decl>scope)
{return is_global_scope(scope.get());}

/// Tests whether a given declaration is at global scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at global scope.
bool
is_at_global_scope(const decl_base& decl)
{return (is_global_scope(decl.get_scope()));}

/// Tests whether a given declaration is at global scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at global scope.
bool
is_at_global_scope(const shared_ptr<decl_base> decl)
{return (decl && is_global_scope(decl->get_scope()));}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
bool
is_at_class_scope(const shared_ptr<decl_base> decl)
{return (decl && dynamic_cast<class_decl*>(decl->get_scope()));}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
bool
is_at_class_scope(const decl_base* decl)
{return (decl && dynamic_cast<class_decl*>(decl->get_scope()));}

/// Tests whether a given decl is at class scope.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is at class scope.
bool
is_at_class_scope(const decl_base& decl)
{return (dynamic_cast<class_decl*>(decl.get_scope()));}

/// Tests whether a given decl is at template scope.
///
/// Note that only template parameters , types that are compositions,
/// and template patterns (function or class) can be at template scope.
///
/// @param decl the decl to consider.
///
/// @return true iff the decl is at template scope.
bool
is_at_template_scope(const shared_ptr<decl_base> decl)
{return (decl && dynamic_cast<template_decl*>(decl->get_scope()));}

/// Tests whether a decl is a template parameter.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is a template parameter.
bool
is_template_parameter(const shared_ptr<decl_base> decl)
{
  return (decl && (dynamic_pointer_cast<type_tparameter>(decl)
		   || dynamic_pointer_cast<non_type_tparameter>(decl)
		   || dynamic_pointer_cast<template_tparameter>(decl)));
}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a shared pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl*
is_function_decl(const decl_base* d)
{return dynamic_cast<function_decl*>(const_cast<decl_base*>(d));}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return true if @p d is a function_decl.
bool
is_function_decl(const decl_base& d)
{return is_function_decl(&d);}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a shared pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl_sptr
is_function_decl(decl_base_sptr d)
{return dynamic_pointer_cast<function_decl>(d);}

/// Test whether a declaration is a @ref function_decl.
///
/// @param d the declaration to test for.
///
/// @return a pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl::parameter*
is_function_parameter(const type_or_decl_base* tod)
{
  return dynamic_cast<function_decl::parameter*>
    (const_cast<type_or_decl_base*>(tod));
}

/// Test whether an ABI artifact is a @ref function_decl.
///
/// @param tod the declaration to test for.
///
/// @return a pointer to @ref function_decl if @p d is a @ref
/// function_decl.  Otherwise, a nil shared pointer.
function_decl::parameter_sptr
is_function_parameter(const type_or_decl_base_sptr tod)
{return dynamic_pointer_cast<function_decl::parameter>(tod);}

/// Test if an ABI artifact is a declaration.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
/// declaration, or NULL if it is not.
decl_base*
is_decl(const decl_base* d)
{return dynamic_cast<decl_base*>(const_cast<decl_base*>(d));}

/// Test if an ABI artifact is a declaration.
///
/// @param d the artifact to consider.
///
/// @param return the declaration sub-object of @p d if it's a
/// declaration, or NULL if it is not.
decl_base_sptr
is_decl(const type_or_decl_base_sptr& d)
{return dynamic_pointer_cast<decl_base>(d);}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
bool
is_type(const type_or_decl_base& tod)
{
  if (dynamic_cast<const type_base*>(&tod))
    return true;
  return false;
}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
type_base*
is_type(const type_or_decl_base* tod)
{return const_cast<type_base*>(dynamic_cast<const type_base*>(tod));}

/// Test whether a declaration is a type.
///
/// @param d the IR artefact to test for.
///
/// @return true if the artifact is a type, false otherwise.
type_base_sptr
is_type(const type_or_decl_base_sptr& tod)
{return dynamic_pointer_cast<type_base>(tod);}

/// Test whether a declaration is a type.
///
/// @param d the declaration to test for.
///
/// @return true if the declaration is a type, false otherwise.
bool
is_type(const decl_base& d)
{
  if (dynamic_cast<const type_base*>(&d))
    return true;
  return false;
}

/// Tests whether a declaration is a type, and return it properly
/// converted into a type in that case.
///
/// @param decl the declaration to consider.
///
/// @return the pointer to type_base representing @p decl converted as
/// a type, iff it's a type, or NULL otherwise.
type_base_sptr
is_type(const decl_base_sptr decl)
{return dynamic_pointer_cast<type_base>(decl);}

/// Tests whether a declaration is a type, and return it properly
/// converted into a type in that case.
///
/// @param decl the declaration to consider.
///
/// @return the pointer to type_base representing @p decl converted as
/// a type, iff it's a type, or NULL otherwise.
type_base*
is_type(decl_base* decl)
{return dynamic_cast<type_base*>(decl);}

/// Test if a given type is anonymous.
///
/// @param t the type to consider.
///
/// @return true iff @p t is anonymous.
bool
is_anonymous_type(type_base* t)
{
  decl_base* d = get_type_declaration(t);
  if (!d)
    return false;
  return d->get_is_anonymous();
}

/// Test if a given type is anonymous.
///
/// @param t the type to consider.
///
/// @return true iff @p t is anonymous.
bool
is_anonymous_type(const type_base_sptr& t)
{return is_anonymous_type(t.get());}

/// Test whether a type is a type_decl (a builtin type).
///
/// @return the type_decl_sptr for @t if it's type_decl, otherwise,
/// return nil.
type_decl_sptr
is_type_decl(const type_base_sptr t)
{return dynamic_pointer_cast<type_decl>(t);}

/// Test whether a type is a typedef.
///
/// @param t the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
typedef_decl_sptr
is_typedef(const type_base_sptr t)
{return dynamic_pointer_cast<typedef_decl>(t);}

/// Test whether a type is a typedef.
///
/// @param t the declaration of the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
typedef_decl_sptr
is_typedef(const decl_base_sptr d)
{return is_typedef(is_type(d));}

/// Test whether a type is a typedef.
///
/// @param t the declaration of the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
const typedef_decl*
is_typedef(const type_base* t)
{return dynamic_cast<const typedef_decl*>(t);}

/// Test whether a type is a typedef.
///
/// @param t the declaration of the type to test for.
///
/// @return the typedef declaration of the @p t, or NULL if it's not a
/// typedef.
typedef_decl*
is_typedef(type_base* t)
{return dynamic_cast<typedef_decl*>(t);}

/// Test if a decl is an enum_type_decl
///
/// @param d the decl to test for.
///
/// @return the enum_type_decl_sptr if @p d is an enum, nil otherwise.
enum_type_decl_sptr
is_enum_type(const decl_base_sptr& d)
{return dynamic_pointer_cast<enum_type_decl>(d);}

/// Test if a type is an enum_type_decl
///
/// @param t the type to test for.
///
/// @return the enum_type_decl_sptr if @p t is an enum, nil otherwise.
enum_type_decl_sptr
is_enum_type(const type_base_sptr& t)
{return dynamic_pointer_cast<enum_type_decl>(t);}

/// Test if a type is a class. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_compatible_with_class_type(const type_base_sptr t)
{
  if (!t)
    return class_decl_sptr();
  type_base_sptr ty = strip_typedef(t);
  return is_class_type(ty);
}

/// Test if a type is a class. This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_compatible_with_class_type(const decl_base_sptr t)
{return is_compatible_with_class_type(is_type(t));}

/// Test whether a type is a class.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl*
is_class_type(const type_base* t)
{return dynamic_cast<class_decl*>(const_cast<type_base*>(t));}

/// Test whether a type is a class.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_class_type(const type_base_sptr t)
{
  if (!t)
    return class_decl_sptr();
  return dynamic_pointer_cast<class_decl>(t);
}

/// Test if a the declaration of a type is a class.
///
/// This function looks through typedefs.
///
/// @parm d the declaration of the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl*
is_class_type(const decl_base *d)
{return dynamic_cast<class_decl*>(const_cast<decl_base*>(d));}

/// Test whether a type is a class.
///
/// This function looks through typedefs.
///
/// @parm d the declaration of the type to consider.
///
/// @return the class_decl if @p d is a class_decl or null otherwise.
class_decl_sptr
is_class_type(const decl_base_sptr d)
{return is_class_type(is_type(d));}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
pointer_type_def*
is_pointer_type(type_base* t)
{return dynamic_cast<pointer_type_def*>(t);}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
const pointer_type_def*
is_pointer_type(const type_base* t)
{return dynamic_cast<const pointer_type_def*>(t);}

/// Test whether a type is a pointer_type_def.
///
/// @param t the type to test.
///
/// @return the @ref pointer_type_def_sptr if @p t is a
/// pointer_type_def, null otherwise.
pointer_type_def_sptr
is_pointer_type(const type_base_sptr t)
{return dynamic_pointer_cast<pointer_type_def>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
reference_type_def*
is_reference_type(type_base* t)
{return dynamic_cast<reference_type_def*>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
const reference_type_def*
is_reference_type(const type_base* t)
{return dynamic_cast<const reference_type_def*>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
reference_type_def_sptr
is_reference_type(const type_base_sptr t)
{return dynamic_pointer_cast<reference_type_def>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
qualified_type_def*
is_qualified_type(const type_base* t)
{return dynamic_cast<qualified_type_def*>(const_cast<type_base*>(t));}

/// Test whether a type is a qualified_type_def.
///
/// @param t the type to test.
///
/// @return the @ref qualified_type_def_sptr if @p t is a
/// qualified_type_def, null otherwise.
qualified_type_def_sptr
is_qualified_type(const type_base_sptr t)
{return dynamic_pointer_cast<qualified_type_def>(t);}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
shared_ptr<function_type>
is_function_type(const shared_ptr<type_base> t)
{return dynamic_pointer_cast<function_type>(t);}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
function_type*
is_function_type(type_base* t)
{return dynamic_cast<function_type*>(t);}

/// Test whether a type is a function_type.
///
/// @param t the type to test.
///
/// @return the @ref function_type_sptr if @p t is a
/// function_type, null otherwise.
const function_type*
is_function_type(const type_base* t)
{return dynamic_cast<const function_type*>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
shared_ptr<method_type>
is_method_type(const shared_ptr<type_base> t)
{return dynamic_pointer_cast<method_type>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
const method_type*
is_method_type(const type_base* t)
{return dynamic_cast<const method_type*>(t);}

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
method_type*
is_method_type(type_base* t)
{return dynamic_cast<method_type*>(t);}

/// If a class is a decl-only class, get its definition.  Otherwise,
/// just return the initial class.
///
/// @param klass the class to consider.
///
/// @return either the definition of the class, or the class itself.
class_decl_sptr
look_through_decl_only_class(class_decl_sptr klass)
{
  if (!klass)
    return klass;

  while (klass
	 && klass->get_is_declaration_only()
	 && klass->get_definition_of_declaration())
    klass = klass->get_definition_of_declaration();

  assert(klass);
  return klass;
}

/// Tests if a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return the var_decl_sptr iff decl is a variable declaration; nil
/// otherwise.
var_decl*
is_var_decl(const type_or_decl_base* tod)
{return dynamic_cast<var_decl*>(const_cast<type_or_decl_base*>(tod));}

/// Tests if a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return the var_decl_sptr iff decl is a variable declaration; nil
/// otherwise.
var_decl_sptr
is_var_decl(const shared_ptr<decl_base> decl)
{return dynamic_pointer_cast<var_decl>(decl);}

/// Tests if a declaration is a namespace declaration.
///
/// @param d the decalration to consider.
///
/// @return the namespace declaration if @p d is a namespace.
namespace_decl_sptr
is_namespace(const decl_base_sptr& d)
{return dynamic_pointer_cast<namespace_decl>(d);}

/// Tests if a declaration is a namespace declaration.
///
/// @param d the decalration to consider.
///
/// @return the namespace declaration if @p d is a namespace.
namespace_decl*
is_namespace(const decl_base* d)
{return dynamic_cast<namespace_decl*>(const_cast<decl_base*>(d));}

/// Tests whether a decl is a template parameter composition type.
///
/// @param decl the declaration to consider.
///
/// @return true iff decl is a template parameter composition type.
bool
is_template_parm_composition_type(const shared_ptr<decl_base> decl)
{
  return (decl
	  && is_at_template_scope(decl)
	  && is_type(decl)
	  && !is_template_parameter(decl));
}

/// Test whether a decl is the pattern of a function template.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is the pattern of a function template.
bool
is_function_template_pattern(const shared_ptr<decl_base> decl)
{
  return (decl
	  && dynamic_pointer_cast<function_decl>(decl)
	  && dynamic_cast<template_decl*>(decl->get_scope()));
}

/// Test if a type is an array_type_def.
///
/// @param type the type to consider.
///
/// @return true iff type is an array_type_def.
array_type_def*
is_array_type(const type_base* type)
{return dynamic_cast<array_type_def*>(const_cast<type_base*>(type));}

/// Test if a type is an array_type_def.
///
/// @param type the type to consider.
///
/// @return true iff type is an array_type_def.
array_type_def_sptr
is_array_type(const type_base_sptr type)
{return dynamic_pointer_cast<array_type_def>(type);}

/// Tests whether a decl is a template.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is a function template, class template, or
/// template template parameter.
bool
is_template_decl(const shared_ptr<decl_base> decl)
{return decl && dynamic_pointer_cast<template_decl>(decl);}

/// This enum describe the kind of entity to lookup, while using the
/// lookup API.
enum lookup_entity_kind
{
  LOOKUP_ENTITY_TYPE,
  LOOKUP_ENTITY_VAR,
};

/// Find the first relevant delimiter (the "::" string) in a fully
/// qualified C++ type name, starting from a given position.  The
/// delimiter returned separates a type name from the name of its
/// context.
///
/// This is supposed to work correctly on names in cases like this:
///
///    foo<ns1::name1, ns2::name2>
///
/// In that case when called with with parameter @p begin set to 0, no
/// delimiter is returned, because the type name in this case is:
/// 'foo<ns1::name1, ns2::name2>'.
///
/// But in this case:
///
///   foo<p1, bar::name>::some_type
///
/// The "::" returned is the one right before 'some_type'.
///
/// @param fqn the fully qualified name of the type to consider.
///
/// @param begin the position from which to look for the delimiter.
///
/// @param delim_pos out parameter. Is set to the position of the
/// delimiter iff the function returned true.
///
/// @return true iff the function found and returned the delimiter.
static bool
find_next_delim_in_cplus_type(const string&	fqn,
			      size_t		begin,
			      size_t&		delim_pos)
{
  int angle_count = 0;
  bool found = false;
  size_t i = begin;
  for (; i < fqn.size(); ++i)
    {
      if (fqn[i] == '<')
	++angle_count;
      else if (fqn[i] == '>')
	--angle_count;
      else if (i + 1 < fqn.size()
	       && !angle_count
	       && fqn[i] == ':'
	       && fqn[i+1] == ':')
	{
	  delim_pos = i;
	  found = true;
	  break;
	}
    }
  return found;
}

/// Decompose a fully qualified name into the list of its components.
///
/// @param fqn the fully qualified name to decompose.
///
/// @param comps the resulting list of component to fill.
void
fqn_to_components(const string& fqn,
		  list<string>& comps)
{
  string::size_type fqn_size = fqn.size(), comp_begin = 0, comp_end = fqn_size;
  do
    {
      if (!find_next_delim_in_cplus_type(fqn, comp_begin, comp_end))
	comp_end = fqn_size;

      string comp = fqn.substr(comp_begin, comp_end - comp_begin);
      comps.push_back(comp);

      comp_begin = comp_end + 2;
      if (comp_begin >= fqn_size)
	break;
    } while (true);
}

/// Turn a set of qualified name components (that name a type) into a
/// qualified name string.
///
/// @param comps the name components
///
/// @return the resulting string, which would be the qualified name of
/// a type.
string
components_to_type_name(const list<string>& comps)
{
  string result;
  for (list<string>::const_iterator c = comps.begin();
       c != comps.end();
       ++c)
    if (c == comps.begin())
      result = *c;
    else
      result += "::" + *c;
  return result;
}

/// This predicate returns true if a given container iterator points
/// to the last element of the container, false otherwise.
///
/// @tparam T the type of the container of the iterator.
///
/// @param container the container the iterator points into.
///
/// @param i the iterator to consider.
///
/// @return true iff the iterator points to the last element of @p
/// container.
template<typename T>
static bool
iterator_is_last(T& container,
		 typename T::const_iterator i)
{
  typename T::const_iterator next = i;
  ++next;
  return (next == container.end());
}

/// Lookup a type in a translation unit, starting from the global
/// namespace.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param tu the translation unit to consider.
///
/// @return the declaration of the type if found, NULL otherwise.
const decl_base_sptr
lookup_type_in_translation_unit(const string& fqn,
				const translation_unit& tu)
{
  decl_base_sptr result;
  const string_type_base_wptr_map_type& types = tu.get_types();
  string_type_base_wptr_map_type::const_iterator it, nil = types.end();
  it = types.find(fqn);
  if (it != nil)
    if (!it->second.expired())
      result = get_type_declaration(type_base_sptr(it->second));

  return result;
}

/// Lookup a class type from a translation unit.
///
/// @param fqn the fully qualified name of the class type node to look
/// up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the class type IR node found, NULL
/// otherwise.
const class_decl_sptr
lookup_class_type_in_translation_unit(const string& fqn,
				      const translation_unit& tu)
{return is_class_type(lookup_type_in_translation_unit(fqn, tu));}

/// Lookup a function type from a translation unit.
///
/// This walks all the function types held by the translation unit and
/// compare their sub-type *names*.  If the names match then return
/// the function type found in the translation unit.
///
/// @param t the function type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the function type found, or NULL of none was found.
function_type_sptr
lookup_function_type_in_translation_unit(const function_type& t,
					 const translation_unit& tu)
{
  string type_name = get_type_name(t), n;
  function_types_type& fn_types = tu.priv_->function_types_;
  for (function_types_type::const_iterator i = fn_types.begin();
       i != fn_types.end();
       ++i)
    {
      n = get_type_name(**i);
      bool skip_this = false;
      if (type_name == n)
	{
	  for (function_decl::parameters::const_iterator p0 =
		 t.get_parameters().begin(),
		 p1 = (**i).get_parameters().begin();
	       (p0 != t.get_parameters().end()
		&& p1 != (**i).get_parameters().end());
	       ++p0, ++p1)
	    if ((*p0)->get_artificial() != (*p1)->get_artificial()
		|| (*p0)->get_variadic_marker() != (*p1)->get_variadic_marker())
	      {
		skip_this = true;
		break;
	      }
	  if (skip_this)
	    continue;
	  return *i;
	}
    }
  return function_type_sptr();
}

/// Lookup a function type from a translation unit.
///
/// This walks all the function types held by the translation unit and
/// compare their sub-type *names*.  If the names match then return
/// the function type found in the translation unit.
///
/// @param t the function type to look for.
///
/// @param tu the translation unit to look into.
///
/// @return the function type found, or NULL of none was found.
function_type_sptr
lookup_function_type_in_translation_unit(const function_type_sptr& t,
					 const translation_unit& tu)
{return lookup_function_type_in_translation_unit(*t, tu);}

/// In a translation unit, lookup a given type or synthesize it if
/// it's a qualified type.
///
/// So this function first looks the type up in the translation unit.
/// If it's found, then OK, it's returned.  Otherwise, if it's a
/// qualified type, lookup the unqualified underlying type and
/// synthesize the qualified type from it.
///
/// If the unqualified underlying type is not found either, then give
/// up and return nil.
///
/// If the type is not
type_base_sptr
synthesize_type_from_translation_unit(const type_base_sptr& type,
				      translation_unit& tu)
{
  type_base_sptr result;

  // TODO: Maybe handle the case of a function type here?
  result = lookup_type_in_translation_unit(type, tu);

  if (!result)
    if (qualified_type_def_sptr qual = is_qualified_type(type))
      {
	type_base_sptr underlying_type =
	  synthesize_type_from_translation_unit(qual->get_underlying_type(),
						tu);
	if (underlying_type)
	  {
	    result.reset(new qualified_type_def(underlying_type,
						qual->get_cv_quals(),
						qual->get_location()));
	    // The new qualified type must be in the same environment
	    // as its underlying type.
	    result->set_environment(underlying_type->get_environment());
	  }
	tu.priv_->synthesized_types_.push_back(result);
      }

  return result;
}

/// In a translation unit, lookup the sub-types that make up a given
/// function type and if the sub-types are all found, synthesize and
/// return a function_type with them.
///
/// This function is like lookup_function_type_in_translation_unit()
/// execept that it constructs the function type from the sub-types
/// found in the translation, rather than just looking for the
/// function types held by the translation unit.  This can be useful
/// if the translation unit doesnt hold the function type we are
/// looking for (i.e, lookup_function_type_in_translation_unit()
/// returned NULL) but we still want to see if the sub-types of the
/// function types are present in the translation unit.
///
/// @param fn_type the function type to consider.
///
/// @param tu the translation unit to look into.
///
/// @return the resulting synthesized function type if all its
/// sub-types have been found, NULL otherwise.
function_type_sptr
synthesize_function_type_from_translation_unit(const function_type& fn_type,
					       translation_unit& tu)
{
  function_type_sptr nil = function_type_sptr();

  environment* env = tu.get_environment();
  assert(env);

  type_base_sptr return_type = fn_type.get_return_type();
  type_base_sptr result_return_type;
  if (!return_type
      || return_type.get() == env->get_void_type_decl().get())
    result_return_type = type_base_sptr(env->get_void_type_decl());
  else
    result_return_type = synthesize_type_from_translation_unit(return_type, tu);
  if (!result_return_type)
    return nil;

  function_type::parameters parms;
  type_base_sptr parm_type;
  function_decl::parameter_sptr parm;
  for (function_type::parameters::const_iterator i =
	 fn_type.get_parameters().begin();
       i != fn_type.get_parameters().end();
       ++i)
    {
      type_base_sptr t = (*i)->get_type();
      parm_type = synthesize_type_from_translation_unit(t, tu);
      if (!parm_type)
	return nil;
      parm.reset(new function_decl::parameter(parm_type,
					      (*i)->get_index(),
					      (*i)->get_name(),
					      (*i)->get_location()));
      parms.push_back(parm);
    }

  function_type_sptr result_fn_type
    (new function_type(result_return_type,
		       parms,
		       fn_type.get_size_in_bits(),
		       fn_type.get_alignment_in_bits()));

  tu.priv_->synthesized_types_.push_back(result_fn_type);
  // The new synthesized type must be in the same environment as its
  // translation unit.
  result_fn_type->set_environment(tu.get_environment());

  return result_fn_type;
}

/// Lookup a type in a scope.
///
/// @param fqn the fully qualified name of the type to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the type if found, NULL otherwise.
const decl_base_sptr
lookup_type_in_scope(const string& fqn,
		     const scope_decl_sptr& skope)
{
  list<string> comps;
  fqn_to_components(fqn, comps);
  return lookup_type_in_scope(comps, skope);
}

/// Lookup a @ref var_decl in a scope.
///
/// @param fqn the fuly qualified name of the @var_decl to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the @ref var_decl if found, NULL
/// otherwise.
const decl_base_sptr
lookup_var_decl_in_scope(const string& fqn,
			 const scope_decl_sptr& skope)
{
  list<string> comps;
  fqn_to_components(fqn, comps);
  return lookup_var_decl_in_scope(comps, skope);
}

/// A generic function (template) to get the name of a node, whatever
/// node it is.  This has to specialized for the kind of node we want
///
/// @tparam NodeKind the kind of node to consider.
///
/// @param node the node to get the name from.
///
/// @return the name of the node.
template<typename NodeKind>
static const string&
get_node_name(shared_ptr<NodeKind> node);

/// Gets the name of a decl_base node.
///
/// @param node the decl_base node to get the name from.
///
/// @return the name of the node.
template<>
const string&
get_node_name(decl_base_sptr node)
{return node->get_name();}

/// Gets the name of a class_decl node.
///
/// @param node the decl_base node to get the name from.
///
/// @return the name of the node.
template<>
const string&
get_node_name(class_decl_sptr node)
{return node->get_name();}

/// Gets the name of a type_base node.
///
/// @param node the type_base node to get the name from.
///
/// @return the name of the node.
template<>
const string&
get_node_name(type_base_sptr node)
{return get_type_declaration(node)->get_name();}

/// Gets the name of a var_decl node.
///
/// @param node the var_decl node to get the name from.
///
/// @return the name of the node.
template<>
const string&
get_node_name(var_decl_sptr node)
{return node->get_name();}

/// Generic function to get the declaration of a given node, whatever
/// it is.  There has to be specializations for the kind of the nodes
/// we want to support.
///
/// @tparam NodeKind the type of the node we are looking at.
///
/// @return the declaration.
template<typename NodeKind>
static decl_base_sptr
convert_node_to_decl(shared_ptr<NodeKind> node);

/// Get the declaration of a given decl_base node
///
/// @param node the decl_base node to consider.
///
/// @return the declaration of the node.
template<>
decl_base_sptr
convert_node_to_decl(decl_base_sptr node)
{return node;}

/// Get the declaration of a given class_decl node
///
/// @param node the class_decl node to consider.
///
/// @return the declaration of the node.
template<>
decl_base_sptr
convert_node_to_decl(class_decl_sptr node)
{return node;}


/// Get the declaration of a type_base node.
///
/// @param node the type node to consider.
///
/// @return the declaration of the type_base.
template<>
decl_base_sptr
convert_node_to_decl(type_base_sptr node)
{return get_type_declaration(node);}

/// Get the declaration of a var_decl.
///
/// @param node the var_decl to consider.
///
/// @return the declaration of the var_decl.
template<>
decl_base_sptr
convert_node_to_decl(var_decl_sptr node)
{return node;}

/// Lookup a node in a given scope.
///
/// @tparam the type of the node to lookup.
///
/// @param fqn the components of the fully qualified name of the the
/// node to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the looked up node, or NULL if it
/// wasn't found.
template<typename NodeKind>
static const decl_base_sptr
lookup_node_in_scope(const list<string>& fqn,
		     const scope_decl_sptr skope)
{
  decl_base_sptr resulting_decl;
  shared_ptr<NodeKind> node;
  bool it_is_last = false;
  scope_decl_sptr cur_scope = skope, new_scope, scope;

  for (list<string>::const_iterator c = fqn.begin(); c != fqn.end(); ++c)
    {
      new_scope.reset();
      it_is_last = iterator_is_last(fqn, c);
      for (scope_decl::declarations::const_iterator m =
	     cur_scope->get_member_decls().begin();
	   m != cur_scope->get_member_decls().end();
	   ++m)
	{
	  if (!it_is_last)
	    {
	      // looking for a scope
	      scope = dynamic_pointer_cast<scope_decl>(*m);
	      if (scope && scope->get_name() == *c)
		{
		  new_scope = scope;
		  break;
		}
	    }
	  else
	    {
	      //looking for a final type.
	      node = dynamic_pointer_cast<NodeKind>(*m);
	      if (node && get_node_name(node) == *c)
		{
		  if (class_decl_sptr cl =
		      dynamic_pointer_cast<class_decl>(node))
		    if (cl->get_is_declaration_only()
			&& !cl->get_definition_of_declaration())
		      continue;
		  resulting_decl = convert_node_to_decl(node);
		  break;
		}
	    }
	}
      if (!new_scope && !resulting_decl)
	return decl_base_sptr();
      cur_scope = new_scope;
    }
  assert(resulting_decl);
  return resulting_decl;
}

/// lookup a type in a scope.
///
/// @param comps the components of the fully qualified name of the
/// type to lookup.
///
/// @param skope the scope to look into.
///
/// @return the declaration of the type found.
const decl_base_sptr
lookup_type_in_scope(const list<string>& comps,
		     const scope_decl_sptr& scope)
{return lookup_node_in_scope<type_base>(comps, scope);}

/// lookup a type in a scope.
///
/// @param type the type to look for.
///
/// @param access_path a vector of scopes the path of scopes to follow
/// before reaching the scope into which to look for @p type.  Note
/// that the deepest scope (the one immediately containing @p type) is
/// at index 0 of this vector, and the top-most scope is the last
/// element of the vector.
///
/// @param scope the top-most scope into which to look for @p type.
///
/// @return the scope found in @p scope, or NULL if it wasn't found.
static const type_base_sptr
lookup_type_in_scope(const type_base& type,
		     const vector<scope_decl*>& access_path,
		     const scope_decl* scope)
{
  vector<scope_decl*> a = access_path;
  type_base_sptr result;

  scope_decl* first_scope = a.back();
  assert(first_scope->get_name() == scope->get_name());
  a.pop_back();

  if (a.empty())
    {
      string n = get_type_name(type, false);
      for (scope_decl::declarations::const_iterator i =
	     scope->get_member_decls().begin();
	   i != scope->get_member_decls().end();
	   ++i)
	if (is_type(*i) && (*i)->get_name() == n)
	  {
	    result = is_type(*i);
	    break;
	  }
    }
  else
    {
      first_scope = a.back();
      string scope_name, cur_scope_name = first_scope->get_name();
      for (scope_decl::scopes::const_iterator i =
	     scope->get_member_scopes().begin();
	   i != scope->get_member_scopes().end();
	   ++i)
	{
	  scope_name = (*i)->get_name();
	  if (scope_name == cur_scope_name)
	    {
	      result = lookup_type_in_scope(type, a, (*i).get());
	      break;
	    }
	}
    }
  return result;
}

/// lookup a type in a scope.
///
/// @param type the type to look for.
///
/// @param scope the top-most scope into which to look for @p type.
///
/// @return the scope found in @p scope, or NULL if it wasn't found.
static const type_base_sptr
lookup_type_in_scope(const type_base_sptr type,
		     const scope_decl* scope)
{
  if (!type || is_function_type(type))
    return type_base_sptr();

  decl_base_sptr type_decl = get_type_declaration(type);
  assert(type_decl);
  vector<scope_decl*> access_path;
  for (scope_decl* s = type_decl->get_scope(); s != 0; s = s->get_scope())
    {
      access_path.push_back(s);
      if (is_global_scope(s))
	break;
    }
  return lookup_type_in_scope(*type, access_path, scope);
}

/// lookup a var_decl in a scope.
///
/// @param comps the components of the fully qualified name of the
/// var_decl to lookup.
///
/// @param skope the scope to look into.
const decl_base_sptr
lookup_var_decl_in_scope(const std::list<string>& comps,
			 const scope_decl_sptr& skope)
{return lookup_node_in_scope<var_decl>(comps, skope);}

/// Lookup an IR node from a translation unit.
///
/// @tparam NodeKind the type of the IR node to lookup from the
/// translation unit.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
template<typename NodeKind>
static const decl_base_sptr
lookup_node_in_translation_unit(const list<string>& fqn,
				const translation_unit& tu)
{return lookup_node_in_scope<NodeKind>(fqn, tu.get_global_scope());}

/// Lookup a type from a translation unit.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
const decl_base_sptr
lookup_type_in_translation_unit(const list<string>& fqn,
				const translation_unit& tu)
{return lookup_node_in_translation_unit<type_base>(fqn, tu);}

/// Lookup a class type from a translation unit.
///
/// @param fqn the components of the fully qualified name of the class
/// type node to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the class type IR node found, NULL
/// otherwise.
const class_decl_sptr
lookup_class_type_in_translation_unit(const list<string>& fqn,
				      const translation_unit& tu)
{return is_class_type(lookup_node_in_translation_unit<class_decl>(fqn, tu));}

/// Lookup a type from a translation unit.
///
/// @param fqn the components of the fully qualified name of the node
/// to look up.
///
/// @param tu the translation unit to perform lookup from.
///
/// @return the declaration of the IR node found, NULL otherwise.
const type_base_sptr
lookup_type_in_translation_unit(const type_base_sptr type,
				const translation_unit& tu)
{
  if (function_type_sptr fn_type = is_function_type(type))
    return lookup_function_type_in_translation_unit(fn_type, tu);
  return lookup_type_in_scope(type, tu.get_global_scope().get());
}

/// Demangle a C++ mangled name and return the resulting string
///
/// @param mangled_name the C++ mangled name to demangle.
///
/// @return the resulting mangled name.
string
demangle_cplus_mangled_name(const string& mangled_name)
{
  if (mangled_name.empty())
    return "";

  size_t l = 0;
  int status = 0;
  char * str = abi::__cxa_demangle(mangled_name.c_str(),
				   NULL, &l, &status);
  string demangled_name = mangled_name;
  if (str)
    {
      assert(status == 0);
      demangled_name = str;
      free(str);
      str = 0;
    }
  return demangled_name;
}

/// Return either the type given in parameter if it's non-null, or the
/// void type.
///
/// @param t the type to consider.
///
/// @param env the environment to use.  If NULL, just abort the
/// process.
///
/// @return either @p t if it is non-null, or the void type.
type_base_sptr
type_or_void(const type_base_sptr t, const environment* env)
{
  type_base_sptr r;

  if (t)
    r = t;
  else
    {
      assert(env);
      r = type_base_sptr(env->get_void_type_decl());
    }

  return r;
}

global_scope::~global_scope()
{
}

// <type_base definitions>

/// Definition of the private data of @ref type_base.
struct type_base::priv
{
  size_t		size_in_bits;
  size_t		alignment_in_bits;
  type_base_wptr	canonical_type;
  // The data member below holds the canonical type that is managed by
  // the smart pointer referenced by the canonical_type data member
  // above.  We are storing this underlying (naked) pointer here, so
  // that users can access it *fast*.  Otherwise, accessing
  // canonical_type above implies creating a shared_ptr, and that has
  // been measured to be slow for some performance hot spots.
  type_base*		naked_canonical_type;

  priv()
    : size_in_bits(),
      alignment_in_bits(),
      canonical_type()
  {}

  priv(size_t s,
       size_t a,
       type_base_sptr c = type_base_sptr())
    : size_in_bits(s),
      alignment_in_bits(a),
      canonical_type(c),
      naked_canonical_type(c.get())
  {}
}; // end struct type_base::priv

/// Compute the canonical type for a given instance of @ref type_base.
///
/// Consider two types T and T'.  The canonical type of T, denoted
/// C(T) is a type such as T == T' if and only if C(T) == C(T').  Said
/// otherwise, to compare two types, one just needs to compare their
/// canonical types using pointer equality.  That makes type
/// comparison faster than the structural comparison performed by the
/// abigail::ir::equals() overloads.
///
/// If there is not yet any canonical type for @p t, then @p t is its
/// own canonical type.  Otherwise, this function returns the
/// canonical type of @p t which is the canonical type that has the
/// same hash value as @p t and that structurally equals @p t.  Note
/// that after invoking this function, the life time of the returned
/// canonical time is then equals to the life time of the current
/// process.
///
/// @param t a smart pointer to instance of @ref type_base we want to
/// compute a canonical type for.
///
/// @return the canonical type for the current instance of @ref
/// type_base.
type_base_sptr
type_base::get_canonical_type_for(type_base_sptr t)
{
  if (!t)
    return t;

  environment* env = t->get_environment();
  assert(env);

  class_decl_sptr is_class;
  // Look through declaration-only classes
  if (class_decl_sptr class_declaration = is_class_type(t))
    {
      if (class_declaration->get_is_declaration_only())
	{
	  if (class_decl_sptr def =
	      class_declaration->get_definition_of_declaration())
	    t = def;
	  else
	    return type_base_sptr();
	}
      is_class = is_class_type(t);
    }

  if (t->get_canonical_type())
    return t->get_canonical_type();

  // We want the pretty representation of the type, but for an
  // internal use, not for a user-facing purpose.
  //
  // If two classe types Foo are declared, one as a class and the
  // other as a struct, but are otherwise equivalent, we want their
  // pretty representation to be the same.  Hence the 'internal'
  // argument of ir::get_pretty_representation() is set to true here.
  // So in this case, the pretty representation of Foo is going to be
  // "class Foo", regardless of its struct-ness. This also applies to
  // composite types which would have "class Foo" as a sub-type.
  string repr = ir::get_pretty_representation(t, /*internal=*/true);

  environment::canonical_types_map_type& types =
    env->get_canonical_types_map();

  type_base_sptr result;
  environment::canonical_types_map_type::iterator i = types.find(repr);
  if (i == types.end())
    {
      vector<type_base_sptr> v;
      v.push_back(t);
      types[repr] = v;
      result = t;
    }
  else
    {
      const corpus* t_corpus = t->get_corpus();
      vector<type_base_sptr> &v = i->second;
      // Let's compare 't' structurally (i.e, compare its sub-types
      // recursively) against the canonical types of the system. If it
      // equals a given canonical type C, then it means C is the
      // canonical type of 't'.  Otherwise, if 't' is different from
      // all the canonical types of the system, then it means 't' is a
      // canonical type itself.
      for (vector<type_base_sptr>::const_reverse_iterator it = v.rbegin();
	   it != v.rend();
	   ++it)
	{
	  // We are going to use the One Definition Rule[1] to perform
	  // a speed optimization here.
	  //
	  // Here is how I'd phrase that optimization: If 't' has the
	  // same *name* as a canonical type C which comes from the
	  // same *abi corpus* as 't', then C is the canonical type of
	  // 't.
	  //
	  // [1]: https://en.wikipedia.org/wiki/One_Definition_Rule
	  //
	  // Note how we walk the vector of canonical types by
	  // starting from the end; that is because since canonical
	  // types of a given corpus are added at the end of the this
	  // vector, when two ABI corpora have been loaded and their
	  // canonical types are present in this vector (e.g, when
	  // comparing two ABI corpora), the canonical types of the
	  // second corpus are going to be near the end the vector.
	  // As this function is likely to called during the loading
	  // of the ABI corpus, looking from the end of the vector
	  // maximizes the changes of triggering the optimization,
	  // even when we are reading the second corpus.
	  if (t_corpus
	      // We are not doing the optimizatin for anymous types
	      // because, well, two anonymous type have the same name
	      // (okay, they have no name), but that doesn't mean they
	      // are equal.
	      && !is_anonymous_type(t)
	      // We are not doing it for typedefs either, as I've seen
	      // instances of two typedefs with the same name but
	      // pointing to deferent types, e.g, in some boost
	      // library in our testsuite.
	      && !is_typedef(t)
	      // We are not doing it for pointers/references/arrays as
	      // two pointers to a type 'foo' might point to 'foo'
	      // meaning different things, as we've seen above.
	      && !is_pointer_type(t)
	      && !is_reference_type(t)
	      && !is_array_type(t)
	      // And we are not doing it for function types either,
	      // for similar reasons.
	      && !is_function_type(t))
	    {
	      if (const corpus* it_corpus = (*it)->get_corpus())
		{
		  if (it_corpus == t_corpus
		      // Let's add one more size constraint to rule
		      // out programs that break the One Definition
		      // Rule too easily.
		      && (*it)->get_size_in_bits() == t->get_size_in_bits())
		    {
		      // Both types come from the same ABI corpus and
		      // have the same name; the One Definition Rule
		      // of C and C++ says that these two types should
		      // be equal.  Using that rule would saves us
		      // from a potentially expensive type comparison
		      // here.
		      result = *it;
		      break;
		    }
		}
	    }
	  if (*it == t)
	    {
	      result = *it;
	      break;
	    }
	}
      if (!result)
	{
	  v.push_back(t);
	  result = t;
	}
    }
  return result;
}

/// Compute the canonical type of a given type.
///
/// It means that after invoking this function, comparing the intance
/// instance @ref type_base and another one (on which
/// type_base::enable_canonical_equality() would have been invoked as
/// well) is performed by just comparing the pointer values of the
/// canonical types of both types.  That equality comparison is
/// supposedly faster than structural comparison of the types.
///
/// @param t a smart pointer to the instance of @ref type_base for
/// which to compute the canonical type.  After this call,
/// t->get_canonical_type() will return the newly computed canonical
/// type.
///
/// @return the canonical type computed for @p t.
type_base_sptr
canonicalize(type_base_sptr t)
{
  if (!t)
    return t;

  if (t->get_canonical_type())
    return t->get_canonical_type();

  type_base_sptr canonical = type_base::get_canonical_type_for(t);

  t->priv_->canonical_type = canonical;
  t->priv_->naked_canonical_type = canonical.get();

  if (class_decl_sptr cl = is_class_type(t))
    if (type_base_sptr d = is_type(cl->get_earlier_declaration()))
      if (d->get_canonical_type())
	{
	  d->priv_->canonical_type = canonical;
	  d->priv_->naked_canonical_type = canonical.get();
	}

  return canonical;
}

/// The constructor of @ref type_base.
///
/// @param s the size of the type, in bits.
///
/// @param a the alignment of the type, in bits.
type_base::type_base(size_t s, size_t a)
  : priv_(new priv(s, a))
{}

/// Getter of the canonical type of the current instance of @ref
/// type_base.
///
/// @return a smart pointer to the canonical type of the current
/// intance of @ref type_base, or an empty smart pointer if the
/// current instance of @ref type_base doesn't have any canonical
/// type.
type_base_sptr
type_base::get_canonical_type() const
{
  if (priv_->canonical_type.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->canonical_type);
}

/// Getter of the canonical type pointer.
///
/// Note that this function doesn't return a smart pointer, but rather
/// the underlying pointer managed by the smart pointer.  So it's as
/// fast as possible.  This getter is to be used in code paths that
/// are proven to be performance hot spots; especially, when comparing
/// sensitive types like class, function, pointers and reference
/// types.  Those are compared extremely frequently and thus, their
/// accessing the canonical type must be fast.
///
/// @return the canonical type pointer, not managed by a smart
/// pointer.
type_base*
type_base::get_naked_canonical_type() const
{return priv_->naked_canonical_type;}

/// Compares two instances of @ref type_base.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p is non-null and if the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const type_base& l, const type_base& r, change_kind* k)
{
  bool result = (l.get_size_in_bits() == r.get_size_in_bits()
		 && l.get_alignment_in_bits() == r.get_alignment_in_bits());
  if (!result)
    if (k)
      *k |= LOCAL_CHANGE_KIND;
  return result;
}

/// Return true iff both type declarations are equal.
///
/// Note that this doesn't test if the scopes of both types are equal.
bool
type_base::operator==(const type_base& other) const
{return equals(*this, other, 0);}

/// Setter for the size of the type.
///
/// @param s the new size -- in bits.
void
type_base::set_size_in_bits(size_t s)
{priv_->size_in_bits = s;}

/// Getter for the size of the type.
///
/// @return the size in bits of the type.
size_t
type_base::get_size_in_bits() const
{return priv_->size_in_bits;}

/// Setter for the alignment of the type.
///
/// @param a the new alignment -- in bits.
void
type_base::set_alignment_in_bits(size_t a)
{priv_->alignment_in_bits = a;}

/// Getter for the alignment of the type.
///
/// @return the alignment of the type in bits.
size_t
type_base::get_alignment_in_bits() const
{return priv_->alignment_in_bits;}

/// Default implementation of traversal for types.  This function does
/// nothing.  It must be implemented by every single new type that is
/// written.
///
/// Please look at e.g, class_decl::traverse() for an example of how
/// to implement this.
///
/// @param v the visitor used to visit the type.
bool
type_base::traverse(ir_node_visitor& v)
{
  v.visit_begin(this);
  return v.visit_end(this);
}

type_base::~type_base()
{delete priv_;}

// </type_base definitions>

//<type_decl definitions>

type_decl::type_decl(const std::string&	name,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     const location&		locus,
		     const std::string&	linkage_name,
		     visibility		vis)

  : decl_base(name, locus, linkage_name, vis),
    type_base(size_in_bits, alignment_in_bits)
{
}

/// Compares two instances of @ref type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const type_decl& l, const type_decl& r, change_kind* k)
{
  bool result = equals(static_cast<const decl_base&>(l),
		       static_cast<const decl_base&>(r),
		       k);
  if (!k && !result)
    return false;

  result &= equals(static_cast<const type_base&>(l),
		   static_cast<const type_base&>(r),
		   k);
  return result;
}

/// Return true if both types equals.
///
/// This operator re-uses the overload that takes a decl_base.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check agains.
bool
type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check against.
bool
type_decl::operator==(const decl_base& o) const
{
  const type_decl* other = dynamic_cast<const type_decl*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check against.
bool
type_decl::operator==(const type_decl& o) const
{
  const decl_base& other = o;
  return *this == other;
}

/// Equality operator for @ref type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l equals @p r.
bool
operator==(const type_decl_sptr& l, const type_decl_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  return *l == *r;
}

/// Get the pretty representation of the current instance of @ref
/// type_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representatin of the @ref type_decl.
string
type_decl::get_pretty_representation(bool internal) const
{return get_qualified_name(internal);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
type_decl::traverse(ir_node_visitor& v)
{
  v.visit_begin(this);

  return v.visit_end(this);
}

type_decl::~type_decl()
{}
//</type_decl definitions>

// <scope_type_decl definitions>

scope_type_decl::scope_type_decl(const std::string&	name,
				 size_t		size_in_bits,
				 size_t		alignment_in_bits,
				 const location&	locus,
				 visibility		vis)
  : decl_base(name, locus, "", vis),
    type_base(size_in_bits, alignment_in_bits),
    scope_decl(name, locus)
{
}

/// Compares two instances of @ref scope_type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const scope_type_decl& l, const scope_type_decl& r, change_kind* k)
{
  bool result = true;

  result = equals(static_cast<const scope_decl&>(l),
		  static_cast<const scope_decl&>(r),
		  k);

  if (!k && !result)
    return false;

  result &= equals(static_cast<const type_base&>(l),
		   static_cast<const type_base&>(r),
		   k);

  return result;
}

/// Equality operator between two scope_type_decl.
///
/// Note that this function does not consider the scope of the scope
/// types themselves.
///
/// @return true iff both scope types are equal.
bool
scope_type_decl::operator==(const decl_base& o) const
{
  const scope_type_decl* other = dynamic_cast<const scope_type_decl*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

/// Equality operator between two scope_type_decl.
///
/// This re-uses the equality operator that takes a decl_base.
///
/// @param o the other scope_type_decl to compare against.
///
/// @return true iff both scope types are equal.
bool
scope_type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;

  return *this == *other;
}

/// Traverses an instance of @ref scope_type_decl, visiting all the
/// sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
scope_type_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      for (scope_decl::declarations::const_iterator i =
	     get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	if (!(*i)->traverse(v))
	  break;
      visiting(false);
    }

  return v.visit_end(this);
}

scope_type_decl::~scope_type_decl()
{}
// </scope_type_decl definitions>

// <namespace_decl>
namespace_decl::namespace_decl(const std::string&	name,
			       const location&		locus,
			       visibility		vis)
  : // We need to call the constructor of decl_base directly here
    // because it is virtually inherited by scope_decl.  Note that we
    // just implicitely call the default constructor for scope_decl
    // here, as what we really want is to initialize the decl_base
    // subobject.  Wow, virtual inheritance is useful, but setting it
    // up is ugly.
  decl_base(name, locus, "", vis),
  scope_decl(name, locus)
{
}

/// Build and return a copy of the pretty representation of the
/// namespace.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the namespace.
string
namespace_decl::get_pretty_representation(bool internal) const
{
  string r = "namespace " + scope_decl::get_pretty_representation(internal);
  return r;
}

/// Return true iff both namespaces and their members are equal.
///
/// Note that this function does not check if the scope of these
/// namespaces are equal.
bool
namespace_decl::operator==(const decl_base& o) const
{
  const namespace_decl* other = dynamic_cast<const namespace_decl*>(&o);
  if (!other)
    return false;
  return scope_decl::operator==(*other);
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// member nodes.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
namespace_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      scope_decl::declarations::const_iterator i;
      for (i = get_member_decls().begin();
	   i != get_member_decls ().end();
	   ++i)
	{
	  ir_traversable_base_sptr t =
	    dynamic_pointer_cast<ir_traversable_base>(*i);
	  if (t)
	    if (!t->traverse (v))
	      break;
	}
      visiting(false);
    }
  return v.visit_end(this);
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

/// Type of the private data of qualified_type_def.
class qualified_type_def::priv
{
  friend class qualified_type_def;

  qualified_type_def::CV	cv_quals_;
  // Before the type is canonicalized, this is used as a temporary
  // internal name.
  string			temporary_internal_name_;
  // Once the type is canonicalized, this is used as the internal
  // name.
  string			internal_name_;
  weak_ptr<type_base>		underlying_type_;

  priv()
    : cv_quals_(CV_NONE)
  {}

  priv(qualified_type_def::CV quals,
       type_base_sptr t)
    : cv_quals_(quals),
      underlying_type_(t)
  {}
};// end class qualified_type_def::priv

/// Build the name of the current instance of qualified type.
///
/// @param fully_qualified if true, build a fully qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the newly-built name.
string
qualified_type_def::build_name(bool fully_qualified, bool internal) const
{
  assert(get_underlying_type());
  string quals = get_cv_quals_string_prefix();
  string name = get_type_name(get_underlying_type(),
			      fully_qualified,
			      internal);

  if (quals.empty() && internal)
    // We are asked to return the internal name, that might be used
    // for type canonicalization.  For that canonicalization, we need
    // to make a difference between a no-op qualified type which
    // underlying type is foo (the qualified type is named "none
    // foo"), and the name of foo, which is just "foo".
    quals = "none";

  if (!quals.empty())
    {
      if (is_pointer_type(get_underlying_type())
	  || is_reference_type(get_underlying_type()))
	{
	  name += " ";
	  name += quals;
	}
      else
	name = quals + " " + name;
    }

  return name;
}

/// Constructor of the qualified_type_def
///
/// @param type the underlying type
///
/// @param quals a bitfield representing the const/volatile qualifiers
///
/// @param locus the location of the qualified type definition
qualified_type_def::qualified_type_def(type_base_sptr	type,
				       CV		quals,
				       const location&	locus)
  : type_base(type->get_size_in_bits(),
	      type->get_alignment_in_bits()),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(type)->get_visibility()),
    priv_(new priv(quals, type))
{
  assert(type);
  string name = build_name(false);
  set_name(name);
}

/// Get the size of the qualified type def.
///
/// This is an overload for type_base::get_size_in_bits().
///
/// @return the size of the qualified type.
size_t
qualified_type_def::get_size_in_bits() const
{
  size_t s = get_underlying_type()->get_size_in_bits();
  if (s != type_base::get_size_in_bits())
    const_cast<qualified_type_def*>(this)->set_size_in_bits(s);
  return type_base::get_size_in_bits();
}

/// Compares two instances of @ref qualified_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const qualified_type_def& l, const qualified_type_def& r, change_kind* k)
{
  bool result = true;
  if (l.get_cv_quals() != r.get_cv_quals())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (l.get_underlying_type() != r.get_underlying_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	// okay strictly speaking this is not necessary, but I am
	// putting it here to maintenance; that is, so that adding
	// subsequent clauses needed to compare two qualified types
	// later still works.
	return false;
    }

  return result;
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const decl_base& o) const
{
  const qualified_type_def* other =
    dynamic_cast<const qualified_type_def*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();


  return equals(*this, *other, 0);
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
/// Also, this re-uses the equality operator above that takes a
/// decl_base.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator for qualified types.
///
/// Note that this function does not check for equality of the scopes.
/// Also, this re-uses the equality operator above that takes a
/// decl_base.
///
///@param o the other qualified type to compare against.
///
/// @return true iff both qualified types are equal.
bool
qualified_type_def::operator==(const qualified_type_def& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Implementation for the virtual qualified name builder for @ref
/// qualified_type_def.
///
/// @param qualified_name the output parameter to hold the resulting
/// qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
qualified_type_def::get_qualified_name(string& qualified_name,
				       bool internal) const
{qualified_name = get_qualified_name(internal);}

/// Implementation of the virtual qualified name builder/getter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const string&
qualified_type_def::get_qualified_name(bool internal) const
{
  if (!get_canonical_type())
    {
      // The type hasn't been canonicalized yet. We want to return a
      // temporary name.
      if (internal)
	{
	  // We are asked to return a temporary *internal* name.  So
	  // let's return it from the right cache.
	  if (priv_->temporary_internal_name_.empty())
	    priv_->temporary_internal_name_ =
	      build_name(true, /*internal=*/true);
	  return priv_->temporary_internal_name_;
	}
      else
	{
	  // We are asked to return a temporary non-internal name.
	  // This comes from a different cache.
	  if (peek_temporary_qualified_name().empty())
	    set_temporary_qualified_name(build_name(true, /*internal=*/false));
	  return peek_temporary_qualified_name();
	}
    }
  else
    {
      // The type has already been canonicalized. We want to return
      // the definitive name.
      if (internal)
	{
	  if (priv_->internal_name_.empty())
	    priv_->internal_name_ =
	      build_name(/*qualified=*/true, /*internal=*/true);
	  return priv_->internal_name_;
	}
      else
	{
	  if (peek_qualified_name().empty())
	    set_qualified_name(build_name(/*qualified=*/true,
					  /*internal=*/false));
	  return peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
qualified_type_def::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

qualified_type_def::~qualified_type_def()
{
}

/// Getter of the const/volatile qualifier bit field
qualified_type_def::CV
qualified_type_def::get_cv_quals() const
{return priv_->cv_quals_;}

/// Setter of the const/value qualifiers bit field
void
qualified_type_def::set_cv_quals(CV cv_quals)
{priv_->cv_quals_ = cv_quals;}

/// Compute and return the string prefix or suffix representing the
/// qualifiers hold by the current instance of @ref
/// qualified_type_def.
///
/// @return the newly-built cv string.
string
qualified_type_def::get_cv_quals_string_prefix() const
{
  string prefix;
  if (priv_->cv_quals_ & qualified_type_def::CV_RESTRICT)
    prefix = "restrict";
  if (priv_->cv_quals_ & qualified_type_def::CV_CONST)
    {
      if (!prefix.empty())
	prefix += ' ';
      prefix += "const";
    }
  if (priv_->cv_quals_ & qualified_type_def::CV_VOLATILE)
    {
      if (!prefix.empty())
	prefix += ' ';
      prefix += "volatile";
    }
  return prefix;
}

/// Getter of the underlying type
shared_ptr<type_base>
qualified_type_def::get_underlying_type() const
{
  if (priv_->underlying_type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->underlying_type_);
}

/// Non-member equality operator for @ref qualified_type_def
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r equals.
bool
operator==(const qualified_type_def_sptr& l, const qualified_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// Overloaded bitwise OR operator for cv qualifiers.
qualified_type_def::CV
operator| (qualified_type_def::CV lhs,
	   qualified_type_def::CV rhs)
{
  return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

/// Overloaded bitwise AND operator for CV qualifiers.
qualified_type_def::CV
operator&(qualified_type_def::CV lhs, qualified_type_def::CV rhs)
{
    return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
}

/// Overloaded bitwise inverting operator for CV qualifiers.
qualified_type_def::CV
operator~(qualified_type_def::CV q)
{return static_cast<qualified_type_def::CV>(~static_cast<unsigned>(q));}

/// Streaming operator for qualified_type_decl::CV
///
/// @param o the output stream to serialize the cv qualifier to.
///
/// @param cv the cv qualifier to serialize.
///
/// @return the output stream used.
std::ostream&
operator<<(std::ostream& o, qualified_type_def::CV cv)
{
  string str;

  switch (cv)
    {
    case qualified_type_def::CV_NONE:
      str = "none";
      break;
    case qualified_type_def::CV_CONST:
      str = "const";
      break;
    case qualified_type_def::CV_VOLATILE:
      str = "volatile";
      break;
    case qualified_type_def::CV_RESTRICT:
      str = "restrict";
      break;
    }

  o << str;
  return o;
}

// </qualified_type_def>

//<pointer_type_def definitions>

/// Private data structure of the @ref pointer_type_def.
struct pointer_type_def::priv
{
  type_base_wptr pointed_to_type_;
  string internal_qualified_name_;
  string temp_internal_qualified_name_;

  priv(const type_base_sptr& t)
    : pointed_to_type_(t)
  {}

  priv()
  {}
}; //end struct pointer_type_def

pointer_type_def::pointer_type_def(const type_base_sptr&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   const location&		locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, ""),
    priv_(new priv)
{
  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name = (pto ? pto->get_name() : string("void")) + "*";
      set_name(name);
      if (pto)
        set_visibility(pto->get_visibility());
      priv_->pointed_to_type_ = type_base_wptr(type_or_void(pointed_to, 0));
    }
  catch (...)
    {}
}

/// Compares two instances of @ref pointer_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const pointer_type_def& l, const pointer_type_def& r, change_kind* k)
{
  bool result = (l.get_pointed_to_type() == r.get_pointed_to_type());
  if (!result)
    if (k)
      *k |= SUBTYPE_CHANGE_KIND;

  return result;
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const decl_base& o) const
{
  const pointer_type_def* other = dynamic_cast<const pointer_type_def*>(&o);
  if (!other)
    return false;

  type_base* canonical_type = get_naked_canonical_type();
  type_base* other_canonical_type = other->get_naked_canonical_type();

  if (canonical_type && other_canonical_type)
    return canonical_type == other_canonical_type;

  return equals(*this, *other, 0);
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the
/// types.
///
/// @param other the other type to compare against.
///
/// @return true iff @p other equals the current instance.
bool
pointer_type_def::operator==(const type_base& other) const
{
  const decl_base* o = dynamic_cast<const decl_base*>(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the
/// types.
///
/// @param other the other type to compare against.
///
/// @return true iff @p other equals the current instance.
bool
pointer_type_def::operator==(const pointer_type_def& other) const
{
  const decl_base& o = other;
  return *this == o;
}

const type_base_sptr
pointer_type_def::get_pointed_to_type() const
{
  if (priv_->pointed_to_type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->pointed_to_type_);
}

/// Build and return the qualified name of the current instance of
/// @ref pointer_type_def.
///
/// @param qn output parameter.  The resulting qualified name.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
pointer_type_def::get_qualified_name(string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Build, cache and return the qualified name of the current instance
/// of @ref pointer_type_def.  Subsequent invocations of this function
/// return the cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const string&
pointer_type_def::get_qualified_name(bool internal) const
{
  if (internal)
    {
      if (get_canonical_type())
	{
	  if (priv_->internal_qualified_name_.empty())
	    priv_->internal_qualified_name_ =
	      get_type_name(get_pointed_to_type(),
			    /*qualified_name=*/true,
			    /*internal=*/true) + "*";
	  return priv_->internal_qualified_name_;
	}
      else
	{
	  if (priv_->temp_internal_qualified_name_.empty())
	    priv_->temp_internal_qualified_name_ =
	      get_type_name(get_pointed_to_type(),
			    /*qualified_name=*/true,
			    /*internal=*/true) + "*";
	  return priv_->temp_internal_qualified_name_;
	}
    }
  else
    {
      if (get_canonical_type())
	{
	  if (decl_base::peek_qualified_name().empty())
	    set_qualified_name(get_type_name(get_pointed_to_type(),
					     /*qualified_name=*/true,
					     /*internal=*/false) + "*");
	  return decl_base::peek_qualified_name();
	}
      else
	{
	  set_qualified_name(get_type_name(get_pointed_to_type(),
					   /*qualified_name=*/true,
					   /*internal=*/false) + "*");
	  return decl_base::peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
pointer_type_def::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_pointed_to_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

pointer_type_def::~pointer_type_def()
{}

/// Turn equality of shared_ptr of @ref pointer_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref pointer_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref pointer_type_def on
/// right-hand-side of the equality.
///
/// @return true if the @ref pointer_type_def pointed to by the
/// shared_ptrs are equal, false otherwise.
bool
operator==(const pointer_type_def_sptr& l, const pointer_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

// </pointer_type_def definitions>

// <reference_type_def definitions>

reference_type_def::reference_type_def(const type_base_sptr	pointed_to,
				       bool			lvalue,
				       size_t			size_in_bits,
				       size_t			align_in_bits,
				       const location&		locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, ""),
    is_lvalue_(lvalue)
{
  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name;
      if (pto)
        {
          set_visibility(pto->get_visibility());
          name = pto->get_name() + "&";
        }
      else
	name = get_type_name(is_function_type(pointed_to),
			     /*qualified_name=*/true) + "&";

      if (!is_lvalue())
	name += "&";
      set_name(name);

      pointed_to_type_ = type_base_wptr(type_or_void(pointed_to, 0));
    }
  catch (...)
    {}
}

/// Compares two instances of @ref reference_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const reference_type_def& l, const reference_type_def& r, change_kind* k)
{
  if (l.is_lvalue() != r.is_lvalue())
    {
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      return false;
    }

  bool result = (l.get_pointed_to_type() == r.get_pointed_to_type());
  if (!result)
    if (k)
      *k |= SUBTYPE_CHANGE_KIND;
  return result;
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const decl_base& o) const
{
  const reference_type_def* other =
    dynamic_cast<const reference_type_def*>(&o);
  if (!other)
    return false;

  type_base* canonical_type = get_naked_canonical_type();
  type_base* other_canonical_type = other->get_naked_canonical_type();

  if (canonical_type && other_canonical_type)
    return canonical_type == other_canonical_type;

  return equals(*this, *other, 0);
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator of the @ref reference_type_def type.
///
/// @param o the other instance of @ref reference_type_def to compare
/// against.
///
/// @return true iff the two instances are equal.
bool
reference_type_def::operator==(const reference_type_def& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

type_base_sptr
reference_type_def::get_pointed_to_type() const
{
  if (pointed_to_type_.expired())
    return type_base_sptr();
  return type_base_sptr(pointed_to_type_);
}

bool
reference_type_def::is_lvalue() const
{return is_lvalue_;}

/// Build and return the qualified name of the current instance of the
/// @ref reference_type_def.
///
/// @param qn output parameter.  Is set to the newly-built qualified
/// name of the current instance of @ref reference_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
reference_type_def::get_qualified_name(string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Build, cache and return the qualified name of the current instance
/// of the @ref reference_type_def.  Subsequent invocations of this
/// function return the cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the newly-built qualified name of the current instance of
/// @ref reference_type_def.
const string&
reference_type_def::get_qualified_name(bool internal) const
{
  if (peek_qualified_name().empty()
      || !get_canonical_type())
    {
      string name = get_type_name(get_pointed_to_type(),
				  /*qualified_name=*/true,
				  internal);
      if (is_lvalue())
	set_qualified_name(name + "&");
      else
	set_qualified_name(name + "&&");
    }
  return peek_qualified_name();
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
reference_type_def::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_pointed_to_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

reference_type_def::~reference_type_def()
{}

/// Turn equality of shared_ptr of @ref reference_type_def into a deep
/// equality; that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of @ref reference_type_def on left-hand-side
/// of the equality.
///
/// @param r the shared_ptr of @ref reference_type_def on
/// right-hand-side of the equality.
///
/// @return true if the @ref reference_type_def pointed to by the
/// shared_ptrs are equal, false otherwise.
bool
operator==(const reference_type_def_sptr& l, const reference_type_def_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

// </reference_type_def definitions>

// <array_type_def definitions>

// <array_type_def::subrange_type>
struct array_type_def::subrange_type::priv
{
  size_t	lower_bound_;
  size_t	upper_bound_;
  location	location_;
  priv(size_t ub, const location& loc)
    : lower_bound_(0), upper_bound_(ub), location_(loc) {}

  priv(size_t lb, size_t ub, const location& loc)
    : lower_bound_(lb), upper_bound_(ub), location_(loc) {}
};

array_type_def::subrange_type::subrange_type(size_t		lower_bound,
					     size_t		upper_bound,
					     const location&	loc)
  : priv_(new priv(lower_bound, upper_bound, loc))
{}

array_type_def::subrange_type::subrange_type(size_t		upper_bound,
					     const location&	loc)
  : priv_(new priv(upper_bound, loc))
{}

size_t
array_type_def::subrange_type::get_upper_bound() const
{return priv_->upper_bound_;}

size_t
array_type_def::subrange_type::get_lower_bound() const
{return priv_->lower_bound_;}

void
array_type_def::subrange_type::set_upper_bound(size_t ub)
{priv_->upper_bound_ = ub;}

void
array_type_def::subrange_type::set_lower_bound(size_t lb)
{priv_->lower_bound_ = lb;}

size_t
array_type_def::subrange_type::get_length() const
{return get_upper_bound() - get_lower_bound() + 1;}

bool
array_type_def::subrange_type::is_infinite() const
{return get_length() == 0;}

bool
array_type_def::subrange_type::operator==(const subrange_type& o) const
{
  return (get_lower_bound() == o.get_lower_bound()
	  && get_upper_bound() == o.get_upper_bound());
}

const location&
array_type_def::subrange_type::get_location() const
{return priv_->location_;}

// </array_type_def::subrange_type>
struct array_type_def::priv
{
  type_base_wptr	element_type_;
  subranges_type	subranges_;
  string		temp_internal_qualified_name_;
  string		internal_qualified_name_;

  priv(type_base_sptr t)
    : element_type_(t) {}
  priv(type_base_sptr t, subranges_type subs)
    : element_type_(t), subranges_(subs) {}
};

/// Constructor for the type array_type_def
///
/// Note how the constructor expects a vector of subrange
/// objects. Parsing of the array information always entails
/// parsing the subrange info as well, thus the class subrange_type
/// is defined inside class array_type_def and also parsed
/// simultaneously.
///
/// @param e_type the type of the elements contained in the array
///
/// @param subs a vector of the array's subranges(dimensions)
///
/// @param locus the source location of the array type definition.
array_type_def::array_type_def(const type_base_sptr			e_type,
			       const std::vector<subrange_sptr>&	subs,
			       const location&				locus)
  : type_base(0, e_type->get_alignment_in_bits()),
    decl_base(locus),
    priv_(new priv(e_type))
{append_subranges(subs);}

string
array_type_def::get_subrange_representation() const
{
  string r;
  for (std::vector<subrange_sptr >::const_iterator i = get_subranges().begin();
       i != get_subranges().end(); ++i)
    {
      r += "[";
      std::ostringstream o;
      o << (*i)->get_length();
      r += o.str();
      r += "]";
    }
  return r;
}

/// Get the string representation of an @ref array_type_def.
///
/// @param a the array type to consider.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
static string
get_type_representation(const array_type_def& a, bool internal)
{
  type_base_sptr e_type = a.get_element_type();
  decl_base_sptr d = get_type_declaration(e_type);
  string r;

  if (internal)
    r = get_type_name(e_type, /*qualified=*/true, /*internal=*/true)
      + a.get_subrange_representation();
  else
    r = get_type_name(e_type, /*qualified=*/false, /*internal=*/false)
      + a.get_subrange_representation();

  return r;
}

/// Get the pretty representation of the current instance of @ref
/// array_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
string
array_type_def::get_pretty_representation(bool internal) const
{return get_type_representation(*this, internal);}

/// Compares two instances of @ref array_type_def.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const array_type_def& l, const array_type_def& r, change_kind* k)
{
  std::vector<array_type_def::subrange_sptr > this_subs = l.get_subranges();
  std::vector<array_type_def::subrange_sptr > other_subs = r.get_subranges();

  bool result = true;
  if (this_subs.size() != other_subs.size())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  std::vector<array_type_def::subrange_sptr >::const_iterator i,j;
  for (i = this_subs.begin(), j = other_subs.begin();
       i != this_subs.end();
       ++i, ++j)
    if (**i != **j)
      {
	result = false;
	if (k)
	  {
	    *k |= LOCAL_CHANGE_KIND;
	    break;
	  }
	else
	  return false;
      }

  if (l.get_element_type() != r.get_element_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

bool
array_type_def::operator==(const decl_base& o) const
{
  const array_type_def* other =
    dynamic_cast<const array_type_def*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

bool
array_type_def::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Getter of the type of an array element.
///
/// @return the type of an array element.
const type_base_sptr
array_type_def::get_element_type() const
{
  if (priv_->element_type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->element_type_);
}

// Append a single subrange @param sub.
void
array_type_def::append_subrange(subrange_sptr sub)
{
  priv_->subranges_.push_back(sub);
  size_t s = get_size_in_bits();
  s += sub->get_length() * get_element_type()->get_size_in_bits();
  set_size_in_bits(s);
  string r = get_pretty_representation();
  set_name(r);
}

/// Append subranges from the vector @param subs to the current
/// vector of subranges.
void
array_type_def::append_subranges(const std::vector<subrange_sptr>& subs)
{
  for (std::vector<shared_ptr<subrange_type> >::const_iterator i = subs.begin();
       i != subs.end();
       ++i)
    append_subrange(*i);
}

/// @return true iff one of the sub-ranges of the array is infinite.
bool
array_type_def::is_infinite() const
{

  for (std::vector<shared_ptr<subrange_type> >::const_iterator i =
	 priv_->subranges_.begin();
       i != priv_->subranges_.end();
       ++i)
    if ((*i)->is_infinite())
      return true;

  return false;
}

int
array_type_def::get_dimension_count() const
{return priv_->subranges_.size();}

/// Build and return the qualified name of the current instance of the
/// @ref array_type_def.
///
/// @param qn output parameter.  Is set to the newly-built qualified
/// name of the current instance of @ref array_type_def.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
void
array_type_def::get_qualified_name(string& qn, bool internal) const
{qn = get_qualified_name(internal);}

/// Compute the qualified name of the array.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the resulting qualified name.
const string&
array_type_def::get_qualified_name(bool internal) const
{
  if (internal)
    {
      if (get_canonical_type())
	{
	  if (priv_->internal_qualified_name_.empty())
	    priv_->internal_qualified_name_ =
	      get_type_representation(*this, /*internal=*/true);
	  return priv_->internal_qualified_name_;
	}
      else
	{
	  if (priv_->temp_internal_qualified_name_.empty())
	    priv_->temp_internal_qualified_name_ =
	      get_type_representation(*this, /*internal=*/true);
	  return priv_->temp_internal_qualified_name_;
	}
    }
  else
    {
      if (get_canonical_type())
	{
	  if (decl_base::peek_qualified_name().empty())
	    set_qualified_name(get_type_representation(*this,
						       /*internal=*/false));
	  return decl_base::peek_qualified_name();
	}
      else
	{
	  set_qualified_name(get_type_representation(*this,
						     /*internal=*/false));
	  return decl_base::peek_qualified_name();
	}
    }
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
array_type_def::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_element_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

const location&
array_type_def::get_location() const
{return decl_base::get_location();}

/// Get the array's subranges
const std::vector<array_type_def::subrange_sptr>&
array_type_def::get_subranges() const
{return priv_->subranges_;}

array_type_def::~array_type_def()
{}

// </array_type_def definitions>

// <enum_type_decl definitions>

class enum_type_decl::priv
{
  type_base_sptr	underlying_type_;
  enumerators		enumerators_;

  friend class enum_type_decl;

  priv();

public:

  priv(type_base_sptr underlying_type,
       enumerators& enumerators)
    : underlying_type_(underlying_type),
      enumerators_(enumerators)
  {}
}; // end class enum_type_decl::priv

enum_type_decl::enum_type_decl(const string& name,
			       const location& locus,
			       type_base_sptr underlying_type,
			       enumerators& enums,
			       const string& mangled_name,
			       visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    priv_(new priv(underlying_type, enums))
{
  for (enumerators::iterator e = get_enumerators().begin();
       e != get_enumerators().end();
       ++e)
    e->set_enum_type(this);
}

/// Return the underlying type of the enum.
type_base_sptr
enum_type_decl::get_underlying_type() const
{return priv_->underlying_type_;}

/// @return the list of enumerators of the enum.
const enum_type_decl::enumerators&
enum_type_decl::get_enumerators() const
{return priv_->enumerators_;}

/// @return the list of enumerators of the enum.
enum_type_decl::enumerators&
enum_type_decl::get_enumerators()
{return priv_->enumerators_;}

/// Get the pretty representation of the current instance of @ref
/// enum_type_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation of the enum type.
string
enum_type_decl::get_pretty_representation(bool internal) const
{
  string r = "enum " + decl_base::get_pretty_representation(internal);
  return r;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
enum_type_decl::traverse(ir_node_visitor &v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{}

/// Compares two instances of @ref enum_type_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const enum_type_decl& l, const enum_type_decl& r, change_kind* k)
{
  bool result = true;
  if (*l.get_underlying_type() != *r.get_underlying_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  enum_type_decl::enumerators::const_iterator i, j;
  for (i = l.get_enumerators().begin(), j = r.get_enumerators().begin();
       i != l.get_enumerators().end() && j != r.get_enumerators().end();
       ++i, ++j)
    if (*i != *j)
      {
	result = false;
	if (k)
	  {
	    *k |= LOCAL_CHANGE_KIND;
	    break;
	  }
	else
	  return false;
      }

  if (i != l.get_enumerators().end() || j != r.get_enumerators().end())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (!(l.decl_base::operator==(r) && l.type_base::operator==(r)))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Equality operator.
///
/// @param o the other enum to test against.
///
/// @return true iff @p o is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const decl_base& o) const
{
  const enum_type_decl* op = dynamic_cast<const enum_type_decl*>(&o);
  if (!op)
    return false;

  if (get_canonical_type() && op->get_canonical_type())
    return get_canonical_type().get() == op->get_canonical_type().get();

  return equals(*this, *op, 0);
}

/// Equality operator.
///
/// @param o the other enum to test against.
///
/// @return true iff @p o is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Equality operator for @ref enum_type_decl_sptr.
///
/// @param l the first operand to compare.
///
/// @param r the second operand to compare.
///
/// @return true iff @p l equals @p r.
bool
operator==(const enum_type_decl_sptr& l, const enum_type_decl_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (l.get() == r.get())
    return true;
  decl_base_sptr o = r;
  return *l == *o;
}

/// The type of the private data of an @ref
/// enum_type_decl::enumerator.
class enum_type_decl::enumerator::priv
{
  string name_;
  size_t value_;
  string qualified_name_;
  enum_type_decl* enum_type_;

  friend class enum_type_decl::enumerator;

public:

  priv()
    : enum_type_()
  {}

  priv(const string& name, size_t value, enum_type_decl* e = 0)
    : name_(name),
      value_(value),
      enum_type_(e)
  {}
}; // end class enum_type_def::enumerator::priv

/// Default constructor of the @ref enum_type_decl::enumerator type.
enum_type_decl::enumerator::enumerator()
  : priv_(new priv)
{}

/// Constructor of the @ref enum_type_decl::enumerator type.
///
/// @param name the name of the enumerator.
///
/// @param value the value of the enumerator.
enum_type_decl::enumerator::enumerator(const string& name, size_t value)
  : priv_(new priv(name, value))
{}

/// Copy constructor of the @ref enum_type_decl::enumerator type.
///
/// @param other enumerator to copy.
enum_type_decl::enumerator::enumerator(const enumerator& other)
  : priv_(new priv(other.get_name(),
		   other.get_value(),
		   other.get_enum_type()))
{}

/// Equality operator
///
/// @param other the enumerator to compare to the current instance of
/// enum_type_decl::enumerator.
///
/// @return true if @p other equals the current instance of
/// enum_type_decl::enumerator.
bool
enum_type_decl::enumerator::operator==(const enumerator& other) const
{return (get_name() == other.get_name()
	 && get_value() == other.get_value());}

/// Getter for the name of the current instance of
/// enum_type_decl::enumerator.
///
/// @return a reference to the name of the current instance of
/// enum_type_decl::enumerator.
const string&
enum_type_decl::enumerator::get_name() const
{return priv_->name_;}

/// Getter for the qualified name of the current instance of
/// enum_type_decl::enumerator.  The first invocation of the method
/// builds the qualified name, caches it and return a reference to the
/// cached qualified name.  Subsequent invocations just return the
/// cached value.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the qualified name of the current instance of
/// enum_type_decl::enumerator.
const string&
enum_type_decl::enumerator::get_qualified_name(bool internal) const
{
  if (priv_->qualified_name_.empty())
    priv_->qualified_name_ =
      get_enum_type()->get_qualified_name(internal) + "::" + get_name();
  return priv_->qualified_name_;
}

/// Setter for the name of @ref enum_type_decl::enumerator.
///
/// @param n the new name.
void
enum_type_decl::enumerator::set_name(const string& n)
{priv_->name_ = n;}

/// Getter for the value of @ref enum_type_decl::enumerator.
///
/// @return the value of the current instance of
/// enum_type_decl::enumerator.
size_t
enum_type_decl::enumerator::get_value() const
{return priv_->value_;}

/// Setter for the value of @ref enum_type_decl::enumerator.
///
/// @param v the new value of the enum_type_decl::enumerator.
void
enum_type_decl::enumerator::set_value(size_t v)
{priv_->value_= v;}

/// Getter for the enum type that this enumerator is for.
///
/// @return the enum type that this enumerator is for.
enum_type_decl*
enum_type_decl::enumerator::get_enum_type() const
{return priv_->enum_type_;}

/// Setter for the enum type that this enumerator is for.
///
/// @param e the new enum type.
void
enum_type_decl::enumerator::set_enum_type(enum_type_decl* e)
{priv_->enum_type_ = e;}
// </enum_type_decl definitions>

// <typedef_decl definitions>

/// Private data structure of the @ref typedef_decl.
struct typedef_decl::priv
{
  type_base_wptr	underlying_type_;
  string		internal_qualified_name_;
  string		temp_internal_qualified_name_;

  priv(const type_base_sptr& t)
    : underlying_type_(t)
  {}
}; // end struct typedef_decl::priv

/// Constructor of the typedef_decl type.
///
/// @param name the name of the typedef.
///
/// @param underlying_type the underlying type of the typedef.
///
/// @param locus the source location of the typedef declaration.
///
/// @param linkage_name the mangled name of the typedef.
///
/// @param vis the visibility of the typedef type.
typedef_decl::typedef_decl(const string&		name,
			   const type_base_sptr	underlying_type,
			   const location&		locus,
			   const std::string&		linkage_name,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, linkage_name, vis),
    priv_(new priv(underlying_type))
{}

/// Return the size of the typedef.
///
/// This function looks at the size of the underlying type and ensures
/// that it's the same as the size of the typedef.
///
/// @return the size of the typedef.
size_t
typedef_decl::get_size_in_bits() const
{
  size_t s = get_underlying_type()->get_size_in_bits();
  if (s != type_base::get_size_in_bits())
    const_cast<typedef_decl*>(this)->set_size_in_bits(s);
  return type_base::get_size_in_bits();
}

/// Return the alignment of the typedef.
///
/// This function looks at the alignment of the underlying type and
/// ensures that it's the same as the alignment of the typedef.
///
/// @return the size of the typedef.
size_t
typedef_decl::get_alignment_in_bits() const
{
    size_t s = get_underlying_type()->get_alignment_in_bits();
  if (s != type_base::get_alignment_in_bits())
    const_cast<typedef_decl*>(this)->set_alignment_in_bits(s);
  return type_base::get_alignment_in_bits();
}

/// Compares two instances of @ref typedef_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const typedef_decl& l, const typedef_decl& r, change_kind* k)
{
  bool result = true;
  if (!l.decl_base::operator==(r))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (*l.get_underlying_type() != *r.get_underlying_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Equality operator
///
/// @param o the other typedef_decl to test against.
bool
typedef_decl::operator==(const decl_base& o) const
{
  const typedef_decl* other = dynamic_cast<const typedef_decl*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

/// Equality operator
///
/// @param o the other typedef_decl to test against.
///
/// @return true if the current instance of @ref typedef_decl equals
/// @p o.
bool
typedef_decl::operator==(const type_base& o) const
{
  const decl_base* other = dynamic_cast<const decl_base*>(&o);
  if (!other)
    return false;
  return *this == *other;
}

/// Build a pretty representation for a typedef_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current
/// instance of typedef_decl.
string
typedef_decl::get_pretty_representation(bool internal) const
{
  string result = "typedef " + get_qualified_name(internal);
  return result;
}

/// Getter of the underlying type of the typedef.
///
/// @return the underlying_type.
type_base_sptr
typedef_decl::get_underlying_type() const
{
  if (priv_->underlying_type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->underlying_type_);
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
typedef_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_underlying_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

typedef_decl::~typedef_decl()
{}
// </typedef_decl definitions>

// <var_decl definitions>

struct var_decl::priv
{
  type_base_wptr	type_;
  decl_base::binding	binding_;
  elf_symbol_sptr	symbol_;
  string		id_;

  priv()
    : binding_(decl_base::BINDING_GLOBAL)
  {}

  priv(type_base_sptr t,
       decl_base::binding b)
    : type_(t),
      binding_(b)
  {}
}; // end struct var_decl::priv

var_decl::var_decl(const std::string&		name,
		   shared_ptr<type_base>	type,
		   const location&		locus,
		   const std::string&		linkage_name,
		   visibility			vis,
		   binding			bind)
  : decl_base(name, locus, linkage_name, vis),
    priv_(new priv(type, bind))
{}

const type_base_sptr
var_decl::get_type() const
{
  if (priv_->type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->type_);
}

decl_base::binding
var_decl::get_binding() const
{return priv_->binding_;}

void
var_decl::set_binding(decl_base::binding b)
{priv_->binding_ = b;}

/// Sets the underlying ELF symbol for the current variable.
///
/// And underlyin$g ELF symbol for the current variable might exist
/// only if the corpus that this variable originates from was
/// constructed from an ELF binary file.
///
/// Note that comparing two variables that have underlying ELF symbols
/// involves comparing their underlying elf symbols.  The decl name
/// for the variable thus becomes irrelevant in the comparison.
///
/// @param sym the new ELF symbol for this variable decl.
void
var_decl::set_symbol(const elf_symbol_sptr& sym)
{priv_->symbol_ = sym;}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using var_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this variable decl, if
/// one exists.
const elf_symbol_sptr&
var_decl::get_symbol() const
{return priv_->symbol_;}

/// Create a new var_decl that is a clone of the current one.
///
/// @return the cloned var_decl.
var_decl_sptr
var_decl::clone() const
{
  var_decl_sptr v(new var_decl(get_name(),
			       get_type(),
			       get_location(),
			       get_linkage_name(),
			       get_visibility(),
			       get_binding()));

  v->set_symbol(get_symbol());

  if (is_member_decl(*this))
    {
      class_decl* scope = dynamic_cast<class_decl*>(get_scope());
      scope->add_data_member(v, get_member_access_specifier(*this),
			     get_data_member_is_laid_out(*this),
			     get_member_is_static(*this),
			     get_data_member_offset(*this));
    }
  else
    add_decl_to_scope(v, get_scope());

  return v;
}
/// Setter of the scope of the current var_decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
///
/// @param scope the new scope.
void
var_decl::set_scope(scope_decl* scope)
{
  if (!get_context_rel())
    {
      context_rel_sptr c(new dm_context_rel(scope));
      set_context_rel(c);
    }
  else
    get_context_rel()->set_scope(scope);
}

/// Compares two instances of @ref var_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const var_decl& l, const var_decl& r, change_kind* k)
{
  bool result = true;
  // If there are underlying elf symbols for these variables,
  // compare them.  And then compare the other parts.
  elf_symbol_sptr s0 = l.get_symbol(), s1 = r.get_symbol();
  if (!!s0 != !!s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }
  else if (s0 && s0 != s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }
  bool symbols_are_equal = (s0 && s1 && result);

  if (symbols_are_equal)
    {
      // The variables have underlying elf symbols that are equal, so
      // now, let's compare the decl_base part of the variables w/o
      // considering their decl names.
      string n1 = l.get_name(), n2 = r.get_name();
      const_cast<var_decl&>(l).set_name("");
      const_cast<var_decl&>(r).set_name("");
      bool decl_bases_different = !l.decl_base::operator==(r);
      const_cast<var_decl&>(l).set_name(n1);
      const_cast<var_decl&>(r).set_name(n2);

      if (decl_bases_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }
  else
    if (!l.decl_base::operator==(r))
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  return false;
      }

  const dm_context_rel* c0 =
    dynamic_cast<const dm_context_rel*>(l.get_context_rel());
  const dm_context_rel* c1 =
    dynamic_cast<const dm_context_rel*>(r.get_context_rel());
  assert(c0 && c1);

  if (*c0 != *c1)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (l.get_type() != r.get_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Comparison operator of @ref var_decl.
///
/// @param o the instance of @ref var_decl to compare against.
///
/// @return true iff the current instance of @ref var_decl equals @p o.
bool
var_decl::operator==(const decl_base& o) const
{
  const var_decl* other = dynamic_cast<const var_decl*>(&o);
  if (!other)
    return false;

  return equals(*this, *other, 0);
}

/// Return an ID that tries to uniquely identify the variable inside a
/// program or a library.
///
/// So if the variable has an underlying elf symbol, the ID is the
/// concatenation of the symbol name and its version.  Otherwise, the
/// ID is the linkage name if its non-null.  Otherwise, it's the
/// pretty representation of the variable.
///
/// @return the ID.
const string&
var_decl::get_id() const
{
  if (priv_->id_.empty())
    {
      if (elf_symbol_sptr s = get_symbol())
	priv_->id_ = s->get_id_string();
      else if (!get_linkage_name().empty())
	priv_->id_ = get_linkage_name();
      else
	priv_->id_ = get_pretty_representation();
    }
  return priv_->id_;
}

/// Return the hash value for the current instance.
///
/// @return the hash value.
size_t
var_decl::get_hash() const
{
  var_decl::hash hash_var;
  return hash_var(this);
}

/// Build and return the pretty representation of this variable.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of this variable.
string
var_decl::get_pretty_representation(bool internal) const
{
  string result;

  if (is_member_decl(this) && get_member_is_static(this))
    result = "static ";
  if (array_type_def_sptr t = is_array_type(get_type()))
    result +=
      get_type_declaration(t->get_element_type())->get_qualified_name(internal)
      + " " + get_qualified_name(internal) + t->get_subrange_representation();
  else
    result += get_type_declaration(get_type())->get_qualified_name(internal)
      + " " + get_qualified_name(internal);
  return result;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
var_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

var_decl::~var_decl()
{}

// </var_decl definitions>

// <function_type>

/// The type of the private data of the @ref function_type type.
struct function_type::priv
{
  parameters parms_;
  type_base_wptr return_type_;

  priv()
  {}

  priv(const parameters&	parms,
       type_base_sptr		return_type)
    : parms_(parms),
      return_type_(return_type)
  {}

  priv(type_base_sptr return_type)
    : return_type_(return_type)
  {}
};// end struc function_type::priv

/// The most straightforward constructor for the function_type class.
///
/// @param return_type the return type of the function type.
///
/// @param parms the list of parameters of the function type.
/// Stricto sensu, we just need a list of types; we are using a list
/// of parameters (where each parameter also carries the name of the
/// parameter and its source location) to try and provide better
/// diagnostics whenever it makes sense.  If it appears that this
/// wasts too many resources, we can fall back to taking just a
/// vector of types here.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
///
/// @param size_in_bits the size of this type.
function_type::function_type(type_base_sptr	return_type,
			     const parameters&	parms,
			     size_t		size_in_bits,
			     size_t		alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    priv_(new priv(parms, return_type))
{
  for (parameters::size_type i = 0, j = 1;
       i < priv_->parms_.size();
       ++i, ++j)
    {
      if (i == 0 && priv_->parms_[i]->get_artificial())
	// If the first parameter is artificial, then it certainly
	// means that this is a member function, and the first
	// parameter is the implicit this pointer.  In that case, set
	// the index of that implicit parameter to zero.  Otherwise,
	// the index of the first parameter starts at one.
	j = 0;
      priv_->parms_[i]->set_index(j);
    }
}

/// A constructor for a function_type that takes no parameters.
///
/// @param return_type the return type of this function_type.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
function_type::function_type(shared_ptr<type_base> return_type,
			     size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    priv_(new priv(return_type))
{}

/// A constructor for a function_type that takes no parameter and
/// that has no return_type yet.  These missing parts can (and must)
/// be added later.
///
/// @param size_in_bits the size of this type, in bits.
///
/// @param alignment_in_bits the alignment of this type, in bits.
function_type::function_type(size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    priv_(new priv)
{}

/// Getter for the return type of the current instance of @ref
/// function_type.
///
/// @return the return type.
type_base_sptr
function_type::get_return_type() const
{
  if (priv_->return_type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->return_type_);
}

/// Setter of the return type of the current instance of @ref
/// function_type.
///
/// @param t the new return type to set.
void
function_type::set_return_type(type_base_sptr t)
{priv_->return_type_ = t;}

/// Getter for the set of parameters of the current intance of @ref
/// function_type.
///
/// @return the parameters of the current instance of @ref
/// function_type.
const function_decl::parameters&
function_type::get_parameters() const
{return priv_->parms_;}

/// Get the Ith parameter of the vector of parameters of the current
/// instance of @ref function_type.
///
/// Note that the first parameter is at index 0.  That parameter is
/// the first parameter that comes after the possible implicit "this"
/// parameter, when the current instance @ref function_type is for a
/// member function.  Otherwise, if the current instance of @ref
/// function_type is for a non-member function, the parameter at index
/// 0 is the first parameter of the function.
///
///
/// @param i the index of the parameter to return.  If i is greater
/// than the index of the last parameter, then this function returns
/// an empty parameter (smart) pointer.
///
/// @return the @p i th parameter that is not implicit.
const function_decl::parameter_sptr
function_type::get_parm_at_index_from_first_non_implicit_parm(size_t i) const
{
  parameter_sptr result;
  if (dynamic_cast<const method_type*>(this))
    {
      if (i + 1 < get_parameters().size())
	result = get_parameters()[i + 1];
    }
  else
    {
      if (i < get_parameters().size())
	result = get_parameters()[i];
    }
  return result;
}

/// Setter for the parameters of the current instance of @ref
/// function_type.
///
/// @param p the new vector of parameters to set.
void
function_type::set_parameters(const parameters &p)
{
  priv_->parms_ = p;
  for (parameters::size_type i = 0, j = 1;
       i < priv_->parms_.size();
       ++i, ++j)
    {
      if (i == 0 && priv_->parms_[i]->get_artificial())
	// If the first parameter is artificial, then it certainly
	// means that this is a member function, and the first
	// parameter is the implicit this pointer.  In that case, set
	// the index of that implicit parameter to zero.  Otherwise,
	// the index of the first parameter starts at one.
	j = 0;
      priv_->parms_[i]->set_index(j);
    }
}

/// Append a new parameter to the vector of parameters of the current
/// instance of @ref function_type.
///
/// @param parm the parameter to append.
void
function_type::append_parameter(parameter_sptr parm)
{
  parm->set_index(priv_->parms_.size());
  priv_->parms_.push_back(parm);
}

/// Test if the current instance of @ref function_type is for a
/// variadic function.
///
/// A variadic function is a function that takes a variable number of
/// arguments.
///
/// @return true iff the current instance of @ref function_type is for
/// a variadic function.
bool
function_type::is_variadic() const
{
  return (!priv_->parms_.empty()
	 && priv_->parms_.back()->get_variadic_marker());
}

/// Compare two function types.
///
/// In case these function types are actually method types, this
/// function avoids comparing two parameters (of the function types)
/// if the types of the parameters are actually the types of the
/// classes of the method types.  This prevents infinite recursion
/// during the comparison of two classes that are structurally
/// identical.
///
/// This is a subroutine of the equality operator of function_type.
///
/// @param lhs the first function type to consider
///
/// @param rhs the second function type to consider
///
/// @param k a pointer to a bitfield set by the function to give
/// information about the kind of changes carried by @p lhs and @p
/// rhs.  It is set iff @p k is non-null and the function returns
/// false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
///@return true if lhs == rhs, false otherwise.
bool
equals(const function_type& lhs,
       const function_type& rhs,
       change_kind* k)
{
  bool result = true;

  if (!lhs.type_base::operator==(rhs))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  class_decl* lhs_class = 0, *rhs_class = 0;
  if (const method_type* m = dynamic_cast<const method_type*>(&lhs))
    lhs_class = m->get_class_type().get();

  if (const method_type* m = dynamic_cast<const method_type*>(&rhs))
    rhs_class = m->get_class_type().get();

  // Compare the names of the class of the method

  if (!!lhs_class != !!rhs_class)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }
  else if (lhs_class
	   && (lhs_class->get_qualified_name()
	       != rhs_class->get_qualified_name()))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  // Then compare the return type; Beware if it's t's a class type
  // that is the same as the method class name; we can recurse for
  // ever in that case.

  decl_base* lhs_return_type_decl =
    get_type_declaration(lhs.get_return_type()).get();
  decl_base* rhs_return_type_decl =
    get_type_declaration(rhs.get_return_type()).get();
  bool compare_result_types = true;
  string lhs_rt_name = lhs_return_type_decl
    ? lhs_return_type_decl->get_qualified_name()
    : "";
  string rhs_rt_name = rhs_return_type_decl
    ? rhs_return_type_decl->get_qualified_name()
    : "";

  if ((lhs_class && (lhs_class->get_qualified_name() == lhs_rt_name))
      ||
      (rhs_class && (rhs_class->get_qualified_name() == rhs_rt_name)))
    compare_result_types = false;

  if (compare_result_types)
    {
      if (lhs.get_return_type() != rhs.get_return_type())
	{
	  result = false;
	  if (k)
	    *k |= SUBTYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }
  else
    if (lhs_rt_name != rhs_rt_name)
      {
	result = false;
	if (k)
	  *k |= SUBTYPE_CHANGE_KIND;
	else
	  return false;
      }

  class_decl* lcl = 0, * rcl = 0;
  vector<shared_ptr<function_decl::parameter> >::const_iterator i,j;
  for (i = lhs.get_first_non_implicit_parm(),
	 j = rhs.get_first_non_implicit_parm();
       (i != lhs.get_parameters().end()
	&& j != rhs.get_parameters().end());
       ++i, ++j)
    {
      if (lhs_class)
	lcl = dynamic_cast<class_decl*>((*i)->get_type().get());
      if (rhs_class)
	rcl = dynamic_cast<class_decl*>((*j)->get_type().get());
      if (lcl && rcl
	  && lcl == lhs_class
	  && rcl == rhs_class)
	// Do not compare the class types of two methods that we are
	// probably comparing atm; otherwise we can recurse indefinitely.
	continue;
      if (**i != **j)
	{
	  result = false;
	  if (k)
	    *k |= SUBTYPE_CHANGE_KIND;
	  else
	    return false;
	}
    }

  if ((i != lhs.get_parameters().end()
       || j != rhs.get_parameters().end()))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

/// Get the parameter of the function.
///
/// If the function is a non-static member function, the parameter
/// returned is the first one following the implicit 'this' parameter.
///
/// @return the first non implicit parm.
function_type::parameters::const_iterator
function_type::get_first_non_implicit_parm() const
{
  if (get_parameters().empty())
    return get_parameters().end();

  bool is_method = dynamic_cast<const method_type*>(this);

  parameters::const_iterator i = get_parameters().begin();

  if (is_method)
    ++i;

  return i;
}

/// Equality operator for function_type.
///
/// @param o the other function_type to compare against.
///
/// @return true iff the two function_type are equal.
bool
function_type::operator==(const type_base& other) const
{
  type_base* canonical_type = get_naked_canonical_type();
  type_base* other_canonical_type = other.get_naked_canonical_type();

  if (canonical_type && other_canonical_type)
    return canonical_type == other_canonical_type;

  const function_type* o = dynamic_cast<const function_type*>(&other);
  if (!o)
    return false;

  return equals(*this, *o, 0);
}

/// Return a copy of the pretty representation of the current @ref
/// function_type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current @ref
/// function_type.
string
function_type::get_pretty_representation(bool internal) const
{return ir::get_pretty_representation(this, internal);}

/// Traverses an instance of @ref function_type, visiting all the
/// sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
function_type::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool keep_going = true;

      if (type_base_sptr t = get_return_type())
	{
	  if (!t->traverse(v))
	    keep_going = false;
	}

      if (keep_going)
	for (parameters::const_iterator i = get_parameters().begin();
	     i != get_parameters().end();
	     ++i)
	  if (type_base_sptr parm_type = (*i)->get_type())
	    if (!parm_type->traverse(v))
	      break;

      visiting(false);
    }
  return v.visit_end(this);
}

function_type::~function_type()
{}
// </function_type>

// <method_type>

/// Constructor for instances of method_type.
///
/// Instances of method_decl must be of type method_type.
///
/// @param return_type the type of the return value of the method.
///
/// @param class_type the base type of the method type.  That is, the
/// type of the class the method belongs to.
///
/// @param parms the vector of the parameters of the method.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.
method_type::method_type
(shared_ptr<type_base> return_type,
 shared_ptr<class_decl> class_type,
 const std::vector<shared_ptr<function_decl::parameter> >& parms,
 size_t size_in_bits,
 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(return_type, parms, size_in_bits, alignment_in_bits)
{set_class_type(class_type);}

/// Constructor of instances of method_type.
///
///Instances of method_decl must be of type method_type.
///
/// @param return_type the type of the return value of the method.
///
/// @param class_type the type of the class the method belongs to.
/// The actual (dynamic) type of class_type must be a pointer
/// class_type.  We are setting it to pointer to type_base here to
/// help client code that is compiled without rtti and thus cannot
/// perform dynamic casts.
///
/// @param parms the vector of the parameters of the method type.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.

method_type::method_type(shared_ptr<type_base> return_type,
			 shared_ptr<type_base> class_type,
			 const std::vector<shared_ptr<function_decl::parameter> >& parms,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(return_type, parms, size_in_bits, alignment_in_bits)
{set_class_type(dynamic_pointer_cast<class_decl>(class_type));}

/// Constructor of the qualified_type_def
///
/// @param size_in_bits the size of the type, expressed in bits.
///
/// @param alignment_in_bits the alignment of the type, expressed in bits
method_type::method_type(size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(size_in_bits, alignment_in_bits)
{}

/// Constructor of instances of method_type.
///
/// When constructed with this constructor, and instane of method_type
/// must set a return type using method_type::set_return_type
///
/// @param class_type the base type of the method type.  That is, the
/// type of the class the method belongs to.
///
/// @param size_in_bits the size of an instance of method_type,
/// expressed in bits.
///
/// @param alignment_in_bits the alignment of an instance of
/// method_type, expressed in bits.
method_type::method_type(shared_ptr<class_decl> class_type,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(size_in_bits, alignment_in_bits)
{set_class_type(class_type);}

/// Sets the class type of the current instance of method_type.
///
/// The class type is the type of the class the method belongs to.
///
/// @param t the new class type to set.
void
method_type::set_class_type(shared_ptr<class_decl> t)
{
  if (!t)
    return;

  class_type_ = t;
}

/// Return a copy of the pretty representation of the current @ref
/// method_type.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the pretty representation of the current @ref
/// method_type.
string
method_type::get_pretty_representation(bool internal) const
{return ir::get_pretty_representation(*this, internal);}

/// The destructor of method_type
method_type::~method_type()
{}

// </method_type>

// <function_decl definitions>

struct function_decl::priv
{
  bool			declared_inline_;
  decl_base::binding	binding_;
  function_type_wptr	type_;
  function_type*	naked_type_;
  elf_symbol_sptr	symbol_;
  string id_;

  priv()
    : declared_inline_(false),
      binding_(decl_base::BINDING_GLOBAL),
      naked_type_()
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t),
      naked_type_(t.get())
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding,
       elf_symbol_sptr s)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t),
      naked_type_(t.get()),
      symbol_(s)
  {}
}; // end sruct function_decl::priv



function_decl::function_decl(const std::string& name,
			     function_type_sptr function_type,
			     bool declared_inline,
			     const location& locus,
			     const std::string& mangled_name,
			     visibility vis,
			     binding bind)
  : decl_base(name, locus, mangled_name, vis),
    priv_(new priv(function_type, declared_inline, bind))
{}

/// Constructor of the function_decl type.
///
/// This flavour of constructor is for when the pointer to the
/// instance of function_type that the client code has is presented as
/// a pointer to type_base.  In that case, this constructor saves the
/// client code from doing a dynamic_cast to get the function_type
/// pointer.
///
/// @param name the name of the function declaration.
///
/// @param fn_type the type of the function declaration.  The dynamic
/// type of this parameter should be 'pointer to function_type'
///
/// @param declared_inline whether this function was declared inline
///
/// @param locus the source location of the function declaration.
///
/// @param linkage_name the mangled name of the function declaration.
///
/// @param vis the visibility of the function declaration.
///
/// @param bind  the kind of the binding of the function
/// declaration.
function_decl::function_decl(const std::string& name,
			     shared_ptr<type_base> fn_type,
			     bool	declared_inline,
			     const location& locus,
			     const std::string& linkage_name,
			     visibility vis,
			     binding bind)
  : decl_base(name, locus, linkage_name, vis),
    priv_(new priv(dynamic_pointer_cast<function_type>(fn_type),
		   declared_inline,
		   bind))
{}

/// Get the pretty representation of the current instance of @ref function_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representation for a function.
string
function_decl::get_pretty_representation(bool internal) const
{
  const class_decl::method_decl* mem_fn =
    dynamic_cast<const class_decl::method_decl*>(this);

  string result = mem_fn ? "method ": "function ";

  if (mem_fn
      && is_member_function(mem_fn)
      && get_member_function_is_virtual(mem_fn))
    result += "virtual ";

  decl_base_sptr type;
  if ((mem_fn
       && is_member_function(mem_fn)
       && (get_member_function_is_dtor(*mem_fn)
	   || get_member_function_is_ctor(*mem_fn))))
    /*cdtors do not have return types.  */;
  else
    type = mem_fn
      ? get_type_declaration(mem_fn->get_type()->get_return_type())
      : get_type_declaration(get_type()->get_return_type());

  if (type)
    result += type->get_qualified_name(internal) + " ";

  result += get_pretty_representation_of_declarator();

  return result;
}

/// Compute and return the pretty representation for the part of the
/// function declaration that starts at the declarator.  That is, the
/// return type and the other specifiers of the beginning of the
/// function's declaration ar omitted.
///
/// @return the pretty representation for the part of the function
/// declaration that starts at the declarator.
string
function_decl::get_pretty_representation_of_declarator () const
{
  const class_decl::method_decl* mem_fn =
    dynamic_cast<const class_decl::method_decl*>(this);

  string result;

  if (mem_fn)
    {
      result += mem_fn->get_type()->get_class_type()->get_qualified_name()
	+ "::" + mem_fn->get_name();
    }
  else
    result += get_qualified_name();

  result += "(";

  parameters::const_iterator i = get_parameters().begin(),
    end = get_parameters().end();

  // Skip the first parameter if this is a method.
  if (mem_fn && i != end)
    ++i;
  parameter_sptr parm;
  parameter_sptr first_parm;
  if (i != end)
    first_parm = *i;
  for (; i != end; ++i)
    {
      parm = *i;
      if (parm.get() != first_parm.get())
	result += ", ";
      if (parm->get_variadic_marker())
	result += "...";
      else
	{
	  decl_base_sptr type_decl = get_type_declaration(parm->get_type());
	  result += type_decl->get_qualified_name();
	}
    }
  result += ")";

  if (mem_fn
      && is_member_function(mem_fn)
      && get_member_function_is_const(*mem_fn))
    result += " const";

  return result;
}

/// Getter for the first non-implicit parameter of a function decl.
///
/// If the function is a non-static member function, the parameter
/// returned is the first one following the implicit 'this' parameter.
///
/// @return the first non implicit parm.
function_decl::parameters::const_iterator
function_decl::get_first_non_implicit_parm() const
{
  if (get_parameters().empty())
    return get_parameters().end();

  bool is_method = dynamic_cast<const class_decl::method_decl*>(this);

  parameters::const_iterator i = get_parameters().begin();
  if (is_method)
    ++i;

  return i;
}

/// Return the type of the current instance of @ref function_decl.
///
/// It's either a function_type or method_type.
/// @return the type of the current instance of @ref function_decl.
const shared_ptr<function_type>
function_decl::get_type() const
{
  if (priv_->type_.expired())
    return function_type_sptr();
  return function_type_sptr(priv_->type_);
}

/// Fast getter of the type of the current instance of @ref function_decl.
///
/// Note that this function returns the underlying pointer managed by
/// the smart pointer returned by function_decl::get_type().  It's
/// faster than function_decl::get_type().  This getter is to be used
/// in code paths that are proven to be performance hot spots;
/// especially (for instance) when comparing function types.  Those
/// are compared extremely frequently when libabigail is used to
/// handle huge binaries with a lot of functions.
///
/// @return the type of the current instance of @ref function_decl.
const function_type*
function_decl::get_naked_type() const
{return priv_->naked_type_;}

void
function_decl::set_type(const function_type_sptr& fn_type)
{
  priv_->type_ = fn_type;
  priv_->naked_type_ = fn_type.get();
}

/// This sets the underlying ELF symbol for the current function decl.
///
/// And underlyin$g ELF symbol for the current function decl might
/// exist only if the corpus that this function decl originates from
/// was constructed from an ELF binary file.
///
/// Note that comparing two function decls that have underlying ELF
/// symbols involves comparing their underlying elf symbols.  The decl
/// name for the function thus becomes irrelevant in the comparison.
///
/// @param sym the new ELF symbol for this function decl.
void
function_decl::set_symbol(const elf_symbol_sptr& sym)
{priv_->symbol_ = sym;}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using function_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this function decl, if
/// one exists.
const elf_symbol_sptr&
function_decl::get_symbol() const
{return priv_->symbol_;}

bool
function_decl::is_declared_inline() const
{return priv_->declared_inline_;}

decl_base::binding
function_decl::get_binding() const
{return priv_->binding_;}

/// @return the return type of the current instance of function_decl.
const shared_ptr<type_base>
function_decl::get_return_type() const
{return get_type()->get_return_type();}

/// @return the parameters of the function.
const std::vector<shared_ptr<function_decl::parameter> >&
function_decl::get_parameters() const
{return get_type()->get_parameters();}

/// Append a parameter to the type of this function.
///
/// @param parm the parameter to append.
void
function_decl::append_parameter(shared_ptr<parameter> parm)
{get_type()->append_parameter(parm);}

/// Append a vector of parameters to the type of this function.
///
/// @param parms the vector of parameters to append.
void
function_decl::append_parameters(std::vector<shared_ptr<parameter> >& parms)
{
  for (std::vector<shared_ptr<parameter> >::const_iterator i = parms.begin();
       i != parms.end();
       ++i)
    get_type()->append_parameter(*i);
}

/// Create a new instance of function_decl that is a clone of the
/// current one.
///
/// @return the new clone.
function_decl_sptr
function_decl::clone() const
{
  function_decl_sptr f;
  if (is_member_function(*this))
    {
      class_decl::method_decl_sptr
	m(new class_decl::method_decl(get_name(),
				      get_type(),
				      is_declared_inline(),
				      get_location(),
				      get_linkage_name(),
				      get_visibility(),
				      get_binding()));
      class_decl* scope = dynamic_cast<class_decl*>(get_scope());
      assert(scope);
      scope->add_member_function(m, get_member_access_specifier(*this),
				 get_member_function_is_virtual(*this),
				 get_member_function_vtable_offset(*this),
				 get_member_is_static(*this),
				 get_member_function_is_ctor(*this),
				 get_member_function_is_dtor(*this),
				 get_member_function_is_const(*this));
      f = m;
    }
  else
    {
      f.reset(new function_decl(get_name(),
				get_type(),
				is_declared_inline(),
				get_location(),
				get_linkage_name(),
				get_visibility(),
				get_binding()));
      add_decl_to_scope(f, get_scope());
    }
  f->set_symbol(get_symbol());

  return f;
}

/// Compares two instances of @ref function_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const function_decl& l, const function_decl& r, change_kind* k)
{
  bool result = true;

  // Compare function types
  const type_base* t0 = l.get_naked_type(), *t1 = r.get_naked_type();
  if (t0 == t1 || *t0 == *t1)
    ; // the types are equal, let's move on to compare the other
      // properties of the functions.
  else
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  const elf_symbol_sptr &s0 = l.get_symbol(), &s1 = r.get_symbol();
  if (!!s0 != !!s1)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }
  else if (s0 && s0 != s1)
    {
      if (!elf_symbols_alias(s0, s1))
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }
  bool symbols_are_equal = (s0 && s1 && result);

  if (symbols_are_equal)
    {
      // The functions have underlying elf symbols that are equal,
      // so now, let's compare the decl_base part of the functions
      // w/o considering their decl names.
      string n1 = l.get_name(), n2 = r.get_name();
      string ln1 = l.get_linkage_name(), ln2 = r.get_linkage_name();
      const_cast<function_decl&>(l).set_name("");
      const_cast<function_decl&>(l).set_linkage_name("");
      const_cast<function_decl&>(r).set_name("");
      const_cast<function_decl&>(r).set_linkage_name("");

      bool decl_bases_different = !l.decl_base::operator==(r);

      const_cast<function_decl&>(l).set_name(n1);
      const_cast<function_decl&>(l).set_linkage_name(ln1);
      const_cast<function_decl&>(r).set_name(n2);
      const_cast<function_decl&>(r).set_linkage_name(ln2);

      if (decl_bases_different)
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }
  else
    if (!l.decl_base::operator==(r))
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  return false;
      }

  // Compare the remaining properties
  if (l.is_declared_inline() != r.is_declared_inline()
      || l.get_binding() != r.get_binding())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_function(l) != is_member_function(r))
    {
      result = false;
      if (k)
	  *k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_function(l) && is_member_function(r))
    {
      if (!((get_member_function_is_ctor(l)
	     == get_member_function_is_ctor(r))
	    && (get_member_function_is_dtor(l)
		== get_member_function_is_dtor(r))
	    && (get_member_is_static(l)
		== get_member_is_static(r))
	    && (get_member_function_is_const(l)
		== get_member_function_is_const(r))
	    && (get_member_function_is_virtual(l)
		== get_member_function_is_virtual(r))
	    && (get_member_function_vtable_offset(l)
		== get_member_function_vtable_offset(r))))
	{
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }

  return result;
}

/// Comparison operator for @ref function_decl.
///
/// @param other the other instance of @ref function_decl to compare
/// against.
///
/// @return true iff the current instance of @ref function_decl equals
/// @p other.
bool
function_decl::operator==(const decl_base& other) const
{
  const function_decl* o = dynamic_cast<const function_decl*>(&other);
  if (!o)
    return false;
  return equals(*this, *o, 0);
}

/// Return true iff the function takes a variable number of
/// parameters.
///
/// @return true if the function taks a variable number
/// of parameters.
bool
function_decl::is_variadic() const
{
  return (!get_parameters().empty()
	  && get_parameters().back()->get_variadic_marker());
}

/// The virtual implementation of 'get_hash' for a function_decl.
///
/// This allows decl_base::get_hash to work for function_decls.
///
/// @return the hash value for function decl.
size_t
function_decl::get_hash() const
{
  function_decl::hash hash_fn;
  return hash_fn(*this);
}

/// Return an ID that tries to uniquely identify the function inside a
/// program or a library.
///
/// So if the function has an underlying elf symbol, the ID is the
/// concatenation of the symbol name and its version.  Otherwise, the
/// ID is the linkage name if its non-null.  Otherwise, it's the
/// pretty representation of the function.
///
/// @return the ID.
const string&
function_decl::get_id() const
{
  if (priv_->id_.empty())
    {
      if (elf_symbol_sptr s = get_symbol())
	priv_->id_ = s->get_id_string();
      else if (!get_linkage_name().empty())
	priv_->id_= get_linkage_name();
      else
	priv_->id_ = get_pretty_representation();
    }
  return priv_->id_;
}

/// Test if two function declarations are aliases.
///
/// Two functions declarations are aliases if their symbols are
/// aliases, in the ELF sense.
///
/// @param f1 the first function to consider.
///
/// @param f2 the second function to consider.
///
/// @return true iff @p f1 is an alias of @p f2
bool
function_decls_alias(const function_decl& f1, const function_decl& f2)
{
  elf_symbol_sptr s1 = f1.get_symbol(), s2 = f2.get_symbol();

  if (!s1 || !s2)
    return false;

  return elf_symbols_alias(s1, s2);
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
function_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Destructor of the @ref function_decl type.
function_decl::~function_decl()
{delete priv_;}

// <function_decl definitions>

// <function_decl::parameter definitions>

struct function_decl::parameter::priv
{
  type_base_wptr	type_;
  unsigned		index_;
  bool			variadic_marker_;
  bool			artificial_;

  priv()
    : index_(),
      variadic_marker_(),
      artificial_()
  {}

  priv(type_base_sptr type,
       unsigned index,
       bool variadic_marker,
       bool artificial)
    : type_(type),
      index_(index),
      variadic_marker_(variadic_marker),
      artificial_(artificial)
  {}
};// end struct function_decl::parameter::priv

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    const std::string&		name,
				    const location&		loc,
				    bool			is_variadic)
  : decl_base(name, loc),
    priv_(new priv(type, index, is_variadic, /*is_artificial=*/false))
{}

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    const std::string&		name,
				    const location&		loc,
				    bool			is_variadic,
				    bool			is_artificial)
  : decl_base(name, loc),
    priv_(new priv(type, index, is_variadic, is_artificial))
{}

function_decl::parameter::parameter(const type_base_sptr	type,
				    const std::string&		name,
				    const location&		loc,
				    bool			is_variadic,
				    bool			is_artificial)
  : decl_base(name, loc),
    priv_(new priv(type, 0, is_variadic, is_artificial))
{}

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    bool			variad)
  : decl_base("", location()),
    priv_(new priv(type, index, variad, /*is_artificial=*/false))
{}

const type_base_sptr
function_decl::parameter::get_type()const
{
  if (priv_->type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->type_);
}

/// @return a copy of the type name of the parameter.
const string
function_decl::parameter::get_type_name() const
{
  string str;
  if (get_variadic_marker())
    str = "...";
  else
    {
	type_base_sptr t = get_type();
	assert(t);
	str += abigail::ir::get_type_name(t);
    }
  return str;
}

/// @return a copy of the pretty representation of the type of the
/// parameter.
const string
function_decl::parameter::get_type_pretty_representation() const
{
  string str;
  if (get_variadic_marker())
    str = "...";
  else
    {
	type_base_sptr t = get_type();
	assert(t);
	str += get_type_declaration(t)->get_pretty_representation();
    }
  return str;
}

/// Get a name uniquely identifying the parameter in the function.
///
///@return the unique parm name id.
const string
function_decl::parameter::get_name_id() const
{
  std::ostringstream o;
  o << "parameter-" << get_index();
  return o.str();
}

unsigned
function_decl::parameter::get_index() const
{return priv_->index_;}

void
function_decl::parameter::set_index(unsigned i)
{priv_->index_ = i;}

/// Test if the parameter is artificial.
///
/// Being artificial means the parameter was not explicitely
/// mentionned in the source code, but was rather artificially
/// created by the compiler.
///
/// @return true if the parameter is artificial, false otherwise.
bool
function_decl::parameter::get_artificial() const
{return priv_->artificial_;}

bool
function_decl::parameter::get_variadic_marker() const
{return priv_->variadic_marker_;}

/// Getter for the artificial-ness of the parameter.
///
/// Being artificial means the parameter was not explicitely
/// mentionned in the source code, but was rather artificially
/// created by the compiler.
///
/// @param f set to true if the parameter is artificial.
void
function_decl::parameter::set_artificial(bool f)
{priv_->artificial_ = f;}

/// Compares two instances of @ref function_decl::parameter.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const function_decl::parameter& l,
       const function_decl::parameter& r,
       change_kind* k)
{
  bool result = true;

  if ((l.get_variadic_marker() != r.get_variadic_marker())
      || (l.get_index() != r.get_index())
      || (!!l.get_type() != !!r.get_type()))
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (l.get_type() != r.get_type())
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
      else
	return false;
    }

  return result;
}

bool
function_decl::parameter::operator==(const parameter& o) const
{return equals(*this, o, 0);}

bool
function_decl::parameter::operator==(const decl_base& o) const
{
  const function_decl::parameter* p =
    dynamic_cast<const function_decl::parameter*>(&o);
  if (!p)
    return false;
  return function_decl::parameter::operator==(*p);
}

/// Non-member equality operator for @ref function_decl::parameter.
///
/// @param l the left-hand side of the equality operator
///
/// @param r the right-hand side of the equality operator
///
/// @return true iff @p l and @p r equals.
bool
operator==(const function_decl::parameter_sptr& l,
	   const function_decl::parameter_sptr& r)
{
  if (!!l != !!r)
    return false;
  if (!l)
    return true;
  return *l == *r;
}

/// Traverse the diff sub-tree under the current instance
/// function_decl.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going on, false
/// otherwise.
bool
function_decl::parameter::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (type_base_sptr t = get_type())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Get the hash of a decl.  If the hash hasn't been computed yet,
/// compute it ans store its value; otherwise, just return the hash.
///
/// @return the hash of the decl.
size_t
function_decl::parameter::get_hash() const
{
  function_decl::parameter::hash hash_fn_parm;
  return hash_fn_parm(this);
}

/// Compute the qualified name of the parameter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @param qn the resulting qualified name.
void
function_decl::parameter::get_qualified_name(string& qualified_name,
					     bool /*internal*/) const
{qualified_name = get_name();}

/// Compute and return a copy of the pretty representation of the
/// current function parameter.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return a copy of the textual representation of the current
/// function parameter.
string
function_decl::parameter::get_pretty_representation(bool internal) const
{
  const environment* env = get_environment();

  string type_repr;
  type_base_sptr t = get_type();
  if (!t)
    type_repr = "void";
  else if (env
	   && (t ==
	       dynamic_pointer_cast<type_base>
	       (env->get_variadic_parameter_type_decl())))
    type_repr = "...";
  else
    type_repr = ir::get_pretty_representation(t, internal);

  string result = type_repr;
  string parm_name = get_name_id();

  if (!parm_name.empty())
    result += " " + parm_name;

  return result;
}

// </function_decl::parameter definitions>

// <class_decl definitions>

static void
sort_virtual_member_functions(class_decl::member_functions& mem_fns);

/// The private data for the class_decl type.
struct class_decl::priv
{
  bool				is_declaration_only_;
  bool				is_struct_;
  decl_base_sptr		declaration_;
  class_decl_sptr		definition_of_declaration_;
  base_specs			bases_;
  member_types			member_types_;
  data_members			data_members_;
  data_members			non_static_data_members_;
  member_functions		member_functions_;
  member_functions		virtual_mem_fns_;
  member_function_templates	member_function_templates_;
  member_class_templates	member_class_templates_;

  priv()
    : is_declaration_only_(false),
      is_struct_(false)
  {}

  priv(bool is_struct, class_decl::base_specs& bases,
       class_decl::member_types& mbr_types,
       class_decl::data_members& data_mbrs,
       class_decl::member_functions& mbr_fns)
    : is_declaration_only_(false),
      is_struct_(is_struct),
      bases_(bases),
      member_types_(mbr_types),
      data_members_(data_mbrs),
      member_functions_(mbr_fns)
  {
    for (data_members::const_iterator i = data_members_.begin();
	 i != data_members_.end();
	 ++i)
      if (!get_member_is_static(*i))
	non_static_data_members_.push_back(*i);
  }

  priv(bool is_struct)
    : is_declaration_only_(false),
      is_struct_(is_struct)
  {}

  priv(bool is_declaration_only, bool is_struct)
    : is_declaration_only_(is_declaration_only),
      is_struct_(is_struct)
  {}

  /// Mark a class as being currently compared using the class_decl==
  /// operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class. If via the comparison of a data member or a
  /// member function a recursive re-comparison of the class is
  /// attempted, the marking business help to detect that infinite
  /// loop possibility and avoid it.
  ///
  /// @param klass the class to mark as being currently compared.
  void
  mark_as_being_compared(const class_decl& klass) const
  {
    const environment* env = klass.get_environment();
    assert(env);
    env->priv_->classes_being_compared_[klass.get_qualified_name()] = true;
  }

  /// Mark a class as being currently compared using the class_decl==
  /// operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class. If via the comparison of a data member or a
  /// member function a recursive re-comparison of the class is
  /// attempted, the marking business help to detect that infinite
  /// loop possibility and avoid it.
  ///
  /// @param klass the class to mark as being currently compared.
  void
  mark_as_being_compared(const class_decl* klass) const
  {mark_as_being_compared(*klass);}

  /// Mark a class as being currently compared using the class_decl==
  /// operator.
  ///
  /// Note that is marking business is to avoid infinite loop when
  /// comparing a class. If via the comparison of a data member or a
  /// member function a recursive re-comparison of the class is
  /// attempted, the marking business help to detect that infinite
  /// loop possibility and avoid it.
  ///
  /// @param klass the class to mark as being currently compared.
  void
  mark_as_being_compared(const class_decl_sptr& klass) const
  {mark_as_being_compared(*klass);}

  /// If the instance of class_decl has been previously marked as
  /// being compared -- via an invocation of mark_as_being_compared()
  /// this method unmarks it.  Otherwise is has no effect.
  ///
  /// This method is not thread safe because it uses the static data
  /// member classes_being_compared_.  If you wish to use it in a
  /// multi-threaded environment you should probably protect the
  /// access to that static data member with a mutex or somesuch.
  ///
  /// @param klass the instance of class_decl to unmark.
  void
  unmark_as_being_compared(const class_decl& klass) const
  {
    const environment* env = klass.get_environment();
    assert(env);
    env->priv_->classes_being_compared_.erase(klass.get_qualified_name());
  }

  /// If the instance of class_decl has been previously marked as
  /// being compared -- via an invocation of mark_as_being_compared()
  /// this method unmarks it.  Otherwise is has no effect.
  ///
  /// @param klass the instance of class_decl to unmark.
  void
  unmark_as_being_compared(const class_decl* klass) const
  {
    const environment* env = klass->get_environment();
    assert(env);
    env->priv_->classes_being_compared_.erase(klass->get_qualified_name());
  }

  /// Test if a given instance of class_decl is being currently
  /// compared.
  ///
  ///@param klass the class to test.
  ///
  /// @return true if @p klass is being compared, false otherwise.
  bool
  comparison_started(const class_decl& klass) const
  {
    const environment* env = klass.get_environment();
    assert(env);
    unordered_map<string, bool>& c = env->priv_->classes_being_compared_;
    return (c.find(klass.get_qualified_name()) != c.end());
  }

  /// Test if a given instance of class_decl is being currently
  /// compared.
  ///
  ///@param klass the class to test.
  ///
  /// @return true if @p klass is being compared, false otherwise.
  bool
  comparison_started(const class_decl* klass) const
  {return comparison_started(*klass);}
};// end struct class_decl::priv

/// A Constructor for instances of \ref class_decl
///
/// @param name the identifier of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
///
/// @param bases the vector of base classes for this instance of class_decl.
///
/// @param mbrs the vector of member types of this instance of
/// class_decl.
///
/// @param data_mbrs the vector of data members of this instance of
/// class_decl.
///
/// @param mbr_fns the vector of member functions of this instance of
/// class_decl.
class_decl::class_decl(const std::string& name, size_t size_in_bits,
		       size_t align_in_bits, bool is_struct,
		       const location& locus, visibility vis,
		       base_specs& bases, member_types& mbr_types,
		       data_members& data_mbrs,
		       member_functions& mbr_fns)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
    scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
    priv_(new priv(is_struct, bases, mbr_types, data_mbrs, mbr_fns))
{
  for (member_types::iterator i = mbr_types.begin(); i != mbr_types.end(); ++i)
    if (!has_scope(get_type_declaration(*i)))
      add_decl_to_scope(get_type_declaration(*i), this);

  for (data_members::iterator i = data_mbrs.begin(); i != data_mbrs.end();
       ++i)
    if (!has_scope(*i))
      add_decl_to_scope(*i, this);

  for (member_functions::iterator i = mbr_fns.begin(); i != mbr_fns.end();
       ++i)
    if (!has_scope(static_pointer_cast<decl_base>(*i)))
      add_decl_to_scope(*i, this);

}

/// A constructor for instances of class_decl.
///
/// @param name the name of the class.
///
/// @param size_in_bits the size of an instance of class_decl, expressed
/// in bits
///
/// @param align_in_bits the alignment of an instance of class_decl,
/// expressed in bits.
///
/// @param locus the source location of declaration point this class.
///
/// @param vis the visibility of instances of class_decl.
class_decl::class_decl(const std::string& name, size_t size_in_bits,
		       size_t align_in_bits, bool is_struct,
		       const location& locus, visibility vis)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
    scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
    priv_(new priv(is_struct))
{}

/// A constuctor for instances of class_decl that represent a
/// declaration without definition.
///
/// @param name the name of the class.
///
/// @param is_declaration_only a boolean saying whether the instance
/// represents a declaration only, or not.
class_decl::class_decl(const std::string& name,
		       bool is_struct,
		       bool is_declaration_only)
  : decl_base(name, location(), name),
    type_base(0, 0),
    scope_type_decl(name, 0, 0, location()),
    priv_(new priv(is_declaration_only, is_struct))
{}

/// Setter of the size of the class type.
///
/// If this class is a declaration of a definition that is elsewhere,
/// then the new size is set to the definition.
///
/// @param s the new size.
void
class_decl::set_size_in_bits(size_t s)
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    get_definition_of_declaration()->set_size_in_bits(s);
  else
    type_base::set_size_in_bits(s);
}

/// Getter of the size of the class type.
///
/// If this class is a declaration of a definition that is elsewhere,
/// then the size of the definition is returned.
///
/// @return the size of the class type.
size_t
class_decl::get_size_in_bits() const
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    return get_definition_of_declaration()->get_size_in_bits();

  return type_base::get_size_in_bits();
}

/// Getter of the alignment of the class type.
///
/// If this class is a declaration of a definition that is elsewhere,
/// then the size of the definition is returned.
///
/// @return the alignment of the class type.
size_t
class_decl::get_alignment_in_bits() const
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    return get_definition_of_declaration()->get_alignment_in_bits();

   return type_base::get_alignment_in_bits();
}

/// Setter of the alignment of the class type.
///
/// If this class is a declaration of a definition that is elsewhere,
/// then the new alignment is set to the definition.
///
/// @param s the new alignment.
void
class_decl::set_alignment_in_bits(size_t a)
{
  if (get_is_declaration_only() && get_definition_of_declaration())
    get_definition_of_declaration()->set_alignment_in_bits(a);
  else
    type_base::set_alignment_in_bits(a);
}

/// Test if a class is a declaration-only class.
///
/// @return true iff the current class is a declaration-only class.
bool
class_decl::get_is_declaration_only() const
{return priv_->is_declaration_only_;}

/// Set a flag saying if the class is a declaration-only class.
///
/// @param f true if the class is a decalaration-only class.
void
class_decl::set_is_declaration_only(bool f)
{
  priv_->is_declaration_only_ = f;
  if (!f)
    if (scope_decl* s = get_scope())
      {
	declarations::iterator i;
	if (s->find_iterator_for_member(this, i))
	  maybe_update_types_lookup_map(s, *i);
	else
	  abort();
      }
}

/// Set the "is-struct" flag of the class.
///
/// @param f the new value of the flag.
void
class_decl::is_struct(bool f)
{priv_->is_struct_ = f;}

/// Test if the class is a struct.
///
/// @return true iff the class is a struct.
bool
class_decl::is_struct() const
{return priv_->is_struct_;}

/// If this class is declaration-only, get its definition, if any.
///
/// @return the definition of this decl-only class.
const class_decl_sptr&
class_decl::get_definition_of_declaration() const
{return priv_->definition_of_declaration_;}

/// If this class is a definitin, get its earlier declaration.
///
/// @return the earlier declaration of the class, if any.
decl_base_sptr
class_decl::get_earlier_declaration() const
{return priv_->declaration_;}

/// Add a base specifier to this class.
///
/// @param b the new base specifier.
void
class_decl::add_base_specifier(base_spec_sptr b)
{
  priv_->bases_.push_back(b);
  assert(!b->get_environment());
  if (environment* env = get_environment())
    b->set_environment(env);
}

/// Get the base specifiers for this class.
///
/// @return a vector of the base specifiers.
const class_decl::base_specs&
class_decl::get_base_specifiers() const
{return priv_->bases_;}

/// Find a base class of a given qualified name for the current class.
///
/// @param qualified_name the qualified name of the base class to look for.
///
/// @return a pointer to the @ref class_decl that represents the base
/// class of name @p qualified_name, if found.
class_decl_sptr
class_decl::find_base_class(const string& qualified_name) const
{
  for (base_specs::const_iterator i = get_base_specifiers().begin();
       i != get_base_specifiers().end();
       ++i)
    if ((*i)->get_base_class()->get_qualified_name()
	== qualified_name)
      return (*i)->get_base_class();

  return class_decl_sptr();
}

/// Get the member types of this class.
///
/// @return a vector of the member types of this class.
const class_decl::member_types&
class_decl::get_member_types() const
{return priv_->member_types_;}

/// Find a member type of a given name, inside the current class @ref
/// class_decl.
///
/// @param name the name of the member type to look for.
///
/// @return a pointer to the @ref type_base that represents the member
/// type of name @p name, for the current class.
type_base_sptr
class_decl::find_member_type(const string& name) const
{
  for (member_types::const_iterator i = get_member_types().begin();
       i != get_member_types().end();
       ++i)
    if (get_type_name(*i, /*qualified*/false) == name)
      return *i;
  return type_base_sptr();
}

/// Get the data members of this class.
///
/// @return a vector of the data members of this class.
const class_decl::data_members&
class_decl::get_data_members() const
{return priv_->data_members_;}

/// Find a data member of a given name in the current class.
///
/// @param name the name of the data member to find in the current class.
///
/// @return a pointer to the @ref var_decl that represents the data
/// member to find inside the current class.
const var_decl_sptr
class_decl::find_data_member(const string& name) const
{
  for (data_members::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    if ((*i)->get_name() == name)
      return *i;
  return var_decl_sptr();
}

/// Get the non-static data memebers of this class.
///
/// @return a vector of the non-static data members of this class.
const class_decl::data_members&
class_decl::get_non_static_data_members() const
{return priv_->non_static_data_members_;}

/// Get the member functions of this class.
///
/// @return a vector of the member functions of this class.
const class_decl::member_functions&
class_decl::get_member_functions() const
{return priv_->member_functions_;}

/// Get the virtual member functions of this class.
///
/// @param return a vector of the virtual member functions of this
/// class.
const class_decl::member_functions&
class_decl::get_virtual_mem_fns() const
{return priv_->virtual_mem_fns_;}

void
class_decl::sort_virtual_mem_fns()
{sort_virtual_member_functions(priv_->virtual_mem_fns_);}

/// Get the member function templates of this class.
///
/// @return a vector of the member function templates of this class.
const class_decl::member_function_templates&
class_decl::get_member_function_templates() const
{return priv_->member_function_templates_;}

/// Get the member class templates of this class.
///
/// @return a vector of the member class templates of this class.
const class_decl::member_class_templates&
class_decl::get_member_class_templates() const
{return priv_->member_class_templates_;}

/// Getter of the pretty representation of the current instance of
/// @ref class_decl.
///
/// @param internal set to true if the call is intended for an
/// internal use (for technical use inside the library itself), false
/// otherwise.  If you don't know what this is for, then set it to
/// false.
///
/// @return the pretty representaion for a class_decl.
string
class_decl::get_pretty_representation(bool internal) const
{
  string cl = "class ";
  if (!internal && is_struct())
    cl = "struct ";
  return cl + get_qualified_name(internal);
}

/// Set the definition of this declaration-only class.
///
/// @param d the new definition to set.
void
class_decl::set_definition_of_declaration(class_decl_sptr d)
{
  assert(get_is_declaration_only());
  priv_->definition_of_declaration_ = d;
  if (d->get_canonical_type())
    type_base::priv_->canonical_type = d->get_canonical_type();
}

/// set the earlier declaration of this class definition.
///
/// @param declaration the earlier declaration to set.  Note that it's
/// set only if it's a pure declaration.
void
class_decl::set_earlier_declaration(decl_base_sptr declaration)
{
  class_decl_sptr cl = dynamic_pointer_cast<class_decl>(declaration);
  if (cl && cl->get_is_declaration_only())
    priv_->declaration_ = declaration;
}

decl_base_sptr
class_decl::insert_member_decl(decl_base_sptr d,
			       declarations::iterator before)
{
  if (type_base_sptr t = dynamic_pointer_cast<type_base>(d))
    insert_member_type(t, before);
  else if (var_decl_sptr v = dynamic_pointer_cast<var_decl>(d))
    {
      add_data_member(v, public_access,
		      /*is_laid_out=*/false,
		      /*is_static=*/true,
		      /*offset_in_bits=*/0);
      d = v;
    }
  else if (method_decl_sptr f = dynamic_pointer_cast<method_decl>(d))
    add_member_function(f, public_access,
			/*is_virtual=*/false,
			/*vtable_offset=*/0,
			/*is_static=*/false,
			/*is_ctor=*/false,
			/*is_dtor=*/false,
			/*is_const=*/false);
  else if (member_function_template_sptr f =
	   dynamic_pointer_cast<member_function_template>(d))
    add_member_function_template(f);
  else if (member_class_template_sptr c =
	   dynamic_pointer_cast<member_class_template>(d))
    add_member_class_template(c);
  else
    scope_decl::add_member_decl(d);

  return d;
}

/// Add a member declaration to the current instance of class_decl.
/// The member declaration can be either a member type, data member,
/// member function, or member template.
///
/// @param d the member declaration to add.
decl_base_sptr
class_decl::add_member_decl(decl_base_sptr d)
{return insert_member_decl(d, get_member_decls().end());}

/// Remove a given decl from the current class scope.
///
/// Note that only type declarations are supported by this method for
/// now.  Support for the other kinds of declaration is left as an
/// exercise for the interested reader of the code.
///
/// @param decl the declaration to remove from this class scope.
void
class_decl::remove_member_decl(decl_base_sptr decl)
{
  type_base_sptr t = is_type(decl);

  // For now we want to support just removing types from classes.  For
  // other kinds of IR node, we need more work.
  assert(t);

  remove_member_type(t);
}

void
class_decl::insert_member_type(type_base_sptr t,
			       declarations::iterator before)
{
  decl_base_sptr d = get_type_declaration(t);
  assert(d);
  assert(!has_scope(d));

  priv_->member_types_.push_back(t);
  scope_decl::insert_member_decl(d, before);
}

/// Add a member type to the current instance of class_decl.
///
/// @param t the member type to add.  It must not have been added to a
/// scope, otherwise this will violate an assertion.
void
class_decl::add_member_type(type_base_sptr t)
{insert_member_type(t, get_member_decls().end());}

/// Add a member type to the current instance of class_decl.
///
/// @param t the type to be added as a member type to the current
/// instance of class_decl.  An instance of class_decl::member_type
/// will be created out of @p t and and added to the the class.
///
/// @param a the access specifier for the member type to be created.
type_base_sptr
class_decl::add_member_type(type_base_sptr t, access_specifier a)
{
  decl_base_sptr d = get_type_declaration(t);
  assert(d);
  assert(!is_member_decl(d));
  add_member_type(t);
  set_member_access_specifier(d, a);
  return t;
}

/// Remove a member type from the current class scope.
///
/// @param t the type to remove.
void
class_decl::remove_member_type(type_base_sptr t)
{
  for (member_types::iterator i = priv_->member_types_.begin();
       i != priv_->member_types_.end();
       ++i)
    {
      if (*((*i)) == *t)
	{
	  priv_->member_types_.erase(i);
	  return;
	}
    }
}

/// The private data structure of class_decl::base_spec.
struct class_decl::base_spec::priv
{
  class_decl_wptr	base_class_;
  long			offset_in_bits_;
  bool			is_virtual_;

  priv(const class_decl_sptr& cl,
       long offset_in_bits,
       bool is_virtual)
    : base_class_(cl),
      offset_in_bits_(offset_in_bits),
      is_virtual_(is_virtual)
  {}
};

/// Constructor for base_spec instances.
///
/// @param base the base class to consider
///
/// @param a the access specifier of the base class.
///
/// @param offset_in_bits if positive or null, represents the offset
/// of the base in the layout of its containing type..  If negative,
/// means that the current base is not laid out in its containing type.
///
/// @param is_virtual if true, means that the current base class is
/// virtual in it's containing type.
class_decl::base_spec::base_spec(shared_ptr<class_decl> base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : decl_base(base->get_name(), base->get_location(),
	      base->get_linkage_name(), base->get_visibility()),
    member_base(a),
    priv_(new priv(base, offset_in_bits, is_virtual))
{}

/// Get the base class referred to by the current base class
/// specifier.
///
/// @return the base class.
class_decl_sptr
class_decl::base_spec::get_base_class() const
{
  if (priv_->base_class_.expired())
    return class_decl_sptr();
  return class_decl_sptr(priv_->base_class_);
}

/// Getter of the "is-virtual" proprerty of the base class specifier.
///
/// @return true iff this specifies a virtual base class.
bool
class_decl::base_spec::get_is_virtual() const
{return priv_->is_virtual_;}

/// Getter of the offset of the base.
///
/// @return the offset of the base.
long
class_decl::base_spec::get_offset_in_bits() const
{return priv_->offset_in_bits_;}

/// Calculate the hash value for a class_decl::base_spec.
///
/// @return the hash value.
size_t
class_decl::base_spec::get_hash() const
{
  base_spec::hash h;
  return h(*this);
}

/// Traverses an instance of @ref class_decl::base_spec, visiting all
/// the sub-types and decls that it might contain.
///
/// @param v the visitor that is used to visit every IR sub-node of
/// the current node.
///
/// @return true if either
///  - all the children nodes of the current IR node were traversed
///    and the calling code should keep going with the traversing.
///  - or the current IR node is already being traversed.
/// Otherwise, returning false means that the calling code should not
/// keep traversing the tree.
bool
class_decl::base_spec::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      get_base_class()->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Constructor for base_spec instances.
///
/// Note that this constructor is for clients that don't support RTTI
/// and that have a base class of type_base, but of dynamic type
/// class_decl.
///
/// @param base the base class to consider.  Must be a pointer to an
/// instance of class_decl
///
/// @param a the access specifier of the base class.
///
/// @param offset_in_bits if positive or null, represents the offset
/// of the base in the layout of its containing type..  If negative,
/// means that the current base is not laid out in its containing type.
///
/// @param is_virtual if true, means that the current base class is
/// virtual in it's containing type.
class_decl::base_spec::base_spec(shared_ptr<type_base> base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : decl_base(get_type_declaration(base)->get_name(),
	      get_type_declaration(base)->get_location(),
	      get_type_declaration(base)->get_linkage_name(),
	      get_type_declaration(base)->get_visibility()),
      member_base(a),
      priv_(new priv(dynamic_pointer_cast<class_decl>(base),
		     offset_in_bits,
		     is_virtual))
{}

/// Compares two instances of @ref class_decl::base_spec.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const class_decl::base_spec& l,
       const class_decl::base_spec& r,
       change_kind* k)
{
  if (!l.member_base::operator==(r))
    {
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      return false;
    }

  return (*l.get_base_class() == *r.get_base_class());
}

/// Comparison operator for @ref class_decl::base_spec.
///
/// @param other the instance of @ref class_decl::base_spec to compare
/// against.
///
/// @return true if the current instance of @ref class_decl::base_spec
/// equals @p other.
bool
class_decl::base_spec::operator==(const decl_base& other) const
{
  const class_decl::base_spec* o =
    dynamic_cast<const class_decl::base_spec*>(&other);

  if (!o)
    return false;

  return equals(*this, *o, 0);
}

/// Comparison operator for @ref class_decl::base_spec.
///
/// @param other the instance of @ref class_decl::base_spec to compare
/// against.
///
/// @return true if the current instance of @ref class_decl::base_spec
/// equals @p other.
bool
class_decl::base_spec::operator==(const member_base& other) const
{
  const class_decl::base_spec* o =
    dynamic_cast<const class_decl::base_spec*>(&other);
  if (!o)
    return false;

  return operator==(static_cast<const decl_base&>(*o));
}

/// Add a data member to the current instance of class_decl.
///
/// @param v a var_decl to add as a data member.  A proper
/// class_decl::data_member is created from @p v and added to the
/// class_decl.  This var_decl should not have been already added to a
/// scope.
///
/// @param access the access specifier for the data member.
///
/// @param is_laid_out whether the data member was laid out.  That is,
/// if its offset has been computed.  In the pattern of a class
/// template for instance, this would be set to false.
///
/// @param is_static whether the data memer is static.
///
/// @param offset_in_bits if @p is_laid_out is true, this is the
/// offset of the data member, expressed (oh, surprise) in bits.
void
class_decl::add_data_member(var_decl_sptr v, access_specifier access,
			    bool is_laid_out, bool is_static,
			    size_t offset_in_bits)
{
  assert(!has_scope(v));

  priv_->data_members_.push_back(v);
  scope_decl::add_member_decl(v);
  set_data_member_is_laid_out(v, is_laid_out);
  set_data_member_offset(v, offset_in_bits);
  set_member_access_specifier(v, access);
  set_member_is_static(v, is_static);


  if (!is_static)
    {
      // If this is a non-static variable, add it to the set of
      // non-static variables, if it's not only in there.
      bool is_already_in = false;
      for (data_members::const_iterator i =
	     priv_->non_static_data_members_.begin();
	   i != priv_->non_static_data_members_.end();
	   ++i)
	if (*i == v)
	  {
	    is_already_in = true;
	    break;
	  }
      if (!is_already_in)
	priv_->non_static_data_members_.push_back(v);
    }
}

mem_fn_context_rel::~mem_fn_context_rel()
{
}

/// A constructor for instances of class_decl::method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
class_decl::method_decl::method_decl
(const std::string&			name,
 shared_ptr<method_type>		type,
 bool					declared_inline,
 const location&			locus,
 const std::string&			linkage_name,
 visibility				vis,
 binding				bind)
  : decl_base(name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>(type),
		  declared_inline, locus,
		  linkage_name, vis, bind)
{}

/// A constructor for instances of class_decl::method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.  Must be an instance of
/// method_type.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
class_decl::method_decl::method_decl(const std::string&	name,
				     shared_ptr<function_type>	type,
				     bool			declared_inline,
				     const location&		locus,
			const std::string&			linkage_name,
			visibility				vis,
			binding				bind)
  : decl_base(name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, linkage_name, vis, bind)
{}

/// A constructor for instances of class_decl::method_decl.
///
/// @param name the name of the method.
///
/// @param type the type of the method.  Must be an instance of
/// method_type.
///
/// @param declared_inline whether the method was
/// declared inline or not.
///
/// @param locus the source location of the method.
///
/// @param linkage_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
class_decl::method_decl::method_decl(const std::string&	name,
				     shared_ptr<type_base>	type,
				     bool			declared_inline,
				     const location&		locus,
				     const std::string&	linkage_name,
				     visibility		vis,
				     binding			bind)
  : decl_base(name, locus, linkage_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, linkage_name, vis, bind)
{}

class_decl::method_decl::~method_decl()
{}

const method_type_sptr
class_decl::method_decl::get_type() const
{
  method_type_sptr result;
  if (function_decl::get_type())
    result = dynamic_pointer_cast<method_type>(function_decl::get_type());
  return result;
}

/// Set the containing class of a method_decl.
///
/// @param scope the new containing class_decl.
void
class_decl::method_decl::set_scope(scope_decl* scope)
{
  if (!get_context_rel())
    {
      context_rel_sptr c(new mem_fn_context_rel(scope));
      set_context_rel(c);
    }
  else
    get_context_rel()->set_scope(scope);
}

/// A "less than" functor to sort a vector of instances of
/// class_decl::method_decl that are virtual.
struct virtual_member_function_less_than
{
  /// The less than operator.  First, it sorts the methods by their
  /// vtable index.  If they have the same vtable index, it sorts them
  /// by the name of their ELF symbol.  If they don't have elf
  /// symbols, it sorts them by considering their pretty
  /// representation.
  ///
  ///  Note that this method expects virtual methods.
  ///
  /// @param f the first method to consider.
  ///
  /// @param s the second method to consider.
  bool
  operator()(const class_decl::method_decl& f,
	     const class_decl::method_decl& s)
  {
    assert(get_member_function_is_virtual(f));
    assert(get_member_function_is_virtual(s));

    if (get_member_function_vtable_offset(f)
	== get_member_function_vtable_offset(s))
      {
	string fn, sn;

	if (f.get_symbol())
	  fn = f.get_symbol()->get_id_string();
	else
	  fn = f.get_linkage_name();

	if (s.get_symbol())
	  sn = s.get_symbol()->get_id_string();
	else
	  sn = s.get_linkage_name();

	if (fn.empty())
	  fn = f.get_pretty_representation();
	if (sn.empty())
	  sn = s.get_pretty_representation();

	return fn < sn;
      }

    return (get_member_function_vtable_offset(f)
	    < get_member_function_vtable_offset(s));
  }

  /// The less than operator.  First, it sorts the methods by their
  /// vtable index.  If they have the same vtable index, it sorts them
  /// by the name of their ELF symbol.  If they don't have elf
  /// symbols, it sorts them by considering their pretty
  /// representation.
  ///
  ///  Note that this method expects to take virtual methods.
  ///
  /// @param f the first method to consider.
  ///
  /// @param s the second method to consider.
  bool
  operator()(const class_decl::method_decl_sptr f,
	     const class_decl::method_decl_sptr s)
  {return operator()(*f, *s);}
}; // end struct virtual_member_function_less_than

/// Sort a vector of instances of virtual member functions.
///
/// @param mem_fns the vector of member functions to sort.
static void
sort_virtual_member_functions(class_decl::member_functions& mem_fns)
{
  virtual_member_function_less_than lt;
  std::sort(mem_fns.begin(), mem_fns.end(), lt);
}

/// Add a member function to the current instance of class_decl.
///
/// @param f a method_decl to add to the current class.  This function
/// should not have been already added to a scope.
///
/// @param access the access specifier for the member function to add.
///
/// @param vtable_offset the offset of the member function in the
/// virtual table.  If the member function is not virtual, this offset
/// must be 0 (zero).
///
/// @param is_static whether the member function is static.
///
/// @param is_ctor whether the member function is a constructor.
///
/// @param is_dtor whether the member function is a destructor.
///
/// @param is_const whether the member function is const.
void
class_decl::add_member_function(method_decl_sptr f,
				access_specifier a,
				bool is_virtual,
				size_t vtable_offset,
				bool is_static, bool is_ctor,
				bool is_dtor, bool is_const)
{
  assert(!has_scope(f));

  scope_decl::add_member_decl(f);

  set_member_function_is_ctor(f, is_ctor);
  set_member_function_is_dtor(f, is_dtor);
  set_member_function_is_virtual(f, is_virtual);
  set_member_function_vtable_offset(f, vtable_offset);
  set_member_access_specifier(f, a);
  set_member_is_static(f, is_static);
  set_member_function_is_const(f, is_const);

  priv_->member_functions_.push_back(f);
  if (is_virtual)
    sort_virtual_member_functions(priv_->virtual_mem_fns_);
}

/// When a virtual member function has seen its virtualness set by
/// set_member_function_is_virtual(), this function ensures that the
/// member function is added to the specific vectors of virtual member
/// function of its class.
///
/// @param method the method to fixup.
void
fixup_virtual_member_function(class_decl::method_decl_sptr method)
{
  if (!method || !get_member_function_is_virtual(method))
    return;

  class_decl_sptr klass = method->get_type()->get_class_type();
  class_decl::member_functions::iterator m;
  for (m = klass->priv_->virtual_mem_fns_.begin();
       m != klass->priv_->virtual_mem_fns_.end();
       ++m)
    if (m->get() == method.get())
      break;
  if (m == klass->priv_->virtual_mem_fns_.end())
    {
      klass->priv_->virtual_mem_fns_.push_back(method);
      klass->sort_virtual_mem_fns();
    }
}

/// Append a member function template to the class.
///
/// @param m the member function template to append.
void
class_decl::add_member_function_template
(shared_ptr<member_function_template> m)
{
  decl_base* c = m->as_function_tdecl()->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  priv_->member_function_templates_.push_back(m);
  if (!c)
    scope_decl::add_member_decl(m->as_function_tdecl());
}

/// Append a member class template to the class.
///
/// @param m the member function template to append.
void
class_decl::add_member_class_template(shared_ptr<member_class_template> m)
{
  decl_base* c = m->as_class_tdecl()->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  m->set_scope(this);
  priv_->member_class_templates_.push_back(m);
  if (!c)
    scope_decl::add_member_decl(m->as_class_tdecl());
}

/// Return true iff the class has no entity in its scope.
bool
class_decl::has_no_base_nor_member() const
{
  return (priv_->bases_.empty()
	  && priv_->member_types_.empty()
	  && priv_->data_members_.empty()
	  && priv_->member_functions_.empty()
	  && priv_->member_function_templates_.empty()
	  && priv_->member_class_templates_.empty());
}

/// Test if the current instance of @ref class_decl has virtual member
/// functions.
///
/// @return true iff the current instance of @ref class_decl has
/// virtual member functions.
bool
class_decl::has_virtual_member_functions() const
{return !get_virtual_mem_fns().empty();}

/// Test if the current instance of @ref class_decl has at least one
/// virtual base.
///
/// @return true iff the current instance of @ref class_decl has a
/// virtual member function.
bool
class_decl::has_virtual_bases() const
{
  for (base_specs::const_iterator b = get_base_specifiers().begin();
       b != get_base_specifiers().end();
       ++b)
    if ((*b)->get_is_virtual()
	|| (*b)->get_base_class()->has_virtual_bases())
      return true;

  return false;
}

/// Test if the current instance has a vtable.
///
/// This is only valid for a C++ program.
///
/// Basically this function checks if the class has either virtual
/// functions, or virtual bases.
bool
class_decl::has_vtable() const
{
  if (has_virtual_member_functions()
      || has_virtual_bases())
    return true;
  return false;
}

/// Return the hash value for the current instance.
///
/// @return the hash value.
size_t
class_decl::get_hash() const
{
  class_decl::hash hash_class;
  return hash_class(this);
}

/// Compares two instances of @ref class_decl.
///
/// If the two intances are different, set a bitfield to give some
/// insight about the kind of differences there are.
///
/// @param l the first artifact of the comparison.
///
/// @param r the second artifact of the comparison.
///
/// @param k a pointer to a bitfield that gives information about the
/// kind of changes there are between @p l and @p r.  This one is set
/// iff @p k is non-null and the function returns false.
///
/// Please note that setting k to a non-null value does have a
/// negative performance impact because even if @p l and @p r are not
/// equal, the function keeps up the comparison in order to determine
/// the different kinds of ways in which they are different.
///
/// @return true if @p l equals @p r, false otherwise.
bool
equals(const class_decl& l, const class_decl& r, change_kind* k)
{
#define RETURN(value)				\
  do {						\
    l.priv_->unmark_as_being_compared(l);	\
    l.priv_->unmark_as_being_compared(r);	\
    return value;				\
  } while(0)

  // if one of the classes is declaration-only, look through it to
  // get its definition.
  bool l_is_decl_only = l.get_is_declaration_only();
  bool r_is_decl_only = r.get_is_declaration_only();
  if (l_is_decl_only || r_is_decl_only)
    {
      const class_decl* def1 = l_is_decl_only
	? l.get_definition_of_declaration().get()
	: &l;

      const class_decl* def2 = r_is_decl_only
	? r.get_definition_of_declaration().get()
	: &r;

      if (!def1 || !def2)
	{
	  const string& q1 = l.get_qualified_name();
	  const string& q2 = r.get_qualified_name();
	  if (q1 == q2)
	    // Not using RETURN(true) here, because that causes
	    // performance issues.  We don't need to do
	    // l.priv_->unmark_as_being_compared({l,r}) here because
	    // we haven't marked l or r as being compared yet, and
	    // doing so has a peformance cost that shows up on
	    // performance profiles for *big* libraries.
	    return true;
	  else
	    {
	      if (k)
		*k |= LOCAL_CHANGE_KIND;
	      // Not using RETURN(true) here, because that causes
	      // performance issues.  We don't need to do
	      // l.priv_->unmark_as_being_compared({l,r}) here because
	      // we haven't marked l or r as being compared yet, and
	      // doing so has a peformance cost that shows up on
	      // performance profiles for *big* libraries.
	      return false;
	    }
	}

      if (l.priv_->comparison_started(l)
	  || l.priv_->comparison_started(r))
	return true;

      l.priv_->mark_as_being_compared(l);
      l.priv_->mark_as_being_compared(r);

      bool val = *def1 == *def2;
      if (!val)
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
      RETURN(val);
    }

  // No need to go further if the classes have different names or
  // different size / alignment.
  if (!(l.decl_base::operator==(r) && l.type_base::operator==(r)))
    {
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      RETURN(false);
    }

  if (l.priv_->comparison_started(l)
      || l.priv_->comparison_started(r))
    return true;

  l.priv_->mark_as_being_compared(l);
  l.priv_->mark_as_being_compared(r);

  bool result = true;

  // Compare bases.
  {
    if (l.get_base_specifiers().size() != r.get_base_specifiers().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::base_specs::const_iterator
	   b0 = l.get_base_specifiers().begin(),
	   b1 = r.get_base_specifiers().begin();
	 (b0 != l.get_base_specifiers().end()
	 && b1 != r.get_base_specifiers().end());
	 ++b0, ++b1)
      if (*b0 != *b1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= SUBTYPE_CHANGE_KIND;
	      break;
	    }
	  RETURN(false);
	}
  }

  //compare data_members
  {
    if (l.get_non_static_data_members().size()
	!= r.get_non_static_data_members().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::data_members::const_iterator
	   d0 = l.get_non_static_data_members().begin(),
	   d1 = r.get_non_static_data_members().begin();
	 (d0 != l.get_non_static_data_members().end()
	  && d1 != r.get_non_static_data_members().end());
	 ++d0, ++d1)
      if (**d0 != **d1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= SUBTYPE_CHANGE_KIND;
	      break;
	    }
	  else
	    RETURN(false);
	}
  }

  // Do not compare member functions.  DWARF does not necessarily
  // all the member functions, be they virtual or not, in all
  // translation units.  So we cannot have a clear view of them, per
  // class

  // compare member function templates
  {
    if (l.get_member_function_templates().size()
	!= r.get_member_function_templates().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::member_function_templates::const_iterator
	   fn_tmpl_it0 = l.get_member_function_templates().begin(),
	   fn_tmpl_it1 = r.get_member_function_templates().begin();
	 fn_tmpl_it0 != l.get_member_function_templates().end()
	   &&  fn_tmpl_it1 != r.get_member_function_templates().end();
	 ++fn_tmpl_it0, ++fn_tmpl_it1)
      if (**fn_tmpl_it0 != **fn_tmpl_it1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= LOCAL_CHANGE_KIND;
	      break;
	    }
	  else
	    RETURN(false);
	}
  }

  // compare member class templates
  {
    if (l.get_member_class_templates().size()
	!= r.get_member_class_templates().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::member_class_templates::const_iterator
	   cl_tmpl_it0 = l.get_member_class_templates().begin(),
	   cl_tmpl_it1 = r.get_member_class_templates().begin();
	 cl_tmpl_it0 != l.get_member_class_templates().end()
	   &&  cl_tmpl_it1 != r.get_member_class_templates().end();
	 ++cl_tmpl_it0, ++cl_tmpl_it1)
      if (**cl_tmpl_it0 != **cl_tmpl_it1)
	{
	  result = false;
	  if (k)
	    {
	      *k |= LOCAL_CHANGE_KIND;
	      break;
	    }
	  else
	    RETURN(false);
	}
  }

  RETURN(result);
}

/// Comparison operator for @ref class_decl.
///
/// @param other the instance of @ref class_decl to compare against.
///
/// @return true iff the current instance of @ref class_decl equals @p
/// other.
bool
class_decl::operator==(const decl_base& other) const
{
  const class_decl* op = dynamic_cast<const class_decl*>(&other);
  if (!op)
    return false;

  type_base *canonical_type = get_naked_canonical_type(),
    *other_canonical_type = op->get_naked_canonical_type();

  // If this is a declaration only class with no canonical class, use
  // the canonical type of the definition, if any.
  if (!canonical_type
      && get_is_declaration_only()
      && get_definition_of_declaration())
    canonical_type =
      get_definition_of_declaration()->get_naked_canonical_type();

  // Likewise for the other class.
  if (!other_canonical_type
      && op->get_is_declaration_only()
      && op->get_definition_of_declaration())
    other_canonical_type =
      op->get_definition_of_declaration()->get_naked_canonical_type();

  if (canonical_type && other_canonical_type)
    return canonical_type == other_canonical_type;

  const class_decl& o = *op;
  return equals(*this, o, 0);
}

/// Equality operator for class_decl.
///
/// Re-uses the equality operator that takes a decl_base.
///
/// @param other the other class_decl to compare against.
///
/// @return true iff the current instance equals the other one.
bool
class_decl::operator==(const type_base& other) const
{
  const decl_base* o = dynamic_cast<const decl_base*>(&other);
  if (!o)
    return false;
  return *this == *o;
}

/// Comparison operator for @ref class_decl.
///
/// @param other the instance of @ref class_decl to compare against.
///
/// @return true iff the current instance of @ref class_decl equals @p
/// other.
bool
class_decl::operator==(const class_decl& other) const
{
  const decl_base& o = other;
  return *this == o;
}

/// Turn equality of shared_ptr of class_decl into a deep equality;
/// that is, make it compare the pointed to objects too.
///
/// @param l the shared_ptr of class_decl on left-hand-side of the
/// equality.
///
/// @param r the shared_ptr of class_decl on right-hand-side of the
/// equality.
///
/// @return true if the class_decl pointed to by the shared_ptrs are
/// equal, false otherwise.
bool
operator==(const class_decl_sptr& l, const class_decl_sptr& r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// members.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_decl::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      bool stop = false;

      for (base_specs::const_iterator i = get_base_specifiers().begin();
	   i != get_base_specifiers().end();
	   ++i)
	{
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
	}

      if (!stop)
	for (data_members::const_iterator i = get_data_members().begin();
	     i != get_data_members().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_functions::const_iterator i= get_member_functions().begin();
	     i != get_member_functions().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_types::const_iterator i = get_member_types().begin();
	     i != get_member_types().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_function_templates::const_iterator i =
	       get_member_function_templates().begin();
	     i != get_member_function_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }

      if (!stop)
	for (member_class_templates::const_iterator i =
	       get_member_class_templates().begin();
	     i != get_member_class_templates().end();
	     ++i)
	  if (!(*i)->traverse(v))
	    {
	      stop = true;
	      break;
	    }
      visiting(false);
    }

  return v.visit_end(this);
}

/// Destructor of the @ref class_decl type.
class_decl::~class_decl()
{delete priv_;}

context_rel::~context_rel()
{}

bool
class_decl::member_base::operator==(const member_base& o) const
{
  return (get_access_specifier() == o.get_access_specifier()
	  && get_is_static() == o.get_is_static());
}

bool
operator==(const class_decl::base_spec_sptr l,
	   const class_decl::base_spec_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == static_cast<const decl_base&>(*r);
}

/// Test if an ABI artifact is a class base specifier.
///
/// @param tod the ABI artifact to consider.
///
/// @return a pointer to the @ref class_decl::base_spec sub-object of
/// @p tod iff it's a class base specifier.
class_decl::base_spec*
is_class_base_spec(const type_or_decl_base* tod)
{
  return dynamic_cast<class_decl::base_spec*>
    (const_cast<type_or_decl_base*>(tod));
}

/// Test if an ABI artifact is a class base specifier.
///
/// @param tod the ABI artifact to consider.
///
/// @return a pointer to the @ref class_decl::base_spec sub-object of
/// @p tod iff it's a class base specifier.
class_decl::base_spec_sptr
is_class_base_spec(type_or_decl_base_sptr tod)
{return dynamic_pointer_cast<class_decl::base_spec>(tod);}

bool
class_decl::member_function_template::operator==(const member_base& other) const
{
  try
    {
      const class_decl::member_function_template& o =
	dynamic_cast<const class_decl::member_function_template&>(other);

      if (!(is_constructor() == o.is_constructor()
	    && is_const() == o.is_const()
	    && member_base::operator==(o)))
	return false;

      if (function_tdecl_sptr ftdecl = as_function_tdecl())
	{
	  function_tdecl_sptr other_ftdecl = o.as_function_tdecl();
	  if (other_ftdecl)
	    return ftdecl->function_tdecl::operator==(*other_ftdecl);
	}
    }
  catch(...)
    {}
  return false;
}

bool
operator==(class_decl::member_function_template_sptr l,
	   class_decl::member_function_template_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// underlying function template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_decl::member_function_template::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (function_tdecl_sptr f = as_function_tdecl())
	f->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

bool
class_decl::member_class_template::operator==(const member_base& other) const
{
  try
    {
      const class_decl::member_class_template& o =
	dynamic_cast<const class_decl::member_class_template&>(other);

      if (!member_base::operator==(o))
	return false;

      return as_class_tdecl()->class_tdecl::operator==(o);
    }
  catch(...)
    {return false;}
}

/// Comparison operator for the @ref class_decl::member_class_template
/// type.
///
/// @param other the other instance of @ref
/// class_decl::member_class_template to compare against.
///
/// @return true iff the two instances are equal.
bool
class_decl::member_class_template::operator==
(const member_class_template& other) const
{
  const decl_base* o = dynamic_cast<const decl_base*>(&other);
  return *this == *o;
}

/// Comparison operator for the @ref class_decl::member_class_template
/// type.
///
/// @param l the first argument of the operator.
///
/// @param r the second argument of the operator.
///
/// @return true iff the two instances are equal.
bool
operator==(class_decl::member_class_template_sptr l,
	   class_decl::member_class_template_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_decl::member_class_template::traverse(ir_node_visitor& v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (class_tdecl_sptr t = as_class_tdecl())
	t->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

/// Streaming operator for class_decl::access_specifier.
///
/// @param o the output stream to serialize the access specifier to.
///
/// @param a the access specifier to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, access_specifier a)
{
  string r;

  switch (a)
  {
  case no_access:
    r = "none";
    break;
  case private_access:
    r = "private";
    break;
  case protected_access:
    r = "protected";
    break;
  case public_access:
    r= "public";
    break;
  };
  o << r;
  return o;
}

/// Sets the static-ness property of a class member.
///
/// @param d the class member to set the static-ness property for.
/// Note that this must be a class member otherwise the function
/// aborts the current process.
///
/// @param s this must be true if the member is to be static, false
/// otherwise.
void
set_member_is_static(decl_base& d, bool s)
{
  assert(is_member_decl(d));

  context_rel* c = d.get_context_rel();
  assert(c);

  c->set_is_static(s);

  scope_decl* scope = d.get_scope();
  assert(scope);

  if (class_decl* cl = is_class_type(scope))
    {
      if (var_decl* v = is_var_decl(&d))
	{
	  if (s)
	    // remove from the non-static data members
	    for (class_decl::data_members::iterator i =
		   cl->priv_->non_static_data_members_.begin();
		 i != cl->priv_->non_static_data_members_.end();
		 ++i)
	      {
		if (**i == *v)
		  {
		    cl->priv_->non_static_data_members_.erase(i);
		    break;
		  }
	      }
	  else
	    {
	      bool is_already_in_non_static_data_members = false;
	      for (class_decl::data_members::iterator i =
		     cl->priv_->non_static_data_members_.begin();
		   i != cl->priv_->non_static_data_members_.end();
		   ++i)
	      {
		if (**i == *v)
		  {
		    is_already_in_non_static_data_members = true;
		    break;
		  }
	      }
	      if (!is_already_in_non_static_data_members)
		{
		  var_decl_sptr var;
		  // add to non-static data members.
		  for (class_decl::data_members::const_iterator i =
			 cl->priv_->data_members_.begin();
		       i != cl->priv_->data_members_.end();
		       ++i)
		    {
		      if (**i == *v)
			{
			  var = *i;
			  break;
			}
		    }
		  assert(var);
		  cl->priv_->non_static_data_members_.push_back(var);
		}
	    }
	}
    }
}

/// Sets the static-ness property of a class member.
///
/// @param d the class member to set the static-ness property for.
/// Note that this must be a class member otherwise the function
/// aborts the current process.
///
/// @param s this must be true if the member is to be static, false
/// otherwise.
void
set_member_is_static(const decl_base_sptr& d, bool s)
{set_member_is_static(*d, s);}

// </class_decl>

// <template_decl stuff>

/// Data type of the private data of the @template_decl type.
class template_decl::priv
{
  friend class template_decl;

  std::list<template_parameter_sptr> parms_;
public:

  priv()
  {}
}; // end class template_decl::priv

/// Add a new template parameter to the current instance of @ref
/// template_decl.
///
/// @param p the new template parameter to add.
void
template_decl::add_template_parameter(const template_parameter_sptr p)
{priv_->parms_.push_back(p);}

/// Get the list of template parameters of the current instance of
/// @ref template_decl.
///
/// @return the list of template parameters.
const std::list<template_parameter_sptr>&
template_decl::get_template_parameters() const
{return priv_->parms_;}

template_decl::template_decl(const string& name,
			     const location& locus,
			     visibility vis)
  : decl_base(name, locus, /*mangled_name=*/"", vis),
    priv_(new priv)
{
}

template_decl::~template_decl()
{}

bool
template_decl::operator==(const template_decl& o) const
{
  try
    {
      list<shared_ptr<template_parameter> >::const_iterator t0, t1;
      for (t0 = get_template_parameters().begin(),
	     t1 = o.get_template_parameters().begin();
	   (t0 != get_template_parameters().end()
	    && t1 != o.get_template_parameters().end());
	   ++t0, ++t1)
	{
	  if (**t0 != **t1)
	    return false;
	}

      if (t0 != get_template_parameters().end()
	  || t1 != o.get_template_parameters().end())
	return false;

      return true;
    }
  catch(...)
    {return false;}
}

// </template_decl stuff>

//<template_parameter>

/// The type of the private data of the @ref template_parameter type.
class template_parameter::priv
{
  friend class template_parameter;

  unsigned index_;
  template_decl_wptr template_decl_;
  mutable bool hashing_started_;
  mutable bool comparison_started_;

  priv();

public:

  priv(unsigned index, template_decl_sptr enclosing_template_decl)
    : index_(index),
      template_decl_(enclosing_template_decl),
      hashing_started_(),
      comparison_started_()
  {}
}; // end class template_parameter::priv

template_parameter::template_parameter(unsigned	 index,
				       template_decl_sptr enclosing_template)
  : priv_(new priv(index, enclosing_template))
  {}

unsigned
template_parameter::get_index() const
{return priv_->index_;}

const template_decl_sptr
template_parameter::get_enclosing_template_decl() const
{
  if (priv_->template_decl_.expired())
    return template_decl_sptr();
  return template_decl_sptr(priv_->template_decl_);
}

bool
template_parameter::get_hashing_has_started() const
{return priv_->hashing_started_;}

void
template_parameter::set_hashing_has_started(bool f) const
{priv_->hashing_started_ = f;}

bool
template_parameter::operator==(const template_parameter& o) const
{
  if (get_index() != o.get_index())
    return false;

  if (priv_->comparison_started_)
    return true;

  bool result = false;

  // Avoid inifite loops due to the fact that comparison the enclosing
  // template decl might lead to comparing this very same template
  // parameter with another one ...
  priv_->comparison_started_ = true;

  if (!!get_enclosing_template_decl() != !!o.get_enclosing_template_decl())
    ;
  else if (get_enclosing_template_decl()
	   && (*get_enclosing_template_decl()
	       != *o.get_enclosing_template_decl()))
    ;
  else
    result = true;

  priv_->comparison_started_ = false;

  return result;
}

template_parameter::~template_parameter()
{}

/// The type of the private data of the @ref type_tparameter type.
class type_tparameter::priv
{
  friend class type_tparameter;
}; // end class type_tparameter::priv

/// Constructor of the @ref type_tparameter type.
///
/// @param index the index the type template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration.
///
/// @param name the name of the template parameter.
///
/// @param locus the location of the declaration of this type template
/// parameter.
type_tparameter::type_tparameter(unsigned		index,
				 template_decl_sptr	enclosing_tdecl,
				 const std::string&	name,
				 const location&	locus)
  : decl_base(name, locus),
    type_base(0, 0),
    type_decl(name, 0, 0, locus),
    template_parameter(index, enclosing_tdecl),
    priv_(new priv)
{}

bool
type_tparameter::operator==(const type_base& other) const
{
  if (!type_decl::operator==(other))
    return false;

  try
    {
      const type_tparameter& o = dynamic_cast<const type_tparameter&>(other);
      return template_parameter::operator==(o);
    }
  catch (...)
    {return false;}
}

bool
type_tparameter::operator==(const template_parameter& other) const
{
  try
    {
      const type_base& o = dynamic_cast<const type_base&>(other);
      return *this == o;
    }
  catch(...)
    {return false;}
}

bool
type_tparameter::operator==(const type_tparameter& other) const
{return *this == static_cast<const type_base&>(other);}

type_tparameter::~type_tparameter()
{}

/// The type of the private data of the @ref non_type_tparameter type.
class non_type_tparameter::priv
{
  friend class non_type_tparameter;

  type_base_wptr type_;

  priv();

public:

  priv(type_base_sptr type)
    : type_(type)
  {}
}; // end class non_type_tparameter::priv

/// The constructor for the @ref non_type_tparameter type.
///
/// @param index the index of the template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration that
/// holds this parameter parameter.
///
/// @param name the name of the template parameter.
///
/// @param type the type of the template parameter.
///
/// @param locus the location of the declaration of this template
/// parameter.
non_type_tparameter::non_type_tparameter(unsigned		index,
					 template_decl_sptr	enclosing_tdecl,
					 const std::string&	name,
					 type_base_sptr	type,
					 const location&	locus)
    : decl_base(name, locus, ""),
      template_parameter(index, enclosing_tdecl),
      priv_(new priv(type))
{}

/// Getter for the type of the template parameter.
///
/// @return the type of the template parameter.
const type_base_sptr
non_type_tparameter::get_type() const
{
  if (priv_->type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->type_);
}

/// Get the hash value of the current instance.
///
/// @return the hash value.
size_t
non_type_tparameter::get_hash() const
{
  non_type_tparameter::hash hash_tparm;
  return hash_tparm(this);
}

bool
non_type_tparameter::operator==(const decl_base& other) const
{
  if (!decl_base::operator==(other))
    return false;

  try
    {
      const non_type_tparameter& o =
	dynamic_cast<const non_type_tparameter&>(other);
      return (template_parameter::operator==(o)
	      && get_type() == o.get_type());
    }
  catch(...)
    {return false;}
}

bool
non_type_tparameter::operator==(const template_parameter& other) const
{
  try
    {
      const decl_base& o = dynamic_cast<const decl_base&>(other);
      return *this == o;
    }
  catch(...)
    {return false;}
}

non_type_tparameter::~non_type_tparameter()
{}

// <template_tparameter stuff>

/// Type of the private data of the @ref template_tparameter type.
class template_tparameter::priv
{
}; //end class template_tparameter::priv

/// Constructor for the @ref template_tparameter.
///
/// @param index the index of the template parameter.
///
/// @param enclosing_tdecl the enclosing template declaration.
///
/// @param name the name of the template parameter.
///
/// @param locus the location of the declaration of the template
/// parameter.
template_tparameter::template_tparameter(unsigned		index,
					 template_decl_sptr	enclosing_tdecl,
					 const std::string&	name,
					 const location&	locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus, name, VISIBILITY_DEFAULT),
      type_tparameter(index, enclosing_tdecl, name, locus),
      template_decl(name, locus),
      priv_(new priv)
{}

bool
template_tparameter::operator==(const type_base& other) const
{
  try
    {
      const template_tparameter& o =
	dynamic_cast<const template_tparameter&>(other);
      return (type_tparameter::operator==(o)
	      && template_decl::operator==(o));
    }
  catch(...)
    {return false;}
}

bool
template_tparameter::operator==(const template_parameter& o) const
{
  try
    {
      const template_tparameter& other =
	dynamic_cast<const template_tparameter&>(o);
      return *this == static_cast<const type_base&>(other);
    }
  catch(...)
    {return false;}
}

bool
template_tparameter::operator==(const template_decl& o) const
{
  try
    {
      const template_tparameter& other =
	dynamic_cast<const template_tparameter&>(o);
      return type_base::operator==(other);
    }
  catch(...)
    {return false;}
}

template_tparameter::~template_tparameter()
{}

// </template_tparameter stuff>

// <type_composition stuff>

/// The type of the private data of the @ref type_composition type.
class type_composition::priv
{
  friend class type_composition;

  type_base_wptr type_;

  // Forbid this.
  priv();

public:

  priv(type_base_wptr type)
    : type_(type)
  {}
}; //end class type_composition::priv

/// Constructor for the @ref type_composition type.
///
/// @param index the index of the template type composition.
///
/// @param tdecl the enclosing template parameter that owns the
/// composition.
///
/// @param t the resulting type.
type_composition::type_composition(unsigned		index,
				   template_decl_sptr	tdecl,
				   type_base_sptr	t)
  : decl_base("", location()),
    template_parameter(index, tdecl),
    priv_(new priv(t))
{}

/// Getter for the resulting composed type.
///
/// @return the composed type.
const type_base_sptr
type_composition::get_composed_type() const
{
  if (priv_->type_.expired())
    return type_base_sptr();
  return type_base_sptr(priv_->type_);
}

/// Setter for the resulting composed type.
///
/// @param t the composed type.
void
type_composition::set_composed_type(type_base_sptr t)
{priv_->type_ = t;}

/// Get the hash value for the current instance.
///
/// @return the hash value.
size_t
type_composition::get_hash() const
{
  type_composition::hash hash_type_composition;
  return hash_type_composition(this);
}

type_composition::~type_composition()
{}

// </type_composition stuff>

//</template_parameter stuff>

// <function_template>

class function_tdecl::priv
{
  friend class function_tdecl;

  function_decl_sptr pattern_;
  binding binding_;

  priv();

public:

  priv(function_decl_sptr pattern, binding bind)
    : pattern_(pattern), binding_(bind)
  {}

  priv(binding bind)
    : binding_(bind)
  {}
}; // end class function_tdecl::priv

/// Constructor for a function template declaration.
///
/// @param locus the location of the declaration.
///
/// @param vis the visibility of the declaration.  This is the
/// visibility the functions instantiated from this template are going
/// to have.
///
/// @param bind the binding of the declaration.  This is the binding
/// the functions instantiated from this template are going to have.
function_tdecl::function_tdecl(const location&	locus,
			       visibility	vis,
			       binding		bind)
  : decl_base("", locus, "", vis),
    template_decl("", locus, vis),
    scope_decl("", locus),
    priv_(new priv(bind))
{}

/// Constructor for a function template declaration.
///
/// @param pattern the pattern of the template.
///
/// @param locus the location of the declaration.
///
/// @param vis the visibility of the declaration.  This is the
/// visibility the functions instantiated from this template are going
/// to have.
///
/// @param bind the binding of the declaration.  This is the binding
/// the functions instantiated from this template are going to have.
function_tdecl::function_tdecl(function_decl_sptr	pattern,
			       const location&		locus,
			       visibility		vis,
			       binding			bind)
  : decl_base(pattern->get_name(), locus,
	      pattern->get_name(), vis),
    template_decl(pattern->get_name(), locus, vis),
    scope_decl(pattern->get_name(), locus),
    priv_(new priv(pattern, bind))
{}

/// Set a new pattern to the function template.
///
/// @param p the new pattern.
void
function_tdecl::set_pattern(function_decl_sptr p)
{
  priv_->pattern_ = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

/// Get the pattern of the function template.
///
/// @return the pattern.
function_decl_sptr
function_tdecl::get_pattern() const
{return priv_->pattern_;}

/// Get the binding of the function template.
///
/// @return the binding
decl_base::binding
function_tdecl::get_binding() const
{return priv_->binding_;}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param other the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const decl_base& other) const
{
  const function_tdecl* o = dynamic_cast<const function_tdecl*>(&other);
  if (o)
    return *this == *o;
  return false;
}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param other the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const template_decl& other) const
{
  const function_tdecl* o = dynamic_cast<const function_tdecl*>(&other);
  if (o)
    return *this == *o;
  return false;
}

/// Comparison operator for the @ref function_tdecl type.
///
/// @param o the other instance of @ref function_tdecl to compare against.
///
/// @return true iff the two instance are equal.
bool
function_tdecl::operator==(const function_tdecl& o) const
{
  if (!(get_binding() == o.get_binding()
	&& template_decl::operator==(o)
	&& scope_decl::operator==(o)
	&& !!get_pattern() == !!o.get_pattern()))
    return false;

  if (get_pattern())
    return (*get_pattern() == *o.get_pattern());

  return true;
}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the
/// function pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
function_tdecl::traverse(ir_node_visitor&v)
{
  if (visiting())
    return true;

  if (!v.visit_begin(this))
    {
      visiting(true);
      get_pattern()->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

function_tdecl::~function_tdecl()
{}

// </function_template>

// <class template>

/// Type of the private data of the the @ref class_tdecl type.
class class_tdecl::priv
{
  friend class class_tdecl;
  class_decl_sptr pattern_;

public:

  priv()
  {}

  priv(class_decl_sptr pattern)
    : pattern_(pattern)
  {}
}; // end class class_tdecl::priv

/// Constructor for the @ref class_tdecl type.
///
/// @param locus the location of the declaration of the class_tdecl
/// type.
///
/// @param vis the visibility of the instance of class instantiated
/// from this template.
class_tdecl::class_tdecl(const location& locus, visibility vis)
  : decl_base("", locus, "", vis),
    template_decl("", locus, vis),
    scope_decl("", locus),
    priv_(new priv)
{}

/// Constructor for the @ref class_tdecl type.
///
/// @param pattern The details of the class template. This must NOT be a
/// null pointer.  If you really this to be null, please use the
/// constructor above instead.
///
/// @param locus the source location of the declaration of the type.
///
/// @param vis the visibility of the instances of class instantiated
/// from this template.
class_tdecl::class_tdecl(class_decl_sptr pattern,
			 const location& locus,
			 visibility vis)
: decl_base(pattern->get_name(), locus,
	    pattern->get_name(), vis),
  template_decl(pattern->get_name(), locus, vis),
  scope_decl(pattern->get_name(), locus),
  priv_(new priv(pattern))
{}

/// Setter of the pattern of the template.
///
/// @param p the new template.
void
class_tdecl::set_pattern(class_decl_sptr p)
{
  priv_->pattern_ = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

/// Getter of the pattern of the template.
///
/// @return p the new template.
class_decl_sptr
class_tdecl::get_pattern() const
{return priv_->pattern_;}

bool
class_tdecl::operator==(const decl_base& other) const
{
  try
    {
      const class_tdecl& o = dynamic_cast<const class_tdecl&>(other);

      if (!(template_decl::operator==(o)
	    && scope_decl::operator==(o)
	    && !!get_pattern() == !!o.get_pattern()))
	return false;

      return get_pattern()->decl_base::operator==(*o.get_pattern());
    }
  catch(...)
    {return false;}
}

bool
class_tdecl::operator==(const template_decl& other) const
{
  try
    {
      const class_tdecl& o = dynamic_cast<const class_tdecl&>(other);
      return *this == static_cast<const decl_base&>(o);
    }
  catch(...)
    {return false;}
}

bool
class_tdecl::operator==(const class_tdecl& o) const
{return *this == static_cast<const decl_base&>(o);}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
class_tdecl::traverse(ir_node_visitor&v)
{
  if (visiting())
    return true;

  if (v.visit_begin(this))
    {
      visiting(true);
      if (class_decl_sptr pattern = get_pattern())
	pattern->traverse(v);
      visiting(false);
    }
  return v.visit_end(this);
}

class_tdecl::~class_tdecl()
{}

/// This visitor checks if a given type as non-canonicalized sub
/// types.
class non_canonicalized_subtype_detector : public ir::ir_node_visitor
{
  type_base* type_;
  type_base* has_non_canonical_type_;

private:
  non_canonicalized_subtype_detector();

public:
  non_canonicalized_subtype_detector(type_base* type)
    : type_(type),
      has_non_canonical_type_()
  {}

  /// Return true if the visitor detected that there is a
  /// non-canonicalized sub-type.
  ///
  /// @return true if the visitor detected that there is a
  /// non-canonicalized sub-type.
  type_base*
  has_non_canonical_type() const
  {return has_non_canonical_type_;}

  /// The intent of this visitor handler is to avoid looking into
  /// sub-types of member functions of the type we are traversing.
  bool
  visit_begin(function_decl* f)
  {
    // Do not look at sub-types of non-virtual member functions.
    if (is_member_function(f)
	&& get_member_function_is_virtual(*f))
      return false;
    return true;
  }

  /// When visiting a sub-type, if it's *NOT* been canonicalized, set
  /// the 'has_non_canonical_type' flag.  And in any case, when
  /// visiting a sub-type, do not visit its children nodes.  So this
  /// function only goes to the level below the level of the top-most
  /// type.
  ///
  /// @return true if we are at the same level as the top-most type,
  /// otherwise return false.
  bool
  visit_begin(type_base* t)
  {
    if (t != type_)
      {
	if (!t->get_canonical_type())
	  // We are looking a sub-type of 'type_' which has no
	  // canonical type.  So tada! we found one!  Get out right
	  // now with the trophy.
	  has_non_canonical_type_ = t;

	return false;
      }
    return true;
  }

  /// When we are done visiting a sub-type, if it's been flagged as
  /// been non-canonicalized, then stop the traversing.
  ///
  /// Otherwise, keep going.
  ///
  /// @return false iff the sub-type that has been visited is
  /// non-canonicalized.
  bool
  visit_end(type_base* )
  {
    if (has_non_canonical_type_)
      return false;
    return true;
  }
}; //end class non_canonicalized_subtype_detector

/// Test if a type has sub-types that are non-canonicalized.
///
/// @param t the type which sub-types to consider.
///
/// @return true if a type has sub-types that are non-canonicalized.
type_base*
type_has_non_canonicalized_subtype(type_base_sptr t)
{
  if (!t)
    return 0;

  non_canonicalized_subtype_detector v(t.get());
  t->traverse(v);
  return v.has_non_canonical_type();
}

/// Tests if the change of a given type effectively comes from just
/// its sub-types.  That is, if the type has changed but its type name
/// hasn't changed, then the change of the type mostly likely is a
/// sub-type change.
///
/// @param t_v1 the first version of the type.
///
/// @param t_v2 the second version of the type.
///
/// @return true iff the type changed and the change is about its
/// sub-types.
bool
type_has_sub_type_changes(const type_base_sptr t_v1,
			  const type_base_sptr t_v2)
{
  type_base_sptr t1 = strip_typedef(t_v1);
  type_base_sptr t2 = strip_typedef(t_v2);

  string repr1 = get_pretty_representation(t1),
    repr2 = get_pretty_representation(t2);
  return (t1 != t2 && repr1 == repr2);
}

/// Make sure that the life time of a given (smart pointer to a) type
/// is the same as the life time of the libabigail library.
///
/// @param t the type to consider.
void
keep_type_alive(type_base_sptr t)
{
  environment* env = t->get_environment();
  assert(env);
  env->priv_->extra_live_types_.push_back(t);
}

/// Hash an ABI artifact that is either a type or a decl.
///
/// This function intends to provides the fastest possible hashing for
/// types and decls, while being completely correct.
///
/// Note that if the artifact is a type and if it has a canonical
/// type, the hash value is going to be the pointer value of the
/// canonical type.  Otherwise, this function computes a hash value
/// for the type by recursively walking the type members.  This last
/// code path is possibly *very* slow and should only be used when
/// only handful of types are going to be hashed.
///
/// If the artifact is a decl, then a combination of the hash of its
/// type and the hash of the other properties of the decl is computed.
///
/// @param tod the type or decl to hash.
///
/// @return the resulting hash value.
size_t
hash_type_or_decl(const type_or_decl_base *tod)
{
  size_t result = 0;

  if (tod == 0)
    ;
  else if (const type_base* t = dynamic_cast<const type_base*>(tod))
    {
      // If the type has a canonical type, then use the pointer value
      // as a hash.  This is the fastest we can get.
      if (t->get_canonical_type())
	result = reinterpret_cast<size_t>(t->get_canonical_type().get());
      else if (const class_decl* cl = is_class_type(t))
	{
	  if (cl->get_is_declaration_only()
	      && cl->get_definition_of_declaration())
	    // The is a declaration-only class, so it has no canonical
	    // type; but then it's class definition has one.  Let's
	    // use that one.
	    return hash_type_or_decl(cl->get_definition_of_declaration());
	  else
	    {
	      // The class really has no canonical type, let's use the
	      // slow path of hashing the class recursively.  Well
	      // it's not that slow as the hash value is quickly going
	      // to result to zero anyway.
	      type_base::dynamic_hash hash;
	      result = hash(t);
	    }
	}
      else
	{
	  // Let's use the slow path of hashing the class recursively.
	  type_base::dynamic_hash hash;
	  result = hash(t);
	}
    }
  else if (const decl_base* d = dynamic_cast<const decl_base*>(tod))
    {
      if (var_decl* v = is_var_decl(d))
	{
	  assert(v->get_type());
	  size_t h = hash_type_or_decl(v->get_type());
	  string repr = v->get_pretty_representation();
	  std::tr1::hash<string> hash_string;
	  h = hashing::combine_hashes(h, hash_string(repr));
	  result = h;
	}
      else if (function_decl* f = is_function_decl(d))
	{
	  assert(f->get_type());
	  size_t h = hash_type_or_decl(f->get_type());
	  string repr = f->get_pretty_representation();
	  std::tr1::hash<string> hash_string;
	  h = hashing::combine_hashes(h, hash_string(repr));
	  result = h;
	}
      else if (function_decl::parameter* p = is_function_parameter(d))
	{
	  type_base_sptr parm_type = p->get_type();
	  assert(parm_type);
	  std::tr1::hash<bool> hash_bool;
	  std::tr1::hash<unsigned> hash_unsigned;
	  size_t h = hash_type_or_decl(parm_type);
	  h = hashing::combine_hashes(h, hash_unsigned(p->get_index()));
	  h = hashing::combine_hashes(h, hash_bool(p->get_variadic_marker()));
	  result = h;
	}
      else if (class_decl::base_spec *bs = is_class_base_spec(d))
	{
	  class_decl::member_base::hash hash_member;
	  std::tr1::hash<size_t> hash_size;
	  std::tr1::hash<bool> hash_bool;
	  type_base_sptr type = bs->get_base_class();
	  size_t h = hash_type_or_decl(type);
	  h = hashing::combine_hashes(h, hash_member(*bs));
	  h = hashing::combine_hashes(h, hash_size(bs->get_offset_in_bits()));
	  h = hashing::combine_hashes(h, hash_bool(bs->get_is_virtual()));
	  result = h;
	}
      else
	// This is a *really* *SLOW* path.  If it shows up in a
	// performan profile, I bet it'd be a good idea to try to
	// avoid it altogether.
	result = d->get_hash();
    }
  else
    // We should never get here.
    abort();
  return result;
}

/// Hash an ABI artifact that is either a type of a decl.
///
/// @param tod the ABI artifact to hash.
///
/// @return the hash value of the ABI artifact.
size_t
hash_type_or_decl(const type_or_decl_base_sptr& tod)
{return hash_type_or_decl(tod.get());}

bool
ir_traversable_base::traverse(ir_node_visitor&)
{return true;}

bool
ir_node_visitor::visit_begin(decl_base*)
{return true;}

bool
ir_node_visitor::visit_end(decl_base*)
{return true;}

bool
ir_node_visitor::visit_begin(scope_decl*)
{return true;}

bool
ir_node_visitor::visit_end(scope_decl*)
{return true;}

bool
ir_node_visitor::visit_begin(type_base*)
{return true;}

bool
ir_node_visitor::visit_end(type_base*)
{return true;}

bool
ir_node_visitor::visit_begin(scope_type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(scope_type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(namespace_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(namespace_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(qualified_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(qualified_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(pointer_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(pointer_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(reference_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(reference_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(array_type_def* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(array_type_def* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(enum_type_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(enum_type_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(typedef_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(typedef_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(function_type* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(function_type* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(var_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(var_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_decl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_decl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_decl::parameter* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_decl::parameter* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(function_tdecl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(function_tdecl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_tdecl* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_tdecl* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_decl* t)
{return visit_begin(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_end(class_decl* t)
{return visit_end(static_cast<type_base*>(t));}

bool
ir_node_visitor::visit_begin(class_decl::base_spec* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_decl::base_spec* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_decl::member_function_template* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_decl::member_function_template* d)
{return visit_end(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_begin(class_decl::member_class_template* d)
{return visit_begin(static_cast<decl_base*>(d));}

bool
ir_node_visitor::visit_end(class_decl::member_class_template* d)
{return visit_end(static_cast<decl_base*>(d));}

// <debugging facilities>

/// Generate a different string at each invocation.
///
/// @return the resulting string.
static string
get_next_string()
{
  static __thread size_t counter;
  ++counter;
  std::ostringstream o;
  o << counter;
  return o.str();
}

/// Convenience typedef for a hash map of pointer to function_decl and
/// string.
typedef unordered_map<const function_decl*, string,
		      function_decl::hash,
		      function_decl::ptr_equal> fns_to_str_map_type;

/// Return a string associated to a given function.  Two functions
/// that compare equal would yield the same string, as far as this
/// routine is concerned.  And two functions that are different would
/// yield different strings.
///
/// This is used to debug core diffing issues on functions.  The
/// sequence of strings can be given to the 'testdiff2' program that
/// is in the tests/ directory of the source tree, to reproduce core
/// diffing issues on string and thus ease the debugging.
///
/// @param fn the function to generate a string for.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @return the resulting string.
static const string&
fn_to_str(const function_decl* fn,
	  fns_to_str_map_type& m)
{
  fns_to_str_map_type::const_iterator i = m.find(fn);
  if (i != m.end())
    return i->second;
  string s = get_next_string();
  return m[fn]= s;
}

/// Generate a sequence of string that matches a given sequence of
/// function.  In the resulting sequence, each function is "uniquely
/// representated" by a string.  For instance, if the same function "foo"
/// appears at indexes 1 and 3, then the same string 'schmurf' (okay,
/// we don't care about the actual string) would appear at index 1 and 3.
///
/// @param begin the beginning of the sequence of functions to consider.
///
/// @param end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
static void
fns_to_str(vector<function_decl*>::const_iterator begin,
	   vector<function_decl*>::const_iterator end,
	   fns_to_str_map_type& m,
	   std::ostream& o)
{
  vector<function_decl*>::const_iterator i;
  for (i = begin; i != end; ++i)
    o << "'" << fn_to_str(*i, m) << "' ";
}

/// For each sequence of functions given in argument, generate a
/// sequence of string that matches a given sequence of function.  In
/// the resulting sequence, each function is "uniquely representated"
/// by a string.  For instance, if the same function "foo" appears at
/// indexes 1 and 3, then the same string 'schmurf' (okay, we don't
/// care about the actual string) would appear at index 1 and 3.
///
/// @param a_begin the beginning of the sequence of functions to consider.
///
/// @param a_end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param b_begin the beginning of the second sequence of functions
/// to consider.
///
/// @param b_end the end of the second sequence of functions.
///
/// @param m the function_decl* <-> string map to be used by this
/// function to generate strings associated to a function.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
static void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   fns_to_str_map_type& m,
	   std::ostream& o)
{
  fns_to_str(a_begin, a_end, m, o);
  o << "->|<- ";
  fns_to_str(b_begin, b_end, m, o);
  o << "\n";
}

/// For each sequence of functions given in argument, generate a
/// sequence of string that matches a given sequence of function.  In
/// the resulting sequence, each function is "uniquely representated"
/// by a string.  For instance, if the same function "foo" appears at
/// indexes 1 and 3, then the same string 'schmurf' (okay, we don't
/// care about the actual string) would appear at index 1 and 3.
///
/// @param a_begin the beginning of the sequence of functions to consider.
///
/// @param a_end the end of the sequence of functions.  This points to
/// one-passed-the-end of the actual sequence.
///
/// @param b_begin the beginning of the second sequence of functions
/// to consider.
///
/// @param b_end the end of the second sequence of functions.
///
/// @param o the output stream where to emit the generated list of
/// strings to.
void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   std::ostream& o)
{
  fns_to_str_map_type m;
  fns_to_str(a_begin, a_end, b_begin, b_end, m, o);
}

// </debugging facilities>

// </class template>

}// end namespace ir
}//end namespace abigail

namespace
{

/// Update the qualified parent name and qualified name of a tree decl
/// node.
///
/// @return true if the tree walking should continue, false otherwise.
///
/// @param d the tree node to take in account.
bool
qualified_name_setter::do_update(abigail::ir::decl_base* d)
{
  std::string parent_qualified_name;
  if (abigail::ir::scope_decl* parent = d->get_scope())
    d->priv_->qualified_parent_name_ = parent->get_qualified_name();
  else
    d->priv_->qualified_parent_name_.clear();

  if (!d->priv_->qualified_parent_name_.empty())
    {
      if (d->get_name().empty())
	d->priv_->qualified_name_.clear();
      else
	d->priv_->qualified_name_ =
	  d->priv_->qualified_parent_name_ + "::" + d->get_name();
    }

  if (!is_scope_decl(d))
    return false;

  return true;
}

/// This is called when we start visiting a decl node, during the
/// udpate of the qualified name of a given sub-tree.
///
/// @param d the decl node we are visiting.
///
/// @return true iff the traversal should keep going.
bool
qualified_name_setter::visit_begin(abigail::ir::decl_base* d)
{return do_update(d);}

/// This is called when we start visiting a type node, during the
/// udpate of the qualified name of a given sub-tree.
///
/// @param d the decl node we are visiting.
///
/// @return true iff the traversal should keep going.
bool
qualified_name_setter::visit_begin(abigail::ir::type_base* t)
{
  if (abigail::ir::decl_base* d = get_type_declaration(t))
    return do_update(d);
  return false;
}
}// end anonymous namespace.
