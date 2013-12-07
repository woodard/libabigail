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
//
//Author: Dodji Seketeli

/// @file

#include <assert.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <tr1/memory>
#include <tr1/unordered_map>
#include "abg-ir.h"

namespace abigail
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
  { }

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

struct location_manager::_Impl
{
  /// This sorted vector contains the expanded locations of the tokens
  /// coming from a given ABI Corpus.  The index of a given expanded
  /// location in the table gives us an integer that is used to build
  /// instance of location types.
  std::vector<expanded_location> 	locs;
};

location_manager::location_manager()
{priv_ = shared_ptr<location_manager::_Impl>(new location_manager::_Impl);}

/// Insert the triplet representing a source locus into our internal
/// vector of location triplet.  Return an instance of location type,
/// built from an integral type that represents the index of the
/// source locus triplet into our source locus table.
///
/// @param fle the file path of the source locus
/// @param lne the line number of the source location
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

typedef unordered_map<shared_ptr<type_base>,
		      bool,
		      type_base::shared_ptr_hash,
		      type_shared_ptr_equal> type_ptr_map;

/// Private type to hold private members of @ref translation_unit
struct translation_unit::priv
{
  std::string			path_;
  location_manager		loc_mgr_;
  mutable global_scope_sptr	global_scope_;
  type_ptr_map			canonical_types_;
}; // end translation_unit::priv

// <translation_unit stuff>

/// Constructor of translation_unit.
///
/// @param path the location of the translation unit.
translation_unit::translation_unit(const std::string& path)
  : priv_(new priv)
{priv_->path_ = path;}

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

/// If the current translation_unit "knows" about a type T' that is
/// equivalent to a given T, then this method returns T' when passed
/// T.  Otherwise, the function stores T, so that next time it sees a
/// T'', it can say that it is equivalent to T and then return that T.
///
/// In other words, this methods can be used to help enforce that if
/// we build two types that are equivalent at the same scope then we
/// can decide to just keep one.
///
/// @param t the type to canonicalize.
///
/// @return the canonical type for @ref t.
type_base_sptr
translation_unit::canonicalize_type(type_base_sptr t) const
{
  if (!t)
    return t;

  type_ptr_map::iterator e =
    priv_->canonical_types_.find(t);

  if (e == priv_->canonical_types_.end())
    {
      priv_->canonical_types_[t] = true;
      return t;
    }
  return e->first;
}

/// Canonicalize the type of a given type declaration.
///
/// To understand more about type canonicalization, please read the
/// API doc of the overload of this function that takes a @ref
/// type_base_sptr.
///
/// @param t the type declaration to return a canonical type
/// declaration for.
///
/// @return the declaration for the canonical type of the type for
/// @ref t.
decl_base_sptr
translation_unit::canonicalize_type(decl_base_sptr t) const
{
  type_base_sptr type = is_type(t);

  if (!type)
    return t;

  type = canonicalize_type(type);
  assert(type);

  return get_type_declaration(type);
}
/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
void
translation_unit::traverse(ir_node_visitor& v)
{get_global_scope()->traverse(v);}

translation_unit::~translation_unit()
{}

// </translation_unit stuff>

// <Decl definition>

decl_base::decl_base(const std::string&	name, location locus,
		     const std::string&	mangled_name, visibility vis)
: location_(locus),
  name_(name),
  mangled_name_(mangled_name),
  context_(0),
  visibility_(vis)
{ }

decl_base::decl_base(location l)
: location_(l),
  context_(0),
  visibility_(VISIBILITY_DEFAULT)
{ }

decl_base::decl_base(const decl_base& d)
{
  location_ = d.location_;
  name_ = d.name_;
  mangled_name_ = d.mangled_name_;
  context_ = d.context_;
  visibility_ = visibility_;
}

/// Compute the qualified name of the decl.
///
/// @param qn the resulting qualified name.
///
/// @param sep the separator used to separate the components of the
/// qualified name.
void
decl_base::get_qualified_name(string& qn,
			      const string& sep) const
{
  list<string> qn_components;

  qn_components.push_front(get_name());
  for (scope_decl* s = get_scope(); !is_global_scope(s); s = s->get_scope())
    qn_components.push_front(s->get_name());

  qn.clear();
  for (list<string>::const_iterator i = qn_components.begin();
       i != qn_components.end();
       ++i)
    if (i == qn_components.begin())
      qn += *i;
    else
      qn += sep + *i;
}

/// @return the default pretty representation for a decl.  This is
/// basically the fully qualified name of the decl optionally prefixed
/// with a meaningful string to add context for the user.
string
decl_base::get_pretty_representation() const
{return get_qualified_name();}

/// Compute the qualified name of the decl.
///
/// @param sep the separator used to separate the components of the
/// qualified name.
///
/// @return qn the resulting qualified name.
string
decl_base::get_qualified_name(const string& separator) const
{
  string result;
  get_qualified_name(result, separator);
  return result;
}

/// Return true iff the two decls have the same name.
///
/// This function doesn't test if the scopes of the the two decls are
/// equal.
///
/// Note that this virtual function is to be implemented by classes
/// that extend the \a decl_base class.
bool
decl_base::operator==(const decl_base& other) const
{return get_name() == other.get_name();}

decl_base::~decl_base()
{}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the member nodes of the translation
/// unit during the traversal.
void
decl_base::traverse(ir_node_visitor&)
{
  // Do nothing in the base class.
}

