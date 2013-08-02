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
using std::vector;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;

namespace abigail
{

/// @brief the location of a token represented in its simplest form.
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

  expanded_location(const string& path, unsigned line, unsigned column)
  : m_path(path), m_line(line), m_column(column)
  { }

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

struct location_manager::_Impl
{
  /// This sorted vector contains the expanded locations of the tokens
  /// coming from a given ABI Corpus.  The index of a given expanded
  /// location in the table gives us an integer that is used to build
  /// instance of location types.
  std::vector<expanded_location> 	locs;
};

location_manager::location_manager()
{
  m_priv = shared_ptr<location_manager::_Impl>(new location_manager::_Impl);
}

location
location_manager::create_new_location(const std::string&	file_path,
				      size_t			line,
				      size_t			col)
{
  expanded_location l(file_path, line, col);

  // Just append the new expanded location to the end of the vector
  // and return its index.  Note that indexes start at 1.  This way
  // the returning location number is doesn't grow monotonically
  // with respect to the expanded locations, but it has the advantage
  // to keep the location numbers stable accross the filling of the
  // expanded location vector.
  m_priv->locs.push_back(l);
  return location(m_priv->locs.size());
}

void
location_manager::expand_location(const location	location,
				  std::string&		path,
				  unsigned&		line,
				  unsigned&		column) const
{
  if (location.m_value == 0)
    return;
  expanded_location &l = m_priv->locs[location.m_value - 1];
  path = l.m_path;
  line = l.m_line;
  column = l.m_column;
}

translation_unit::translation_unit(const std::string& path)
  : m_path (path)
{
}

const shared_ptr<global_scope>
translation_unit::get_global_scope() const
{
  if (!m_global_scope)
    m_global_scope.reset(new global_scope(const_cast<translation_unit*>(this)));
  return m_global_scope;
}

const std::string&
translation_unit::get_path() const
{ return m_path; }

location_manager&
translation_unit::get_loc_mgr()
{ return m_loc_mgr; }

const location_manager&
translation_unit::get_loc_mgr() const
{ return m_loc_mgr; }

bool
translation_unit::is_empty() const
{ return get_global_scope()->is_empty(); }


void
translation_unit::traverse(ir_node_visitor& v)
{ get_global_scope()->traverse(v); }

translation_unit::~translation_unit()
{ }

// <Decl definition>

decl_base::decl_base(const std::string&	name, location locus,
		     const std::string&	mangled_name, visibility vis)
: m_location(locus),
  m_name(name),
  m_mangled_name(mangled_name),
  m_context(0),
  m_visibility(vis)
{ }

decl_base::decl_base(location l)
: m_location(l),
  m_context(0),
  m_visibility(VISIBILITY_DEFAULT)
{ }

decl_base::decl_base(const decl_base& d)
{
  m_location = d.m_location;
  m_name = d.m_name;
  m_mangled_name = d.m_mangled_name;
  m_context = d.m_context;
  m_visibility = m_visibility;
}

bool
decl_base::operator==(const decl_base& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return get_name() == other.get_name();
}

decl_base::~decl_base()
{ }

void
decl_base::set_scope(scope_decl* scope)
{ m_context = scope; }

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

void
scope_decl::add_member_decl(const shared_ptr<decl_base> member)
{
  m_members.push_back(member);

  if (shared_ptr<scope_decl> m = dynamic_pointer_cast<scope_decl>(member))
    m_member_scopes.push_back(m);
}

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

void
scope_decl::traverse(ir_node_visitor &v)
{
  v.visit(*this);

  std::list<shared_ptr<decl_base> >::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse (v);
    }
}

scope_decl::~scope_decl()
{ }

void
add_decl_to_scope(shared_ptr<decl_base> decl, scope_decl* scope)
{
  if (scope && decl && !decl->get_scope())
    {
      scope->add_member_decl (decl);
      decl->set_scope(scope);
    }
}

