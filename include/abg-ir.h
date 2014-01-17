// -*- Mode: C++ -*-
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
// Author: Dodji Seketeli

/// @file

#ifndef __ABG_IR_H__
#define __ABG_IR_H__

#include "abg-fwd.h"
#include "abg-hash.h"
#include "abg-traverse.h"

namespace abigail
{

/// @brief The source location of a token.
///
/// This represents the location of a token coming from a given
/// translation unit.  This location is actually an abstraction of
/// cursor in the table of all the locations of all the tokens of the
/// translation unit.  That table is managed by the location_manager
/// type.
class location
{
  unsigned		value_;

  location(unsigned v) : value_(v) { }

public:

  location() : value_(0) { }

  unsigned
  get_value() const
  {return value_;}

  operator bool() const
  { return !!value_; }

  bool
  operator==(const location other) const
  {return value_ == other.value_;}

  bool
  operator<(const location other) const
  { return value_ < other.value_; }

  friend class location_manager;
};

/// @brief The entry point to manage locations.
///
/// This type keeps a table of all the locations for tokens of a
/// given translation unit.
class location_manager
{
  struct priv;

  /// Pimpl.
  shared_ptr<priv> priv_;

public:

  location_manager();

  location
  create_new_location(const std::string& fle, size_t lne, size_t col);

  void
  expand_location(const location location, std::string& path,
		  unsigned& line, unsigned& column) const;
};

struct ir_node_visitor;

/// Convenience typedef for a shared pointer on a @ref
/// translation_unit type.
typedef shared_ptr<translation_unit> translation_unit_sptr;

/// Convenience typedef for a vector of @ref translation_unit_sptr.
typedef std::vector<translation_unit_sptr> translation_units;

/// Convenience typedef for a shared pointer on a @ref type_base
typedef shared_ptr<type_base> type_base_sptr;

/// Convenience typedef for a smart pointer on @ref decl_base.
typedef shared_ptr<decl_base> decl_base_sptr;

struct ir_traversable_base;

/// Convenience typedef for a shared pointer to @ref
/// ir_traversable_base.
typedef shared_ptr<ir_traversable_base> ir_traversable_base_sptr;

/// The base of an entity of the intermediate representation that is
/// to be traversed.
struct ir_traversable_base : public traversable_base
{
  virtual void
  traverse(ir_node_visitor&);
}; // end class ir_traversable_base

/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// into a translation unit.
class translation_unit : public traversable_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  translation_unit();
public:
  /// Convenience typedef for a shared pointer on a @ref global_scope.
  typedef shared_ptr<global_scope> global_scope_sptr;

public:
  translation_unit(const std::string& path,
		   char address_size = 0);

  virtual ~translation_unit();

  const std::string&
  get_path() const;

  void
  set_path(const string&);

  const global_scope_sptr
  get_global_scope() const;

  location_manager&
  get_loc_mgr();

  const location_manager&
  get_loc_mgr() const;

  bool
  is_empty() const;

  char
  get_address_size() const;

  void
  set_address_size(char);

  bool
  operator==(const translation_unit&) const;

  virtual void
  traverse(ir_node_visitor& v);
};//end class translation_unit

bool
operator==(translation_unit_sptr, translation_unit_sptr);

/// The base type of all declarations.
class decl_base : public ir_traversable_base
{
public:
  /// Facility to hash instances of decl_base.
  struct hash;

  /// ELF visibility
  enum visibility
  {
    VISIBILITY_NONE,
    VISIBILITY_DEFAULT,
    VISIBILITY_PROTECTED,
    VISIBILITY_HIDDEN,
    VISIBILITY_INTERNAL
  };

  /// ELF binding
  enum binding
  {
    BINDING_NONE,
    BINDING_LOCAL,
    BINDING_GLOBAL,
    BINDING_WEAK
  };

protected:
  mutable size_t	hash_;

private:
  location		location_;
  std::string		name_;
  std::string		mangled_name_;
  scope_decl*		context_;
  visibility		visibility_;

  // Forbidden
  decl_base();

  virtual void
  set_scope(scope_decl*);

public:

  decl_base(const std::string&	name, location locus,
	    const std::string&	mangled_name = "",
	    visibility vis = VISIBILITY_DEFAULT);

  decl_base(location);

  decl_base(const decl_base&);

  virtual bool
  operator==(const decl_base&) const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~decl_base();

  size_t
  get_hash() const;

  void
  set_hash(size_t) const;

  location
  get_location() const
  {return location_;}

  void
  set_location(const location& l)
  {location_ = l;}

  const string&
  get_name() const
  {return name_;}

  virtual string
  get_pretty_representation() const;

  void
  get_qualified_name(string& qualified_name,
		     const string& separator = "::") const;

  string
  get_qualified_name(const string& separator = "::") const;

  void
  set_name(const string& n)
  { name_ = n; }

  const string&
  get_mangled_name() const
  {return mangled_name_;}

  void
  set_mangled_name(const std::string& m)
  { mangled_name_ = m; }

  scope_decl*
  get_scope() const
  {return context_;}

  visibility
  get_visibility() const
  {return visibility_;}

  void
  set_visibility(visibility v)
  {visibility_ = v;}

  friend decl_base_sptr
  add_decl_to_scope(decl_base_sptr dcl, scope_decl* scpe);

  friend decl_base_sptr
  insert_decl_into_scope(decl_base_sptr,
			 vector<shared_ptr<decl_base> >::iterator,
			 scope_decl*);


  friend class class_decl;
};// end class decl_base

bool
operator==(decl_base_sptr, decl_base_sptr);

std::ostream&
operator<<(std::ostream&, decl_base::visibility);

std::ostream&
operator<<(std::ostream&, decl_base::binding);

/// Convenience typedef for a shared pointer on a @ref scope_decl.
typedef shared_ptr<scope_decl> scope_decl_sptr;

/// A declaration that introduces a scope.
class scope_decl : public virtual decl_base
{
public:

  /// Convenience typedef for a vector of @ref decl_base_sptr.
  typedef std::vector<decl_base_sptr >	declarations;
  /// Convenience typedef for a vector of @ref scope_decl_sptr.
  typedef std::vector<scope_decl_sptr>	scopes;

private:
  declarations	members_;
  scopes	member_scopes_;

