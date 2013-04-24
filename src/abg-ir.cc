// -*- mode: C++ -*-
//
// Copyright (C) 2013 Free Software Foundation, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License
// and a copy of the GCC Runtime Library Exception along with this
// program; see the files COPYING3 and COPYING.RUNTIME respectively.
// If not, see <http://www.gnu.org/licenses/>.

/// @file

#include <assert.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <tr1/memory>
#include "abg-ir.h"

using std::string;
using std::list;
using std::tr1::dynamic_pointer_cast;

namespace abigail
{

/// \brief the location of a token represented in its simplest form.
/// Instances of this type are to be stored in a sorted vector, so the
/// type must have proper relational operators.
class expanded_location
{
  string	m_path;
  unsigned	m_line;
  unsigned	m_column;

  expanded_location();

public:

  friend class location_manager;

  expanded_location(const string&	path,
		    unsigned		line,
		    unsigned		column)
    : m_path(path), m_line(line), m_column(column)
  {
  }

  bool
  operator==(const expanded_location& l) const
  {
    return (m_path == l.m_path
	    && m_line == l.m_line
	    && m_column && l.m_column);
  }

  bool
  operator<(const expanded_location& l) const
  {
    if (m_path < l.m_path)
      return true;
    else if (m_path > l.m_path)
      return false;

    if (m_line < l.m_line)
      return true;
    else if (m_line > l.m_line)
      return false;

    return m_column < l.m_column;
  }
};

struct location_manager::priv
{
  // This sorted vector contains the expanded locations of the tokens
  // coming from a given ABI Corpus.  The index of a given expanded
  // location in the table gives us an integer that is used to build
  // instance of location types.
  std::vector<expanded_location> locs;
};// end struct location_manager::priv

location_manager::location_manager()
{
  m_priv =
    shared_ptr<location_manager::priv>(new location_manager::priv);
}

/// Insert the triplet representing a source locus into our internal
/// vector of location triplet.  Return an instance of location type,
/// built from an integral type that represents the index of the
/// source locus triplet into our source locus table.
///
/// \param file_path the file path of the source locus
/// \param line the line number of the source location
/// \param col the column number of the source location
location
location_manager::create_new_location(const std::string&	file_path,
				      size_t			line,
				      size_t			col)
{
  expanded_location l(file_path, line, col);

  std::vector<expanded_location>::iterator i =
    std::upper_bound(m_priv->locs.begin(),
		     m_priv->locs.end(),
		     l);

  if (i == m_priv->locs.end())
    {
      m_priv->locs.push_back(l);
      i = m_priv->locs.end();
      i--;
    }
  else
    {
      std::vector<expanded_location>::iterator prev = i;
      prev--;
      if (*prev == l)
	i = prev;
      else
	i = m_priv->locs.insert(i, l);
    }
  return location(std::distance (m_priv->locs.begin(), i));
}

/// Given an instance of location type, return the triplet
/// {path,line,column} that represents the source locus.  Note that
/// the location must have been previously created from the function
/// location_manager::expand_location otherwise this function yields
/// unexpected results, including possibly a crash.
///
/// \param location the instance of location type to expand
/// \param path the resulting path of the source locus
/// \param line the resulting line of the source locus
/// \param column the resulting colum of the source locus
void
location_manager::expand_location(const location	location,
				  std::string&		path,
				  unsigned&		line,
				  unsigned&		column) const
{
  expanded_location &l = m_priv->locs[location.m_value];
  path = l.m_path;
  line = l.m_line;
  column = l.m_column;
}

/// Constructor of translation_unit.
///
/// \param the path of the translation unit.
translation_unit::translation_unit(const std::string& path)
  : m_path (path)
{
}

/// Getter of the the global scope of the translation unit.
///
/// \return the global scope of the current translation unit.  If
/// there is not global scope allocated yet, this function creates one
/// and returns it.
const shared_ptr<global_scope>
translation_unit::get_global_scope() const
{
  if (!m_global_scope)
    m_global_scope.reset(new global_scope);
  return m_global_scope;
}

/// Getter of the location manager for the current translation unit.
///
/// \return a reference to the location manager for the current
/// translation unit.
location_manager&
translation_unit::get_loc_mgr()
{
  return m_loc_mgr;
}

/// const Getter of the location manager.
///
/// \return a const reference to the location manager for the current
/// translation unit.
const location_manager&
translation_unit::get_loc_mgr() const
{
  return m_loc_mgr;
}

/// Tests whether if the current translation unit contains ABI
/// artifacts or not.
///
/// \return true iff the current translation unit is empty.
bool
translation_unit::is_empty() const
{
  return get_global_scope()->is_empty();
}

// <Decl definition>

decl_base::decl_base(const std::string&	name,
		     location			locus,
		     const std::string&	mangled_name,
		     visibility vis)
  : m_location(locus),
    m_name(name),
    m_mangled_name(mangled_name),
    m_context(0),
    m_visibility(vis)
{
}

decl_base::decl_base(location l)
  : m_location(l),
    m_context(0),
    m_visibility(VISIBILITY_DEFAULT)
{
}

decl_base::decl_base(const decl_base& d)
{
  m_location = d.m_location;
  m_name = d.m_name;
  m_mangled_name = d.m_mangled_name;
  m_context = d.m_context;
  m_visibility = m_visibility;
}

/// Return true iff the two decls have the same name.
///
/// This function doesn't test if the scopes of the the two decls are
/// equal.
bool
decl_base::operator==(const decl_base& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return get_name() == other.get_name();
}

decl_base::~decl_base()
{
}

/// Setter of the scope of the current decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
void
decl_base::set_scope(scope_decl* scope)
{
  m_context = scope;
}

size_t
decl_base_hash::operator()(const decl_base& d) const
{
  hash<string> str_hash;
  hash<unsigned> unsigned_hash;

  size_t v = str_hash(typeid(d).name());
  if (!d.get_name().empty())
    v = hashing::combine_hashes(v, str_hash(d.get_name()));
  if (d.get_location())
    v = hashing::combine_hashes(v, unsigned_hash(d.get_location()));

  return v;
}

// </Decl definition>

/// Return true iff both scopes have the same names and have the same
/// member decls.
///
/// This function doesn't check for equality of the scopes of its
/// arguments.
bool
scope_decl::operator==(const scope_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  if (static_cast<decl_base>(*this) != static_cast<decl_base>(other))
    return false;

  std::list<shared_ptr<decl_base> >::const_iterator i, j;
  for (i = get_member_decls().begin(), j = other.get_member_decls().begin();
       i != get_member_decls().end() && j != other.get_member_decls().end();
       ++i, ++j)
    if (**i != **j)
      return false;

  if (i != get_member_decls().end() || j != other.get_member_decls().end())
    return false;

  return true;
}

scope_decl::~scope_decl()
{
}

/// Appends a decl to a given scope.
///
/// \param the decl to add append to the scope
///
/// \param the scope to append the decl to
void
add_decl_to_scope(shared_ptr<decl_base> decl,
		  scope_decl*		scope)
{
  if (scope && decl)
    {
      scope->add_member_decl (decl);
      decl->set_scope(scope);
    }
}

/// Return the global scope as seen by a given decl.
///
/// \param decl the decl to consider.
///
/// \return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
global_scope*
get_global_scope(const shared_ptr<decl_base> decl)
{
  scope_decl* scope = decl->get_scope();
  while (scope && !dynamic_cast<global_scope*>(scope))
    scope = scope->get_scope();

  return scope ? dynamic_cast<global_scope*> (scope) : 0;
}

/// Return the translation unit a decl belongs to.
///
/// \param decl the decl to consider.
///
/// \return the resulting translation unit, or null if the decl is not
/// yet added to a translation unit.
translation_unit*
get_translation_unit(const shared_ptr<decl_base> decl)
{
  global_scope* global = get_global_scope(decl);

  if (global)
    return global->get_translation_unit();

  return 0;
}

/// Tests whether if a given scope is the global scope.
///
/// \param scope the scope to consider.
///
/// \return true iff the current scope is the global one.
bool
is_global_scope(const shared_ptr<scope_decl>scope)
{
  return !!dynamic_pointer_cast<global_scope>(scope);
}

/// Tests whether if a given scope is the global scope.
///
/// \param scope the scope to consider.
///
/// \return true iff the current scope is the global one.
bool
is_global_scope(const scope_decl* scope)
{
  return !!dynamic_cast<const global_scope*>(scope);
}

/// Tests wether if a given decl is at global scope.
///
/// \param decl the decl to consider.
///
/// \return true iff #decl is at global scope.
bool
is_decl_at_global_scope(const shared_ptr<decl_base> decl)
{
  if (!decl)
    return false;

  return is_global_scope(decl->get_scope());
}

// </scope_decl definition>

global_scope::~global_scope()
{
}

// <type_base definitions>
type_base::type_base(size_t s, size_t a)
  : m_size_in_bits(s),
    m_alignment_in_bits(a)
{
}

/// Return true iff both type declarations are equal.
///
/// Note that this doesn't test if the scopes of both types are equal.
bool
type_base::operator==(const type_base& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (get_size_in_bits() == other.get_size_in_bits()
	  && get_alignment_in_bits() == other.get_alignment_in_bits());
}

void
type_base::set_size_in_bits(size_t s)
{
  m_size_in_bits = s;
}

size_t
type_base::get_size_in_bits() const
{
  return m_size_in_bits;
}

void
type_base::set_alignment_in_bits(size_t a)
{
  m_alignment_in_bits = a;
}

size_t
type_base::get_alignment_in_bits() const
{
  return m_alignment_in_bits;
}

type_base::~type_base()
{
}

size_t
type_base_hash::operator()(const type_base& t) const
{
  hash<size_t> size_t_hash;
  hash<string> str_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, size_t_hash(t.get_size_in_bits()));
  v = hashing::combine_hashes(v, size_t_hash(t.get_alignment_in_bits()));
  return v;
}
// </type_base definitions>