void
add_decl_to_scope(shared_ptr<decl_base> decl, shared_ptr<scope_decl> scope)
{ add_decl_to_scope(decl, scope.get()); }

global_scope*
get_global_scope(const shared_ptr<decl_base> decl)
{
  if (shared_ptr<global_scope> s = dynamic_pointer_cast<global_scope>(decl))
    return s.get();

  scope_decl* scope = decl->get_scope();
  while (scope && !dynamic_cast<global_scope*>(scope))
    scope = scope->get_scope();

  return scope ? dynamic_cast<global_scope*> (scope) : 0;
}

translation_unit*
get_translation_unit(const shared_ptr<decl_base> decl)
{
  global_scope* global = get_global_scope(decl);

  if (global)
    return global->get_translation_unit();

  return 0;
}

bool
is_global_scope(const shared_ptr<scope_decl>scope)
{
  return !!dynamic_pointer_cast<global_scope>(scope);
}

bool
is_global_scope(const scope_decl* scope)
{
  return !!dynamic_cast<const global_scope*>(scope);
}

bool
is_at_global_scope(const shared_ptr<decl_base> decl)
{
  return (decl && is_global_scope(decl->get_scope()));
}

bool
is_at_class_scope(const shared_ptr<decl_base> decl)
{
  return (decl && dynamic_cast<class_decl*>(decl->get_scope()));
}

bool
is_at_template_scope(const shared_ptr<decl_base> decl)
{
  return (decl && dynamic_cast<template_decl*>(decl->get_scope()));
}

bool
is_template_parameter(const shared_ptr<decl_base> decl)
{
  return (decl && (dynamic_pointer_cast<template_type_parameter>(decl)
		   || dynamic_pointer_cast<template_non_type_parameter>(decl)
		   || dynamic_pointer_cast<template_template_parameter>(decl)));
}

bool
is_type(const shared_ptr<decl_base> decl)
{
  return decl && dynamic_pointer_cast<type_base>(decl);
}

bool
is_template_parm_composition_type(const shared_ptr<decl_base> decl)
{
  return (decl
	  && is_at_template_scope(decl)
	  && is_type(decl)
	  && !is_template_parameter(decl));
}

bool
is_function_template_pattern(const shared_ptr<decl_base> decl)
{
  return (decl
	  && dynamic_pointer_cast<function_decl>(decl)
	  && dynamic_cast<template_decl*>(decl->get_scope()));
}

bool
is_template_decl(const shared_ptr<decl_base> decl)
{
  return decl && dynamic_pointer_cast<template_decl>(decl);
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

bool
type_decl::operator==(const type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<decl_base>(*this) == other
	  && static_cast<type_base>(*this) == other);
}

void
type_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);
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
namespace_decl::namespace_decl(const std::string& name, location locus, 
			       visibility vis)
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

bool
namespace_decl::operator==(const namespace_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<scope_decl>(*this) == other);
}

void
namespace_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);

  std::list<shared_ptr<decl_base> >::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse (v);
    }
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

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

bool
qualified_type_def::operator==(const qualified_type_def& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other)
      || get_cv_quals() != other.get_cv_quals())
    return false;

  return *get_underlying_type() == *other.get_underlying_type();
}

void
qualified_type_def::traverse(ir_node_visitor& v)
{
  v.visit(*this);
}

qualified_type_def::~qualified_type_def()
{
}


char
qualified_type_def::get_cv_quals() const
{
  return m_cv_quals;
}

void
qualified_type_def::set_cv_quals(char cv_quals)
{
  m_cv_quals = cv_quals;
}


const shared_ptr<type_base>
qualified_type_def::get_underlying_type() const
{
  return m_underlying_type;
}