  scope_decl();

protected:
  virtual decl_base_sptr
  add_member_decl(const decl_base_sptr member);

  virtual decl_base_sptr
  insert_member_decl(const decl_base_sptr member,
		     declarations::iterator before);

  virtual void
  remove_member_decl(const decl_base_sptr member);

public:
  scope_decl(const std::string& name, location locus,
	     visibility	vis = VISIBILITY_DEFAULT)
  : decl_base(name, locus, /*mangled_name=*/name, vis)
  {}

  scope_decl(location l) : decl_base("", l)
  {}

  virtual bool
  operator==(const decl_base&) const;

  const declarations&
  get_member_decls() const
  {return members_;}

  declarations&
  get_member_decls()
  {return members_;}

  scopes&
  get_member_scopes()
  {return member_scopes_;}

  bool
  is_empty() const
  {return get_member_decls().empty();}

  bool
  find_iterator_for_member(const decl_base*, declarations::iterator&);

  bool
  find_iterator_for_member(const decl_base_sptr, declarations::iterator&);

  virtual void
  traverse(ir_node_visitor&);

  virtual ~scope_decl();

  friend decl_base_sptr
  add_decl_to_scope(decl_base_sptr dcl, scope_decl* scpe);

  friend decl_base_sptr
  insert_decl_into_scope(decl_base_sptr decl,
			 scope_decl::declarations::iterator before,
			 scope_decl* scope);

  friend void
  remove_decl_from_scope(decl_base_sptr decl);
};//end class scope_decl

/// Convenience typedef for shared pointer on @ref global_scope.
typedef shared_ptr<global_scope> global_scope_sptr;

/// This abstracts the global scope of a given translation unit.
///
/// Only one instance of this class must be present in a given
/// translation_unit.  That instance is implicitely created the first
/// time translatin_unit::get_global_scope is invoked.
class global_scope : public scope_decl
{
  translation_unit* translation_unit_;

  global_scope(translation_unit *tu)
  : decl_base("", location()), scope_decl("", location()),
    translation_unit_(tu)
  {}

public:

  friend class translation_unit;

  translation_unit*
  get_translation_unit() const
  {return translation_unit_;}

  virtual ~global_scope();
};

/// An abstraction helper for type declarations
class type_base
{
  size_t	size_in_bits_;
  size_t	alignment_in_bits_;

  // Forbid this.
  type_base();

public:

  /// A hasher for type_base types.
  struct hash;

  /// A hasher for types.  It gets the dynamic type of the current
  /// instance of type and hashes it accordingly.  Note that the hashing
  /// function of this hasher must be updated each time a new kind of
  /// type is added to the IR.
  struct dynamic_hash;

  /// A hasher for shared_ptr<type_base> that will hash it based on the
  /// runtime type of the type pointed to.
  struct shared_ptr_hash;

  struct cached_hash;

  type_base(size_t s, size_t a);

  virtual bool
  operator==(const type_base&) const;

  virtual ~type_base();

  void
  set_size_in_bits(size_t);

  size_t
  get_size_in_bits() const;

  void
  set_alignment_in_bits(size_t);

  size_t
  get_alignment_in_bits() const;
};//end class type_base

/// A predicate for deep equality of instances of
/// type_base*
struct type_ptr_equal
{
  bool
  operator()(const type_base* l, const type_base* r) const
  {
    if (!!l != !!r)
      return false;

    if (l == r)
      return true;

    if (l)
      return *l == *r;

    return true;
  }
};

/// A predicate for deep equality of instances of
/// shared_ptr<type_base>
struct type_shared_ptr_equal
{
  bool
  operator()(const type_base_sptr l, const type_base_sptr r) const
  {
    if (!!l != !!r)
      return false;

    if (l.get() == r.get())
      return true;

    if (l)
      return *l == *r;

    return true;
  }
};

/// Convenience typedef for a shared pointer on a @ref type_decl.
typedef shared_ptr<type_decl> type_decl_sptr;

/// A basic type declaration that introduces no scope.
class type_decl : public virtual decl_base, public virtual type_base
{
  // Forbidden.
  type_decl();

public:

  /// Facility to hash instance of type_decl
  struct hash;

  type_decl(const std::string& name,
	    size_t size_in_bits, size_t alignment_in_bits,
	    location locus, const std::string&	mangled_name = "",
	    visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_decl&) const;

  virtual string
  get_pretty_representation() const;

  virtual void
  traverse(ir_node_visitor&);