//<type_decl definitions>

type_decl::type_decl(const std::string&	name,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     location			locus,
		     const std::string&	mangled_name,
		     visibility		vis)

  : decl_base(name, locus, mangled_name, vis),
    type_base(size_in_bits, alignment_in_bits)
{
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
bool
type_decl::operator==(const type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<decl_base>(*this) == other
	  && static_cast<type_base>(*this) == other);
}

type_decl::~type_decl()
{
}

size_t
type_decl_hash::operator()(const type_decl& t) const
{
  decl_base_hash decl_hash;
  type_base_hash type_hash;
  hash<string> str_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, decl_hash(t));
  v = hashing::combine_hashes(v, type_hash(t));

  return v;
}

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

/// Return true iff both scope types are equal.
///
/// Note that this function does not consider the scope of the scope
/// types themselves.
bool
scope_type_decl::operator==(const scope_type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<scope_decl>(*this) == other
	  && static_cast<type_base>(*this) == other);
}

scope_type_decl::~scope_type_decl()
{
}

size_t
scope_type_decl_hash::operator()(const scope_type_decl& t) const
{
  decl_base_hash decl_hash;
  type_base_hash type_hash;
  hash<string> str_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, decl_hash(t));
  v = hashing::combine_hashes(v, type_hash(t));

  return v;
}

