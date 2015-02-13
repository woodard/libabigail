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
#include "abg-ir.h"

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
  return location(priv_->locs.size());
}

/// Given an instance of location type, return the triplet
/// {path,line,column} that represents the source locus.  Note that
/// the location must have been previously created from the function
/// location_manager::expand_location otherwise this function yields
/// unexpected results, including possibly a crash.
///
/// @param location the instance of location type to expand
/// @param path the resulting path of the source locus
/// @param line the resulting line of the source locus
/// @param column the resulting colum of the source locus
void
location_manager::expand_location(const location	location,
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
  char				address_size_;
  std::string			path_;
  location_manager		loc_mgr_;
  mutable global_scope_sptr	global_scope_;
  mutable fn_type_ptr_map	canonical_function_types_;

  priv()
    : address_size_(0)
  {}
}; // end translation_unit::priv

// <translation_unit stuff>

/// Constructor of translation_unit.
///
/// @param path the location of the translation unit.
///
/// @param address_size the size of addresses in the translation unit,
/// in bits.
translation_unit::translation_unit(const std::string& path,
				   char address_size)
  : priv_(new priv)
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
    priv_->global_scope_.reset
      (new global_scope(const_cast<translation_unit*>(this)));
  return priv_->global_scope_;
}

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

/// Return the canonical version of a given instance of @ref function_type.
///
/// A given translation units keeps only one copy of each function
/// type that is used in the unit.  So each function type that is
/// built somewhere needs to be passed to this function to
/// canonicalize it and/or return the canonical version.
///
/// Note that it really is the translation unit that 'owns' the
/// function types that are live in the translation unit.  This is
/// unlike the other kinds of types that are owned by the scope where
/// they are defined.
///
/// @param ftype the function type to canonicalize.
///
/// @return the resulting canonical type.
function_type_sptr
translation_unit::get_canonical_function_type(function_type_sptr ftype) const
{
  fn_type_ptr_map::iterator i = priv_->canonical_function_types_.find(ftype);
  if (i == priv_->canonical_function_types_.end())
    {
      priv_->canonical_function_types_[ftype] = true;
      return ftype;
    }
  return dynamic_pointer_cast<function_type>(i->first);
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
  string		name_;
  elf_symbol::type	type_;
  elf_symbol::binding	binding_;
  elf_symbol::version	version_;
  bool			is_defined_;
  elf_symbol*		main_symbol_;
  elf_symbol*		next_alias_;
  string		id_string_;

  priv()
    : index_(0),
      type_(elf_symbol::NOTYPE_TYPE),
      binding_(elf_symbol::GLOBAL_BINDING),
      is_defined_(false),
      main_symbol_(0),
      next_alias_(0)
  {}

  priv(size_t				i,
       const string&			n,
       elf_symbol::type		t,
       elf_symbol::binding		b,
       bool				d,
       const elf_symbol::version&	v)
    : index_(i),
      name_(n),
      type_(t),
      binding_(b),
      version_(v),
      is_defined_(d),
      main_symbol_(0),
      next_alias_(0)
  {}
}; // end struct elf_symbol::priv

elf_symbol::elf_symbol()
  : priv_(new priv)
{
  priv_->main_symbol_ = this;
}

elf_symbol::elf_symbol(size_t		i,
		       const string&	n,
		       type		t,
		       binding		b,
		       bool		d,
		       const version&	v)
  : priv_(new priv(i, n, t, b, d, v))
{
  priv_->main_symbol_ = this;
}

elf_symbol::elf_symbol(const elf_symbol& s)
  : priv_(new priv(s.get_index(),
		   s.get_name(),
		   s.get_type(),
		   s.get_binding(),
		   s.is_defined(),
		   s.get_version()))
{
  priv_->main_symbol_ = this;
}