qualified_type_def::CV
operator| (qualified_type_def::CV lhs,
	   qualified_type_def::CV rhs)
{
  return static_cast<qualified_type_def::CV>
    (static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
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

size_t
dynamic_type_hash::operator()(const type_base* t) const
{
  if (t == 0)
    return 0;
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
  if (const method_type* d = dynamic_cast<const method_type*>(t))
    return method_type_hash()(*d);
  if (const function_type* d = dynamic_cast<const function_type*>(t))
    return function_type_hash()(*d);

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

void
pointer_type_def::traverse(ir_node_visitor& v)
{
  v.visit(*this);
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

void
reference_type_def::traverse(ir_node_visitor& v)
{
  v.visit(*this);
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

shared_ptr<type_base>
enum_type_decl::get_underlying_type() const
{ return m_underlying_type; }

const std::list<enum_type_decl::enumerator>&
enum_type_decl::get_enumerators() const
{ return m_enumerators; }

void
enum_type_decl::traverse(ir_node_visitor &v)
{ v.visit(*this); }

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{ }

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

typedef_decl::typedef_decl(const string&		name,
			   const shared_ptr<type_base>	underlying_type,
			   location			locus,
			   const std::string&		mangled_name,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    m_underlying_type(underlying_type)
{ }

bool
typedef_decl::operator==(const typedef_decl& other) const
{
  return (typeid(*this) == typeid(other)
	  && get_name() == other.get_name()
	  && *get_underlying_type() == *other.get_underlying_type());
}

shared_ptr<type_base>
typedef_decl::get_underlying_type() const
{
  return m_underlying_type;
}

void
typedef_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);
}

typedef_decl::~typedef_decl()
{ }

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

void
var_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);
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

// <function_type>

bool
function_type::operator==(const function_type& other) const
{
  if (!!m_return_type != !!other.m_return_type)
    return false;

  vector<shared_ptr<function_decl::parameter> >::const_iterator i,j;
  for (i = get_parameters().begin(),
	 j = other.get_parameters().begin();
       (i != get_parameters().end()
	&& j != other.get_parameters().end());
       ++i, ++j)
    if (**i != **j)
      return false;

  if ((i != get_parameters().end()
       || j != other.get_parameters().end()))
    return false;

  return true;
}

function_type::~function_type()
{
}

size_t
function_type_hash::operator()(const function_type& t) const
{
  hash<string> hash_string;
  type_shared_ptr_hash hash_type_ptr;
  function_decl::parameter_hash hash_parameter;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
  for (vector<shared_ptr<function_decl::parameter> >::const_iterator i =
	 t.get_parameters ().begin();
       i != t.get_parameters().end();
       ++i)
    v = hashing::combine_hashes(v, hash_parameter(**i));
  return v;
}

// </function_type>

// <method_type>

method_type::method_type
(shared_ptr<type_base> return_type,
 shared_ptr<class_decl> class_type,
 const std::vector<shared_ptr<function_decl::parameter> >& parms,
 size_t size_in_bits,
 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(return_type, parms, size_in_bits, alignment_in_bits)
{ set_class_type(class_type); }

method_type::method_type(shared_ptr<type_base> return_type,
			 shared_ptr<type_base> class_type,
			 const std::vector<shared_ptr<function_decl::parameter> >& parms,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(return_type, parms, size_in_bits, alignment_in_bits)
{
  set_class_type(dynamic_pointer_cast<class_decl>(class_type));
}

method_type::method_type(size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(size_in_bits, alignment_in_bits)
{ }


method_type::method_type(shared_ptr<class_decl> class_type,
			 size_t size_in_bits,
			 size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    function_type(size_in_bits, alignment_in_bits)
{ set_class_type(class_type); }

void
method_type::set_class_type(shared_ptr<class_decl> t)
{
  if (!t)
    return;

  function_decl::parameter p(t, "");
  if (m_class_type)
    {
      assert(!m_parms.empty());
      m_parms.erase(m_parms.begin());
    }
    m_class_type = t;
    m_parms.insert(m_parms.begin(),
		   shared_ptr<function_decl::parameter>
		   (new function_decl::parameter(t)));
}

/// The destructor of method_type
method_type::~method_type()
{ }

size_t
method_type_hash::operator()(const method_type& t) const
{
  hash<string> hash_string;
  type_shared_ptr_hash hash_type_ptr;
  function_decl::parameter_hash hash_parameter;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
  vector<shared_ptr<function_decl::parameter> >::const_iterator i =
    t.get_parameters ().begin();

  for (vector<shared_ptr<function_decl::parameter> >::const_iterator i =
	 t.get_parameters ().begin();
       i != t.get_parameters().end(); ++i)
    v = hashing::combine_hashes(v, hash_parameter(**i));

  return v;
}

// </method_type>
// <function_decl definitions>

function_decl::function_decl(const std::string&  name,
			     const std::vector<shared_ptr<parameter> >& parms,
			     shared_ptr<type_base> return_type,
			     size_t fptr_size_in_bits,
			     size_t fptr_align_in_bits,
			     bool declared_inline,
			     location locus,
			     const std::string& mangled_name,
			     visibility vis,
			     binding bind)
  : decl_base(name, locus, mangled_name, vis),
    m_type(new function_type(return_type, parms, fptr_size_in_bits,
			     fptr_align_in_bits)),
    m_declared_inline(declared_inline), m_binding(bind)
  { }

function_decl::function_decl(const std::string& name,
		shared_ptr<type_base> fn_type,
		bool	declared_inline,
		location locus,
		const std::string& mangled_name,
		visibility vis,
		binding bind)
  : decl_base(name, locus, mangled_name, vis),
    m_type(dynamic_pointer_cast<function_type>(fn_type)),
    m_declared_inline(declared_inline),
    m_binding(bind)
  { }

const shared_ptr<function_type>
function_decl::get_type() const
{ return m_type; }

const shared_ptr<type_base>
function_decl::get_return_type() const
{ return m_type->get_return_type();}


const std::vector<shared_ptr<function_decl::parameter> >&
function_decl::get_parameters() const
{ return m_type->get_parameters(); }

void
function_decl::append_parameter(shared_ptr<parameter> parm)
{ m_type->append_parameter(parm); }

void
function_decl::append_parameters(std::vector<shared_ptr<parameter> >& parms)
{
  for (std::vector<shared_ptr<parameter> >::const_iterator i = parms.begin();
       i != parms.end();
       ++i)
    m_type->get_parameters().push_back(*i);
}

size_t
function_decl::parameter_hash::operator()
  (const function_decl::parameter& p) const
{
  type_shared_ptr_hash hash_type_ptr;
  hash<bool> hash_bool;
  size_t v = hash_type_ptr(p.get_type());
  v = hashing::combine_hashes(v, hash_bool(p.get_variadic_marker()));
  return v;
}

bool
function_decl::operator==(const function_decl& o) const
{

  if (!(static_cast<decl_base>(*this) == o))
    return false;

  // Compare function types
  shared_ptr<function_type> t0 = get_type(), t1 = o.get_type();
  if ((t0 && t1 && *t0 != *t1)
      || !!t0 != !!t1)
    return false;

  // Compare the remaining properties
  if (is_declared_inline() != o.is_declared_inline()
      || get_binding() != o.get_binding())
    return false;

  return true;
}

size_t
function_decl_hash::operator()(const function_decl& t) const
{
  hash<int> hash_int;
  hash<bool> hash_bool;
  hash<string> hash_string;
  decl_base_hash hash_decl_base;
  type_shared_ptr_hash hash_type_ptr;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));
  v = hashing::combine_hashes(v, hash_bool(t.is_declared_inline()));
  v = hashing::combine_hashes(v, hash_int(t.get_binding()));

  return v;
}

void
function_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);
}

function_decl::~function_decl()
{
}

// <function_decl definitions>

// <class_decl definitions>

class_decl::class_decl(const std::string& name, size_t size_in_bits,
		       size_t align_in_bits, location	locus,
		       visibility vis,
		       std::list<shared_ptr<base_spec> >& bases,
		       std::list<shared_ptr<member_type> >& member_types,
		       std::list<shared_ptr<data_member> >& data_members,
		       std::list<shared_ptr<member_function> >&	member_fns)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
  scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
  m_hashing_started(false),
  m_is_declaration_only(false),
  m_bases(bases),
  m_member_types(member_types),
  m_data_members(data_members),
  m_member_functions(member_fns)
{
  for (member_types_type::iterator i = member_types.begin();
       i != member_types.end();
       ++i)
    if (!(*i)->get_scope())
      add_decl_to_scope(*i, this);

  for (data_members_type::iterator i = data_members.begin();
       i != data_members.end();
       ++i)
    if (!(*i)->get_scope())
      add_decl_to_scope(*i, this);

  for (member_functions_type::iterator i = member_fns.begin();
       i != member_fns.end();
       ++i)
    if (!(*i)->get_scope())
      add_decl_to_scope(*i, this);

}

class_decl::class_decl(const std::string& name, size_t size_in_bits, 
		       size_t align_in_bits, location locus, visibility vis)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
    scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
    m_hashing_started(false),
    m_is_declaration_only(false)
{ }

class_decl::class_decl(const std::string& name, bool is_declaration_only)
  : decl_base(name, location(), name),
    type_base(0, 0),
    scope_type_decl(name, 0, 0, location()),
    m_hashing_started(false),
    m_is_declaration_only(is_declaration_only)
{ }

void
class_decl::set_earlier_declaration(shared_ptr<class_decl> declaration)
{
  if (declaration && declaration->is_declaration_only())
    m_declaration = declaration;
}


void
class_decl::set_earlier_declaration(shared_ptr<type_base> declaration)
{
  shared_ptr<class_decl> d = dynamic_pointer_cast<class_decl>(declaration);
  set_earlier_declaration(d);
}

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


class_decl::base_spec::base_spec(shared_ptr<class_decl> base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : member(a),
    m_base_class(base),
    m_offset_in_bits(offset_in_bits),
    m_is_virtual(is_virtual)
{ }


class_decl::base_spec::base_spec(shared_ptr<type_base> base,
				 access_specifier a,
				 long offset_in_bits,
				 bool is_virtual)
  : member(a),
    m_base_class(dynamic_pointer_cast<class_decl>(base)),
    m_offset_in_bits(offset_in_bits),
    m_is_virtual(is_virtual)
{ }


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


class_decl::method_decl::method_decl(const std::string&			name,
 const std::vector<shared_ptr<parameter> >& parms,
 shared_ptr<type_base>		return_type,
 shared_ptr<class_decl>		class_type,
 size_t				ftype_size_in_bits,
 size_t				ftype_align_in_bits,
 bool				declared_inline,
 location				locus,
  const std::string&			mangled_name,
 visibility				vis,
 binding				bind)
  : decl_base(name, locus, mangled_name, vis),
	function_decl(name,
		      shared_ptr<function_type>
		      (new method_type(return_type, class_type, parms,
				       ftype_size_in_bits,
				       ftype_align_in_bits)),
		      declared_inline, locus, mangled_name, vis, bind)
{
}


class_decl::method_decl::method_decl
(const std::string&			name,
 shared_ptr<method_type>		type,
 bool					declared_inline,
 location				locus,
 const std::string&			mangled_name,
 visibility				vis,
 binding				bind)
  : decl_base(name, locus, mangled_name, vis),
    function_decl(name, static_pointer_cast<function_type>(type),
		  declared_inline, locus,
		  mangled_name, vis, bind)
{ }


class_decl::method_decl::method_decl(const std::string&	name,
				     shared_ptr<function_type>	type,
				     bool			declared_inline,
				     location			locus,
			const std::string&			mangled_name,
			visibility				vis,
			binding				bind)
  : decl_base(name, locus, mangled_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, mangled_name, vis, bind)
{ }

class_decl::method_decl::method_decl(const std::string&	name,
				     shared_ptr<type_base>	type,
				     bool			declared_inline,
				     location			locus,
				     const std::string&	mangled_name,
				     visibility		vis,
				     binding			bind)
  : decl_base(name, locus, mangled_name, vis),
    function_decl(name, static_pointer_cast<function_type>
		  (dynamic_pointer_cast<method_type>(type)),
		  declared_inline, locus, mangled_name, vis, bind)
{ }

class_decl::method_decl::~method_decl()
{ }

const shared_ptr<method_type>
class_decl::method_decl::get_type() const
{
  return dynamic_pointer_cast<method_type>(m_type);
}


class_decl::member_function::member_function
(shared_ptr<function_decl>	fn,
 access_specifier		access,
 size_t			vtable_offset_in_bits,
 bool			is_static,
 bool			is_constructor,
 bool			is_destructor,
 bool			is_const)
  : decl_base(fn->get_name(), fn->get_location(),
		    fn->get_mangled_name(), fn->get_visibility()),
    method_decl(fn->get_name(),
		dynamic_pointer_cast<method_decl>(fn)->get_type(),
		fn->is_declared_inline(),
		fn->get_location(),
		fn->get_mangled_name(),
		fn->get_visibility(),
		fn->get_binding()),
    member(access, is_static),
  m_vtable_offset_in_bits(vtable_offset_in_bits),
  m_is_constructor(is_constructor),
  m_is_destructor(is_destructor),
  m_is_const(is_const)
{ }

void
class_decl::member_function::traverse(ir_node_visitor& v)
{
  v.visit(*this);
}

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

void
class_decl::add_member_function_template
(shared_ptr<member_function_template> m)
{
  decl_base* c = m->as_function_template_decl()->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c || c == this);
  if (!c)
    add_decl_to_scope(m->as_function_template_decl(), this);

  m_member_function_templates.push_back(m);
}

void
class_decl::add_member_class_template(shared_ptr<member_class_template> m)
{
    decl_base* c = m->as_class_template_decl()->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c || c == this);
  if (!c)
    add_decl_to_scope(m->as_class_template_decl(), this);

  m_member_class_templates.push_back(m);
}

bool
class_decl::has_no_base_nor_member() const
{
  return (m_bases.empty()
	  && m_member_types.empty()
	  && m_data_members.empty()
	  && m_member_functions.empty()
	  && m_member_function_templates.empty()
	  && m_member_class_templates.empty());
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
       f0 != get_member_functions().end()
	 && f1 != o.get_member_functions().end();
       ++f0, ++f1)
    if (**d0 != **d1)
      return false;
  if (f0 != get_member_functions().end()
      || f1 != o.get_member_functions().end())
    return false;

  // compare member function templates
  member_function_templates_type::const_iterator fn_tmpl_it0, fn_tmpl_it1;
  for (fn_tmpl_it0 = get_member_function_templates().begin(),
	 fn_tmpl_it1 = o.get_member_function_templates().begin();
       fn_tmpl_it0 != get_member_function_templates().end()
	 &&  fn_tmpl_it1 != o.get_member_function_templates().end();
       ++fn_tmpl_it0, ++fn_tmpl_it1)
    if (**fn_tmpl_it0 != **fn_tmpl_it1)
      return false;
  if (fn_tmpl_it0 != get_member_function_templates().end()
      || fn_tmpl_it1 != o.get_member_function_templates().end())
    return false;

  // compare member class templates
  member_class_templates_type::const_iterator cl_tmpl_it0, cl_tmpl_it1;
  for (cl_tmpl_it0 = get_member_class_templates().begin(),
	 cl_tmpl_it1 = o.get_member_class_templates().begin();
       cl_tmpl_it0 != get_member_class_templates().end()
	 &&  cl_tmpl_it1 != o.get_member_class_templates().end();
       ++cl_tmpl_it0, ++cl_tmpl_it1)
    if (**cl_tmpl_it0 != **cl_tmpl_it1)
      return false;
  if (cl_tmpl_it0 != get_member_class_templates().end()
      || cl_tmpl_it1 != o.get_member_class_templates().end())
    return false;

  return true;
}

void
class_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);

  for (member_types_type::const_iterator i = get_member_types().begin();
       i != get_member_types().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_function_templates_type::const_iterator i =
	 get_member_function_templates().begin();
       i != get_member_function_templates().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_class_templates_type::const_iterator i =
	 get_member_class_templates().begin();
       i != get_member_class_templates().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse(v);
    }

  for (data_members_type::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_functions_type::const_iterator i= get_member_functions().begin();
       i != get_member_functions().end();
       ++i)
    {
      shared_ptr<traversable> t = dynamic_pointer_cast<traversable>(*i);
      if (t)
	t->traverse(v);
    }
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