// </scope_type_decl definitions>

// <namespace_decl>
namespace_decl::namespace_decl(const std::string&	name,
			       location		locus,
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

/// Return true iff both namespaces and their members are equal.
///
/// Note that this function does not check if the scope of these
/// namespaces are equal.
bool
namespace_decl::operator==(const namespace_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<scope_decl>(*this) == other);
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

/// Constructor of the qualified_type_def
///
/// \param type the underlying type
///
/// \params quals a bitfield representing the const/volatile qualifiers
///
/// \param locus the location of the qualified type definition
qualified_type_def::qualified_type_def(shared_ptr<type_base>	type,
				       CV			quals,
				       location		locus)
  : type_base(type->get_size_in_bits(),
	      type->get_alignment_in_bits()),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(type)->get_visibility()),
    m_cv_quals(quals),
    m_underlying_type(type)
{
  if (quals & qualified_type_def::CV_CONST)
    set_name(get_name() + "const ");
  if (quals & qualified_type_def::CV_VOLATILE)
    set_name(get_name() + "volatile ");
  set_name(get_name() + dynamic_pointer_cast<decl_base>(type)->get_name());
}

/// Return true iff both qualified types are equal.
///
/// Note that this function does not check for equality of the scopes.
bool
qualified_type_def::operator==(const qualified_type_def& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other)
      || get_cv_quals() != other.get_cv_quals())
    return false;

  return *get_underlying_type() == *other.get_underlying_type();
}

