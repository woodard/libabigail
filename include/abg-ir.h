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
  { return value_ == other.value_; }

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
  struct _Impl;

  /// Pimpl.
  shared_ptr<_Impl>			priv_;

public:

  location_manager();

  /// Insert the triplet representing a source locus into our internal
  /// vector of location triplet.  Return an instance of location type,
  /// built from an integral type that represents the index of the
  /// source locus triplet into our source locus table.
  ///
  /// @param fle the file path of the source locus
  /// @param lne the line number of the source location
  /// @param col the column number of the source location
  location
  create_new_location(const std::string& fle, size_t lne, size_t col);

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
  expand_location(const location location, std::string& path,
		  unsigned& line, unsigned& column) const;
};

struct ir_node_visitor;

/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// into a translation unit.
class translation_unit : public traversable_base
{
public:

  typedef shared_ptr<global_scope>		global_scope_sptr;

private:
  std::string				path_;
  location_manager			loc_mgr_;
  mutable global_scope_sptr		global_scope_;

  // Forbidden
  translation_unit();

public:


  translation_unit(const std::string& path);

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

  bool
  read();

  bool
  read(const string& buffer);

  bool
  write(std::ostream& out) const;

  bool
  write(const string& path) const;

  void
  traverse(ir_node_visitor& v);
};//end class translation_unit

typedef shared_ptr<translation_unit> translation_unit_sptr;
typedef std::vector<translation_unit_sptr> translation_units;

/// The base type of all declarations.
class decl_base : public traversable_base
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

private:

  location				location_;
  std::string				name_;
  std::string				mangled_name_;
  scope_decl*				context_;
  visibility				visibility_;

  // Forbidden
  decl_base();

  /// Setter of the scope of the current decl.
  ///
  /// Note that the decl won't hold a reference on the scope.  It's
  /// rather the scope that holds a reference on its members.
  void
  set_scope(scope_decl*);

public:

  decl_base(const std::string&	name, location locus,
	    const std::string&	mangled_name = "",
	    visibility vis = VISIBILITY_DEFAULT);

  decl_base(location);

  decl_base(const decl_base&);

  /// Return true iff the two decls have the same name.
  ///
  /// This function doesn't test if the scopes of the the two decls are
  /// equal.
  virtual bool
  operator==(const decl_base&) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the member nodes of the translation
  /// unit during the traversal.
  void
  traverse(ir_node_visitor& v);

  virtual ~decl_base();

  location
  get_location() const
  {return location_;}

  void
  set_location(const location& l)
  { location_ = l; }

  const string&
  get_name() const
  {return name_;}

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
  { visibility_ = v; }

  friend void
  add_decl_to_scope(shared_ptr<decl_base> dcl, scope_decl* scpe);
};

/// A declaration that introduces a scope.
class scope_decl : public virtual decl_base
{
public:
  typedef std::list<shared_ptr<decl_base> >	declarations;
  typedef std::list<shared_ptr<scope_decl> >	scopes;

private:
  declarations			members_;
  scopes			member_scopes_;

  scope_decl();

  /// Add a member decl to this scope.  Note that user code should not
  /// use this, but rather use add_decl_to_scope.
  ///
  /// @param member the new member decl to add to this scope.
  void
  add_member_decl(const shared_ptr<decl_base> member);

public:
  scope_decl(const std::string& name, location locus,
	     visibility	vis = VISIBILITY_DEFAULT)
  : decl_base(name, locus, /*mangled_name=*/name, vis)
  {}


  scope_decl(location l) : decl_base("", l)
  {}

  /// Return true iff both scopes have the same names and have the same
  /// member decls.
  ///
  /// This function doesn't check for equality of the scopes of its
  /// arguments.
  virtual bool
  operator==(const scope_decl&) const;

  const declarations&
  get_member_decls() const
  {return members_;}

  const scopes&
  get_member_scopes() const
  {return member_scopes_;}

  bool
  is_empty() const
  {return get_member_decls().empty();}

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance of scope_decl
  /// and on its member nodes.
  void
  traverse(ir_node_visitor&);

  virtual ~scope_decl();

  friend void
  add_decl_to_scope(shared_ptr<decl_base> dcl, scope_decl* scpe);
};