void
class_decl::data_member::traverse(ir_node_visitor& v)
{
  v.visit(*this);
}

class_decl::data_member::~data_member()
{ }

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
  v = hashing::combine_hashes(v, hash_bool(t.is_constructor()));
  v = hashing::combine_hashes(v, hash_bool(t.is_const()));

  if (!t.is_static() && !t.is_constructor())
    v = hashing::combine_hashes(v,
				hash_size_t(t.get_vtable_offset_in_bits()));

  return v;
}

bool
class_decl::member_function_template::operator==
(const member_function_template& o) const
{
  if (!(is_constructor() == o.is_constructor()
	&& is_const() == o.is_const()
	&& static_cast<member>(*this) == o))
    return false;

  if (as_function_template_decl())
    return static_cast<function_template_decl>(*this) == o;

  return true;
}

void
class_decl::member_function_template::traverse(ir_node_visitor& v)
{
  v.visit(*this);
  as_function_template_decl()->traverse(v);
}

size_t
class_decl::member_function_template_hash::operator()
(const member_function_template& t) const
{
  hash<bool> hash_bool;
  function_template_decl_hash hash_function_template_decl;
  member_hash hash_member;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_function_template_decl(t));
  v = hashing::combine_hashes(v, hash_bool(t.is_constructor()));
  v = hashing::combine_hashes(v, hash_bool(t.is_const()));

  return v;
}