  virtual ~type_decl();
};// end class type_decl.

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public virtual type_base
{
  scope_type_decl();

public:

  /// Hasher for instances of scope_type_decl
  struct hash;

  scope_type_decl(const std::string& name, size_t size_in_bits,
		  size_t alignment_in_bits, location locus,
		  visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual ~scope_type_decl();
};

/// Convenience typedef for a shared pointer on namespace_decl.
typedef shared_ptr<namespace_decl> namespace_decl_sptr;

/// The abstraction of a namespace declaration
class namespace_decl : public scope_decl
{
public:

  namespace_decl(const std::string& name, location locus,
		 visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual void
  traverse(ir_node_visitor&);

  virtual ~namespace_decl();
};// end class namespace_decl

typedef shared_ptr<qualified_type_def> qualified_type_def_sptr;

/// The abstraction of a qualified type.
class qualified_type_def : public virtual type_base, public virtual decl_base
{
  char			cv_quals_;
  shared_ptr<type_base> underlying_type_;

  // Forbidden.
  qualified_type_def();

public:

  /// A Hasher for instances of qualified_type_def
  struct hash;

  /// Bit field values representing the cv qualifiers of the
  /// underlying type.
  enum CV
  {
    CV_NONE = 0,
    CV_CONST = 1,
    CV_VOLATILE = 1 << 1,
    CV_RESTRICT = 1 << 2
  };

  qualified_type_def(shared_ptr<type_base> type, CV quals, location locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  char
  get_cv_quals() const;

  void
  set_cv_quals(char cv_quals);

  const shared_ptr<type_base>&
  get_underlying_type() const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~qualified_type_def();
}; // end class qualified_type_def.

qualified_type_def::CV
operator|(qualified_type_def::CV, qualified_type_def::CV);

std::ostream&
operator<<(std::ostream&, qualified_type_def::CV);

/// Convenience typedef for a shared pointer on a @ref pointer_type_def
typedef shared_ptr<pointer_type_def> pointer_type_def_sptr;

/// The abstraction of a pointer type.
class pointer_type_def : public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base>	pointed_to_type_;

  // Forbidden.
  pointer_type_def();

public:

  /// A hasher for instances of pointer_type_def
  struct hash;

  pointer_type_def(shared_ptr<type_base>& pointed_to_type, size_t size_in_bits,
		   size_t alignment_in_bits, location locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  const shared_ptr<type_base>&
  get_pointed_to_type() const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~pointer_type_def();
}; // end class pointer_type_def

/// Convenience typedef for a shared pointer on a @ref reference_type_def
typedef shared_ptr<reference_type_def> reference_type_def_sptr;

/// Abstracts a reference type.
class reference_type_def : public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base> pointed_to_type_;
  bool			is_lvalue_;

  // Forbidden.
  reference_type_def();

public:

  /// Hasher for intances of reference_type_def.
  struct hash;

  reference_type_def(const type_base_sptr pointed_to_type,
		     bool lvalue, size_t size_in_bits,
		     size_t alignment_in_bits, location locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  const shared_ptr<type_base>&
  get_pointed_to_type() const;

  bool
  is_lvalue() const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~reference_type_def();
}; // end class reference_type_def

/// Convenience typedef for shared pointer on enum_type_decl.
typedef shared_ptr<enum_type_decl> enum_type_decl_sptr;

/// Abstracts a declaration for an enum type.
class enum_type_decl : public virtual type_base, public virtual decl_base
{
public:

  /// A hasher for an enum_type_decl.
  struct hash;

  /// Enumerator Datum.
  class enumerator
  {
    string	name_;
    size_t	value_;

  public:

    enumerator()
      : value_(0)
    {}

    enumerator(const string& name, size_t value)
    : name_(name), value_(value) { }

    bool
    operator==(const enumerator& other) const
    {
      return (get_name() == other.get_name()
	      && get_value() == other.get_value());
    }
    const string&
    get_name() const
    {return name_;}

    const string
    get_qualified_name(const enum_type_decl_sptr enum_type) const
    {return enum_type->get_qualified_name() + "::" + get_name();}

    void
    set_name(const string& n)
    {name_ = n;}

    size_t
    get_value() const
    {return value_;}

    void
    set_value(size_t v)
    {value_=v;}
  };

  /// Convenience typedef for a list of @ref enumerator.
  typedef std::vector<enumerator> enumerators;
private:

  type_base_sptr	underlying_type_;
  enumerators		enumerators_;

  // Forbidden
  enum_type_decl();

public:

  /// Constructor of an enum type declaration.
  ///
  /// @param name the name of the enum
  ///
  /// @param locus the locus at which the enum appears in the source
  /// code.
  ///
  /// @param underlying_type the underlying type of the enum
  ///
  /// @param enumerators a list of enumerators for this enum.
  enum_type_decl(const string& name, location locus,
		 type_base_sptr underlying_type,
		 enumerators& enms, const std::string& mangled_name = "",
		 visibility vis = VISIBILITY_DEFAULT)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    underlying_type_(underlying_type),
    enumerators_(enms)
  {}

  type_base_sptr
  get_underlying_type() const;

  const enumerators&
  get_enumerators() const;

  virtual string
  get_pretty_representation() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~enum_type_decl();
}; // end class enum_type_decl

/// Convenience typedef for a shared pointer on a @ref typedef_decl.
typedef shared_ptr<typedef_decl> typedef_decl_sptr;

/// The abstraction of a typedef declaration.
class typedef_decl : public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base> underlying_type_;

  // Forbidden
  typedef_decl();

public:

  /// Hasher for the typedef_decl type.
  struct hash;

  typedef_decl(const string& name, const shared_ptr<type_base> underlying_type,
	       location locus, const std::string& mangled_name = "",
	       visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation() const;

  const shared_ptr<type_base>&
  get_underlying_type() const;

  virtual void
  traverse(ir_node_visitor&);

  virtual ~typedef_decl();
};// end class typedef_decl

/// Convenience typedef for a shared pointer on a @ref var_decl
typedef shared_ptr<var_decl> var_decl_sptr;

/// Abstracts a variable declaration.
class var_decl : public virtual decl_base
{
  shared_ptr<type_base>	type_;
  binding			binding_;

  // Forbidden
  var_decl();

public:

  /// Hasher for a var_decl type.
  struct hash;

  var_decl(const std::string&		name,
	   shared_ptr<type_base>	type,
	   location			locus,
	   const std::string&		mangled_name,
	   visibility			vis = VISIBILITY_DEFAULT,
	   binding			bind = BINDING_NONE);

  virtual bool
  operator==(const decl_base&) const;

  shared_ptr<type_base>
  get_type() const
  {return type_;}

  binding
  get_binding() const
  {return binding_;}

  void
  set_binding(binding b)
  {binding_ = b;}

  virtual string
  get_pretty_representation() const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~var_decl();
}; // end class var_decl

/// Convenience typedef for a shared pointer on a @ref function_decl
typedef shared_ptr<function_decl> function_decl_sptr;

/// Abstraction for a function declaration.
class function_decl : public virtual decl_base
{
protected:
  shared_ptr<function_type> type_;

private:
  bool			declared_inline_;
  decl_base::binding	binding_;

public:
  /// Hasher for function_decl
  struct hash;

  class parameter;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::parameter
  typedef shared_ptr<parameter> parameter_sptr;

  /// Convenience typedef for a vector of @ref parameter_sptr
  typedef std::vector<parameter_sptr> parameters;

  /// Abtraction for the parameter of a function.
  class parameter
  {
    shared_ptr<type_base>	type_;
    unsigned			index_;
    bool			variadic_marker_;
    std::string		name_;
    location			location_;
    bool			artificial_;

  public:

    /// Hasher for an instance of function::parameter
    struct hash;

    parameter(const shared_ptr<type_base> type,
	      unsigned index,
	      const std::string& name,
	      location loc, bool variadic_marker = false)
      : type_(type), index_(index), variadic_marker_ (variadic_marker),
	name_(name), location_(loc),
	artificial_(false)
    {}

    parameter(const shared_ptr<type_base> type,
	      const std::string& name,
	      location loc, bool variadic_marker = false,
	      bool is_artificial = false)
      : type_(type), index_(0), variadic_marker_ (variadic_marker),
	name_(name), location_(loc), artificial_(is_artificial)
    {}

    parameter(const shared_ptr<type_base> type,
	      unsigned index = 0,
	      bool variadic_marker = false)
    : type_(type),
      index_(index),
      variadic_marker_ (variadic_marker),
      artificial_(false)
    {}

    const shared_ptr<type_base>
    get_type()const
    {return type_;}

    const shared_ptr<type_base>
    get_type()
    {return type_;}

    unsigned
    get_index() const
    {return index_;}

    void
    set_index(unsigned i)
    {index_ = i;}

    const std::string&
    get_name() const
    {return name_;}

    location
    get_location() const
    {return location_;}

    /// Test if the parameter is artificial.
    ///
    /// Being artificial means the parameter was not explicitely
    /// mentionned in the source code, but was rather artificially
    /// created by the compiler.
    ///
    /// @return true if the parameter is artificial, false otherwise.
    bool
    get_artificial() const
    {return artificial_;}

    /// Getter for the artificial-ness of the parameter.
    ///
    /// Being artificial means the parameter was not explicitely
    /// mentionned in the source code, but was rather artificially
    /// created by the compiler.
    ///
    /// @param f set to true if the parameter is artificial.
    void
    set_artificial(bool f)
    {artificial_ = f;}

    bool
    operator==(const parameter& o) const
    {
      if ((get_variadic_marker() != o.get_variadic_marker())
	  || (get_index() != o.get_index())
	  || (!!get_type() != !!o.get_type())
	  || (get_type() && (*get_type() != *o.get_type())))
	return false;
      return true;
    }

    bool
    get_variadic_marker() const
    {return variadic_marker_;}
  };

  function_decl(const std::string&  name,
		const std::vector<parameter_sptr >& parms,
		shared_ptr<type_base> return_type,
		size_t fptr_size_in_bits,
		size_t fptr_align_in_bits,
		bool declared_inline,
		location locus,
		const std::string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL);

  function_decl(const std::string& name,
		shared_ptr<function_type> function_type, bool declared_inline,
		location locus, const std::string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL)
  : decl_base(name, locus, mangled_name, vis),
    type_(function_type),
    declared_inline_(declared_inline),
    binding_(bind)
  {}

  function_decl(const std::string& name,
		shared_ptr<type_base> fn_type,
		bool declared_inline,
		location locus,
		const std::string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL);

  virtual string
  get_pretty_representation() const;

  const std::vector<parameter_sptr >&
  get_parameters() const;

  void
  append_parameter(parameter_sptr parm);

  void
  append_parameters(std::vector<parameter_sptr >& parms);

  const shared_ptr<function_type>
  get_type() const;

  const shared_ptr<type_base>
  get_return_type() const;

  void
  set_type(shared_ptr<function_type> fn_type)
  {type_ = fn_type; }

  bool
  is_declared_inline() const
  {return declared_inline_;}

  binding
  get_binding() const
  {return binding_;}

  virtual bool
  operator==(const decl_base& o) const;

  /// Return true iff the function takes a variable number of
  /// parameters.
  ///
  /// @return true if the function taks a variable number
  /// of parameters.
  bool
  is_variadic() const
  {
    return (!get_parameters().empty()
	    && get_parameters().back()->get_variadic_marker());
  }

  virtual void
  traverse(ir_node_visitor&);

  virtual ~function_decl();
};

/// Convenience typedef for a shared pointer on a @ref function_type
typedef shared_ptr<function_type> function_type_sptr;

/// Abstraction of a function type.
class function_type : public virtual type_base
{
public:
  /// Hasher for an instance of function_type
  struct hash;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::parameter
  typedef shared_ptr<function_decl::parameter>	parameter_sptr;
  /// Convenience typedef for a vector of @ref parameter_sptr
  typedef std::vector<parameter_sptr>		parameters;

private:
  function_type();

  shared_ptr<type_base> return_type_;

protected:
  parameters parms_;

public:

  /// The most straightforward constructor for the the function_type
  /// class.
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
  function_type(shared_ptr<type_base> return_type, const parameters& parms,
		size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits), return_type_(return_type),
    parms_(parms)
  {}

  /// A constructor for a function_type that takes no parameters.
  ///
  /// @param return_type the return type of this function_type.
  ///
  /// @param size_in_bits the size of this type, in bits.
  ///
  /// @param alignment_in_bits the alignment of this type, in bits.
  function_type(shared_ptr<type_base> return_type,
		size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits),
    return_type_(return_type)
  {}

  /// A constructor for a function_type that takes no parameter and
  /// that has no return_type yet.  These missing parts can (and must)
  /// be added later.
  ///
  /// @param size_in_bits the size of this type, in bits.
  ///
  /// @param alignment_in_bits the alignment of this type, in bits.
  function_type(size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits)
  {}

  const shared_ptr<type_base>
  get_return_type() const
  {return return_type_;}

  void
  set_return_type(shared_ptr<type_base> t)
  {return_type_ = t;}

  const parameters&
  get_parameters() const
  {return parms_;}

  parameters&
  get_parameters()
  {return parms_;}

  void
  set_parameters(const parameters &p)
  {parms_ = p;}

  void
  append_parameter(parameter_sptr parm)
  {
    parm->set_index(parms_.size());
    parms_.push_back(parm);
  }

  bool
  is_variadic() const
  {return !parms_.empty() && parms_.back()->get_variadic_marker();}

  virtual bool
  operator==(const type_base&) const;

  virtual ~function_type();
};//end class function_type

/// Convenience typedef for shared pointer to @ref method_type.
typedef shared_ptr<method_type> method_type_sptr;
/// Abstracts the type of a class member function.
class method_type : public function_type
{

  shared_ptr<class_decl> class_type_;

  method_type();

public:

  /// Hasher for intances of method_type
  struct hash;

  method_type(shared_ptr<type_base> return_type,
	      shared_ptr<class_decl> class_type,
	      const std::vector<shared_ptr<function_decl::parameter> >& parms,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(shared_ptr<type_base> return_type,
	      shared_ptr<type_base> class_type,
	      const std::vector<shared_ptr<function_decl::parameter> >& parms,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(shared_ptr<class_decl> class_type,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  method_type(size_t size_in_bits,
	      size_t alignment_in_bits);

  shared_ptr<class_decl>
  get_class_type() const
  {return class_type_;}

  void
  set_class_type(shared_ptr<class_decl> t);

  virtual ~method_type();
};// end class method_type.

/// The base class of templates.
class template_decl
{
  // XXX
  std::list<shared_ptr<template_parameter> > parms_;

public:

  /// Hasher.
  struct hash;

  template_decl()
  {}

  void
  add_template_parameter(shared_ptr<template_parameter> p)
  {parms_.push_back(p);}

  const std::list<shared_ptr<template_parameter> >&
  get_template_parameters() const
  {return parms_;}

  virtual bool
  operator==(const template_decl& o) const;

  virtual ~template_decl();
};//end class template_decl

/// Base class for a template parameter.  Client code should use the
/// more specialized type_template_parameter,
/// non_type_template_parameter and template_template_parameter below.
class template_parameter
{
  unsigned index_;

  // Forbidden
  template_parameter();

 public:

  /// Hashers.
  struct hash;
  struct dynamic_hash;
  struct shared_ptr_hash;

  template_parameter(unsigned index) : index_(index)
  {}

  virtual bool
  operator==(const template_parameter&) const;

  unsigned
  get_index() const
  {return index_;}

  virtual ~template_parameter();
};//end class template_parameter

/// Abstracts a type template parameter.
class type_tparameter : public template_parameter, public virtual type_decl
{
  // Forbidden
  type_tparameter();

public:

  /// Hasher.
  struct hash;

  type_tparameter(unsigned index,
		  const std::string& name,
		  location locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus),
      template_parameter(index)
  {}

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const type_tparameter&) const;

  virtual ~type_tparameter();
};// end class type_tparameter.

/// Abstracts non type template parameters.
class non_type_tparameter : public template_parameter, public virtual decl_base
{
  shared_ptr<type_base> type_;

  // Forbidden
  non_type_tparameter();

public:
  /// Hasher.
  struct hash;

  non_type_tparameter(unsigned index, const std::string& name,
			      shared_ptr<type_base> type, location locus)
    : decl_base(name, locus, ""),
      template_parameter(index),
      type_(type)
  {}

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  shared_ptr<type_base>
  get_type() const
  {return type_;}

  virtual ~non_type_tparameter();
};// end class non_type_tparameter

/// Abstracts a template template parameter.
class template_tparameter : public type_tparameter, public template_decl
{
  // Forbidden
  template_tparameter();

public:

  /// A hasher for instances of template_tparameter
  struct hash;

  template_tparameter(unsigned index,
			      const std::string& name,
			      location locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus, name, VISIBILITY_DEFAULT),
      type_tparameter(index, name, locus)
  {}

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual ~template_tparameter();
};

/// This abstracts a composition of types based on template type
/// parameters.  The result of the composition is a type that can be
/// referred to by a template non-type parameter.  Instances of this
/// type can appear at the same level as template parameters, in the
/// scope of a template_decl.
class type_composition : public template_parameter, public virtual decl_base
{
  shared_ptr<type_base> type_;

  type_composition();

public:
  type_composition(unsigned			index,
		   shared_ptr<type_base>	composed_type);

  shared_ptr<type_base>
  get_composed_type() const
  {return type_;}

  void
  set_composed_type(shared_ptr<type_base> t)
  {type_ = t;}


  virtual ~type_composition();
};

/// Convenience typedef for a shared pointer on a @ref function_tdecl
typedef shared_ptr<function_tdecl> function_tdecl_sptr;

/// Abstract a function template declaration.
class function_tdecl : public template_decl, public scope_decl
{
  shared_ptr<function_decl> pattern_;
  binding binding_;

  // Forbidden
  function_tdecl();

public:

  /// Hash functor for function templates.
  struct hash;
  struct shared_ptr_hash;

  function_tdecl(location locus,
			 visibility vis = VISIBILITY_DEFAULT,
			 binding bind = BINDING_NONE)
  : decl_base("", locus, "", vis), scope_decl("", locus),
    binding_(bind)
  {}

  function_tdecl(shared_ptr<function_decl> pattern,
			 location locus,
			 visibility vis = VISIBILITY_DEFAULT,
			 binding bind = BINDING_NONE)
  : decl_base(pattern->get_name(), locus,
	      pattern->get_name(), vis),
    scope_decl(pattern->get_name(), locus),
    binding_(bind)
  {set_pattern(pattern);}

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  void
  set_pattern(shared_ptr<function_decl> p)
  {
    pattern_ = p;
    add_decl_to_scope(p, this);
    set_name(p->get_name());
  }

  shared_ptr<function_decl>
  get_pattern() const
  {return pattern_;}

  binding
  get_binding() const
  {return binding_;}

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~function_tdecl();
}; // end class function_tdecl.

/// Convenience typedef for a shared pointer on a @ref class_tdecl
typedef shared_ptr<class_tdecl> class_tdecl_sptr;

/// Abstract a class template.
class class_tdecl : public template_decl, public scope_decl
{
  shared_ptr<class_decl> pattern_;

  // Forbidden
  class_tdecl();

public:

  /// Hashers.
  struct hash;
  struct shared_ptr_hash;

  class_tdecl(location locus, visibility vis = VISIBILITY_DEFAULT)
  : decl_base("", locus, "", vis), scope_decl("", locus)
  {}

  class_tdecl(shared_ptr<class_decl> pattern,
	      location locus, visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual bool
  operator==(const class_tdecl&) const;

  void
  set_pattern(shared_ptr<class_decl> p);

  shared_ptr<class_decl>
  get_pattern() const
  {return pattern_;}

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~class_tdecl();
};// end class class_tdecl

/// Convenience typedef for a shared pointer on a @ref class_decl
typedef shared_ptr<class_decl> class_decl_sptr;

/// Abstracts a class declaration.
class class_decl : public scope_type_decl
{
  // Forbidden
  class_decl();

public:
  /// Hasher.
  struct hash;

  /// Language access specifier.
  enum access_specifier
  {
    no_access,
    private_access,
    protected_access,
    public_access
  };

  /// Forward declarations.
  class member_base;
  class member_type;
  class base_spec;
  class data_member;
  class method_decl;
  class member_function;
  class member_function_template;
  class member_class_template;

  /// Convenience typedef
  /// @{
  typedef shared_ptr<base_spec>			base_spec_sptr;
  typedef std::vector<base_spec_sptr>			base_specs;
  typedef shared_ptr<member_type>			member_type_sptr;
  typedef std::vector<member_type_sptr>		member_types;
  typedef shared_ptr<data_member>			data_member_sptr;
  typedef std::vector<data_member_sptr>		data_members;
  typedef shared_ptr<member_function>			member_function_sptr;
  typedef std::vector<member_function_sptr>		member_functions;
  typedef shared_ptr<member_function_template>		member_function_template_sptr;
  typedef std::vector<member_function_template_sptr>	member_function_templates;
  typedef shared_ptr<member_class_template>		member_class_template_sptr;
  typedef std::vector<member_class_template_sptr>	member_class_templates;
  /// @}

private:
  mutable bool			hashing_started_;
  class_decl_sptr		declaration_;
  bool				is_declaration_only_;
  class_decl_sptr		definition_of_declaration_;
  base_specs			bases_;
  member_types			member_types_;
  data_members			data_members_;
  member_functions		member_functions_;
  member_function_templates	member_function_templates_;
  member_class_templates	member_class_templates_;

protected:

  virtual decl_base_sptr
  add_member_decl(decl_base_sptr);

  virtual decl_base_sptr
  insert_member_decl(decl_base_sptr member, declarations::iterator before);

  virtual void
  remove_member_decl(decl_base_sptr);

public:

  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, location locus, visibility vis,
	     base_specs& bases, member_types& mbrs,
	     data_members& data_mbrs, member_functions& member_fns);

  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, location locus, visibility vis);

  class_decl(const std::string& name, bool is_declaration_only = true);

  bool
  hashing_started() const
  {return hashing_started_;}

  void
  hashing_started(bool b) const
  {hashing_started_ = b;}

  virtual string
  get_pretty_representation() const;

  bool
  is_declaration_only() const
  {return is_declaration_only_;}

  void
  set_definition_of_declaration(class_decl_sptr);

  class_decl_sptr
  get_definition_of_declaration() const;

  void
  set_earlier_declaration(class_decl_sptr declaration);

  void
  set_earlier_declaration(type_base_sptr declaration);

  class_decl_sptr
  get_earlier_declaration() const
  {return declaration_;}

  void
  add_base_specifier(shared_ptr<base_spec> b)
  {bases_.push_back(b);}

  const base_specs&
  get_base_specifiers() const
  {return bases_;}

  void
  insert_member_type(member_type_sptr t,
		     declarations::iterator before);

  void
  add_member_type(member_type_sptr t);

  member_type_sptr
  add_member_type(type_base_sptr t, access_specifier a);

  void
  remove_member_type(type_base_sptr t);

  const member_types&
  get_member_types() const
  {return member_types_;}

  void
  add_data_member(data_member_sptr m);

  void
  add_data_member(var_decl_sptr v, access_specifier a,
		  bool is_laid_out, bool is_static,
		  size_t offset_in_bits);

  const data_members&
  get_data_members() const
  {return data_members_;}

  void
  add_member_function(member_function_sptr m);

  void
  add_member_function(function_decl_sptr f,
		      access_specifier a,
		      size_t vtable_offset,
		      bool is_static, bool is_ctor,
		      bool is_dtor, bool is_const);

  const member_functions&
  get_member_functions() const
  {return member_functions_;}

  size_t
  get_num_virtual_functions() const;

  void
  add_member_function_template(shared_ptr<member_function_template>);

  const member_function_templates&
  get_member_function_templates() const
  {return member_function_templates_;}

  void
  add_member_class_template(shared_ptr<member_class_template> m);

  const member_class_templates&
  get_member_class_templates() const

  {return member_class_templates_;}

  bool
  has_no_base_nor_member() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  bool
  operator==(const class_decl&) const;

  virtual void
  traverse(ir_node_visitor& v);

  virtual ~class_decl();
};// end class class_decl

std::ostream&
operator<<(std::ostream&, class_decl::access_specifier);

bool
operator==(class_decl_sptr l, class_decl_sptr r);

/// The base class for member types, data members and member
/// functions.  Its purpose is mainly to carry the access specifier
/// (and possibly other properties that might be shared by all class
/// members) for the member.
class class_decl::member_base
{
protected:
  enum access_specifier access_;
  bool			is_static_;

private:
  // Forbidden
  member_base();

public:
  /// Hasher.
  struct hash;

  member_base(access_specifier a, bool is_static = false)
    : access_(a), is_static_(is_static)
  {}

  access_specifier
  get_access_specifier() const
  {return access_;}

  bool
  is_static() const
  {return is_static_;}

  virtual bool
  operator==(const member_base& o) const;
};// end class class_decl::member_base

/// Abstracts a member type declaration.
///
/// It's important to understand the interactions between this type
/// and the other types.  When a type T appears in the scope of a
/// class, it becomes a member type MT.  T is said to be the
/// underlying type of MT.  MT and T are different types.  In
/// practice, when the function class_decl::add_member_type is given a
/// type T, it adds it to the class scope and returns MT, which is the
/// resulting member type that is created.  T can be retrieved from MT
/// by invoking either MT::get_underlying_type(), or by invoking
/// as_non_member_type(MT).
class class_decl::member_type : public member_base,
				public virtual decl_base,
				public virtual type_base
{
  type_base_sptr type_;

  //Forbidden
  member_type();

public:
  // Hasher.
  struct hash;

  member_type(shared_ptr<type_base> t, access_specifier access);

  virtual void
  set_scope(scope_decl*);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const member_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation() const;

  void
  set_access_specifier(access_specifier);

  type_base_sptr
  get_underlying_type() const;

  bool
  operator==(const member_type&) const;
};// end class member_type

bool
operator==(class_decl::member_type_sptr l, class_decl::member_type_sptr r);

/// Abstraction of a base specifier in a class declaration.
class class_decl::base_spec : public member_base
{

  shared_ptr<class_decl>	base_class_;
  long				offset_in_bits_;
  bool				is_virtual_;

  // Forbidden
  base_spec();

public:

  /// Hasher.
  struct hash;

  base_spec(shared_ptr<class_decl> base, access_specifier a,
	    long offset_in_bits = -1, bool is_virtual = false);

  base_spec(shared_ptr<type_base> base, access_specifier a,
	    long offset_in_bits = -1, bool is_virtual = false);

  const shared_ptr<class_decl>
  get_base_class() const
  {return base_class_;}

  bool
  get_is_virtual() const
  {return is_virtual_;}

  long
  get_offset_in_bits() const
  {return offset_in_bits_;}

  virtual bool
  operator==(const member_base&) const;
};// end class class_decl::base_spec

bool
operator==(class_decl::base_spec_sptr l, class_decl::base_spec_sptr r);

/// Abstract a data member declaration in a class declaration.
class class_decl::data_member : public var_decl, public member_base
{
  bool		is_laid_out_;
  size_t	offset_in_bits_;

  // Forbidden
  data_member();

public:

  /// Hasher.
  struct hash;

  /// Constructor for instances of class_decl::data_member.
  ///
  /// @param data_member the variable to be used as data member.
  ///
  /// @param access the access specifier for the data member.
  ///
  /// @param is_laid_out set to true if the data member has been laid out.
  ///
  /// @param is_static set ot true if the data member is static.
  ///
  /// @param offset_in_bits the offset of the data member, expressed in bits.
  data_member(shared_ptr<var_decl> data_member, access_specifier access,
	      bool is_laid_out, bool is_static, size_t offset_in_bits)
    : decl_base(data_member->get_name(),
		data_member->get_location(),
		data_member->get_mangled_name(),
		data_member->get_visibility()),
      var_decl(data_member->get_name(),
	       data_member->get_type(),
	       data_member->get_location(),
	       data_member->get_mangled_name(),
	       data_member->get_visibility(),
	       data_member->get_binding()),
      member_base(access, is_static),
    is_laid_out_(is_laid_out),
    offset_in_bits_(offset_in_bits)
  {}


  /// Constructor for instances of class_decl::data_member.
  ///
  /// @param name the name of the data member.
  ///
  /// @param type the type of the data member.
  ///
  /// @param access the access specifier for the data member.
  ///
  /// @param locus the source location of the data member.
  ///
  /// @param mangled_name the mangled name of the data member, or an
  /// empty string if not applicable.
  ///
  /// @param vis the visibility of the data member.
  ///
  /// @param bind the binding of the data member.
  ///
  /// @param is_laid_out set to true if the data member has been laid out.
  ///
  /// @param is_static set ot true if the data member is static.
  ///
  /// @param offset_in_bits the offset of the data member, expressed in bits.
  data_member(const std::string& name,
	      shared_ptr<type_base>& type,
	      access_specifier access,
	      location locus,
	      const std::string& mangled_name,
	      visibility vis,
	      binding	bind,
	      bool is_laid_out,
	      bool is_static,
	      size_t offset_in_bits)
    : decl_base(name, locus, mangled_name, vis),
      var_decl(name, type, locus, mangled_name, vis, bind),
      member_base(access, is_static),
      is_laid_out_(is_laid_out),
      offset_in_bits_(offset_in_bits)
  {}

  bool
  is_laid_out() const
  {return is_laid_out_;}

  size_t
  get_offset_in_bits() const
  {return offset_in_bits_;}

  virtual bool
  operator==(const decl_base& other) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  virtual void
  traverse(ir_node_visitor&);

  virtual ~data_member();
};// end class class_decl::data_member

bool
operator==(class_decl::data_member_sptr l, class_decl::data_member_sptr r);

/// Abstraction of the declaration of a method. This is an
/// implementation detail for class_decl::member_function.
class class_decl::method_decl : public function_decl
{
  method_decl();

public:

  method_decl(const std::string&  name,
	      const std::vector<parameter_sptr >& parms,
	      shared_ptr<type_base> return_type,
	      shared_ptr<class_decl> class_type,
	      size_t	ftype_size_in_bits,
	      size_t ftype_align_in_bits,
	      bool declared_inline,
	      location locus,
	      const std::string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding bind = BINDING_GLOBAL);

  method_decl(const std::string& name, shared_ptr<method_type> type,
	      bool declared_inline, location locus,
	      const std::string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

  method_decl(const std::string& name,
	      shared_ptr<function_type> type,
	      bool declared_inline,
	      location locus,
	      const std::string& mangled_name = "",
	      visibility vis  = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

  method_decl(const std::string& name, shared_ptr<type_base> type,
	      bool declared_inline, location locus,
	      const std::string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding bind = BINDING_GLOBAL);

  /// @return the type of the current instance of the
  /// class_decl::method_decl.
  const shared_ptr<method_type>
  get_type() const;

  void
  set_type(shared_ptr<method_type> fn_type)
  { function_decl::set_type(fn_type); }

  virtual ~method_decl();
};// end class class_decl::method_decl

/// Abstracts a member function declaration in a class declaration.
class class_decl::member_function : public method_decl, public member_base
{
  size_t vtable_offset_in_bits_;
  bool is_constructor_;
  bool is_destructor_;
  bool is_const_;

  // Forbidden
  member_function();

public:

  /// Hasher.
  struct hash;

  member_function(const std::string&	name,
		  std::vector<parameter_sptr > parms,
		  shared_ptr<type_base>	return_type,
		  shared_ptr<class_decl>	class_type,
		  size_t			ftype_size_in_bits,
		  size_t			ftype_align_in_bits,
		  access_specifier		access,
		  bool			declared_inline,
		  location		locus,
		  const std::string&	mangled_name,
		  visibility		vis,
		  binding		bind,
		  size_t		vtable_offset_in_bits,
		  bool			is_static,
		  bool			is_constructor,
		  bool			is_destructor,
		  bool			is_const)
  : decl_base(name, locus, name, vis),
    method_decl(name, parms, return_type, class_type,
		ftype_size_in_bits, ftype_align_in_bits,
		declared_inline, locus,
		mangled_name, vis, bind),
    member_base(access, is_static),
    vtable_offset_in_bits_(vtable_offset_in_bits),
    is_constructor_(is_constructor),
    is_destructor_(is_destructor),
    is_const_(is_const)
  { }

  member_function(shared_ptr<method_decl>	fn,
		  access_specifier		access,
		  size_t			vtable_offset_in_bits,
		  bool				is_static,
		  bool				is_constructor,
		  bool				is_destructor,
		  bool				is_const)
    : decl_base(fn->get_name(), fn->get_location(),
		fn->get_mangled_name(), fn->get_visibility()),
      method_decl(fn->get_name(),
		  fn->get_type(),
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

  member_function(shared_ptr<function_decl>	fn,
		  access_specifier		access,
		  size_t			vtable_offset_in_bits,
		  bool				is_static,
		  bool				is_constructor,
		  bool				is_destructor,
		  bool				is_const);

  size_t
  get_vtable_offset() const
  {return vtable_offset_in_bits_;}

  bool
  is_constructor() const
  {return is_constructor_;}

  bool
  is_destructor() const
  {return is_destructor_;}

  bool
  is_const() const
  {return is_const_;}

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const member_base&) const;

  bool
  operator==(const member_function&) const;

  virtual void
  traverse(ir_node_visitor& v);
};// class_decl::member_function

bool
operator==(class_decl::member_function_sptr l,
	   class_decl::member_function_sptr r);

/// Abstract a member function template.
class class_decl::member_function_template
  : public member_base, public virtual decl_base
{
  bool is_constructor_;
  bool is_const_;
  shared_ptr<function_tdecl> fn_tmpl_;

  // Forbiden
  member_function_template();

public:
  /// Hasher.
  struct hash;

  member_function_template(function_tdecl_sptr f,
			   access_specifier access, bool is_static,
			   bool is_constructor, bool is_const)
    : decl_base(f->get_name(), location()),
      member_base(access, is_static), is_constructor_(is_constructor),
      is_const_(is_const), fn_tmpl_(f)
  {}

  bool
  is_constructor() const
  {return is_constructor_;}

  bool
  is_const() const
  {return is_const_;}

  operator const function_tdecl& () const
  {return *fn_tmpl_;}

  function_tdecl_sptr
  as_function_tdecl() const
  {return fn_tmpl_;}

  virtual bool
  operator==(const member_base& o) const;

  virtual void
  traverse(ir_node_visitor&);
};// end class class_decl::member_function_template

bool
operator==(class_decl::member_function_template_sptr l,
	   class_decl::member_function_template_sptr r);

/// Abstracts a member class template template
class class_decl::member_class_template
  : public member_base,
    public virtual decl_base
{
  shared_ptr<class_tdecl> class_tmpl_;

  // Forbidden
  member_class_template();

public:

  /// Hasher.
  struct hash;

  member_class_template(shared_ptr<class_tdecl> c,
			access_specifier access, bool is_static)
    : decl_base(c->get_name(), location()),
      member_base(access, is_static),
      class_tmpl_(c)
  {}

  operator const class_tdecl& () const
  { return *class_tmpl_; }

  shared_ptr<class_tdecl>
  as_class_tdecl() const
  {return class_tmpl_;}

  virtual bool
  operator==(const member_base& o) const;

  virtual bool
  operator==(const member_class_template&) const;

  virtual void
  traverse(ir_node_visitor& v);
};// end class class_decl::member_class_template

bool
operator==(class_decl::member_class_template_sptr l,
	   class_decl::member_class_template_sptr r);

// Forward declarations for select nested hashers.
struct type_base::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<type_base> t) const;
};

struct type_base::dynamic_hash
{
  size_t
  operator()(const type_base* t) const;
};

/// A hasher that manages the cache the computed hash and re-use it if
/// it is available.
struct type_base::cached_hash
{
  size_t
  operator() (const type_base* t) const;

  size_t
  operator() (const type_base_sptr t) const;
};

/// The hashing functor for class_decl::base_spec.
struct class_decl::base_spec::hash
{
  size_t
  operator()(const base_spec& t) const;
};

/// The hashing functor for class_decl::member_base.
struct class_decl::member_base::hash
{
  size_t
  operator()(const member_base& m) const;
};

/// The hashing functor for class_decl::member_type.
struct class_decl::member_type::hash
{
  size_t
  operator()(const member_type& t) const ;
};

/// The hashing functor for class_decl::data_member.
struct class_decl::data_member::hash
{
  size_t
  operator()(const data_member& t) const;
};

/// The hashing functor for class_decl::member_function.
struct class_decl::member_function::hash
{
  size_t
  operator()(const member_function& t) const;
};

/// The hashing functor for class_decl::member_function_template.
struct class_decl::member_function_template::hash
{
  size_t
  operator()(const member_function_template& t) const;
};

/// The hashing functor for class_decl::member_class_template
struct class_decl::member_class_template::hash
{
  size_t
  operator()(const member_class_template& t) const;
};

struct function_tdecl::hash
{
  size_t
  operator()(const function_tdecl& t) const;
};

struct function_tdecl::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<function_tdecl> f) const;
};

struct class_tdecl::hash
{
  size_t
  operator()(const class_tdecl& t) const;
};

struct class_tdecl::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<class_tdecl> t) const;
};