/// This abstracts the global scope of a given translation unit.
///
/// Only one instance of this class must be present in a given
/// translation_unit.  That instance is implicitely created the first
/// time translatin_unit::get_global_scope is invoked.
class global_scope : public scope_decl
{
  translation_unit*		translation_unit_;

  global_scope(translation_unit *tu)
  : decl_base("", location()), scope_decl("", location()),
    translation_unit_(tu)
  { }

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
  size_t			size_in_bits_;
  size_t			alignment_in_bits_;

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

  type_base(size_t s, size_t a);

  /// Return true iff both type declarations are equal.
  ///
  /// Note that this doesn't test if the scopes of both types are equal.
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
};


/// A predicate for deep equality of instances of shared_ptr<type_base>
struct type_shared_ptr_equal
{
  bool
  operator()(const shared_ptr<type_base> l, const shared_ptr<type_base> r) const
  {
    if (l != r)
      return false;

    if (l)
      return *l == *r;

    return true;
  }
};

/// A basic type declaration that introduces no scope.
class type_decl
: public virtual decl_base, public virtual type_base
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

  /// Return true if both types equals.
  ///
  /// Note that this does not check the scopes of any of the types.
  virtual bool
  operator==(const type_decl&) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor&);

  virtual ~type_decl();
};

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

  /// Return true iff both scope types are equal.
  ///
  /// Note that this function does not consider the scope of the scope
  /// types themselves.
  virtual bool
  operator==(const scope_type_decl&) const;

  virtual ~scope_type_decl();
};

/// The abstraction of a namespace declaration
class namespace_decl : public scope_decl
{
public:

  namespace_decl(const std::string& name, location locus,
		 visibility vis = VISIBILITY_DEFAULT);

  /// Return true iff both namespaces and their members are equal.
  ///
  /// Note that this function does not check if the scope of these
  /// namespaces are equal.
  virtual bool
  operator==(const namespace_decl&) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on its
  /// member nodes.
  void
  traverse(ir_node_visitor&);

  virtual ~namespace_decl();
};

/// The abstraction of a qualified type.
class qualified_type_def
: public virtual type_base, public virtual decl_base
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

  /// Constructor of the qualified_type_def
  ///
  /// @param type the underlying type
  ///
  /// @param quals a bitfield representing the const/volatile qualifiers
  ///
  /// @param locus the location of the qualified type definition
  qualified_type_def(shared_ptr<type_base> type, CV quals, location locus);

  /// Return true iff both qualified types are equal.
  ///
  /// Note that this function does not check for equality of the scopes.
  virtual bool
  operator==(const qualified_type_def&) const;

  /// Getter of the const/volatile qualifier bit field
  char
  get_cv_quals() const;

  /// Setter of the const/value qualifiers bit field
  void
  set_cv_quals(char cv_quals);

  /// Getter of the underlying type
  const shared_ptr<type_base>
  get_underlying_type() const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);

  virtual ~qualified_type_def();
};

/// Overloaded bitwise OR operator for cv qualifiers.
qualified_type_def::CV
operator|(qualified_type_def::CV, qualified_type_def::CV);

/// The abstraction of a pointer type.
class pointer_type_def
: public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base>			pointed_to_type_;

  // Forbidden.
  pointer_type_def();

public:

  /// A hasher for instances of pointer_type_def
  struct hash;

  pointer_type_def(shared_ptr<type_base>& pointed_to_type, size_t size_in_bits,
		   size_t alignment_in_bits, location locus);

  /// Return true iff both instances of pointer_type_def are equal.
  ///
  /// Note that this function does not check for the scopes of the this
  /// types.
  virtual bool
  operator==(const pointer_type_def&) const;

  shared_ptr<type_base>
  get_pointed_to_type() const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);

  virtual ~pointer_type_def();
};


/// Abstracts a reference type.
class reference_type_def
: public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base> pointed_to_type_;
  bool			is_lvalue_;

  // Forbidden.
  reference_type_def();

public:

  /// Hasher for intances of reference_type_def.
  struct hash;

  reference_type_def(shared_ptr<type_base>& pointed_to_type,
		     bool lvalue, size_t size_in_bits,
		     size_t alignment_in_bits, location locus);

  virtual bool
  operator==(const reference_type_def&) const;

  shared_ptr<type_base>
  get_pointed_to_type() const;

  bool
  is_lvalue() const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);

  virtual ~reference_type_def();
};