bool
class_decl::member_class_template::operator==
(const member_class_template& o) const
{
  if (!(static_cast<member>(*this) == o))
    return false;

  if (as_class_template_decl())
    return static_cast<class_template_decl>(*this) == o;

  return true;
}

void
class_decl::member_class_template::traverse(ir_node_visitor& v)
{
  v.visit(*this);
  as_class_template_decl()->get_pattern()->traverse(v);
}

size_t
class_decl::member_class_template_hash::operator()
(member_class_template& t) const
{
  member_hash hash_member;
  class_template_decl_hash hash_class_template_decl;

  size_t v = hash_member(t);
  v = hashing::combine_hashes(v, hash_class_template_decl(t));
  return v;
}

/// A hashing function for instances of class_decl.
size_t
class_decl_hash::operator()(const class_decl& t) const
{
  if (t.hashing_started())
    return 0;

  t.hashing_started(true);

  hash<string> hash_string;
  scope_type_decl_hash hash_scope_type;
  class_decl::base_spec_hash hash_base;
  class_decl::member_type_hash hash_member_type;
  class_decl::data_member_hash hash_data_member;
  class_decl::member_function_hash hash_member_fn;
  class_decl::member_function_template_hash hash_member_fn_tmpl;
  class_decl::member_class_template_hash hash_member_class_tmpl;

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

  // Hash member function templates
  for (class_decl::member_function_templates_type::const_iterator f =
	 t.get_member_function_templates().begin();
       f != t.get_member_function_templates().end();
       ++f)
    v = hashing::combine_hashes(v, hash_member_fn_tmpl(**f));

  // Hash member class templates
  for (class_decl::member_class_templates_type::const_iterator c =
	 t.get_member_class_templates().begin();
       c != t.get_member_class_templates().end();
       ++c)
    v = hashing::combine_hashes(v, hash_member_class_tmpl(**c));

  t.hashing_started(false);

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

tmpl_parm_type_composition::tmpl_parm_type_composition
(unsigned		index,
 shared_ptr<type_base>	t)
  : decl_base("", location()),
    template_parameter(index),
    m_type(std::tr1::dynamic_pointer_cast<type_base>(t))
{
}

tmpl_parm_type_composition::~tmpl_parm_type_composition()
{
}

//</template_parameter>

// <function_template>
bool
function_template_decl::operator==(const function_template_decl& o) const
{
  if (!(get_binding() == o.get_binding()
	&& static_cast<template_decl>(*this) == o
	&& static_cast<scope_decl>(*this) == o
	&& !!get_pattern() == !!o.get_pattern()))
    return false;

  if (get_pattern())
    return (*get_pattern() == *o.get_pattern());

  return true;
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
  if (t.get_pattern())
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

void
function_template_decl::traverse(ir_node_visitor&v)
{
  v.visit(*this);
  get_pattern()->traverse(v);
}

function_template_decl::~function_template_decl()
{
}
// </function_template>

// <class template>
class_template_decl::class_template_decl(shared_ptr<class_decl> pattern,
					 location locus,
					 visibility vis)
  : decl_base(pattern->get_name(), locus,
	      pattern->get_name(), vis),
    scope_decl(pattern->get_name(), locus)
{set_pattern(pattern);}

void
class_template_decl::set_pattern(shared_ptr<class_decl> p)
{
  m_pattern = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

bool
class_template_decl::operator==(const class_template_decl& o) const
{
  if (!(static_cast<template_decl>(*this) == o
	&& static_cast<scope_decl>(*this) == o
	&& !!get_pattern() == !!o.get_pattern()))
    return false;

  return (*get_pattern() == *o.get_pattern());
}

void
class_template_decl::traverse(ir_node_visitor&v)
{
  v.visit(*this);

  shared_ptr<class_decl> pattern = get_pattern();
  if (pattern)
    pattern->traverse(v);
}

class_template_decl::~class_template_decl()
{
}

size_t
class_template_decl_hash::operator()(const class_template_decl& t) const
{
  hash<string> hash_string;
  decl_base_hash hash_decl_base;
  template_decl_hash hash_template_decl;
  class_decl_hash hash_class_decl;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  if (t.get_pattern())
    v = hashing::combine_hashes(v, hash_class_decl(*t.get_pattern()));

  return v;
}

size_t
class_tmpl_shared_ptr_hash::operator()
(const shared_ptr<class_template_decl> t) const
{
  class_template_decl_hash hash_class_tmpl_decl;

  if (t)
    return hash_class_tmpl_decl(*t);
  return 0;
}

void
ir_node_visitor::visit(scope_decl&)
{
}

void
ir_node_visitor::visit(type_decl&)
{
}

void
ir_node_visitor::visit(namespace_decl&)
{
}

void
ir_node_visitor::visit(qualified_type_def&)
{
}

void
ir_node_visitor::visit(pointer_type_def&)
{
}

void
ir_node_visitor::visit(reference_type_def&)
{
}

void
ir_node_visitor::visit(enum_type_decl&)
{
}

void
ir_node_visitor::visit(typedef_decl&)
{
}

void
ir_node_visitor::visit(var_decl&){
}

void
ir_node_visitor::visit(function_decl&)
{
}

void
ir_node_visitor::visit(function_template_decl&)
{
}

void
ir_node_visitor::visit(class_template_decl&)
{
}

void
ir_node_visitor::visit(class_decl&)
{
}

void
ir_node_visitor::visit(class_decl::data_member&)
{
}

void
ir_node_visitor::visit(class_decl::member_function&)
{
}

void
ir_node_visitor::visit(class_decl::member_function_template&)
{
}

void
ir_node_visitor::visit(class_decl::member_class_template&)
{
}

// </class template>
}//end namespace abigail