/// The destructor of the qualified type
qualified_type_def::~qualified_type_def()
{
}

/// Getter of the const/volatile qualifier bit field
char
qualified_type_def::get_cv_quals() const
{
  return m_cv_quals;
}

/// Setter of the const/value qualifiers bit field
void
qualified_type_def::set_cv_quals(char cv_quals)
{
  m_cv_quals = cv_quals;
}

/// Getter of the underlying type
const shared_ptr<type_base>
qualified_type_def::get_underlying_type() const
{
  return m_underlying_type;
}

size_t
qualified_type_def_hash::operator()(const qualified_type_def& t) const
{
  type_base_hash type_hash;
  decl_base_hash decl_hash;
  hash<string> str_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, type_hash(t));
  v = hashing::combine_hashes(v, decl_hash(t));
  v = hashing::combine_hashes(v, t.get_cv_quals());

  return v;
}

// </qualified_type_def>

/// A hashing function for type declarations.
///
/// This function gets the dynamic type of the actual type
/// declaration and calls the right hashing function for that type.
///
/// Note that each time a new type declaration kind is added to the
/// system, this function needs to be updated.
///
/// \param t a pointer to the type declaration to be hashed
///
/// \return the resulting hash
size_t
dynamic_type_hash::operator()(const type_base* t) const
{
  if (const template_template_parameter* d =
      dynamic_cast<const template_template_parameter*>(t))
    return template_template_parameter_hash()(*d);
  if (const template_type_parameter* d =
      dynamic_cast<const template_type_parameter*>(t))
    return template_type_parameter_hash()(*d);
  if (const type_decl* d = dynamic_cast<const type_decl*> (t))
    return type_decl_hash()(*d);
  if (const qualified_type_def* d = dynamic_cast<const qualified_type_def*>(t))
    return qualified_type_def_hash()(*d);
  if (const pointer_type_def* d = dynamic_cast<const pointer_type_def*>(t))
    return pointer_type_def_hash()(*d);
  if (const reference_type_def* d = dynamic_cast<const reference_type_def*>(t))
    return reference_type_def_hash()(*d);
  if (const enum_type_decl* d = dynamic_cast<const enum_type_decl*>(t))
    return enum_type_decl_hash()(*d);
  if (const typedef_decl* d = dynamic_cast<const typedef_decl*>(t))
    return typedef_decl_hash()(*d);
  if (const class_decl* d = dynamic_cast<const class_decl*>(t))
    return class_decl_hash()(*d);
  if (const scope_type_decl* d = dynamic_cast<const scope_type_decl*>(t))
    return scope_type_decl_hash()(*d);

  // Poor man's fallback case.
  return type_base_hash()(*t);
}

//<pointer_type_def definitions>

pointer_type_def::pointer_type_def(shared_ptr<type_base>&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   location			locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    m_pointed_to_type(pointed_to)
{
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const pointer_type_def& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return *get_pointed_to_type() == *other.get_pointed_to_type();
}

shared_ptr<type_base>
pointer_type_def::get_pointed_to_type() const
{
  return m_pointed_to_type;
}

pointer_type_def::~pointer_type_def()
{
}

size_t
pointer_type_def_hash::operator()(const pointer_type_def& t) const
{
  hash<string> str_hash;
  type_base_hash type_base_hash;
  decl_base_hash decl_hash;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, decl_hash(t));
  v = hashing::combine_hashes(v, type_base_hash(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));

  return v;
}

// </pointer_type_def definitions>

// <reference_type_def definitions>