/// Abstracts a declaration for an enum type.
class enum_type_decl
: public virtual type_base, public virtual decl_base
{
public:

  /// A hasher for an enum_type_decl.
  struct hash;

  /// Enumerator Datum.
  class enumerator
  {
    string	name_;
    size_t	value_;

    //Forbidden
    enumerator();

  public:

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

  typedef std::list<enumerator> enumerators;
  typedef shared_ptr<type_base> type_sptr;

private:

  type_sptr	underlying_type_;
  enumerators	enumerators_;

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
  enum_type_decl(const string& name, location locus, type_sptr underlying_type,
		 enumerators& enms, const std::string& mangled_name = "",
		 visibility vis = VISIBILITY_DEFAULT)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, mangled_name, vis),
    underlying_type_(underlying_type),
    enumerators_(enms)
  { }

  /// Return the underlying type of the enum.
  type_sptr
  get_underlying_type() const;

  /// Return the list of enumerators of the enum.
  const enumerators&
  get_enumerators() const;

  /// Equality operator.
  ///
  /// @param other the other enum to test against.
  ///
  /// @return true iff other is equals the current instance of enum type
  /// decl.
  virtual bool
  operator==(const enum_type_decl&) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);

  virtual ~enum_type_decl();
};

/// The abstraction of a typedef declaration.
class typedef_decl
: public virtual type_base, public virtual decl_base
{
  shared_ptr<type_base> underlying_type_;

  // Forbidden
  typedef_decl();

public:

  /// Hasher for the typedef_decl type.
  struct hash;

  /// Constructor of the typedef_decl type.
  ///
  /// @param name the name of the typedef.
  ///
  /// @param underlying_type the underlying type of the typedef.
  ///
  /// @param locus the source location of the typedef declaration.
  typedef_decl(const string& name, const shared_ptr<type_base> underlying_type,
	       location locus, const std::string& mangled_name = "",
	       visibility vis = VISIBILITY_DEFAULT);

  /// Equality operator
  ///
  /// @param other the other typedef_decl to test against.
  virtual bool
  operator==(const typedef_decl&) const;

  /// Getter of the underlying type of the typedef.
  ///
  /// @return the underlying_type.
  shared_ptr<type_base>
  get_underlying_type() const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor&);

  virtual ~typedef_decl();
};

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
  operator==(const var_decl&) const;

  shared_ptr<type_base>
  get_type() const
  {return type_;}

  binding
  get_binding() const
  {return binding_;}

  void
  set_binding(binding b)
  {binding_ = b;}


  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);

  virtual ~var_decl();
};

/// Abstraction for a function declaration.
class function_decl : public virtual decl_base
{
protected:
  shared_ptr<function_type>	type_;

private:
  bool			declared_inline_;
  decl_base::binding	binding_;

public:
  /// Hasher for function_decl
  struct hash;

  /// Abtraction for the parameter of a function.
  class parameter
  {
    shared_ptr<type_base>	type_;
    std::string		name_;
    location			location_;
    bool			variadic_marker_;

  public:

    /// Hasher for an instance of function::parameter
    struct hash;

    parameter(const shared_ptr<type_base> type, const std::string& name,
	      location loc, bool variadic_marker = false)
    : type_(type), name_(name), location_(loc),
      variadic_marker_ (variadic_marker)
    { }

    parameter(const shared_ptr<type_base> type, bool variadic_marker = false)
    : type_(type),
      variadic_marker_ (variadic_marker)
    { }

    const shared_ptr<type_base>
    get_type()const
    {return type_;}

    const shared_ptr<type_base>
    get_type()
    {return type_;}

    const std::string&
    get_name() const
    {return name_;}

    location
    get_location() const
    {return location_;}

    bool
    operator==(const parameter& o) const
    {return *get_type() == *o.get_type();}