elf_symbol&
elf_symbol::operator=(const elf_symbol& s)
{
  *priv_ = *s.priv_;
  priv_->main_symbol_ = this;
  priv_->next_alias_ = 0;

  return *this;
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

const string&
elf_symbol::get_name() const
{return priv_->name_;}

void
elf_symbol::set_name(const string& n)
{
  priv_->name_ = n;
  priv_->id_string_.clear();
}

elf_symbol::type
elf_symbol::get_type() const
{return priv_->type_;}

void
elf_symbol::set_type(type t)
{priv_->type_ = t;}

elf_symbol::binding
elf_symbol::get_binding() const
{return priv_->binding_;}

void
elf_symbol::set_binding(binding b)
{priv_->binding_ = b;}

elf_symbol::version&
elf_symbol::get_version() const
{return priv_->version_;}

void
elf_symbol::set_version(const version& v)
{
  priv_->version_ = v;
  priv_->id_string_.clear();
}

bool
elf_symbol::is_defined() const
{return priv_->is_defined_;}


void
elf_symbol::is_defined(bool d)
{priv_->is_defined_ = d;}

bool
elf_symbol::is_public() const
{
  return (is_defined()
	  && (get_binding() == GLOBAL_BINDING
	      || get_binding() == WEAK_BINDING
	      || get_binding() == GNU_UNIQUE_BINDING));
}

bool
elf_symbol::is_function() const
{return get_type() == FUNC_TYPE || get_type() == GNU_IFUNC_TYPE;}

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
const elf_symbol*
elf_symbol::get_main_symbol() const
{return priv_->main_symbol_;}

/// Get the main symbol of an alias chain.
///
///@return the main symbol.
elf_symbol*
elf_symbol::get_main_symbol()
{return priv_->main_symbol_;}

/// Tests whether this symbol is the main symbol.
///
/// @return true iff this symbol is the main symbol.
bool
elf_symbol::is_main_symbol() const
{return get_main_symbol() == this;}

/// Get the next alias of the current symbol.
///
///@return the alias, or NULL if there is no alias.
elf_symbol*
elf_symbol::get_next_alias() const
{return priv_->next_alias_;}


/// Check if the current elf_symbol has an alias.
///
///@return true iff the current elf_symbol has an alias.
bool
elf_symbol::has_aliases() const
{return get_next_alias();}

/// Add an alias to the current elf symbol.
///
/// @param alias the new alias.  Note that this elf_symbol should *NOT*
/// have aliases prior to the invocation of this function.
void
elf_symbol::add_alias(elf_symbol* alias)
{
  if (!alias)
    return;

  assert(!alias->has_aliases());
  assert(is_main_symbol());

  if (has_aliases())
    {
      elf_symbol* last_alias = 0;
      for (elf_symbol* a = get_next_alias();
	   a != get_main_symbol();
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

/// Return a comma separated list of the id of the current symbol as
/// well as the id string of its aliases.
///
/// @return the string.
string
elf_symbol::get_aliases_id_string(const string_elf_symbols_map_type& syms,
				  bool include_symbol_itself) const
{
  string result;

  if (include_symbol_itself)
      result = get_id_string();

  vector<elf_symbol*> aliases;
  compute_aliases_for_elf_symbol(*this, syms, aliases);
  if (!aliases.empty() && include_symbol_itself)
    result += ", ";

  for (vector<elf_symbol*>::const_iterator i = aliases.begin();
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

bool
elf_symbol::operator==(const elf_symbol& other) const
{
  return (get_name() == other.get_name()
	  && get_type() == other.get_type()
	  && is_public() == other.is_public()
	  && is_defined() == other.is_defined()
	  && get_version() == other.get_version());
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

  for (const elf_symbol* a = get_next_alias();
       a && a != get_main_symbol();
       a = a->get_next_alias())
    {
      if (o == *a)
	return true;
    }
  return false;
}

bool
operator==(const elf_symbol_sptr lhs, const elf_symbol_sptr rhs)
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
			       vector<elf_symbol*>& aliases)
{

  if (elf_symbol* a = sym.get_next_alias())
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
	    for (elf_symbol* s = (*j)->get_next_alias();
		 s && s != (*j)->get_main_symbol();
		 s = s->get_next_alias())
	      aliases.push_back(s);
	  else
	    for (const elf_symbol* s = (*j)->get_next_alias();
		 s && s != (*j)->get_main_symbol();
		 s = s->get_next_alias())
	      if (*s == sym)
		aliases.push_back(j->get());
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

// <Decl definition>

struct decl_base::priv
{
  size_t		hash_;
  bool			hashing_started_;
  bool			in_pub_sym_tab_;
  location		location_;
  context_rel_sptr	context_;
  std::string		name_;
  std::string		qualified_parent_name_;
  std::string		qualified_name_;
  std::string		linkage_name_;
  visibility		visibility_;

  priv()
    : hash_(0),
      hashing_started_(false),
      in_pub_sym_tab_(false),
      visibility_(VISIBILITY_DEFAULT)
  {}

  priv(const std::string& name, location locus,
       const std::string& linkage_name, visibility vis)
    : hash_(0),
      hashing_started_(false),
      in_pub_sym_tab_(false),
      location_(locus),
      name_(name),
      linkage_name_(linkage_name),
      visibility_(vis)
  {}

  priv(location l)
    : hash_(0),
      hashing_started_(0),
      in_pub_sym_tab_(false),
      location_(l),
      visibility_(VISIBILITY_DEFAULT)
  {}
};// end struct decl_base::priv

decl_base::decl_base(const std::string& name,
		     location		locus,
		     const std::string& linkage_name,
		     visibility	vis)
  : priv_(new priv(name, locus, linkage_name, vis))
{}

decl_base::decl_base(location l)
  : priv_(new priv(l))
{}

decl_base::decl_base(const decl_base& d)
{
  priv_->hash_ = d.priv_->hash_;
  priv_->hashing_started_ = d.priv_->hashing_started_;
  priv_->in_pub_sym_tab_ = d.priv_->in_pub_sym_tab_;
  priv_->location_ = d.priv_->location_;
  priv_->name_ = d.priv_->name_;
  priv_->qualified_parent_name_ = d.priv_->qualified_parent_name_;
  priv_->qualified_name_ = d.priv_->qualified_name_;
  priv_->linkage_name_ = d.priv_->linkage_name_;
  priv_->context_ = d.priv_->context_;
  priv_->visibility_ = d.priv_->visibility_;
}

/// Getter for the 'hashing_started' property.
///
/// @return the 'hashing_started' property.
bool
decl_base::hashing_started() const
{return priv_->hashing_started_;}

/// Setter for the 'hashing_started' property.
///
/// @param b the value to set the 'hashing_property' to.
void
decl_base::hashing_started(bool b) const
{priv_->hashing_started_ = b;}

/// Getter for the hash value.
///
/// unlike decl_base::get_hash() this does not try to update the hash
/// value.
///
/// @return the hash value.
size_t
decl_base::peek_hash_value() const
{return priv_->hash_;}

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

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
const context_rel_sptr
decl_base::get_context_rel() const
{return priv_->context_;}

///Getter for the context relationship.
///
///@return the context relationship for the current decl_base.
context_rel_sptr
decl_base::get_context_rel()
{return priv_->context_;}

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
  size_t result = priv_->hash_;

  if (priv_->hash_ == 0 || priv_->hashing_started_)
    {
      const type_base* t = dynamic_cast<const type_base*>(this);
      if (t)
	{
	  type_base::dynamic_hash hash;
	  result = hash(t);

	  if (!priv_->hashing_started_)
	    set_hash(result);
	}
      else
	// If we reach this point, it mean we are missing a virtual
	// overload for decl_base::get_hash.  Add it!
	abort();
    }
  return result;
}

/// Set a new hash for the type.
///
/// @param h the new hash.
void
decl_base::set_hash(size_t h) const
{priv_->hash_ = h;}

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
location
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
{priv_->name_ = n;}

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
string
decl_base::get_qualified_parent_name() const
{
  if (priv_->qualified_parent_name_.empty())
    {
      list<string> qn_components;
      for (scope_decl* s = get_scope();
	   s && !is_global_scope(s);
	   s = s->get_scope())
	qn_components.push_front(s->get_name());

      string qn;
      for (list<string>::const_iterator i = qn_components.begin();
	   i != qn_components.end();
	   ++i)
	if (i == qn_components.begin())
	  qn += *i;
	else
	  qn += "::" + *i;

      priv_->qualified_parent_name_ = qn;
    }

  return priv_->qualified_parent_name_;
}

/// Getter for the name of the current decl.
///
/// @return the name of the current decl.
const string&
decl_base::get_name() const
{return priv_->name_;}

/// Compute the qualified name of the decl.
///
/// @param qn the resulting qualified name.
void
decl_base::get_qualified_name(string& qn) const
{
  if (priv_->qualified_name_.empty())
    {
      priv_->qualified_name_ = get_qualified_parent_name();
      if (!get_name().empty())
	{
	  if (!priv_->qualified_name_.empty())
	    priv_->qualified_name_ += "::";
	  priv_->qualified_name_ += get_name();
	}
    }
  qn = priv_->qualified_name_;
}

/// @return the default pretty representation for a decl.  This is
/// basically the fully qualified name of the decl optionally prefixed
/// with a meaningful string to add context for the user.
string
decl_base::get_pretty_representation() const
{return get_qualified_name();}

/// Compute the qualified name of the decl.
///
/// @return the resulting qualified name.
string
decl_base::get_qualified_name() const
{
  string result;
  get_qualified_name(result);
  return result;
}

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
	  result = false;
	  if (k)
	    *k |= LOCAL_CHANGE_KIND;
	  else
	    return false;
	}
    }

  if (l.get_name() != r.get_name())
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (is_member_decl(l) && is_member_decl(r))
    {
      context_rel_sptr r1 = l.get_context_rel(), r2 = r.get_context_rel();
      if (*r1 != *r2)
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

decl_base::~decl_base()
{}

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
operator==(const decl_base_sptr l, const decl_base_sptr r)
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
operator==(const type_base_sptr l, const type_base_sptr r)
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

/// Tests if a type is a class member.
///
/// @param t the type to consider.
///
/// @return true if @p t is a class member type, false otherwise.
bool
is_member_type(const decl_base_sptr d)
{
  if (type_base_sptr t = is_type(d))
    return is_member_type(t);
  return false;
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

  context_rel_sptr c = d.get_context_rel();
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
get_member_access_specifier(const decl_base_sptr d)
{return get_member_access_specifier(*d);}

/// Sets the access specifier for a class member.
///
/// @param d the class member to set the access specifier for.  Note
/// that this must be a class member otherwise the function aborts the
/// current process.
///
/// @param a the new access specifier to set the class member to.
void
set_member_access_specifier(decl_base_sptr d,
			    access_specifier a)
{
  assert(is_member_decl(d));

  context_rel_sptr c = d->get_context_rel();
  assert(c);

  c->set_access_specifier(a);
}

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

  context_rel_sptr c = d.get_context_rel();
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
get_member_is_static(const decl_base_sptr d)
{return get_member_is_static(*d);}

/// Sets the static-ness property of a class member.
///
/// @param d the class member to set the static-ness property for.
/// Note that this must be a class member otherwise the function
/// aborts the current process.
///
/// @param s this must be true if the member is to be static, false
/// otherwise.
void
set_member_is_static(decl_base_sptr d, bool s)
{
  assert(is_member_decl(d));

  context_rel_sptr c = d->get_context_rel();
  assert(c);

  c->set_is_static(s);
}

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

  dm_context_rel_sptr ctxt_rel =
    dynamic_pointer_cast<dm_context_rel>(m->get_context_rel());
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
  dm_context_rel_sptr ctxt_rel =
    dynamic_pointer_cast<dm_context_rel>(m.get_context_rel());
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
  dm_context_rel_sptr ctxt_rel =
    dynamic_pointer_cast<dm_context_rel>(m->get_context_rel());
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
  dm_context_rel_sptr ctxt_rel =
    dynamic_pointer_cast<dm_context_rel>(m.get_context_rel());

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
is_member_function(const function_decl_sptr f)
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

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  return ctxt->is_constructor();
}

/// Test whether a member function is a constructor.
///
/// @param f the member function to test.
///
/// @return true if @p f is a constructor, false otherwise.
bool
get_member_function_is_ctor(const function_decl_sptr f)
{return get_member_function_is_ctor(*f);}


/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(const function_decl& f, bool c)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  ctxt->is_constructor(c);
}

/// Setter for the is_ctor property of the member function.
///
/// @param f the member function to set.
///
/// @param f the new boolean value of the is_ctor property.  Is true
/// if @p f is a constructor, false otherwise.
void
set_member_function_is_ctor(const function_decl_sptr f, bool c)
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

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  return ctxt->is_destructor();
}

/// Test whether a member function is a destructor.
///
/// @param f the function to test.
///
/// @return true if @p f is a destructor, false otherwise.
bool
get_member_function_is_dtor(const function_decl_sptr f)
{return get_member_function_is_dtor(*f);}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(const function_decl& f, bool d)
{
    assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  ctxt->is_destructor(d);
}

/// Set the destructor-ness property of a member function.
///
/// @param f the function to set.
///
/// @param d true if @p f is a destructor, false otherwise.
void
set_member_function_is_dtor(const function_decl_sptr f, bool d)
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

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  return ctxt->is_const();
}

/// Test whether a member function is const.
///
/// @param f the function to test.
///
/// @return true if @p f is const, false otherwise.
bool
get_member_function_is_const(const function_decl_sptr f)
{return get_member_function_is_const(*f);}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(const function_decl& f, bool is_const)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  ctxt->is_const(is_const);
}