reference_type_def::reference_type_def(shared_ptr<type_base>&	pointed_to,
				       bool			lvalue,
				       size_t			size_in_bits,
				       size_t			align_in_bits,
				       location		locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    m_pointed_to_type(pointed_to),
    m_is_lvalue(lvalue)
{
}

bool
reference_type_def::operator==(const reference_type_def& other) const
{
    // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return *get_pointed_to_type() == *other.get_pointed_to_type();
}

shared_ptr<type_base>
reference_type_def::get_pointed_to_type() const
{
  return m_pointed_to_type;
}

bool
reference_type_def::is_lvalue() const
{
  return m_is_lvalue;
}

reference_type_def::~reference_type_def()
{
}

size_t
reference_type_def_hash::operator()(const reference_type_def& t)
{
  hash<string> hash_str;
  type_base_hash hash_type_base;
  decl_base_hash hash_decl;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = hash_str(typeid(t).name());
  v = hashing::combine_hashes(v, hash_str(t.is_lvalue()
					  ? "lvalue"
					  : "rvalue"));
  v = hashing::combine_hashes(v, hash_type_base(t));
  v = hashing::combine_hashes(v, hash_decl(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));

  return v;
}
// </reference_type_def definitions>

/// Constructor of an enum type declaration.
///
/// \param name the name of the enum
///
/// \param locus the locus at which the enum appears in the source
/// code.
///
/// \param underlying_type the underlying type of the enum
///
/// \param enumerators a list of enumerators for this enum.
enum_type_decl::enum_type_decl(const string&			name,
			       location			locus,
			       shared_ptr<type_base>		underlying_type,
			       const std::list<enumerator>&	enumerators,
			       const string&			mangled_name,
			       visibility			vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    m_underlying_type(underlying_type),
    m_enumerators(enumerators)
  {
  }

/// Return the underlying type of the enum.
shared_ptr<type_base>
enum_type_decl::get_underlying_type() const
{
  return m_underlying_type;
}

/// Return the list of enumerators of the enum.
const std::list<enum_type_decl::enumerator>&
enum_type_decl::get_enumerators() const
{
  return m_enumerators;
}

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{
}

size_t
enum_type_decl_hash::operator()(const enum_type_decl& t) const
{
  hash<string> str_hash;
  type_shared_ptr_hash type_ptr_hash;
  hash<size_t> size_t_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));
  for (std::list<enum_type_decl::enumerator>::const_iterator i =
	   t.get_enumerators().begin();
	 i != t.get_enumerators().end();
	 ++i)
    {
	v = hashing::combine_hashes(v, str_hash(i->get_name()));
	v = hashing::combine_hashes(v, size_t_hash(i->get_value()));
    }
  return v;
}

/// Equality operator.
///
/// \param other the other enum to test against.
///
/// \return true iff other is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const enum_type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other)
      || *get_underlying_type() != *other.get_underlying_type())
    return false;

  std::list<enumerator>::const_iterator i, j;
  for (i = get_enumerators().begin(), j = other.get_enumerators().begin();
       i != get_enumerators().end() && j != other.get_enumerators().end();
       ++i, ++j)
    if (*i != *j)
      return false;

  if (i != get_enumerators().end() || j != other.get_enumerators().end())
    return false;

  return true;
}

// <typedef_decl definitions>

/// Constructor of the typedef_decl type.
///
/// \param name the name of the typedef.
///
/// \param underlying_type the underlying type of the typedef.
///
/// \param locus the source location of the typedef declaration.
typedef_decl::typedef_decl(const string&		name,
			   const shared_ptr<type_base>	underlying_type,
			   location			locus,
			   const std::string&		mangled_name,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    m_underlying_type(underlying_type)
{
}

/// Equality operator
///
/// \param other the other typedef_decl to test against.
bool
typedef_decl::operator==(const typedef_decl& other) const
{
  return (typeid(*this) == typeid(other)
	  && get_name() == other.get_name()
	  && *get_underlying_type() == *other.get_underlying_type());
}

/// Getter of the underlying type of the typedef.
///
/// \return the underlying_type.
shared_ptr<type_base>
typedef_decl::get_underlying_type() const
{
  return m_underlying_type;
}

/// Destructor of the typedef_decl.
typedef_decl::~typedef_decl()
{
}

size_t
typedef_decl_hash::operator()(const typedef_decl& t) const
{
  hash<string> str_hash;
  type_base_hash type_hash;
  decl_base_hash decl_hash;
  type_shared_ptr_hash type_ptr_hash;

  size_t v = str_hash(typeid(t).name());
  v = hashing::combine_hashes(v, type_hash(t));
  v = hashing::combine_hashes(v, decl_hash(t));
  v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));

  return v;
}
// </typedef_decl definitions>

// <var_decl definitions>