    bool
    get_variadic_marker() const
    {return variadic_marker_;}
  };

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
  function_decl(const std::string&  name,
		const std::vector<shared_ptr<parameter> >& parms,
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
  { }

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
  function_decl(const std::string& name,
		shared_ptr<type_base> fn_type,
		bool declared_inline,
		location locus,
		const std::string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL);

  /// @return the parameters of the function.
  const std::vector<shared_ptr<parameter> >&
  get_parameters() const;

  /// Append a parameter to the type of this function.
  ///
  /// @param parm the parameter to append.
  void
  append_parameter(shared_ptr<parameter> parm);

  /// Append a vector of parameters to the type of this function.
  ///
  /// @param parms the vector of parameters to append.
  void
  append_parameters(std::vector<shared_ptr<parameter> >& parms);

  /// Return the type of the current instance of #function_decl.
  ///
  /// It's either a function_type or method_type.
  /// @return the type of the current instance of #function_decl.
  const shared_ptr<function_type>
  get_type() const;

  /// @return the return type of the current instance of function_decl.
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
  operator==(const function_decl& o) const;

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

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor&);

  virtual ~function_decl();
};

/// Abstraction of a function type.
class function_type : public virtual type_base
{
public:
  /// Hasher for an instance of function_type
  struct hash;

  typedef shared_ptr<function_decl::parameter>	parameter_sptr;
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
  { }

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
  { }

  /// A constructor for a function_type that takes no parameter and
  /// that has no return_type yet.  These missing parts can (and must)
  /// be added later.
  ///
  /// @param size_in_bits the size of this type, in bits.
  ///
  /// @param alignment_in_bits the alignment of this type, in bits.
  function_type(size_t size_in_bits, size_t alignment_in_bits)
  : type_base(size_in_bits, alignment_in_bits)
  { }

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
  {parms_.push_back(parm);}

  bool
  is_variadic() const
  {return !parms_.empty() && parms_.back()->get_variadic_marker();}

  bool
  operator==(const function_type&) const;

  virtual ~function_type();
};

/// Abstracts the type of a class member function.
class method_type : public function_type
{

  shared_ptr<class_decl> class_type_;

  method_type();

public:

  /// Hasher for intances of method_type
  struct hash;

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
  method_type(shared_ptr<type_base> return_type,
	      shared_ptr<class_decl> class_type,
	      const std::vector<shared_ptr<function_decl::parameter> >& parms,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

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
  method_type(shared_ptr<type_base> return_type,
	      shared_ptr<type_base> class_type,
	      const std::vector<shared_ptr<function_decl::parameter> >& parms,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

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
  method_type(shared_ptr<class_decl> class_type,
	      size_t size_in_bits,
	      size_t alignment_in_bits);

  /// Constructor of instances of method_type.
  ///
  /// When constructed with this constructor, and instane of method_type
  /// must set a return type and class type using
  /// method_type::set_return_type and method_type::set_class_type.
  ///
  /// @param size_in_bits the size of an instance of method_type,
  /// expressed in bits.
  ///
  /// @param alignment_in_bits the alignment of an instance of
  /// method_type, expressed in bits.
  method_type(size_t size_in_bits,
	      size_t alignment_in_bits);

  shared_ptr<class_decl>
  get_class_type() const
  {return class_type_;}

  /// Sets the class type of the current instance of method_type.
  ///
  /// The class type is the type of the class the method belongs to.
  ///
  /// @param t the new class type to set.
  void
  set_class_type(shared_ptr<class_decl> t);

  virtual ~method_type();
};

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
};


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
};

/// Abstracts a type template parameter.
class type_tparameter
: public template_parameter, public virtual type_decl
{
  // Forbidden
  type_tparameter();

public:

  /// Hasher.
  struct hash;

  type_tparameter(unsigned index, const std::string& name,
			  location locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus),
      template_parameter(index)
  {}

  virtual bool
  operator==(const type_tparameter&) const;

  virtual ~type_tparameter();
};

/// Abstracts non type template parameters.
class non_type_tparameter
: public template_parameter, public virtual decl_base
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
  operator==(const non_type_tparameter&) const;


  shared_ptr<type_base>
  get_type() const
  {return type_;}

  virtual ~non_type_tparameter();
};

/// Abstracts a template template parameter.
class template_tparameter
: public type_tparameter, public template_decl
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
  operator==(const template_tparameter& o) const;

  virtual ~template_tparameter();
};

/// This abstracts a composition of types based on template type
/// parameters.  The result of the composition is a type that can be
/// referred to by a template non-type parameter.  Instances of this
/// type can appear at the same level as template parameters, in the
/// scope of a template_decl.
class type_composition
: public template_parameter, public virtual decl_base
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