/// Setter of the scope of the current decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
void
decl_base::set_scope(scope_decl* scope)
{context_ = scope;}

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
      r = "non";
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
operator==(decl_base_sptr l, decl_base_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

// </decl_base definition>

/// Add a member decl to this scope.  Note that user code should not
/// use this, but rather use add_decl_to_scope.
///
/// @param member the new member decl to add to this scope.
void
scope_decl::add_member_decl(const shared_ptr<decl_base> member)
{
  members_.push_back(member);

  if (shared_ptr<scope_decl> m = dynamic_pointer_cast<scope_decl>(member))
    member_scopes_.push_back(m);
}

/// Return true iff both scopes have the same names and have the same
/// member decls.
///
/// This function doesn't check for equality of the scopes of its
/// arguments.
bool
scope_decl::operator==(const decl_base& o) const
{

  if (!decl_base::operator==(o))
    return false;

  try
    {
      const scope_decl& other = dynamic_cast<const scope_decl&>(o);

      scope_decl::declarations::const_iterator i, j;
      for (i = get_member_decls().begin(), j = other.get_member_decls().begin();
	   i != get_member_decls().end() && j != other.get_member_decls().end();
	   ++i, ++j)
	if (**i != **j)
	  return false;

      if (i != get_member_decls().end() || j != other.get_member_decls().end())
	return false;
    }
  catch(...)
    {return false;}

  return true;
}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance of scope_decl
/// and on its member nodes.
void
scope_decl::traverse(ir_node_visitor &v)
{
  v.visit(*this);

  scope_decl::declarations::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      shared_ptr<traversable_base> t =
	dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse (v);
    }
}

scope_decl::~scope_decl()
{}

/// Appends a declaration to a given scope, if the declaration
/// doesn't already belong to one.
///
/// @param decl the declaration to add to the scope
///
/// @param scope the scope to append the declaration to
void
add_decl_to_scope(shared_ptr<decl_base> decl, scope_decl* scope)
{
  if (scope && decl && !decl->get_scope())
    {
      scope->add_member_decl(decl);
      decl->set_scope(scope);
    }
}

/// Appends a declaration to a given scope, if the declaration doesn't
/// already belong to a scope.
///
/// @param decl the declaration to add append to the scope
///
/// @param scope the scope to append the decl to
void
add_decl_to_scope(shared_ptr<decl_base> decl, shared_ptr<scope_decl> scope)
{add_decl_to_scope(decl, scope.get());}

/// Return the global scope as seen by a given declaration.
///
/// @param decl the declaration to consider.
///
/// @return the global scope of the decl, or a null pointer if the
/// decl is not yet added to a translation_unit.
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

/// Get the declaration for a given type.
///
/// @param the type to consider.
///
/// @return the declaration for the type to return.
decl_base_sptr
get_type_declaration(const type_base_sptr t)
{return dynamic_pointer_cast<decl_base>(t);}

/// Return the translation unit a declaration belongs to.
///
/// @param decl the declaration to consider.
///
/// @return the resulting translation unit, or null if the decl is not
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
/// @param scope the scope to consider.
///
/// @return true iff the current scope is the global one.
bool
is_global_scope(const shared_ptr<scope_decl>scope)
{return !!dynamic_pointer_cast<global_scope>(scope);}

bool
is_global_scope(const scope_decl* scope)
{return !!dynamic_cast<const global_scope*>(scope);}

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

/// Tests whether a declaration is a type, and return it properly
/// converted into a type in that case.
///
/// @param decl the decl to consider.
///
/// @return the pointer to type_base representing @decl converted as a
/// type, iff it's a type, or NULL otherwise.
type_base_sptr
is_type(const decl_base_sptr decl)
{return dynamic_pointer_cast<type_base>(decl);}

/// Tests wheter a declaration is a variable declaration.
///
/// @param decl the decl to test.
///
/// @return true iff decl is a variable declaration.
bool
is_var_decl(const shared_ptr<decl_base> decl)
{return decl && dynamic_pointer_cast<var_decl>(decl);}

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

/// Tests whether a decl is a template.
///
/// @param decl the decl to consider.
///
/// @return true iff decl is a function template, class template, or
/// template template parameter.
bool
is_template_decl(const shared_ptr<decl_base> decl)
{return decl && dynamic_pointer_cast<template_decl>(decl);}

// </scope_decl definition>

global_scope::~global_scope()
{
}

// <type_base definitions>
type_base::type_base(size_t s, size_t a)
  : size_in_bits_(s),
    alignment_in_bits_(a)
{
}

/// Return true iff both type declarations are equal.
///
/// Note that this doesn't test if the scopes of both types are equal.
bool
type_base::operator==(const type_base& other) const
{
  return (get_size_in_bits() == other.get_size_in_bits()
	  && get_alignment_in_bits() == other.get_alignment_in_bits());
}

void
type_base::set_size_in_bits(size_t s)
{size_in_bits_ = s;}

size_t
type_base::get_size_in_bits() const
{return size_in_bits_;}

void
type_base::set_alignment_in_bits(size_t a)
{alignment_in_bits_ = a;}

size_t
type_base::get_alignment_in_bits() const
{return alignment_in_bits_;}