var_decl::var_decl(const std::string&		name,
		   shared_ptr<type_base>	type,
		   location			locus,
		   const std::string&		mangled_name,
		   visibility			vis,
		   binding			bind)
  : decl_base(name, locus, mangled_name, vis),
    m_type(type),
    m_binding(bind)
{
}

bool
var_decl::operator==(const var_decl& other) const
{
  return (typeid(*this) == typeid(other)
	  && static_cast<decl_base>(*this) == static_cast<decl_base>(other)
	  && *get_type() == *other.get_type());
}

var_decl::~var_decl()
{
}

size_t
var_decl_hash::operator()(const var_decl& t) const
{
  hash<string> hash_string;
  decl_base_hash hash_decl;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));
  return v;
}

// </var_decl definitions>

// <function_decl definitions>

bool
function_decl::operator==(const function_decl& o) const
{

  // Compare function return types.
  shared_ptr<type_base> r0 = get_return_type(), r1 = o.get_return_type();
  if ((r0 && r1 && *r0 != *r1)
      || !!r0 != !!r1)
    return false;

  // Compare function parameters.
  list<shared_ptr<parameter> >::const_iterator p0, p1;
  for (p0 = get_parameters().begin(), p1 = o.get_parameters().begin();
       p0 != get_parameters().end(), p1 != o.get_parameters().end();
       ++p0, ++p1)
    {
      if (**p0 != **p1)
	return false;
    }
  if (p0 != get_parameters().end() || p1 != o.get_parameters().end())
    return false;

  // Compare the remaining properties
  if (is_declared_inline() != o.is_declared_inline()
      || get_binding() != o.get_binding())
    return false;

  return true;
}

/// A hashing function for instances of function_decl.
///
/// \param t the instance of function_decl to calculate a hash value for.
///
/// \return a the computed hash value.
size_t
function_decl_hash::operator()(const function_decl& t) const
{
  hash<int> hash_int;
  hash<bool> hash_bool;
  hash<string> hash_string;
  decl_base_hash hash_decl_base;
  function_decl::parameter_hash hash_parm;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
  for (std::list<shared_ptr<function_decl::parameter> >::const_iterator p =
	   t.get_parameters().begin();
	 p != t.get_parameters().end();
	 ++p)
    v = hashing::combine_hashes(v, hash_parm(**p));

  v = hashing::combine_hashes(v, hash_bool(t.is_declared_inline()));
  v = hashing::combine_hashes(v, hash_int(t.get_binding()));

  return v;
}

function_decl::~function_decl()
{
}

// <function_decl definitions>

// <class_decl definitions>

/// Add a member type to the current instnace of class_decl
///
/// \param the member type to add.
void
class_decl::add_member_type(shared_ptr<member_type>t)
{
  decl_base* c = dynamic_pointer_cast<decl_base>(t->as_type())->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c || c == this);
  if (!c)
    add_decl_to_scope(t, this);

  m_member_types.push_back(t);
}

/// Add a data member to the current instance of class_decl.
///
/// \param m the data member to add.
void
class_decl::add_data_member(shared_ptr<data_member> m)
{
  decl_base* c = m->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c || c == this);
  if (!c)
    add_decl_to_scope(m, this);

  m_data_members.push_back(m);
}

/// Add a member function to the current instance of class_decl.
///
/// \param m the member function to add.
void
class_decl::add_member_function(shared_ptr<member_function> m)
{
  decl_base* c = m->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c || c == this);
  if (!c)
    add_decl_to_scope(m, this);

  m_member_functions.push_back(m);
}