/// Abstract a function template declaration.
class function_tdecl
: public template_decl, public scope_decl
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
  operator==(const function_tdecl&) const;

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

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on the
  /// function pattern of the template.
  void
  traverse(ir_node_visitor& v);

  virtual ~function_tdecl();
};


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

  /// Constructor for the class_tdecl type.
  ///
  /// @param pattrn The details of the class template. This must NOT be a
  /// null pointer.  If you really this to be null, please use the
  /// constructor above instead.
  ///
  /// @param locus the source location of the declaration of the type.
  ///
  /// @param vis the visibility of the instances of class instantiated
  /// from this template.
  class_tdecl(shared_ptr<class_decl> pattrn,
		      location locus, visibility vis = VISIBILITY_DEFAULT);


  virtual bool
  operator==(const class_tdecl&) const;

  void
  set_pattern(shared_ptr<class_decl> p);

  shared_ptr<class_decl>
  get_pattern() const
  {return pattern_;}

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on the class
  /// pattern of the template.
  void
  traverse(ir_node_visitor& v);

  virtual ~class_tdecl();
};


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

  /// Typedefs.
  typedef shared_ptr<base_spec>			base_spec_sptr;
  typedef std::list<base_spec_sptr>			base_specs;
  typedef shared_ptr<member_type>			member_type_sptr;
  typedef std::list<member_type_sptr>			member_types;
  typedef shared_ptr<data_member>			data_member_sptr;
  typedef std::list<data_member_sptr>			data_members;
  typedef shared_ptr<member_function>			member_function_sptr;
  typedef std::list<member_function_sptr>		member_functions;
  typedef shared_ptr<member_function_template>		member_function_template_sptr;
  typedef std::list<member_function_template_sptr>	member_function_templates;
  typedef shared_ptr<member_class_template>		member_class_template_sptr;
  typedef std::list<member_class_template_sptr>	member_class_templates;

private:
  mutable bool			hashing_started_;
  shared_ptr<class_decl>	declaration_;
  bool				is_declaration_only_;
  base_specs			bases_;
  member_types			member_types_;
  data_members			data_members_;
  member_functions		member_functions_;
  member_function_templates	member_function_templates_;
  member_class_templates	member_class_templates_;

public:

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
  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, location locus, visibility vis,
	     base_specs& bases, member_types& mbrs,
	     data_members& data_mbrs, member_functions& member_fns);

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
  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, location locus, visibility vis);

  /// A constuctor for instances of class_decl that represent a
  /// declaration without definition.
  ///
  /// @param name the name of the class.
  ///
  /// @param is_declaration_only a boolean saying whether the instance
  /// represents a declaration only, or not.
  class_decl(const std::string& name, bool is_declaration_only = true);

  bool
  hashing_started() const
  {return hashing_started_;}

  void
  hashing_started(bool b) const
  {hashing_started_ = b;}

  bool
  is_declaration_only() const
  {return is_declaration_only_;}

  /// Set the earlier declaration of this class definition.
  ///
  /// @param declaration the earlier declaration to set.  Note that it's
  /// set only if it's a pure declaration.
  void
  set_earlier_declaration(shared_ptr<class_decl> declaration);

  /// Set the earlier declaration of this class definition.
  ///
  /// @param declaration the earlier declaration to set.  Note that it's
  /// set only if it's a pure declaration.  It's dynamic type must be
  /// pointer to class_decl.
  void
  set_earlier_declaration(shared_ptr<type_base> declaration);

  shared_ptr<class_decl>
  get_earlier_declaration() const
  {return declaration_;}

  void
  add_base_specifier(shared_ptr<base_spec> b)
  {bases_.push_back(b);}

  const base_specs&
  get_base_specifiers() const
  {return bases_;}

  /// Add a member type to the current instance of class_decl
  ///
  /// @param t the member type to add.
  void
  add_member_type(shared_ptr<member_type> t);

  const member_types&
  get_member_types() const
  {return member_types_;}

  /// Add a data member to the current instance of class_decl.
  ///
  /// @param m the data member to add.
  void
  add_data_member(shared_ptr<data_member> m);

  const data_members&
  get_data_members() const
  {return data_members_;}

  /// Add a member function to the current instance of class_decl.
  ///
  /// @param m the member function to add.
  void
  add_member_function(shared_ptr<member_function> m);

  const member_functions&
  get_member_functions() const
  {return member_functions_;}

  /// Append a member function template to the class.
  ///
  /// @param m the member function template to append.
  void
  add_member_function_template(shared_ptr<member_function_template>);

  const member_function_templates&
  get_member_function_templates() const
  {return member_function_templates_;}

  /// Append a member class template to the class.
  ///
  /// @param m the member function template to append.
  void
  add_member_class_template(shared_ptr<member_class_template> m);

  const member_class_templates&
  get_member_class_templates() const

  {return member_class_templates_;}

  /// Return true iff the class has no entity in its scope.
  bool
  has_no_base_nor_member() const;

  virtual bool
  operator==(const class_decl&) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on its members.
  void
  traverse(ir_node_visitor& v);

  virtual ~class_decl();
};// end class class_decl