/// set the const-ness property of a member function.
///
/// @param f the function to set.
///
/// @param is_const the new value of the const-ness property of @p f
void
set_member_function_is_const(const function_decl_sptr f, bool is_const)
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

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  return ctxt->vtable_offset();
}

/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @return the vtable offset of @p f.
size_t
get_member_function_vtable_offset(const function_decl_sptr f)
{return get_member_function_vtable_offset(*f);}

/// Set the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.
void
set_member_function_vtable_offset(const function_decl& f, size_t s)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  ctxt->vtable_offset(s);
}
/// Get the vtable offset of a member function.
///
/// @param f the member function to consider.
///
/// @param s the new vtable offset.
void
set_member_function_vtable_offset(const function_decl_sptr f, size_t s)
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

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

  return ctxt->is_virtual();
}

/// Test if a given member function is virtual.
///
/// @param mem_fn the member function to consider.
///
/// @return true iff a @p mem_fn is virtual.
bool
get_member_function_is_virtual(const function_decl_sptr mem_fn)
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
set_member_function_is_virtual(const function_decl& f, bool is_virtual)
{
  assert(is_member_function(f));

  const class_decl::method_decl* m =
    dynamic_cast<const class_decl::method_decl*>(&f);
  assert(m);

  mem_fn_context_rel_sptr ctxt =
    dynamic_pointer_cast<mem_fn_context_rel>(m->get_context_rel());

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
/// Also recursively strip typedefs from the sub-types of the type
/// given in arguments.
///
/// @param type the type to strip the typedefs from.
///
/// @return the resulting type stripped from its typedefs, or just
/// return @p type if it has no typedef in any of its sub-types.
type_base_sptr
strip_typedef(const type_base_sptr type)
{
  type_base_sptr t = type;

  if (const typedef_decl_sptr ty = is_typedef(t))
    t = strip_typedef(ty->get_underlying_type());
  else if (const reference_type_def_sptr ty = is_reference_type(t))
    t.reset(new reference_type_def(strip_typedef(type_or_void
						 (ty->get_pointed_to_type())),
				   ty->is_lvalue(),
				   ty->get_size_in_bits(),
				   ty->get_alignment_in_bits(),
				   ty->get_location()));
  else if (const pointer_type_def_sptr ty = is_pointer_type(t))
    t.reset(new pointer_type_def(strip_typedef(type_or_void
					       (ty->get_pointed_to_type())),
				 ty->get_size_in_bits(),
				 ty->get_alignment_in_bits(),
				 ty->get_location()));
  else if (const qualified_type_def_sptr ty = is_qualified_type(t))
    t.reset(new qualified_type_def(strip_typedef(ty->get_underlying_type()),
				   ty->get_cv_quals(),
				   ty->get_location()));
  else if (const array_type_def_sptr ty = is_array_type(t))
    t.reset(new array_type_def(strip_typedef(ty->get_element_type()),
			       ty->get_subranges(),
			       ty->get_location()));
  else if (const method_type_sptr ty = is_method_type(t))
    {
      function_decl::parameters parm;
      for (function_decl::parameters::const_iterator i =
	     ty->get_parameters().begin();
	   i != ty->get_parameters().end();
	   ++i)
	{
	  function_decl::parameter_sptr p = *i;
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(strip_typedef(p->get_type()),
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_artificial()));
	  parm.push_back(stripped);
	}
      t.reset(new method_type(ty->get_return_type(),
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
	  function_decl::parameter_sptr stripped
	    (new function_decl::parameter(strip_typedef(p->get_type()),
					  p->get_index(),
					  p->get_name(),
					  p->get_location(),
					  p->get_variadic_marker(),
					  p->get_artificial()));
	  parm.push_back(stripped);
	}
      t.reset(new function_type(ty->get_return_type(),
				parm,
				ty->get_size_in_bits(),
				ty->get_alignment_in_bits()));
    }
  return t;
}
/// Add a member decl to this scope.  Note that user code should not
/// use this, but rather use add_decl_to_scope.
///
/// @param member the new member decl to add to this scope.
decl_base_sptr
scope_decl::add_member_decl(const shared_ptr<decl_base> member)
{
  members_.push_back(member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
    member_scopes_.push_back(m);
  return member;
}

/// Insert a member decl to this scope, right before an element
/// pointed to by a given iterator.  Note that user code should not
/// use this, but rather use insert_decl_into_scope.
///
/// @param member the new member decl to add to this scope.
///
/// @param before an interator pointing to the element before which
/// the new member should be inserted.
decl_base_sptr
scope_decl::insert_member_decl(const decl_base_sptr member,
			       declarations::iterator before)
{
  members_.insert(before, member);

  if (scope_decl_sptr m = dynamic_pointer_cast<scope_decl>(member))
    member_scopes_.push_back(m);
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
  if (class_decl* klass = dynamic_cast<class_decl*>(this))
    assert(!klass->get_is_declaration_only());

  if (!decl)
    return false;

  if (get_member_decls().empty())
    {
      i = get_member_decls().end();
      return true;
    }

  const class_decl* is_class = dynamic_cast<const class_decl*>(decl);
  if (is_class)
    assert(!is_class->get_is_declaration_only());

  string qual_name1 = decl->get_qualified_name();
  for (declarations::iterator it = get_member_decls().begin();
       it != get_member_decls().end();
       ++it)
    {
      string qual_name2 = (*it)->get_qualified_name();
      if (qual_name1 == qual_name2)
	{
	  if (is_class)
	    {
	      class_decl_sptr cur_class =
		dynamic_pointer_cast<class_decl>(*it);
	      assert(cur_class);
	      if (cur_class->get_is_declaration_only())
		continue;
	    }
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
  if (!v.visit(this))
    return false;

  scope_decl::declarations::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      if (t)
	if (!t->traverse (v))
	  return false;
    }
  return true;
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
add_decl_to_scope(shared_ptr<decl_base> decl, scope_decl* scope)
{
  if (scope && decl && !decl->get_scope())
    {
      decl = scope->add_member_decl(decl);
      decl->set_scope(scope);
    }
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
      decl->set_scope(scope);
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

/// Get the name of a given type and return a copy of it.
///
/// @return a copy of the type name if the type has a name, or the
/// empty string if it does not.
string
get_type_name(const type_base_sptr t)
{
  decl_base_sptr d = dynamic_pointer_cast<decl_base>(t);
  return d->get_name();
}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base* d)
{
  if (!d)
    return "";
  return d->get_pretty_representation();
}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base* t)
{
  const decl_base* d = get_type_declaration(t);
  return get_pretty_representation(d);
}

/// Get a copy of the pretty representation of a decl.
///
/// @param d the decl to consider.
///
/// @return the pretty representation of the decl.
string
get_pretty_representation(const decl_base_sptr& d)
{return get_pretty_representation(d.get());}

/// Get a copy of the pretty representation of a type.
///
/// @param d the type to consider.
///
/// @return the pretty representation of the type.
string
get_pretty_representation(const type_base_sptr& t)
{return get_pretty_representation(t.get());}

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
{
  const global_scope* global = get_global_scope(decl);

  if (global)
    return global->get_translation_unit();
  return 0;
}

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
/// @return true iff the current scope is the global one.
bool
is_global_scope(const scope_decl* scope)
{return scope ? is_global_scope(*scope) : false;}

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
function_decl_sptr
is_function_decl(decl_base_sptr d)
{return dynamic_pointer_cast<function_decl>(d);}

/// Test whether a declaration is a type.
///
/// @param d the declaration to test for.
///
/// @return true if the declaration is a type, false otherwise.
bool
is_type(const decl_base& d)
{
  try {dynamic_cast<const type_base&>(d);}
  catch(...) {return false;}
  return true;
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

/// Test whether a type is a class.
///
/// This function looks through typedefs.
///
/// @parm t the type to consider.
///
/// @return the class_decl if @p t is a class_decl or null otherwise.
class_decl_sptr
is_class_type(const type_base_sptr t)
{
  if (!t)
    return class_decl_sptr();
  type_base_sptr ty = strip_typedef(t);
  return dynamic_pointer_cast<class_decl>(ty);
}

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
pointer_type_def_sptr
is_pointer_type(const type_base_sptr t)
{return dynamic_pointer_cast<pointer_type_def>(t);}

/// Test whether a type is a reference_type_def.
///
/// @param t the type to test.
///
/// @return the @ref reference_type_def_sptr if @p t is a
/// reference_type_def, null otherwise.
reference_type_def_sptr
is_reference_type(const type_base_sptr t)
{return dynamic_pointer_cast<reference_type_def>(t);}

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

/// Test whether a type is a method_type.
///
/// @param t the type to test.
///
/// @return the @ref method_type_sptr if @p t is a
/// method_type, null otherwise.
shared_ptr<method_type>
is_method_type(const shared_ptr<type_base> t)
{return dynamic_pointer_cast<method_type>(t);}

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

/// Tests wheter a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return the var_decl_sptr iff decl is a variable declaration; nil
/// otherwise.
var_decl_sptr
is_var_decl(const shared_ptr<decl_base> decl)
{return dynamic_pointer_cast<var_decl>(decl);}

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

/// Test if a decl is an array_type_def.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is an array_type_def.
array_type_def_sptr
is_array_type(const type_base_sptr decl)
{return dynamic_pointer_cast<array_type_def>(decl);}

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

/// Decompose a fully qualified name into the list of its components.
///
/// @param fqn the fully qualified name to decompose.
///
/// @param comps the resulting list of component to fill.
void
fqn_to_components(const string& fqn,
		  list<string>& comps)
{
  string::size_type comp_begin = 0, comp_end = 0, fqn_size = fqn.size();
  do
    {
      comp_end = fqn.find_first_of("::", comp_begin);
      if (comp_end == fqn.npos)
	comp_end = fqn_size;

      string comp = fqn.substr(comp_begin, comp_end - comp_begin);
      comps.push_back(comp);

      comp_begin = comp_end + 2;
      if (comp_begin >= fqn_size)
	break;
    } while (true);
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
  list<string> comps;
  fqn_to_components(fqn, comps);
  return lookup_type_in_translation_unit(comps, tu);
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
		     const scope_decl_sptr skope)
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
			 const scope_decl_sptr skope)
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
/// @return the declaration of the lookuped node, or NULL if it wasn't
/// found.
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
const decl_base_sptr
lookup_type_in_scope(const list<string>& comps,
		     const scope_decl_sptr skope)
{return lookup_node_in_scope<type_base>(comps, skope);}

/// lookup a var_decl in a scope.
///
/// @param comps the components of the fully qualified name of the
/// var_decl to lookup.
///
/// @param skope the scope to look into.
const decl_base_sptr
lookup_var_decl_in_scope(const std::list<string>& comps,
			 const scope_decl_sptr skope)
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
/// @return either @p t if it is non-null, or the void type.
type_base_sptr
type_or_void(const type_base_sptr t)
{return t ? t : type_base_sptr(type_decl::get_void_type_decl());}

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

  priv()
    : size_in_bits(),
      alignment_in_bits()
  {}

  priv(size_t s,
       size_t a,
       type_base_sptr c = type_base_sptr())
    : size_in_bits(s),
      alignment_in_bits(a),
      canonical_type(c)
  {}
}; // end struct type_base::priv

/// Getter of the map that contains the canonical types of all the
/// types known to libabigail at a certain point in time.
///
/// That map is a global value that is initialized at the first
/// invocation of this function and that is freed when the containing
/// process is shut down.
///
/// The key of the map is the hash value of the type and the value a
/// list of canonical types that have the same map as the key.
///
/// @return the map that contains the canonical types of all the types
/// known the running instance of libabigail in a given process.
type_base::canonical_types_map_type&
type_base::get_canonical_types_map()
{
  static canonical_types_map_type m;
  return m;
}

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

  size_t h = 0;
  decl_base_sptr d = get_type_declaration(t);
  if (d)
    h = d->get_hash();
  else
    {
      type_base::hash type_base_hash;
      h = type_base_hash(t);
    }

  canonical_types_map_type& m = get_canonical_types_map();
  canonical_types_map_type::iterator i = m.find(h);

  if (i == m.end())
    {
      m[h].push_back(t);
      return t;
    }

  for (std::list<type_base_sptr>::iterator j = i->second.begin();
       j != i->second.end();
       ++j)
    if (t == *j)
      return *j;

  i->second.push_back(t);
  return t;
}

/// Enable canonical equality for a given current instance of @ref
/// type_base.
///
/// In concrete terms, this function computes the canonical type for a
/// given instance of @ref type_base.
///
/// It means that after invoking this function, comparing the intance
/// instance @ref type_base and another one (on which
/// type_base::enable_canonical_equality() would have been invoked as
/// well) is performed by just comparing the pointer values of the
/// canonical types of both types.  That equality comparison is
/// supposedly faster than structural comparison of the types.
///
/// @param t a smart pointer to the instance of @ref type_base for
/// which to enable canoncial equality.
void
enable_canonical_equality(type_base_sptr t)
{
  if (!t)
    return;

  if (t->get_canonical_type())
    return;

  type_base_sptr canonical = type_base::get_canonical_type_for(t);
  assert(canonical);

  t->priv_->canonical_type = canonical;
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

type_base::~type_base()
{}
// </type_base definitions>

//<type_decl definitions>

type_decl::type_decl(const std::string&	name,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     location			locus,
		     const std::string&	linkage_name,
		     visibility		vis)

  : decl_base(name, locus, linkage_name, vis),
    type_base(size_in_bits, alignment_in_bits)
{
}

/// Get a singleton representing a void type node.
///
/// @return the void type node.
type_decl_sptr
type_decl::get_void_type_decl()
{
  static type_decl_sptr void_type_decl;
  if (!void_type_decl)
    void_type_decl.reset(new type_decl("void", 0, 0, location()));
  return void_type_decl;
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

string
type_decl::get_pretty_representation() const
{return get_qualified_name();}

/// This implements the ir_traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
///
/// @return true if the entire IR node tree got traversed, false
/// otherwise.
bool
type_decl::traverse(ir_node_visitor& v)
{return v.visit(this);}

type_decl::~type_decl()
{}
//</type_decl definitions>

// <scope_type_decl definitions>

scope_type_decl::scope_type_decl(const std::string&		name,
				 size_t			size_in_bits,
				 size_t			alignment_in_bits,
				 location			locus,
				 visibility			vis)
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

scope_type_decl::~scope_type_decl()
{}
// </scope_type_decl definitions>

// <namespace_decl>
namespace_decl::namespace_decl(const std::string& name,
			       location	  locus,
			       visibility	  vis)
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
  if (!v.visit(this))
    return false;

  scope_decl::declarations::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      if (t)
	if (!t->traverse (v))
	  return false;
    }
  return true;
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
/// @return a copy of the newly-built name.
string
qualified_type_def::build_name(bool fully_qualified) const
{
  string quals = get_cv_quals_string_prefix();
  decl_base_sptr td =
    get_type_declaration(get_underlying_type());
  string name;
  if (fully_qualified)
    name = td->get_qualified_name();
  else
    name = td->get_name();
  if (dynamic_pointer_cast<pointer_type_def>(get_underlying_type())
      || (!priv_->underlying_type_.expired()
	  && dynamic_pointer_cast<reference_type_def>
	  (type_base_sptr(priv_->underlying_type_))))
    {
      name += " ";
      name += quals;
    }
  else
    name = quals + " " + name;
  return name;
}

/// Constructor of the qualified_type_def
///
/// @param type the underlying type
///
/// @param quals a bitfield representing the const/volatile qualifiers
///
/// @param locus the location of the qualified type definition
qualified_type_def::qualified_type_def(shared_ptr<type_base>	type,
				       CV			quals,
				       location		locus)
  : type_base(type->get_size_in_bits(),
	      type->get_alignment_in_bits()),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(type)->get_visibility()),
    priv_(new priv(quals, type))
{
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

/// Implementation for the virtual qualified name builder for.
///
/// @param qualified_name the output parameter to hold the resulting
/// qualified name.
void
qualified_type_def::get_qualified_name(string& qualified_name) const
{
  if (peek_qualified_name().empty())
    set_qualified_name(build_name(true));
  qualified_name = peek_qualified_name();
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
{return v.visit(this);}

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

/// Overloaded bitwise OR operator for cv qualifiers.
qualified_type_def::CV
operator| (qualified_type_def::CV lhs,
	   qualified_type_def::CV rhs)
{
  return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

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

pointer_type_def::pointer_type_def(const type_base_sptr&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   location			locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, "",
	      get_type_declaration(type_or_void(pointed_to))->get_visibility()),
    pointed_to_type_(type_or_void(pointed_to))
{
  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name = (pto ? pto->get_name() : string("void")) + "*";
      set_name(name);
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

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const type_base& o) const
{
  if (get_canonical_type() && o.get_canonical_type())
    return get_canonical_type().get() == o.get_canonical_type().get();

  const pointer_type_def* other = dynamic_cast<const pointer_type_def*>(&o);
  if (!other)
    return false;
  return get_pointed_to_type() == other->get_pointed_to_type();
}

const type_base_sptr
pointer_type_def::get_pointed_to_type() const
{
  if (pointed_to_type_.expired())
    return type_base_sptr();
  return type_base_sptr(pointed_to_type_);
}

/// Build and return the qualified name of the current instance of
/// @ref pointer_type_def.
///
/// @param qn output parameter.  The resulting qualified name.
void
pointer_type_def::get_qualified_name(string& qn) const
{
  if (peek_qualified_name().empty())
    {
      decl_base_sptr td =
	get_type_declaration(get_pointed_to_type());
      string name;
      if (!td)
	name = "void";
      else
	td->get_qualified_name(name);
      set_qualified_name(name + "*");
    }
  qn = peek_qualified_name();
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
{return v.visit(this);}

pointer_type_def::~pointer_type_def()
{}

// </pointer_type_def definitions>

// <reference_type_def definitions>

reference_type_def::reference_type_def(const type_base_sptr	pointed_to,
				       bool			lvalue,
				       size_t			size_in_bits,
				       size_t			align_in_bits,
				       location		locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(type_or_void(pointed_to))->get_visibility()),
    pointed_to_type_(type_or_void(pointed_to)),
    is_lvalue_(lvalue)
{
  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name = pto->get_name() + "&";
      set_name(name);
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
  bool result = (l.get_pointed_to_type() == r.get_pointed_to_type());
  if (!result)
    if (k)
      *k |= SUBTYPE_CHANGE_KIND;
  return result;
}

bool
reference_type_def::operator==(const decl_base& o) const
{
  const reference_type_def* other =
    dynamic_cast<const reference_type_def*>(&o);
  if (!other)
    return false;

  if (get_canonical_type() && other->get_canonical_type())
    return get_canonical_type().get() == other->get_canonical_type().get();

  return equals(*this, *other, 0);
}

bool
reference_type_def::operator==(const type_base& o) const
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
void
reference_type_def::get_qualified_name(string& qn) const
{
  if (peek_qualified_name().empty())
    {
      decl_base_sptr td =
	get_type_declaration(type_or_void(get_pointed_to_type()));
      string name;
      td->get_qualified_name(name);
      set_qualified_name(name + "&");
    }
  qn = peek_qualified_name();
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
{return v.visit(this);}

reference_type_def::~reference_type_def()
{}

// </reference_type_def definitions>

// <array_type_def definitions>

// <array_type_def::subrange_type>
struct array_type_def::subrange_type::priv
{
  size_t	lower_bound_;
  size_t	upper_bound_;
  location	location_;
  priv(size_t ub, location loc)
    : lower_bound_(0), upper_bound_(ub), location_(loc) {}
  priv(size_t lb, size_t ub, location loc)
    : lower_bound_(lb), upper_bound_(ub), location_(loc) {}
};

array_type_def::subrange_type::subrange_type(size_t	lower_bound,
					     size_t	upper_bound,
					     location	loc)
  : priv_(new priv(lower_bound, upper_bound, loc))
{}

array_type_def::subrange_type::subrange_type(size_t upper_bound, location loc)
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

location
array_type_def::subrange_type::get_location() const
{return priv_->location_;}

// </array_type_def::subrange_type>
struct array_type_def::priv
{
  type_base_wptr	element_type_;
  subranges_type	subranges_;

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
			       location				locus)
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

string
get_type_representation(const array_type_def& a)
{
  type_base_sptr e_type = a.get_element_type();
  decl_base_sptr d = get_type_declaration(e_type);
  string r = d->get_name() + a.get_subrange_representation();
  return r;
}

string
array_type_def::get_pretty_representation() const
{return get_type_representation(*this);}

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

bool
array_type_def::is_infinite() const
{

  for (std::vector<shared_ptr<subrange_type> >::const_iterator i = priv_->subranges_.begin();
       i != priv_->subranges_.end();
       ++i)
    {
      if ((*i)->is_infinite())
        return true;
    }
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
void
array_type_def::get_qualified_name(string& qn) const
{qn = get_type_representation(*this);}

/// Compute the qualified name of the array.
///
/// @return the resulting qualified name.
string
array_type_def::get_qualified_name() const
{
  string result;
  get_qualified_name(result);
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
array_type_def::traverse(ir_node_visitor& v)
{return v.visit(this);}

location
array_type_def::get_location() const
{return decl_base::get_location();}

/// Get the array's subranges
const std::vector<array_type_def::subrange_sptr>&
array_type_def::get_subranges() const
{return priv_->subranges_;}

array_type_def::~array_type_def()
{}

// </array_type_def definitions>

/// Return the underlying type of the enum.
shared_ptr<type_base>
enum_type_decl::get_underlying_type() const
{return underlying_type_;}

/// Return the list of enumerators of the enum.
const enum_type_decl::enumerators&
enum_type_decl::get_enumerators() const
{return enumerators_;}

/// @return the pretty representation of the enum type.
string
enum_type_decl::get_pretty_representation() const
{
  string r = "enum " + decl_base::get_pretty_representation();
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
{return v.visit(this);}

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

// <typedef_decl definitions>

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
			   const shared_ptr<type_base>	underlying_type,
			   location			locus,
			   const std::string&		linkage_name,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, linkage_name, vis),
    underlying_type_(underlying_type)
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
/// @return a copy of the pretty representation of the current
/// instance of typedef_decl.
string
typedef_decl::get_pretty_representation() const
{
  string result = "typedef " + get_qualified_name();
  return result;
}

/// Getter of the underlying type of the typedef.
///
/// @return the underlying_type.
type_base_sptr
typedef_decl::get_underlying_type() const
{
  if (underlying_type_.expired())
    return type_base_sptr();
  return type_base_sptr(underlying_type_);
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
{return v.visit(this);}

typedef_decl::~typedef_decl()
{}
// </typedef_decl definitions>

// <var_decl definitions>

struct var_decl::priv
{
  type_base_wptr	type_;
  decl_base::binding	binding_;
  elf_symbol_sptr	symbol_;

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
		   location			locus,
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
var_decl::set_symbol(elf_symbol_sptr sym)
{priv_->symbol_ = sym;}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using var_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this variable decl, if
/// one exists.
elf_symbol_sptr
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

  dm_context_rel_sptr c0 =
    dynamic_pointer_cast<dm_context_rel>(l.get_context_rel());
  dm_context_rel_sptr c1 =
    dynamic_pointer_cast<dm_context_rel>(r.get_context_rel());
  assert(c0 && c1);

  if (*c0 != *c1)
    {
      result = false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }

  if (*l.get_type() != *r.get_type())
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
string
var_decl::get_id() const
{
  if (elf_symbol_sptr s = get_symbol())
    return s->get_id_string();
  else if (!get_linkage_name().empty())
    return get_linkage_name();
  return get_pretty_representation();
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
/// @return a copy of the pretty representation of this variable.
string
var_decl::get_pretty_representation() const
{
  string result;

  if (is_member_decl(this) && get_member_is_static(this))
    result = "static ";
  if(array_type_def_sptr t = is_array_type(get_type()))
    result += get_type_declaration(t->get_element_type())->get_qualified_name()
      + " " + get_qualified_name() + t->get_subrange_representation();
  else
    result += get_type_declaration(get_type())->get_qualified_name()
      + " " + get_qualified_name();
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
{return v.visit(this);}

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
  for (parameters::size_type i = 0; i < priv_->parms_.size(); ++i)
    priv_->parms_[i]->set_index(i);
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

/// Getter for the set of parameters of the current intance of @ref
/// function_type.
///
/// @return the parameters of the current instance of @ref
/// function_type.
function_decl::parameters&
function_type::get_parameters()
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
{priv_->parms_ = p;}

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

  class_decl_sptr lhs_class, rhs_class;
  try
    {
      const method_type& m = dynamic_cast<const method_type&>(lhs);
      lhs_class = m.get_class_type();
    }
  catch (...)
    {}

  try
    {
      const method_type& m = dynamic_cast<const method_type&>(rhs);
      rhs_class = m.get_class_type();
    }
  catch (...)
    {}

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

  decl_base_sptr lhs_return_type_decl =
    get_type_declaration(lhs.get_return_type());
  decl_base_sptr rhs_return_type_decl =
    get_type_declaration(rhs.get_return_type());
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

  class_decl_sptr lcl, rcl;
  vector<shared_ptr<function_decl::parameter> >::const_iterator i,j;
  for (i = lhs.get_first_non_implicit_parm(),
	 j = rhs.get_first_non_implicit_parm();
       (i != lhs.get_parameters().end()
	&& j != rhs.get_parameters().end());
       ++i, ++j)
    {
      if (lhs_class)
	lcl = dynamic_pointer_cast<class_decl>((*i)->get_type());
      if (rhs_class)
	rcl = dynamic_pointer_cast<class_decl>((*j)->get_type());
      if (lcl && rcl
	  && lcl.get() == lhs_class.get()
	  && rcl.get() == rhs_class.get())
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
  if (get_canonical_type() && other.get_canonical_type())
    return get_canonical_type().get() == other.get_canonical_type().get();

  const function_type* o = dynamic_cast<const function_type*>(&other);
  if (!o)
    return false;

  return equals(*this, *o, 0);
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
  elf_symbol_sptr	symbol_;

  priv()
    : declared_inline_(false),
      binding_(decl_base::BINDING_GLOBAL)
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t)
  {}

  priv(function_type_sptr t,
       bool declared_inline,
       decl_base::binding binding,
       elf_symbol_sptr s)
    : declared_inline_(declared_inline),
      binding_(binding),
      type_(t),
      symbol_(s)
  {}
}; // end sruct function_decl::priv



function_decl::function_decl(const std::string& name,
			     function_type_sptr function_type,
			     bool declared_inline,
			     location locus,
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
			     location locus,
			     const std::string& linkage_name,
			     visibility vis,
			     binding bind)
  : decl_base(name, locus, linkage_name, vis),
    priv_(new priv(dynamic_pointer_cast<function_type>(fn_type),
		   declared_inline,
		   bind))
{}

/// @return the pretty representation for a function.
string
function_decl::get_pretty_representation() const
{
  const class_decl::method_decl* mem_fn =
    dynamic_cast<const class_decl::method_decl*>(this);

  string result = mem_fn ? "method ": "function ";

  if (get_member_function_is_virtual(mem_fn))
    result += "virtual ";

  decl_base_sptr type =
    mem_fn
    ? get_type_declaration(mem_fn->get_type()->get_return_type())
    : get_type_declaration(get_type()->get_return_type());

  if (type)
    result += type->get_qualified_name() + " ";
  else if (!(mem_fn && (get_member_function_is_dtor(*mem_fn)
			|| get_member_function_is_ctor(*mem_fn))))
    result += "void ";

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

  if (mem_fn && get_member_function_is_const(*mem_fn))
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

/// Return the type of the current instance of #function_decl.
///
/// It's either a function_type or method_type.
/// @return the type of the current instance of #function_decl.
const shared_ptr<function_type>
function_decl::get_type() const
{
  if (priv_->type_.expired())
    return function_type_sptr();
  return function_type_sptr(priv_->type_);
}

void
function_decl::set_type(shared_ptr<function_type> fn_type)
{priv_->type_ = fn_type;}

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
function_decl::set_symbol(elf_symbol_sptr sym)
{priv_->symbol_ = sym;}

/// Gets the the underlying ELF symbol for the current variable,
/// that was set using function_decl::set_symbol().  Please read the
/// documentation for that member function for more information about
/// "underlying ELF symbols".
///
/// @return sym the underlying ELF symbol for this function decl, if
/// one exists.
elf_symbol_sptr
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
      const_cast<function_decl&>(l).set_name("");
      const_cast<function_decl&>(r).set_name("");
      bool decl_bases_different = !l.decl_base::operator==(r);
      const_cast<function_decl&>(l).set_name(n1);
      const_cast<function_decl&>(r).set_name(n2);

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

  // Compare function types
  shared_ptr<function_type> t0 = l.get_type(), t1 = r.get_type();
  if ((t0 && t1 && *t0 != *t1)
      || !!t0 != !!t1)
    {
      result = false;
      if (k)
	*k |= SUBTYPE_CHANGE_KIND;
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
string
function_decl::get_id() const
{
  if (elf_symbol_sptr s = get_symbol())
    return s->get_id_string();
  else if (!get_linkage_name().empty())
    return get_linkage_name();
  return get_pretty_representation();
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
{return v.visit(this);}

function_decl::~function_decl()
{}

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
				    location			loc,
				    bool			is_variadic)
  : decl_base(name, loc),
    priv_(new priv(type, index, is_variadic, /*is_artificial=*/false))
{}

function_decl::parameter::parameter(const type_base_sptr	type,
				    unsigned			index,
				    const std::string&		name,
				    location			loc,
				    bool			is_variadic,
				    bool			is_artificial)
  : decl_base(name, loc),
    priv_(new priv(type, index, is_variadic, is_artificial))
{}

function_decl::parameter::parameter(const type_base_sptr	type,
				    const std::string&		name,
				    location			loc,
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
  o << get_type_name() << "-" << get_index();
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

bool
function_decl::parameter::traverse(ir_node_visitor& v)
{return v.visit(this);}

size_t
function_decl::parameter::get_hash() const
{
  function_decl::parameter::hash hash_fn_parm;
  return hash_fn_parm(this);
}

void
function_decl::parameter::get_qualified_name(string& qualified_name) const
{qualified_name = get_name();}

// </function_decl::parameter definitions>

// <class_decl definitions>

static void
sort_virtual_member_functions(class_decl::member_functions& mem_fns);

/// The private data for the class_decl type.
struct class_decl::priv
{
  static unordered_map<string, bool> classes_being_compared_;
  bool				is_declaration_only_;
  bool				is_struct_;
  decl_base_sptr		declaration_;
  class_decl_sptr		definition_of_declaration_;
  base_specs			bases_;
  member_types			member_types_;
  data_members			data_members_;
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
  {}

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
  /// This method is not thread safe because it uses the static data
  /// member classes_being_compared_.  If you wish to use it in a
  /// multi-threaded environment you should probably protect the
  /// access to that static data member with a mutex or somesuch.
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
  {classes_being_compared_[klass.get_qualified_name()] = true;}

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
  {classes_being_compared_.erase(klass.get_qualified_name());}

  /// If the instance of class_decl has been previously marked as
  /// being compared -- via an invocation of mark_as_being_compared()
  /// this method unmarks it.  Otherwise is has no effect.
  ///
  /// @param klass the instance of class_decl to unmark.
  void
  unmark_as_being_compared(const class_decl* klass) const
  {classes_being_compared_.erase(klass->get_qualified_name());}

  /// Test if a given instance of class_decl is being currently
  /// compared.
  ///
  ///@param klass the class to test.
  ///
  /// @return true if @p klass is being compared, false otherwise.
  bool
  comparison_started(const class_decl& klass) const
  {
    return (classes_being_compared_.find(klass.get_qualified_name())
	    != classes_being_compared_.end());
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

unordered_map<string, bool> class_decl::priv::classes_being_compared_;

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
		       location locus, visibility vis,
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
		       location locus, visibility vis)
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
{priv_->is_declaration_only_ = f;}

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
class_decl::add_base_specifier(shared_ptr<base_spec> b)
{priv_->bases_.push_back(b);}

/// Get the base specifiers for this class.
///
/// @return a vector of the base specifiers.
const class_decl::base_specs&
class_decl::get_base_specifiers() const
{return priv_->bases_;}

/// Get the member types of this class.
///
/// @return a vector of the member types of this class.
const class_decl::member_types&
class_decl::get_member_types() const
{return priv_->member_types_;}

/// Get the data members of this class.
///
/// @return a vector of the data members of this class.
const class_decl::data_members&
class_decl::get_data_members() const
{return priv_->data_members_;}

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

/// @return the pretty representaion for a class_decl.
string
class_decl::get_pretty_representation() const
{
  string cl= is_struct() ? "struct " : "class ";
  return cl + get_qualified_name();}

/// Set the definition of this declaration-only class.
///
/// @param d the new definition to set.
void
class_decl::set_definition_of_declaration(class_decl_sptr d)
{
  assert(get_is_declaration_only());
  priv_->definition_of_declaration_ = d;
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
		      /*is_static=*/false,
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

  d->set_scope(this);
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
    base_class_(base),
    offset_in_bits_(offset_in_bits),
    is_virtual_(is_virtual)
{}

/// Calculate the hash value for a class_decl::base_spec.
///
/// @return the hash value.
size_t
class_decl::base_spec::get_hash() const
{
  if (peek_hash_value() == 0)
    {
      base_spec::hash h;
      set_hash(h(*this));
    }
  return peek_hash_value();
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
      base_class_(dynamic_pointer_cast<class_decl>(base)),
      offset_in_bits_(offset_in_bits),
      is_virtual_(is_virtual)
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
  bool result = true;

  if (!l.member_base::operator==(r)
      || (*l.get_base_class() != *r.get_base_class()))
    {
      result =false;
      if (k)
	*k |= LOCAL_CHANGE_KIND;
      else
	return false;
    }
  return result;
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

  dm_context_rel_sptr ctxt(new dm_context_rel(this, is_laid_out,
					      offset_in_bits,
					      access, is_static));
  v->set_context_rel(ctxt);
  priv_->data_members_.push_back(v);
  scope_decl::add_member_decl(v);
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
 location				locus,
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
				     location			locus,
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
				     location			locus,
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

  mem_fn_context_rel_sptr ctxt(new mem_fn_context_rel(this, is_ctor,
						      is_dtor, is_const,
						      is_virtual,
						      vtable_offset,
						      a, is_static));

  f->set_context_rel(ctxt);
  priv_->member_functions_.push_back(f);
  scope_decl::add_member_decl(f);
  if (get_member_function_is_virtual(f))
    {
      priv_->virtual_mem_fns_.push_back(f);
      sort_virtual_member_functions(priv_->virtual_mem_fns_);
    }
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
  assert(!c);
  m->as_function_tdecl()->set_scope(this);
  priv_->member_function_templates_.push_back(m);
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
  assert(!c);
  priv_->member_class_templates_.push_back(m);
  m->set_scope(this);
  m->as_class_tdecl()->set_scope(this);
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
  if (l.get_is_declaration_only()
      || r.get_is_declaration_only())
    {
      const class_decl* def1 = l.get_is_declaration_only()
	? l.get_definition_of_declaration().get()
	: &l;

      const class_decl* def2 = r.get_is_declaration_only()
	? r.get_definition_of_declaration().get()
	: &r;

      if (!def1 || !def2
	  || def1->get_is_declaration_only()
	  || def2->get_is_declaration_only())
	{
	  string q1 = l.get_qualified_name();
	  string q2 = r.get_qualified_name();
	  if (q1 != q2)
	    {
	      if (k)
		*k |= LOCAL_CHANGE_KIND;
	      RETURN(false);
	    }
	  RETURN(true);
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

    for(class_decl::base_specs::const_iterator
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
    if (l.get_data_members().size() != r.get_data_members().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::data_members::const_iterator
	   d0 = l.get_data_members().begin(),
	   d1 = r.get_data_members().begin();
	 d0 != l.get_data_members().end() && d1 != r.get_data_members().end();
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

  // compare virtual member functions.  We do not compare
  // non-virtual member functions here because we don't consider
  // them as being meaningful in the *equality* of two classes.
  {
    if (l.get_virtual_mem_fns().size() != r.get_virtual_mem_fns().size())
      {
	result = false;
	if (k)
	  *k |= LOCAL_CHANGE_KIND;
	else
	  RETURN(false);
      }

    for (class_decl::member_functions::const_iterator
	   f0 = l.get_virtual_mem_fns().begin(),
	   f1 = r.get_virtual_mem_fns().begin();
	 f0 != l.get_virtual_mem_fns().end()
	   && f1 != r.get_virtual_mem_fns().end();
	 ++f0, ++f1)
      if (**f0 != **f1)
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

  if (get_canonical_type() && op->get_canonical_type())
    return get_canonical_type().get() == op->get_canonical_type().get();

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
operator==(class_decl_sptr l, class_decl_sptr r)
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
  if (!v.visit(this))
    return false;

  for (member_types::const_iterator i = get_member_types().begin();
       i != get_member_types().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      assert(t);
      if (!t->traverse(v))
	return false;
    }

  for (member_function_templates::const_iterator i =
	 get_member_function_templates().begin();
       i != get_member_function_templates().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      assert(t);
      if (!t->traverse(v))
	return false;
    }

  for (member_class_templates::const_iterator i =
	 get_member_class_templates().begin();
       i != get_member_class_templates().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      assert(t);
      if (!t->traverse(v))
	return false;
    }

  for (data_members::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      assert(t);
      if (!t->traverse(v))
	return false;
    }

  for (member_functions::const_iterator i= get_member_functions().begin();
       i != get_member_functions().end();
       ++i)
    {
      ir_traversable_base_sptr t =
	dynamic_pointer_cast<ir_traversable_base>(*i);
      assert(t);
      if (!t->traverse(v))
	return false;
    }

  return true;
}

class_decl::~class_decl()
{}

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
	return ftdecl->function_tdecl::operator==(static_cast<const decl_base&>(o));
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
  if (!v.visit(this))
    return false;
  return as_function_tdecl()->traverse(v);
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

bool
class_decl::member_class_template::operator==
(const member_class_template& other) const
{
  const class_decl::member_class_template& o = other;
  return *this == o;
}

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
  if (!v.visit(this))
    return false;

  return as_class_tdecl()->get_pattern()->traverse(v);
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

template_decl::template_decl(const string& name, location locus, visibility vis)
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
				 location		locus)
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
					 location		locus)
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
	      && *get_type() == *o.get_type());
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
					 location		locus)
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
function_tdecl::function_tdecl(location	locus,
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
			       location		locus,
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

bool
function_tdecl::operator==(const decl_base& other) const
{
  try
    {
      const function_tdecl& o = dynamic_cast<const function_tdecl&>(other);

      if (!(get_binding() == o.get_binding()
	    && template_decl::operator==(o)
	    && scope_decl::operator==(o)
	    && !!get_pattern() == !!o.get_pattern()))
	return false;

      if (get_pattern())
	return (*get_pattern() == *o.get_pattern());

      return true;
    }
  catch(...)
    {return false;}
}

bool
function_tdecl::operator==(const template_decl& other) const
{
  try
    {
      const function_tdecl& o = dynamic_cast<const function_tdecl&>(other);
      return *this == static_cast<const decl_base&>(o);
    }
  catch(...)
    {return false;}
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
  if (!v.visit(this))
    return false;

  return get_pattern()->traverse(v);
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
class_tdecl::class_tdecl(location locus, visibility vis)
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
			 location locus, visibility vis)
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
  if (!v.visit(this))
    return false;

  shared_ptr<class_decl> pattern = get_pattern();
  if (pattern)
    if (!pattern->traverse(v))
      return false;

  return true;
}

class_tdecl::~class_tdecl()
{}

bool
ir_traversable_base::traverse(ir_node_visitor&)
{return true;}

bool
ir_node_visitor::visit(scope_decl*)
{return true;}

bool
ir_node_visitor::visit(type_decl*)
{return true;}

bool
ir_node_visitor::visit(namespace_decl*)
{return true;}

bool
ir_node_visitor::visit(qualified_type_def*)
{return true;}

bool
ir_node_visitor::visit(pointer_type_def*)
{return true;}

bool
ir_node_visitor::visit(reference_type_def*)
{return true;}

bool
ir_node_visitor::visit(array_type_def*)
{return true;}

bool
ir_node_visitor::visit(enum_type_decl*)
{return true;}

bool
ir_node_visitor::visit(typedef_decl*)
{return true;}

bool
ir_node_visitor::visit(var_decl*)
{return true;}

bool
ir_node_visitor::visit(function_decl*)
{return true;}

bool
ir_node_visitor::visit(function_decl::parameter*)
{return true;}
bool
ir_node_visitor::visit(function_tdecl*)
{return true;}

bool
ir_node_visitor::visit(class_tdecl*)
{return true;}

bool
ir_node_visitor::visit(class_decl*)
{return true;}

bool
ir_node_visitor::visit(class_decl::member_function_template*)
{return true;}

bool
ir_node_visitor::visit(class_decl::member_class_template*)
{return true;}

// <debugging facilities>

/// Generate a different string at each invocation.
///
/// @return the resulting string.
static string
get_next_string()
{
  static size_t counter;
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