bool
class_decl::operator==(const class_decl& o) const
{
  // Compare bases.
  list<shared_ptr<class_decl::base_spec> >::const_iterator b0, b1;
  for(b0 = get_base_specifiers().begin(), b1 = o.get_base_specifiers().begin();
      b0 != get_base_specifiers().end() && b1 != o.get_base_specifiers().end();
      ++b0, ++b1)
      if (**b0 != **b1)
	return false;
  if (b0 != get_base_specifiers().end() || b1 != o.get_base_specifiers().end())
    return false;

  //Compare member types
  list<shared_ptr<class_decl::member_type> >::const_iterator t0, t1;
  for (t0 = get_member_types().begin(), t1 = o.get_member_types().begin();
       t0 != get_member_types().end() && t1 != o.get_member_types().end();
       ++t0, ++t1)
    if (**t0 != **t1)
      return false;
  if (t0 != get_member_types().end() || t1 != o.get_member_types().end())
    return false;

  //compare data_members
  list<shared_ptr<class_decl::data_member> >::const_iterator d0, d1;
  for (d0 = get_data_members().begin(), d1 = o.get_data_members().begin();
       d0 != get_data_members().end() && d1 != o.get_data_members().end();
       ++d0, ++d1)
    if (**d0 != **d1)
      return false;
  if (d0 != get_data_members().end() || d1 != o.get_data_members().end())
    return false;

  //compare member functions
  list<shared_ptr<class_decl::member_function> >::const_iterator f0, f1;
  for (f0 = get_member_functions().begin(),
	 f1 = o.get_member_functions().begin();
       f0 != get_member_functions().end(), f1 != o.get_member_functions().end();
       ++f0, ++f1)
    if (**d0 != **d1)
      return false;
  if (f0 != get_member_functions().end()
      || f1 != o.get_member_functions().end())
    return false;

  return true;
}

class_decl::~class_decl()
{
}

size_t
class_decl::member_type_hash::operator()(const member_type& t)const
{
  member_hash hash_member;
  type_shared_ptr_hash hash_type;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_type(t.as_type()));
  return v;
}

size_t
class_decl::base_spec_hash::operator()(const base_spec& t) const
{
  member_hash hash_member;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_base_class()));
  return v;
}

size_t
class_decl::data_member_hash::operator()(data_member& t)
{
  hash<size_t> hash_size_t;
  var_decl_hash hash_var_decl;
  member_hash hash_member;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_var_decl(t));
  if (t.is_laid_out())
    v = hashing::combine_hashes(v, hash_size_t(t.get_offset_in_bits()));
  v = hashing::combine_hashes(v, t.is_static());

  return v;
}

size_t
class_decl::member_function_hash::operator()(const member_function& t) const
{
  hash<bool> hash_bool;
  hash<size_t> hash_size_t;
  member_hash hash_member;
  function_decl_hash hash_fn;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_fn(t));
  v = hashing::combine_hashes(v, hash_bool(t.is_static()));
  v = hashing::combine_hashes(v, hash_bool(t.is_constructor()));
  v = hashing::combine_hashes(v, hash_bool(t.is_const()));

  if (!t.is_static() && !t.is_constructor())
    v = hashing::combine_hashes(v,
				hash_size_t(t.get_vtable_offset_in_bits()));

  return v;
}

/// A hashing function for instances of class_decl.
size_t
class_decl_hash::operator()(const class_decl& t) const
{
  hash<string> hash_string;
  scope_type_decl_hash hash_scope_type;
  class_decl::base_spec_hash hash_base;
  class_decl::member_type_hash hash_member_type;
  class_decl::data_member_hash hash_data_member;
  class_decl::member_function_hash hash_member_fn;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_scope_type(t));

  // Hash bases.
  for (std::list<shared_ptr<class_decl::base_spec> >::const_iterator b =
	 t.get_base_specifiers().begin();
       b != t.get_base_specifiers().end();
       ++b)
    v = hashing::combine_hashes(v, hash_base(**b));

  // Hash member types.
  for (std::list<shared_ptr<class_decl::member_type> >::const_iterator ti =
	 t.get_member_types().begin();
       ti != t.get_member_types().end();
       ++ti)
    v = hashing::combine_hashes(v, hash_member_type(**ti));

  // Hash data members.
  for (std::list<shared_ptr<class_decl::data_member> >::const_iterator d =
	 t.get_data_members().begin();
       d != t.get_data_members().end();
       ++d)
    v = hashing::combine_hashes(v, hash_data_member(**d));

  // Hash member_function
  for (std::list<shared_ptr<class_decl::member_function> > ::const_iterator f =
	 t.get_member_functions().begin();
       f != t.get_member_functions().end();
       ++f)
    v = hashing::combine_hashes(v, hash_member_fn(**f));

  return v;
}

// </class_decl>

// <template_decl stuff>

template_decl::~template_decl()
{
}