typedef shared_ptr<class_decl> class_decl_sptr;

/// The base class for member types, data members and member
/// functions.  Its purpose is mainly to carry the access specifier
/// (and possibly other properties that might be shared by all class
/// members) for the member.
class class_decl::member_base
{
  enum access_specifier access_;
  bool			is_static_;

  // Forbidden
  member_base();

public:
  /// Hasher.
  struct hash;

  member_base(access_specifier a, bool is_static = false)
    : access_(a), is_static_(is_static)
  { }

  access_specifier
  get_access_specifier() const
  {return access_;}

  bool
  is_static() const
  {return is_static_;}

  bool
  operator==(const member_base& o) const
  {
    return (get_access_specifier() == o.get_access_specifier()
	    && is_static() == o.is_static());
  }
};// end class class_decl::member_base

/// Abstracts a member type declaration.
class class_decl::member_type : public member_base, public virtual decl_base
{
  shared_ptr<type_base> type_;

  //Forbidden
  member_type();

public:
  // Hasher.
  struct hash;

  member_type(shared_ptr<type_base> t, access_specifier access)
    : decl_base("", location()), member_base(access), type_(t)
  { }

  bool
  operator==(const member_type& o) const
  {
    return (*as_type() == *o.as_type() && static_cast<member_base>(*this) == o);
  }

  operator shared_ptr<type_base>() const
  {return type_;}

  shared_ptr<type_base>
  as_type() const
  {return type_;}
};// end class member_type

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
  base_spec(shared_ptr<class_decl> base, access_specifier a,
	    long offset_in_bits = -1, bool is_virtual = false);

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

  bool
  operator==(const base_spec& other) const
  {
    return (static_cast<member_base>(*this) == other
	    && *get_base_class() == *other.get_base_class());
  }
};// end class base_spec


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
  { }


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
  { }

  bool
  is_laid_out() const
  {return is_laid_out_;}

  size_t
  get_offset_in_bits() const
  {return offset_in_bits_;}

  bool
  operator==(const data_member& other) const
  {
    return (is_laid_out() == other.is_laid_out()
	    && get_offset_in_bits() == other.get_offset_in_bits()
	    && static_cast<var_decl>(*this) ==other
	    && static_cast<member_base>(*this) == other);
  }

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor&);

  virtual ~data_member();
};// end class class_decl::data_member


/// Abstraction of the declaration of a method. This is an
/// implementation detail for class_decl::member_function.
class class_decl::method_decl : public function_decl
{
  method_decl();

public:

  /// A constructor for instances of class_decl::method_decl.
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
  method_decl(const std::string&  name,
	      const std::vector<shared_ptr<parameter> >& parms,
	      shared_ptr<type_base> return_type,
	      shared_ptr<class_decl> class_type,
	      size_t	ftype_size_in_bits,
	      size_t ftype_align_in_bits,
	      bool declared_inline,
	      location locus,
	      const std::string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding bind = BINDING_GLOBAL);

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
  method_decl(const std::string& name, shared_ptr<method_type> type,
	      bool declared_inline, location locus,
	      const std::string& mangled_name = "",
	      visibility vis = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

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
  method_decl(const std::string& name,
	      shared_ptr<function_type> type,
	      bool declared_inline,
	      location locus,
	      const std::string& mangled_name = "",
	      visibility vis  = VISIBILITY_DEFAULT,
	      binding	bind = BINDING_GLOBAL);

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
		  std::vector<shared_ptr<parameter> > parms,
		  shared_ptr<type_base>	return_type,
		  shared_ptr<class_decl>	class_type,
		  size_t			ftype_size_in_bits,
		  size_t			ftype_align_in_bits,
		  access_specifier		access,
		  bool			declared_inline,
		  location			locus,
		  const std::string&	mangled_name,
		  visibility		vis,
		  binding			bind,
		  size_t			vtable_offset_in_bits,
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
		  bool			is_static,
		  bool			is_constructor,
		  bool			is_destructor,
		  bool			is_const)
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
  { }