type_base::~type_base()
{}
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
/// This operator re-uses the overload that takes a decl_base.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check agains.
bool
type_decl::operator==(const type_base& o) const
{
  try
    {
      const decl_base& other = dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch (...)
    {return false;}
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
///
/// @param o the other type_decl to check against.
bool
type_decl::operator==(const decl_base& o) const
{
  try
    {
      const type_decl& other = dynamic_cast<const type_decl&>(o);
      return (type_base::operator==(other)
	      &&  decl_base::operator==(other));
    }
  catch(...)
    {return false;}
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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
type_decl::traverse(ir_node_visitor& v)
{v.visit(*this);}

type_decl::~type_decl()
{ }
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

/// Equality operator between two scope_type_decl.
///
/// Note that this function does not consider the scope of the scope
/// types themselves.
///
/// @return true iff both scope types are equal.
bool
scope_type_decl::operator==(const decl_base& o) const
{
  try
    {
      const scope_type_decl& other = dynamic_cast<const scope_type_decl&>(o);
      return (scope_decl::operator==(other)
	      && type_base::operator==(other));
    }
  catch (...)
    {return false;}
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
  try
    {
      const decl_base& other = dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch(...)
    {return false;}
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
  try
    {
      const namespace_decl& other = dynamic_cast<const namespace_decl&>(o);
      return scope_decl::operator==(other);
    }
  catch(...)
    {return false;}
}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// member nodes.
void
namespace_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);

  scope_decl::declarations::const_iterator i;
  for (i = get_member_decls().begin();
       i != get_member_decls ().end();
       ++i)
    {
      shared_ptr<traversable_base> t =
	dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse (v);
    }
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

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
    cv_quals_(quals),
    underlying_type_(type)
{
  if (quals & qualified_type_def::CV_CONST)
    set_name(get_name() + "const ");
  if (quals & qualified_type_def::CV_VOLATILE)
    set_name(get_name() + "volatile ");
  set_name(get_name() + dynamic_pointer_cast<decl_base>(type)->get_name());
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
  try
    {
      const qualified_type_def& other =
	dynamic_cast<const qualified_type_def&>(o);

      if (get_cv_quals() != other.get_cv_quals())
	return false;

      return *get_underlying_type() == *other.get_underlying_type();
    }
  catch(...)
    {return false;}
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
  try
    {
      const decl_base& other = dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch(...)
    {return false;}
}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
qualified_type_def::traverse(ir_node_visitor& v)
{v.visit(*this);}

qualified_type_def::~qualified_type_def()
{
}

/// Getter of the const/volatile qualifier bit field
char
qualified_type_def::get_cv_quals() const
{return cv_quals_;}

/// Setter of the const/value qualifiers bit field
void
qualified_type_def::set_cv_quals(char cv_quals)
{cv_quals_ = cv_quals;}

/// Getter of the underlying type
const shared_ptr<type_base>
qualified_type_def::get_underlying_type() const
{return underlying_type_;}

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

pointer_type_def::pointer_type_def(shared_ptr<type_base>&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   location			locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus, "",
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    pointed_to_type_(pointed_to)
{
  try
    {
      decl_base_sptr pto = dynamic_pointer_cast<decl_base>(pointed_to);
      string name = pto->get_name() + "*";
      set_name(name);
    }
  catch (...)
    {}
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const decl_base& o) const
{
  try
    {
      const pointer_type_def& other = dynamic_cast<const pointer_type_def&>(o);

      return *get_pointed_to_type() == *other.get_pointed_to_type();
    }
  catch(...)
    {return false;}
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const type_base& o) const
{
  try
    {
      const pointer_type_def& other = dynamic_cast<const pointer_type_def&>(o);

      return *get_pointed_to_type() == *other.get_pointed_to_type();
    }
  catch(...)
    {return false;}
}

shared_ptr<type_base>
pointer_type_def::get_pointed_to_type() const
{return pointed_to_type_;}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
pointer_type_def::traverse(ir_node_visitor& v)
{v.visit(*this);}

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
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    pointed_to_type_(pointed_to),
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

bool
reference_type_def::operator==(const decl_base& o) const
{
  try
    {
      const reference_type_def& other =
	dynamic_cast<const reference_type_def&>(o);
      return *get_pointed_to_type() == *other.get_pointed_to_type();
    }
  catch(...)
    {return false;}
}

bool
reference_type_def::operator==(const type_base& o) const
{
  try
    {
      const decl_base& other = dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch(...)
    {return false;}
}

shared_ptr<type_base>
reference_type_def::get_pointed_to_type() const
{return pointed_to_type_;}

bool
reference_type_def::is_lvalue() const
{return is_lvalue_;}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
reference_type_def::traverse(ir_node_visitor& v)
{v.visit(*this);}

reference_type_def::~reference_type_def()
{}

// </reference_type_def definitions>

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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
enum_type_decl::traverse(ir_node_visitor &v)
{v.visit(*this);}

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{}

/// Equality operator.
///
/// @param other the other enum to test against.
///
/// @return true iff other is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const decl_base& o) const
{
  try
    {
      const enum_type_decl& other= dynamic_cast<const enum_type_decl&>(o);

      if (*get_underlying_type() != *other.get_underlying_type())
	return false;

      enumerators::const_iterator i, j;
      for (i = get_enumerators().begin(), j = other.get_enumerators().begin();
	   i != get_enumerators().end() && j != other.get_enumerators().end();
	   ++i, ++j)
	if (*i != *j)
	  return false;

      if (i != get_enumerators().end() || j != other.get_enumerators().end())
	return false;

      return decl_base::operator==(other) && type_base::operator==(other);
    }
  catch(...)
    {return false;}
}

bool
enum_type_decl::operator==(const type_base& o) const
{
  try
    {
      const decl_base& other= dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch(...)
    {return false;}
}

// <typedef_decl definitions>

/// Constructor of the typedef_decl type.
///
/// @param name the name of the typedef.
///
/// @param underlying_type the underlying type of the typedef.
///
/// @param locus the source location of the typedef declaration.
typedef_decl::typedef_decl(const string&		name,
			   const shared_ptr<type_base>	underlying_type,
			   location			locus,
			   const std::string&		mangled_name,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    underlying_type_(underlying_type)
{}

/// Equality operator
///
/// @param other the other typedef_decl to test against.
bool
typedef_decl::operator==(const decl_base& o) const
{
  try
    {
      const typedef_decl& other = dynamic_cast<const typedef_decl&>(o);
      return (*get_underlying_type() == *other.get_underlying_type());
    }
  catch(...)
    {return false;}
}

/// Equality operator
///
/// @param other the other typedef_decl to test against.
bool
typedef_decl::operator==(const type_base& o) const
{
  try
    {
      const decl_base& other = dynamic_cast<const decl_base&>(o);
      return *this == other;
    }
  catch(...)
    {return false;}
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
shared_ptr<type_base>
typedef_decl::get_underlying_type() const
{return underlying_type_;}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
typedef_decl::traverse(ir_node_visitor& v)
{v.visit(*this);}

typedef_decl::~typedef_decl()
{}
// </typedef_decl definitions>

// <var_decl definitions>

var_decl::var_decl(const std::string&		name,
		   shared_ptr<type_base>	type,
		   location			locus,
		   const std::string&		mangled_name,
		   visibility			vis,
		   binding			bind)
  : decl_base(name, locus, mangled_name, vis),
    type_(type),
    binding_(bind)
{}

bool
var_decl::operator==(const decl_base& o) const
{
  try
    {
      const var_decl& other = dynamic_cast<const var_decl&>(o);
      return (decl_base::operator==(other)
	      && *get_type() == *other.get_type());
    }
  catch(...)
    {return false;}
}

/// Build and return the pretty representation of this variable.
///
/// @return a copy of the pretty representation of this variable.
string
var_decl::get_pretty_representation() const
{
  string result;

  result =
    dynamic_pointer_cast<decl_base>(get_type())->get_qualified_name();
  result += " " + get_qualified_name();
  return result;
}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
var_decl::traverse(ir_node_visitor& v)
{v.visit(*this);}

var_decl::~var_decl()
{}

// </var_decl definitions>

// <function_type>

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
///@return true if lhs == rhs, false otherwise.
static bool
compare_function_types(const function_type& lhs,
		       const function_type&rhs)
{
  if (!lhs.type_base::operator==(rhs))
    return false;

  if (!!lhs.get_return_type() != !!rhs.get_return_type())
    return false;

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

  class_decl_sptr lcl, rcl;
  vector<shared_ptr<function_decl::parameter> >::const_iterator i,j;
  for (i = lhs.get_parameters().begin(),
	 j = rhs.get_parameters().begin();
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
	return false;
    }

  if ((i != lhs.get_parameters().end()
       || j != rhs.get_parameters().end()))
    return false;

  return true;
}

/// Equality operator for function_type.
///
/// @param o the other function_type to compare against.
///
/// @return true iff the two function_type are equal.
bool
function_type::operator==(const type_base& o) const
{
 try
   {
     const function_type& other = dynamic_cast<const function_type&>(o);
     return compare_function_types(*this, other);
   }
 catch (...)
   {return false;}
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
/// @param type the underlying type
///
/// @param quals a bitfield representing the const/volatile qualifiers
///
/// @param locus the location of the qualified type definition
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

  function_decl::parameter p(t, 0, "");
  if (class_type_)
    assert(!parms_.empty());
    class_type_ = t;
}

/// The destructor of method_type
method_type::~method_type()
{}

// </method_type>

// <function_decl definitions>

  /// Constructor for function_decl.
  ///
  /// This constructor builds the necessary function_type on behalf of
  /// the client, so it takes parameters -- like the return types, the
  /// function parameters and the size/alignment of the pointer to the
  /// type of the function --  necessary to build the function_type
  /// under the hood.
  ///
  /// If the client code already has the function_type at hand, it
  /// should instead the other constructor that takes the function_decl.
  ///
  /// @param name the name of the function declaration.
  ///
  /// @param parms a vector of parameters of the function.
  ///
  /// @param return_type the return type of the function.
  ///
  /// @param fptr_size_in_bits the size of the type of this function, in
  /// bits.
  ///
  /// @param fptr_align_in_bits the alignment of the type of this
  /// function.
  ///
  /// @param declared_inline whether this function was declared inline.
  ///
  /// @param locus the source location of this function declaration.
  ///
  /// @param mangled_name the mangled name of the function declaration.
  ///
  /// @param vis the visibility of the function declaration.
  ///
  /// @param bind the type of binding of the function.
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
    type_(new function_type(return_type, parms, fptr_size_in_bits,
			     fptr_align_in_bits)),
    declared_inline_(declared_inline), binding_(bind)
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
  /// @param mangled_name the mangled name of the function declaration.
  ///
  /// @param vis the visibility of the function declaration.
  ///
  /// @param binding  the kind of the binding of the function
  /// declaration.
function_decl::function_decl(const std::string& name,
		shared_ptr<type_base> fn_type,
		bool	declared_inline,
		location locus,
		const std::string& mangled_name,
		visibility vis,
		binding bind)
  : decl_base(name, locus, mangled_name, vis),
    type_(dynamic_pointer_cast<function_type>(fn_type)),
    declared_inline_(declared_inline),
    binding_(bind)
  {}

/// @return the pretty representation for a function.
string
function_decl::get_pretty_representation() const
{
  const class_decl::member_function* mem_fn =
    dynamic_cast<const class_decl::member_function*>(this);

  string result = mem_fn ? "method ": "function ";
  decl_base_sptr type = dynamic_pointer_cast<decl_base>(get_return_type());

  if (type)
    result += type->get_qualified_name() + " ";

  if (mem_fn && mem_fn->is_destructor())
    result += "~";

  result += get_qualified_name() + "(";

  parameters::const_iterator i = get_parameters().begin(),
    end = get_parameters().end();

  // Skip the first parameter if this is a method.
  if (mem_fn)
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
	  type = dynamic_pointer_cast<decl_base>(parm->get_type());
	  result += type->get_qualified_name();
	}
    }
  result += ")";

  if (mem_fn && mem_fn->is_const())
    result += " const";

  return result;
}

/// Return the type of the current instance of #function_decl.
///
/// It's either a function_type or method_type.
/// @return the type of the current instance of #function_decl.
const shared_ptr<function_type>
function_decl::get_type() const
{return type_;}

/// @return the return type of the current instance of function_decl.
const shared_ptr<type_base>
function_decl::get_return_type() const
{return type_->get_return_type();}

/// @return the parameters of the function.
const std::vector<shared_ptr<function_decl::parameter> >&
function_decl::get_parameters() const
{return type_->get_parameters();}

/// Append a parameter to the type of this function.
///
/// @param parm the parameter to append.
void
function_decl::append_parameter(shared_ptr<parameter> parm)
{type_->append_parameter(parm);}

/// Append a vector of parameters to the type of this function.
///
/// @param parms the vector of parameters to append.
void
function_decl::append_parameters(std::vector<shared_ptr<parameter> >& parms)
{
  for (std::vector<shared_ptr<parameter> >::const_iterator i = parms.begin();
       i != parms.end();
       ++i)
    type_->append_parameter(*i);
}

bool
function_decl::operator==(const decl_base& other) const
{
  if (!decl_base::operator==(other))
    return false;

  try
    {
      const function_decl& o = dynamic_cast<const function_decl&>(other);

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
  catch(...)
    {return false;}
}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
function_decl::traverse(ir_node_visitor& v)
{v.visit(*this);}

function_decl::~function_decl()
{}

// <function_decl definitions>

// <class_decl definitions>

/// A Constructor for instances of class_decl
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
/// @param member_fns the vector of member functions of this instance of
/// class_decl.
class_decl::class_decl(const std::string& name, size_t size_in_bits,
		       size_t align_in_bits, location locus,
		       visibility vis, base_specs& bases,
		       member_types& mbrs, data_members& data_mbrs,
		       member_functions& mbr_fns)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
  scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
  hashing_started_(false),
  is_declaration_only_(false),
  bases_(bases),
  member_types_(mbrs),
  data_members_(data_mbrs),
  member_functions_(mbr_fns)
{
  for (member_types::iterator i = mbrs.begin(); i != mbrs.end(); ++i)
    if (!(*i)->get_scope())
      add_decl_to_scope(*i, this);

  for (data_members::iterator i = data_mbrs.begin(); i != data_mbrs.end();
       ++i)
    if (!(*i)->get_scope())
      add_decl_to_scope(*i, this);

  for (member_functions::iterator i = mbr_fns.begin(); i != mbr_fns.end();
       ++i)
    if (!(*i)->get_scope())
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
		       size_t align_in_bits, location locus, visibility vis)
  : decl_base(name, locus, name, vis),
    type_base(size_in_bits, align_in_bits),
    scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
    hashing_started_(false),
    is_declaration_only_(false)
{}

/// A constuctor for instances of class_decl that represent a
/// declaration without definition.
///
/// @param name the name of the class.
///
/// @param is_declaration_only a boolean saying whether the instance
/// represents a declaration only, or not.
class_decl::class_decl(const std::string& name, bool is_declaration_only)
  : decl_base(name, location(), name),
    type_base(0, 0),
    scope_type_decl(name, 0, 0, location()),
    hashing_started_(false),
    is_declaration_only_(is_declaration_only)
{}

/// @return the pretty representaion for a class_decl.
string
class_decl::get_pretty_representation() const
{return "class " + get_qualified_name();}

/// Set the earlier declaration of this class definition.
///
/// @param declaration the earlier declaration to set.  Note that it's
/// set only if it's a pure declaration.
void
class_decl::set_earlier_declaration(shared_ptr<class_decl> declaration)
{
  if (declaration && declaration->is_declaration_only())
    declaration_ = declaration;
}

/// Set the earlier declaration of this class definition.
///
/// @param declaration the earlier declaration to set.  Note that it's
/// set only if it's a pure declaration.  It's dynamic type must be
/// pointer to class_decl.
void
class_decl::set_earlier_declaration(shared_ptr<type_base> declaration)
{
  shared_ptr<class_decl> d = dynamic_pointer_cast<class_decl>(declaration);
  set_earlier_declaration(d);
}

/// Add a member declaration to the current instance of class_decl.
/// The member declaration can be either a member type, data member,
/// member function, or member template.
///
/// @param d the member declaration to add.
void
class_decl::add_member_decl(decl_base_sptr d)
{
  if (member_type_sptr t = dynamic_pointer_cast<member_type>(d))
    add_member_type(t);
  else if (data_member_sptr m = dynamic_pointer_cast<data_member>(d))
    add_data_member(m);
  else if (member_function_sptr f = dynamic_pointer_cast<member_function>(d))
    add_member_function(f);
  else if (member_function_template_sptr f =
	   dynamic_pointer_cast<member_function_template>(d))
    add_member_function_template(f);
  else if (member_class_template_sptr c =
	   dynamic_pointer_cast<member_class_template>(d))
    add_member_class_template(c);
  else
    scope_decl::add_member_decl(d);
}

/// Add a member type to the current instance of class_decl.
///
/// @param t the member type to add.  It must not have been added to a
/// scope, otherwise this will violate an assertion.
void
class_decl::add_member_type(shared_ptr<member_type>t)
{
  decl_base* c = dynamic_pointer_cast<decl_base>(t->as_type())->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  //assert(!c || c == this);
  assert(!c);
  t->set_scope(this);
  member_types_.push_back(t);
}

/// Add a member type to the current instance of class_decl.
///
/// @param t the type to be added as a member type to the current
/// instance of class_decl.  An instance of class_decl::member_type
/// will be created out of @ref t and and added to the the class.
///
/// @param the access specifier for the member type to be created.
void
class_decl::add_member_type(type_base_sptr t, access_specifier a)
{
  decl_base_sptr d = get_type_declaration(t);
  assert(!d->get_scope());
  shared_ptr<class_decl::member_type> m(new class_decl::member_type(t, a));
  add_member_type(m);
  add_decl_to_scope(d, this);
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
  : member_base(a),
    base_class_(base),
    offset_in_bits_(offset_in_bits),
    is_virtual_(is_virtual)
{}

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
  : member_base(a),
    base_class_(dynamic_pointer_cast<class_decl>(base)),
    offset_in_bits_(offset_in_bits),
    is_virtual_(is_virtual)
{}

bool
class_decl::base_spec::operator==(const member_base& o) const
{
  try
    {
      const class_decl::base_spec& other =
	dynamic_cast<const class_decl::base_spec&>(o);

      return (member_base::operator==(other)
	      && *get_base_class() == *other.get_base_class());
    }
  catch(...)
    {return false;}
}

/// Add a data member to the current instance of class_decl.
///
/// @param m the data member to add.  This data member should not have
/// been already added to a scope.
void
class_decl::add_data_member(shared_ptr<data_member> m)
{
  decl_base* c = m->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c);
  data_members_.push_back(m);
  m->set_scope(this);
}

/// Add a data member to the current instance of class_decl.
///
/// @param v a var_decl to add as a data member.  A proper
/// class_decl::data_member is created from @ref v and added to the
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
/// @param offset_in_bits if @ref is_laid_out is true, this is the
/// offset of the data member, expressed (oh, surprise) in bits.
void
class_decl::add_data_member(var_decl_sptr v, access_specifier access,
			    bool is_laid_out, bool is_static,
			    size_t offset_in_bits)
{
  assert(!v->get_scope());

  data_member_sptr m(new class_decl::data_member(v, access,
						 is_laid_out,
						 is_static,
						 offset_in_bits));
  add_data_member(m);
  add_decl_to_scope(v, this);
}

/// a constructor for instances of class_decl::method_decl.
///
/// @param name the name of the method.
///
/// @param parms the parameters of the method
///
/// @param return_type the return type of the method.
///
/// @param class_type the type of the class the method belongs to.
///
/// @param ftype_size_in_bits the size of instances of
/// class_decl::method_decl, expressed in bits.
///
/// @param ftype_align_in_bits the alignment of instance of
/// class_decl::method_decl, expressed in bits.
///
/// @param declared_inline whether the method was declared inline or
/// not.
///
/// @param locus the source location of the method.
///
/// @param mangled_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
class_decl::method_decl::method_decl
(const std::string& name,
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
/// @param mangled_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
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
/// @param mangled_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
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
/// @param mangled_name the mangled name of the method.
///
/// @param vis the visibility of the method.
///
/// @param bind the binding of the method.
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
{}

class_decl::method_decl::~method_decl()
{}

const shared_ptr<method_type>
class_decl::method_decl::get_type() const
{
  return dynamic_pointer_cast<method_type>(type_);
}

  /// Constructor for instances of class_decl::member_function.
  ///
  /// @param fn the function decl to be used as a member function.
  /// This must be an intance of class_decl::method_decl.
  ///
  /// @param access the access specifier for the member function.
  ///
  /// @param vtable_offset_in_bits the offset of the this member
  /// function in the vtable, or zero.
  ///
  /// @param is_static set to true if this member function is static.
  ///
  /// @param is_constructor set to true if this member function is a
  /// constructor.
  ///
  /// @param is_destructor set to true if this member function is a
  /// destructor.
  ///
  /// @param is_const set to true if this member function is const.
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
    member_base(access, is_static),
  vtable_offset_in_bits_(vtable_offset_in_bits),
  is_constructor_(is_constructor),
  is_destructor_(is_destructor),
  is_const_(is_const)
{}

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance.
void
class_decl::member_function::traverse(ir_node_visitor& v)
{v.visit(*this);}

/// Return the number of virtual functions of this class_decl.
///
/// @return the number of virtual functions of this class_decl
size_t
class_decl::get_num_virtual_functions() const
{
  size_t result = 0;
  for (class_decl::member_functions::const_iterator i =
	 get_member_functions().begin();
       i != get_member_functions().end();
       ++i)
    if ((*i)->get_vtable_offset())
      ++result;
  return result;
}

/// Add a member function to the current instance of class_decl.
///
/// @param m the member function to add.  This member function should
/// not have been already added to a scope.
void
class_decl::add_member_function(shared_ptr<member_function> m)
{
  decl_base* c = m->get_scope();
  /// TODO: use our own assertion facility that adds a meaningful
  /// error message or something like a structured error.
  assert(!c);
  member_functions_.push_back(m);
  m->set_scope(this);
}

/// Add a member function to the current instance of class_decl.
///
/// @param f a function to add to the current class as a member
/// function.  A proper class_decl::member_function is created for
/// this function and added to the class.  This function should not
/// have been already added to a scope.
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
class_decl::add_member_function(function_decl_sptr f,
				access_specifier access,
				size_t vtable_offset,
				bool is_static, bool is_ctor,
				bool is_dtor, bool is_const)
{
  assert(!f->get_scope());
  member_function_sptr m(new class_decl::member_function(f, access,
							 vtable_offset,
							 is_static,
							 is_ctor, is_dtor,
							 is_const));
  add_member_function(m);
  add_decl_to_scope(f, this);
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
  member_function_templates_.push_back(m);
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
  member_class_templates_.push_back(m);
  m->as_class_tdecl()->set_scope(this);
}

/// Return true iff the class has no entity in its scope.
bool
class_decl::has_no_base_nor_member() const
{
  return (bases_.empty()
	  && member_types_.empty()
	  && data_members_.empty()
	  && member_functions_.empty()
	  && member_function_templates_.empty()
	  && member_class_templates_.empty());
}

bool
class_decl::operator==(const decl_base& other) const
{
  try
    {
      const class_decl& o = dynamic_cast<const class_decl&>(other);

      // No need to go further if the classes have different names or
      // different size / alignment.
      if (!(decl_base::operator==(o) && type_base::operator==(o)))
	return false;

      // Compare bases.
      {
	if (get_base_specifiers().size() != o.get_base_specifiers().size())
	  return false;

	base_specs::const_iterator b0, b1;
	for(b0 = get_base_specifiers().begin(),
	      b1 = o.get_base_specifiers().begin();
	    (b0 != get_base_specifiers().end()
	     && b1 != o.get_base_specifiers().end());
	    ++b0, ++b1)
	  if (**b0 != **b1)
	    return false;
	if (b0 != get_base_specifiers().end()
	    || b1 != o.get_base_specifiers().end())
	  return false;
      }

      //Compare member types
      {
	if (get_member_types().size() != o.get_member_types().size())
	  return false;

	member_types::const_iterator t0, t1;
	for (t0 = get_member_types().begin(), t1 = o.get_member_types().begin();
	     t0 != get_member_types().end() && t1 != o.get_member_types().end();
	     ++t0, ++t1)
	  if (!(**t0 == **t1))
	    return false;
	if (t0 != get_member_types().end() || t1 != o.get_member_types().end())
	  return false;
      }

      //compare data_members
      {
	if (get_data_members().size() != o.get_data_members().size())
	  return false;

	data_members::const_iterator d0, d1;
	for (d0 = get_data_members().begin(), d1 = o.get_data_members().begin();
	     d0 != get_data_members().end() && d1 != o.get_data_members().end();
	     ++d0, ++d1)
	  if (**d0 != **d1)
	    return false;
	if (d0 != get_data_members().end() || d1 != o.get_data_members().end())
	  return false;
      }

      //compare member functions
      {
	if (get_member_functions().size() != o.get_member_functions().size())
	  return false;

	member_functions::const_iterator f0, f1;
	for (f0 = get_member_functions().begin(),
	       f1 = o.get_member_functions().begin();
	     f0 != get_member_functions().end()
	       && f1 != o.get_member_functions().end();
	     ++f0, ++f1)
	  if (**f0 != **f1)
	    return false;
	if (f0 != get_member_functions().end()
	    || f1 != o.get_member_functions().end())
	  return false;
      }

      // compare member function templates
      {
	if (get_member_function_templates().size()
	    != o.get_member_function_templates().size())
	  return false;

	member_function_templates::const_iterator fn_tmpl_it0, fn_tmpl_it1;
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
      }

      // compare member class templates
      {
	if (get_member_class_templates().size()
	    != o.get_member_class_templates().size())
	  return false;

	member_class_templates::const_iterator cl_tmpl_it0, cl_tmpl_it1;
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
      }
    }
  catch (...)
    {return false;}
  return true;
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
  try
    {
      const decl_base& o = dynamic_cast<const decl_base&>(other);
      return *this == o;
    }
  catch(...)
    {return false;}
}

bool
class_decl::operator==(const class_decl& other) const
{return *this == static_cast<const decl_base&>(other);}

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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// members.
void
class_decl::traverse(ir_node_visitor& v)
{
  v.visit(*this);

  for (member_types::const_iterator i = get_member_types().begin();
       i != get_member_types().end();
       ++i)
    {
      shared_ptr<traversable_base> t = dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_function_templates::const_iterator i =
	 get_member_function_templates().begin();
       i != get_member_function_templates().end();
       ++i)
    {
      shared_ptr<traversable_base> t = dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_class_templates::const_iterator i =
	 get_member_class_templates().begin();
       i != get_member_class_templates().end();
       ++i)
    {
      shared_ptr<traversable_base> t = dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse(v);
    }

  for (data_members::const_iterator i = get_data_members().begin();
       i != get_data_members().end();
       ++i)
    {
      shared_ptr<traversable_base> t = dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse(v);
    }

  for (member_functions::const_iterator i= get_member_functions().begin();
       i != get_member_functions().end();
       ++i)
    {
      shared_ptr<traversable_base> t =
	dynamic_pointer_cast<traversable_base>(*i);
      if (t)
	t->traverse(v);
    }
}

class_decl::~class_decl()
{}

bool
class_decl::member_base::operator==(const member_base& o) const
{
  return (get_access_specifier() == o.get_access_specifier()
	  && is_static() == o.is_static());
}

/// Constructor of a class_decl::member_type
///
/// @param t the type to be member of the class
///
/// @param access the access specifier for the member type.
class_decl::member_type::member_type(shared_ptr<type_base> t,
				     access_specifier access)
  : decl_base(*dynamic_pointer_cast<decl_base>(t)),
    member_base(access), type_(t)
{}

bool
class_decl::member_type::operator==(const decl_base& other) const
{
  try
    {
      const class_decl::member_type& o =
	dynamic_cast<const class_decl::member_type&>(other);
      return (*as_type() == *o.as_type()
	      && member_base::operator==(o));
    }
  catch(...)
    {return false;}
}

bool
class_decl::member_type::operator==(const member_base& other) const
{
  try
    {
      const class_decl::member_type& o =
	dynamic_cast<const class_decl::member_type&>(other);
      return *this == static_cast<const decl_base&>(o);
    }
  catch(...)
    {return false;}
}

bool
class_decl::member_type::operator==(const member_type& other) const
{return *this == static_cast<const decl_base&>(other);}

bool
operator==(class_decl::base_spec_sptr l, class_decl::base_spec_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

bool
operator==(class_decl::member_type_sptr l, class_decl::member_type_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

bool
operator==(class_decl::data_member_sptr l, class_decl::data_member_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
}

bool
class_decl::data_member::operator==(const decl_base& o) const
{
  try
    {
      const class_decl::data_member& other =
	dynamic_cast<const class_decl::data_member&>(o);
      return (is_laid_out() == other.is_laid_out()
	      && get_offset_in_bits() == other.get_offset_in_bits()
	      && var_decl::operator==(other)
	      && member_base::operator==(other));
    }
  catch(...)
    {return false;}
}

string
class_decl::member_type::get_pretty_representation() const
{return get_type_declaration(as_type())->get_pretty_representation();}

void
class_decl::data_member::traverse(ir_node_visitor& v)
{v.visit(*this);}

class_decl::data_member::~data_member()
{}

bool
class_decl::member_function::operator==(const decl_base& other) const
{
  try
    {
      const class_decl::member_function& o =
	dynamic_cast<const class_decl::member_function&>(other);

      return (get_vtable_offset() == o.get_vtable_offset()
	      && is_constructor() == o.is_constructor()
	      && is_destructor() == o.is_destructor()
	      && is_const() == o.is_const()
	      && member_base::operator==(o)
	      && function_decl::operator==(o));
    }
  catch(...)
    {return false;}
}

bool
class_decl::member_function::operator==(const member_function& other) const
{
  try
    {
      const class_decl::member_function& o =
	dynamic_cast<const class_decl::member_function&>(other);
      return *this == static_cast<const decl_base&>(o);
    }
  catch(...)
    {return false;}
}

bool
class_decl::member_function::operator==(const member_base& other) const
{
  try
    {
      const class_decl::member_function& o =
	dynamic_cast<const class_decl::member_function&>(other);
      return decl_base::operator==(o);
    }
  catch(...)
    {return false;}
}

bool
operator==(class_decl::member_function_sptr l,
	   class_decl::member_function_sptr r)
{
  if (l.get() == r.get())
    return true;
  if (!!l != !!r)
    return false;

  return *l == *r;
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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on its
/// underlying function template.
void
class_decl::member_function_template::traverse(ir_node_visitor& v)
{
  v.visit(*this);
  as_function_tdecl()->traverse(v);
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
{return *this == static_cast<const member_base&>(other);}

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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
void
class_decl::member_class_template::traverse(ir_node_visitor& v)
{
  v.visit(*this);
  as_class_tdecl()->get_pattern()->traverse(v);
}

/// Streaming operator for class_decl::access_specifier.
///
/// @param o the output stream to serialize the access specifier to.
///
/// @param a the access specifier to serialize.
///
/// @return the output stream.
std::ostream&
operator<<(std::ostream& o, class_decl::access_specifier a)
{
  string r;

  switch (a)
  {
  case class_decl::no_access:
    r = "none";
    break;
  case class_decl::private_access:
    r = "private";
    break;
  case class_decl::protected_access:
    r = "protected";
    break;
  case class_decl::public_access:
    r= "public";
    break;
  };
  o << r;
  return o;
}

// </class_decl>

// <template_decl stuff>

template_decl::~template_decl()
{
}

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

bool
template_parameter::operator==(const template_parameter& o) const
{
  return (get_index() == o.get_index());
}

template_parameter::~template_parameter()
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

type_composition::type_composition(unsigned index, shared_ptr<type_base> t)
: decl_base("", location()), template_parameter(index),
  type_(std::tr1::dynamic_pointer_cast<type_base>(t))
{}

type_composition::~type_composition()
{}

//</template_parameter>

// <function_template>
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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the
/// function pattern of the template.
void
function_tdecl::traverse(ir_node_visitor&v)
{
  v.visit(*this);
  get_pattern()->traverse(v);
}

function_tdecl::~function_tdecl()
{}

// </function_template>

// <class template>

/// Constructor for the class_tdecl type.
///
/// @param pattern The details of the class template. This must NOT be a
/// null pointer.  If you really this to be null, please use the
/// constructor above instead.
///
/// @param locus the source location of the declaration of the type.
///
/// @param vis the visibility of the instances of class instantiated
/// from this template.
class_tdecl::class_tdecl(shared_ptr<class_decl> pattern,
			 location locus, visibility vis)
: decl_base(pattern->get_name(), locus,
	    pattern->get_name(), vis),
  scope_decl(pattern->get_name(), locus)
{set_pattern(pattern);}

void
class_tdecl::set_pattern(shared_ptr<class_decl> p)
{
  pattern_ = p;
  add_decl_to_scope(p, this);
  set_name(p->get_name());
}

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

/// This implements the traversable_base::traverse pure virtual
/// function.
///
/// @param v the visitor used on the current instance and on the class
/// pattern of the template.
void
class_tdecl::traverse(ir_node_visitor&v)
{
  v.visit(*this);

  shared_ptr<class_decl> pattern = get_pattern();
  if (pattern)
    pattern->traverse(v);
}

class_tdecl::~class_tdecl()
{}

void
ir_node_visitor::visit(scope_decl&)
{}

void
ir_node_visitor::visit(type_decl&)
{}

void
ir_node_visitor::visit(namespace_decl&)
{}

void
ir_node_visitor::visit(qualified_type_def&)
{}

void
ir_node_visitor::visit(pointer_type_def&)
{}

void
ir_node_visitor::visit(reference_type_def&)
{}

void
ir_node_visitor::visit(enum_type_decl&)
{}

void
ir_node_visitor::visit(typedef_decl&)
{}

void
ir_node_visitor::visit(var_decl&)
{}

void
ir_node_visitor::visit(function_decl&)
{}

void
ir_node_visitor::visit(function_tdecl&)
{}

void
ir_node_visitor::visit(class_tdecl&)
{}

void
ir_node_visitor::visit(class_decl&)
{}

void
ir_node_visitor::visit(class_decl::data_member&)
{}

void
ir_node_visitor::visit(class_decl::member_function&)
{}

void
ir_node_visitor::visit(class_decl::member_function_template&)
{}

void
ir_node_visitor::visit(class_decl::member_class_template&)
{}

// </class template>
}//end namespace abigail