bool
template_decl::operator==(const template_decl& o) const
{
  if (typeid(*this) != typeid(o))
    return false;

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

size_t
template_decl_hash::operator()(const template_decl& t) const
{
  hash<string> hash_string;
  template_parameter_shared_ptr_hash hash_template_parameter;

  size_t v = hash_string(typeid(t).name());

  for (list<shared_ptr<template_parameter> >::const_iterator p =
	 t.get_template_parameters().begin();
       p != t.get_template_parameters().end();
       ++p)
    {
      v = hashing::combine_hashes(v, hash_template_parameter(*p));
    }
  return v;
}

// </template_decl stuff>

//<template_parameter>

bool
template_parameter::operator==(const template_parameter& o) const
{
  return (get_index() == o.get_index());
}

template_parameter::~template_parameter()
{
}

size_t
template_parameter_hash::operator()(const template_parameter& t) const
{
  hash<unsigned> hash_unsigned;
  hash<std::string> hash_string;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_unsigned(t.get_index()));

  return v;
}

size_t
dynamic_template_parameter_hash::operator()(const template_parameter* t) const
{
  if (const template_template_parameter* p =
      dynamic_cast<const template_template_parameter*>(t))
    return template_template_parameter_hash()(*p);
  else if (const template_type_parameter* p =
	   dynamic_cast<const template_type_parameter*>(t))
    return template_type_parameter_hash()(*p);
  if (const template_non_type_parameter* p =
      dynamic_cast<const template_non_type_parameter*>(t))
    return template_non_type_parameter_hash()(*p);

  // Poor man's fallback.
  return template_parameter_hash()(*t);
}

bool
template_type_parameter::operator==(const template_type_parameter& o) const
{
  return (static_cast<template_parameter>(*this) == o);
}

template_type_parameter::~template_type_parameter()
{
}

size_t
template_type_parameter_hash::operator()(const template_type_parameter& t) const
{
  hash<string> hash_string;
  template_parameter_hash hash_template_parameter;
  type_decl_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_template_parameter(t));
  v = hashing::combine_hashes(v, hash_type(t));

  return v;
}

bool
template_non_type_parameter::operator==
(const template_non_type_parameter& o) const
{
  return (static_cast<template_parameter>(*this) == o
      && *get_type() == *o.get_type());
}

template_non_type_parameter::~template_non_type_parameter()
{
}

size_t
template_non_type_parameter_hash::operator()
  (const template_non_type_parameter& t) const
{
  template_parameter_hash hash_template_parameter;
  hash<string> hash_string;
  type_shared_ptr_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_template_parameter(t));
  v = hashing::combine_hashes(v, hash_string(t.get_name()));
  v = hashing::combine_hashes(v, hash_type(t.get_type()));

  return v;
}

bool
template_template_parameter::operator==
(const template_template_parameter& o) const
{
  return (static_cast<template_type_parameter>(*this) == o
	  && (static_cast<template_decl>(*this) == o));
}

template_template_parameter::~template_template_parameter()
{
}

size_t
template_template_parameter_hash::operator()
  (const template_template_parameter& t) const
{
  hash<string> hash_string;
  template_type_parameter_hash hash_template_type_parm;
  template_decl_hash hash_template_decl;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_template_type_parm(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));

  return v;
}

//</template_parameter>

// <function_template>
bool
function_template_decl::operator==(const function_template_decl& o) const
{
  return (get_binding() == o.get_binding()
	  && static_cast<template_decl>(*this) == o
	  && static_cast<scope_decl>(*this) == o
	  && get_pattern() == o.get_pattern());
}

size_t
function_template_decl_hash::operator()
(const function_template_decl& t) const
{
  hash<string> hash_string;
  decl_base_hash hash_decl_base;
  template_decl_hash hash_template_decl;
  function_decl_hash hash_function_decl;

  size_t v = hash_string(typeid(t).name());

  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  v = hashing::combine_hashes(v, hash_function_decl(*t.get_pattern()));

  return v;
}

size_t
fn_tmpl_shared_ptr_hash::operator()
(const shared_ptr<function_template_decl> f) const
{
  function_template_decl_hash hash_fn_tmpl_decl;
  if (f)
    return hash_fn_tmpl_decl(*f);
  return 0;
}

function_template_decl::~function_template_decl()
{
}
// </function_template>
}//end namespace abigail