  /// Constructor for instances of class_decl::member_function.
  ///
  /// @param fn the method decl to be used as a member function.  This
  /// must be an intance of class_decl::method_decl.
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
  member_function(shared_ptr<function_decl>	fn,
		  access_specifier		access,
		  size_t			vtable_offset_in_bits,
		  bool			is_static,
		  bool			is_constructor,
		  bool			is_destructor,
		  bool			is_const);

  size_t
  get_vtable_offset_in_bits() const
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

  bool
  operator==(const member_function& o) const
  {
    return (get_vtable_offset_in_bits() == o.get_vtable_offset_in_bits()
	    && is_constructor() == o.is_constructor()
	    && is_destructor() == o.is_destructor()
	    && is_const() == o.is_const()
	    && static_cast<member_base>(*this) == o
	    && static_cast<function_decl>(*this) == o);
  }

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance.
  void
  traverse(ir_node_visitor& v);
};// class_decl::member_function


/// Abstract a member function template.
class class_decl::member_function_template
: public member_base, public virtual traversable_base
{
  bool is_constructor_;
  bool is_const_;
  shared_ptr<function_tdecl> fn_tmpl_;

  // Forbiden
  member_function_template();

public:
  /// Hasher.
  struct hash;

  member_function_template(shared_ptr<function_tdecl> f,
			   access_specifier access, bool is_static,
			   bool	is_constructor, bool is_const)
  : member_base(access, is_static), is_constructor_(is_constructor),
    is_const_(is_const), fn_tmpl_(f)
  { }

  bool
  is_constructor() const
  {return is_constructor_;}

  bool
  is_const() const
  {return is_const_;}

  operator const function_tdecl& () const
  { return *fn_tmpl_; }

  shared_ptr<function_tdecl>
  as_function_tdecl() const
  {return fn_tmpl_;}

  bool
  operator==(const member_function_template& o) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on its
  /// underlying function template.
  void
  traverse(ir_node_visitor&);
};// end class class_decl::member_function_template


/// Abstracts a member class template template
class class_decl::member_class_template
: public member_base, public virtual traversable_base
{
  shared_ptr<class_tdecl> class_tmpl_;

  // Forbidden
  member_class_template();

public:

  /// Hasher.
  struct hash;

  member_class_template(shared_ptr<class_tdecl> c,
			access_specifier access, bool is_static)
  : member_base(access, is_static), class_tmpl_(c)
  { }

  operator const class_tdecl& () const
  { return *class_tmpl_; }

  shared_ptr<class_tdecl>
  as_class_tdecl() const
  {return class_tmpl_;}

  bool
  operator==(const member_class_template& o) const;

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the current instance and on the class
  /// pattern of the template.
  void
  traverse(ir_node_visitor& v);
};// end class class_decl::member_class_template


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
  virtual void visit(scope_decl&);
  virtual void visit(type_decl&);
  virtual void visit(namespace_decl&);
  virtual void visit(qualified_type_def&);
  virtual void visit(pointer_type_def&);
  virtual void visit(reference_type_def&);
  virtual void visit(enum_type_decl&);
  virtual void visit(typedef_decl&);
  virtual void visit(var_decl&);
  virtual void visit(function_decl&);
  virtual void visit(function_tdecl&);
  virtual void visit(class_tdecl&);
  virtual void visit(class_decl&);
  virtual void visit(class_decl::data_member&);
  virtual void visit(class_decl::member_function&);
  virtual void visit(class_decl::member_function_template&);
  virtual void visit(class_decl::member_class_template&);
};

} // end namespace abigail
#endif // __ABG_IR_H__