/// The base class for the visitor type hierarchy used for traversing
/// a translation unit.
///
/// Client code willing to get notified for a certain kind of node
/// during the IR traversal might want to define a visitor class that
/// inherit ir_node_visitor, overload the ir_node_visitor::visit
/// method of its choice, and provide and implementation for it.  That
/// new visitor class would then be passed to e.g,
/// translation_unit::traverse or to the traverse method of any type
/// where the traversal is supposed to start from.
struct ir_node_visitor : public node_visitor_base
{
  virtual void visit(scope_decl*);
  virtual void visit(type_decl*);
  virtual void visit(namespace_decl*);
  virtual void visit(qualified_type_def*);
  virtual void visit(pointer_type_def*);
  virtual void visit(reference_type_def*);
  virtual void visit(enum_type_decl*);
  virtual void visit(typedef_decl*);
  virtual void visit(var_decl*);
  virtual void visit(function_decl*);
  virtual void visit(function_tdecl*);
  virtual void visit(class_tdecl*);
  virtual void visit(class_decl*);
  virtual void visit(class_decl::data_member*);
  virtual void visit(class_decl::member_function*);
  virtual void visit(class_decl::member_function_template*);
  virtual void visit(class_decl::member_class_template*);
};

} // end namespace abigail
#endif // __ABG_IR_H__
