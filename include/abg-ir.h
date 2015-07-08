// -*- Mode: C++ -*-
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
// Author: Dodji Seketeli

/// @file

#ifndef __ABG_IR_H__
#define __ABG_IR_H__

#include <assert.h>
#include <tr1/unordered_map>
#include "abg-fwd.h"
#include "abg-hash.h"
#include "abg-traverse.h"

/// @file
///
/// This file contains the declarations of the Internal Representation
/// of libabigail.

/// @defgroup Memory Memory management
/// @{
///
/// How objects' lifetime is handled in libabigail.
///
/// For memory management and garbage collection of libabigail's IR
/// artifacts, we use std::tr1::shared_ptr and std::tr1::weak_ptr.
///
/// When manipulating these IR artifacts, there are a few rules to keep in
/// mind.
///
/// <b>The declaration for a type is owned by only one scope </b>
///
/// This means that for each instance of abigail::type_base (a type) there
/// is an instance of abigail::scope_decl that owns a @ref
/// abigail::decl_base_sptr (a shared pointer to an abigail::decl_base)
/// that points to the declaration of that type.  The
/// abigail::type_base_sptr is added to the scope using the function
/// abigail::add_decl_to_scope().
///
/// There is a kind of type that is usually not syntactically owned by a
/// scope: it's function type.  In libabigail function types are
/// represented by abigail::function_type and abigail::method_type.  These
/// types must be owned by the translation unit they originate from.
/// Adding them to the translation unit must be done by a call to the
/// method function abigail::translation::get_canonical_function_type().
/// The type returned by that function is the one to use.
///
/// <b> A declaration that has a type does NOT own the type </b>
///
/// This means that, for instance, in an abigail::var_decl (a variable
/// declaration), the type of the declaration is not owned by the
/// declaration.  In other (concrete) words, the variable declaration
/// doesn't have a shared pointer to the type.  Rather, it has a *weak*
/// pointer to its type.  That means that it has a data member of type
/// abigail::type_base_wptr that contains the type of the declaration.
///
/// But then abigail::var_decl::get_type() returns a shared pointer that
/// is constructed from the internal weak pointer to the type.  That way,
/// users of the type of the var can own a temporary reference on it and
/// be assured that the type's life time is long enough for their need.
///
/// Likewise, data members, function and template parameters similarly
/// have weak pointers on their type.
///
/// @}

namespace abigail
{

/// The namespace of the internal representation of ABI artifacts like
/// types and decls.
namespace ir
{

// Inject some std::tr1 types in here.
using std::tr1::unordered_map;

/// @brief The source location of a token.
///
/// This represents the location of a token coming from a given
/// translation unit.  This location is actually an abstraction of
/// cursor in the table of all the locations of all the tokens of the
/// translation unit.  That table is managed by the @ref location_manager
/// type.  To get the file path, line and column numbers associated to
/// a given instance of @ref location, you need to use the
/// location_manager::expand_location method.
class location
{
  unsigned		value_;

  location(unsigned v) : value_(v) {}

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
}; // end class location

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

/// Convenience typedef for a weak pointer on a @ref type_base
typedef weak_ptr<type_base> type_base_wptr;

/// Convenience typedef for a smart pointer on @ref decl_base.
typedef shared_ptr<decl_base> decl_base_sptr;

class function_type;

class function_decl;

/// Convenience typedef for a shared pointer on a @ref function_type
typedef shared_ptr<function_type> function_type_sptr;

/// Convenience typedef fo a vector of @ref function_type_sptr
typedef vector<function_type_sptr> function_types_type;

/// Convenience typedef for a weak pointer on a @ref function_type
typedef weak_ptr<function_type> function_type_wptr;

struct ir_traversable_base;

/// Convenience typedef for a shared pointer to @ref
/// ir_traversable_base.
typedef shared_ptr<ir_traversable_base> ir_traversable_base_sptr;

/// The base of an entity of the intermediate representation that is
/// to be traversed.
struct ir_traversable_base : public traversable_base
{
  /// Traverse a given IR node and its children, calling an visitor on
  /// each node.
  ///
  /// @param v the visitor to call on each traversed node.
  ///
  /// @return true if the all the IR node tree was traversed.
  virtual bool
  traverse(ir_node_visitor& v);
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
  is_constructed() const;

  void
  set_is_constructed(bool);

  bool
  operator==(const translation_unit&) const;

  void
  bind_function_type_life_time(function_type_sptr) const;

  virtual bool
  traverse(ir_node_visitor& v);

  friend function_type_sptr
  lookup_function_type_in_translation_unit(const function_type& t,
					   const translation_unit& tu);
};//end class translation_unit

bool
operator==(translation_unit_sptr, translation_unit_sptr);

/// Access specifier for class members.
enum access_specifier
{
  no_access,
  public_access,
  protected_access,
  private_access,
};

class elf_symbol;
/// A convenience typedef for a shared pointer to elf_symbol.
typedef shared_ptr<elf_symbol> elf_symbol_sptr;

/// A convenience typedef for a weak pointer to elf_symbol.
typedef weak_ptr<elf_symbol> elf_symbol_wptr;

/// Convenience typedef for a map which key is a string and which
/// value if the elf symbol of the same name.
typedef std::tr1::unordered_map<string, elf_symbol_sptr>
string_elf_symbol_sptr_map_type;

/// Convenience typedef for a shared pointer to an
/// string_elf_symbol_sptr_map_type.
typedef shared_ptr<string_elf_symbol_sptr_map_type>
string_elf_symbol_sptr_map_sptr;

/// Convenience typedef for a vector of elf_symbol
typedef std::vector<elf_symbol_sptr> elf_symbols;

/// Convenience typedef for a map which key is a string and which
/// value is a vector of elf_symbol.
typedef std::tr1::unordered_map<string, elf_symbols>
string_elf_symbols_map_type;

/// Convenience typedef for a shared pointer to
/// string_elf_symbols_map_type.
typedef shared_ptr<string_elf_symbols_map_type> string_elf_symbols_map_sptr;

/// Abstraction of an elf symbol.
///
/// This is useful when a given corpus has been read from an ELF file.
/// In that case, a given decl might be associated to its underlying
/// ELF symbol, if that decl is publicly exported in the ELF file.  In
/// that case, comparing decls might involve comparing their
/// underlying symbols as well.
class elf_symbol
{
public:
  /// The type of a symbol.
  enum type
  {
    NOTYPE_TYPE = 0,
    OBJECT_TYPE,
    FUNC_TYPE,
    SECTION_TYPE,
    FILE_TYPE,
    COMMON_TYPE,
    TLS_TYPE,
    GNU_IFUNC_TYPE
  };

  /// The binding of a symbol.
  enum binding
  {
    LOCAL_BINDING = 0,
    GLOBAL_BINDING,
    WEAK_BINDING,
    GNU_UNIQUE_BINDING
  };

  /// Inject the elf_symbol::version here.
  class version;

private:
  struct priv;
  shared_ptr<priv> priv_;

  elf_symbol();

  elf_symbol(size_t		i,
	     const string&	n,
	     type		t,
	     binding		b,
	     bool		d,
	     const version&	v);

  elf_symbol(const elf_symbol&);

  elf_symbol&
  operator=(const elf_symbol& s);

public:

  static elf_symbol_sptr
  create();

  static elf_symbol_sptr
  create(size_t	i,
	 const string&	n,
	 type		t,
	 binding	b,
	 bool		d,
	 const version& v);

  size_t
  get_index() const;

  void
  set_index(size_t);

  const string&
  get_name() const;

  void
  set_name(const string& n);

  type
  get_type() const;

  void
  set_type(type t);

  binding
  get_binding() const;

  void
  set_binding(binding b);

  version&
  get_version() const;

  void
  set_version(const version& v);

  bool
  is_defined() const;

  void
  is_defined(bool d);

  bool
  is_public() const;

  bool
  is_function() const;

  bool
  is_variable() const;

  const elf_symbol_sptr
  get_main_symbol() const;

  elf_symbol_sptr
  get_main_symbol();

  bool
  is_main_symbol() const;

  elf_symbol_sptr
  get_next_alias() const;

  bool
  has_aliases() const;

  int
  get_number_of_aliases() const;

  void
  add_alias(elf_symbol_sptr);

  const string&
  get_id_string() const;

  elf_symbol_sptr
  get_alias_from_name(const string& name) const;

  elf_symbol_sptr
  get_alias_which_equals(const elf_symbol& other) const;

  string
  get_aliases_id_string(const string_elf_symbols_map_type& symtab,
			bool include_symbol_itself = true) const;

  string
  get_aliases_id_string(bool include_symbol_itself = true) const;

  static bool
  get_name_and_version_from_id(const string& id,
			       string& name,
			       string& ver);

  bool
  operator==(const elf_symbol&) const;

  bool
  does_alias(const elf_symbol&) const;
}; // end class elf_symbol.

std::ostream&
operator<<(std::ostream& o, elf_symbol::type t);

std::ostream&
operator<<(std::ostream& o, elf_symbol::binding t);

bool
string_to_elf_symbol_type(const string&, elf_symbol::type&);

bool
string_to_elf_symbol_binding(const string&, elf_symbol::binding&);

bool
operator==(const elf_symbol_sptr lhs, const elf_symbol_sptr rhs);

bool
elf_symbols_alias(const elf_symbol& s1, const elf_symbol& s2);

void
compute_aliases_for_elf_symbol(const elf_symbol& symbol,
			       const string_elf_symbols_map_type& symtab,
			       vector<elf_symbol_sptr>& alias_set);

/// The abstraction of the version of an ELF symbol.
class elf_symbol::version
{
  struct priv;
  shared_ptr<priv> priv_;

public:
  version();

  version(const string& v,
	  bool is_default);

  version(const version& v);

  operator const string&() const;

  const string&
  str() const;

  void
  str(const string& s);

  bool
  is_default() const;

  void
  is_default(bool f);

  bool
  is_empty() const;

  bool
  operator==(const version& o) const;

  version&
  operator=(const version& o);
};// end class elf_symbol::version

class context_rel;
/// A convenience typedef for shared pointers to @ref context_rel
typedef shared_ptr<context_rel> context_rel_sptr;

/// The abstraction of the relationship between an entity and its
/// containing scope (its context).  That relationship can carry
/// properties like access rights (if the parent is a class_decl),
/// etc.
///
/// But importantly, this relationship carries a pointer to the
/// actualy parent.
class context_rel
{
protected:
  scope_decl*		scope_;
  enum access_specifier access_;
  bool			is_static_;

public:
  context_rel()
    : scope_(0),
      access_(no_access),
      is_static_(false)
  {}

  context_rel(scope_decl* s)
    : scope_(s),
      access_(no_access),
      is_static_(false)
  {}

  context_rel(scope_decl* s,
	      access_specifier a,
	      bool f)
    : scope_(s),
      access_(a),
      is_static_(f)
  {}

  scope_decl*
  get_scope() const
  {return scope_;}

  access_specifier
  get_access_specifier() const
  {return access_;}

  void
  set_access_specifier(access_specifier a)
  {access_ = a;}

  bool
  get_is_static() const
  {return is_static_;}

  void
  set_is_static(bool s)
  {is_static_ = s;}

  void
  set_scope(scope_decl* s)
  {scope_ = s;}

  bool
  operator==(const context_rel& o)const
  {
    return (access_ == o.access_
	    && is_static_ == o.is_static_);
  }

  virtual ~context_rel();
};// end class context_rel

class class_decl;

/// Convenience typedef for a shared pointer on a @ref class_decl
typedef shared_ptr<class_decl> class_decl_sptr;

/// Convenience typedef for a vector of @ref class_decl_sptr
typedef vector<class_decl_sptr> classes_type;

/// Convenience typedef for a weak pointer on a @ref class_decl.
typedef weak_ptr<class_decl> class_decl_wptr;

/// A bitfield that gives callers of abigail::ir::equals() some
/// insight about how different two internal representation artifacts
/// are.
enum change_kind
{
  NO_CHANGE_KIND = 0,

  /// This means that a given IR artifact has local differences, with
  /// respect to the other artifact it was compared against.  A local
  /// change is a change that is carried by the artifact itself,
  /// rather than by one off its sub-types.
  LOCAL_CHANGE_KIND = 1,

  /// This means that a given IR artifact has changes in some of its
  /// sub-types, with respect to the other artifact it was compared
  /// against.
  SUBTYPE_CHANGE_KIND = 1 << 1
};// end enum change_kink

change_kind
operator|(change_kind, change_kind);

change_kind
operator&(change_kind, change_kind);

change_kind&
operator|=(change_kind&, change_kind);

change_kind&
operator&=(change_kind&, change_kind);

bool
equals(const decl_base&, const decl_base&, change_kind*);

class type_or_decl_base;

/// Convenience typedef for a shared pointer to @ref
/// type_or_decl_base.
typedef shared_ptr<type_or_decl_base> type_or_decl_base_sptr;

/// The base class of both types and declarations.
class type_or_decl_base : public ir_traversable_base
{
public:
  virtual ~type_or_decl_base();
  virtual string
  get_pretty_representation() const = 0;
}; // end class type_or_decl_base

bool
operator==(const type_or_decl_base&, const type_or_decl_base&);

bool
operator==(const type_or_decl_base_sptr&, const type_or_decl_base_sptr&);

/// The base type of all declarations.
class decl_base : public virtual type_or_decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

protected:
  mutable priv_sptr priv_;

  bool
  hashing_started() const;

  void
  hashing_started(bool b) const;

  size_t
  peek_hash_value() const;

  const string&
  peek_qualified_name() const;

  void
  set_qualified_name(const string&) const;

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

  // Forbidden
  decl_base();

  virtual void
  set_scope(scope_decl*);

protected:
  const context_rel*
  get_context_rel() const;

  context_rel*
  get_context_rel();

  void
  set_context_rel(context_rel_sptr c);

public:
  decl_base(const std::string&	name, location locus,
	    const std::string&	mangled_name = "",
	    visibility vis = VISIBILITY_DEFAULT);

  decl_base(location);

  decl_base(const decl_base&);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~decl_base();

  virtual size_t
  get_hash() const;

  virtual string
  get_pretty_representation() const;

  virtual void
  get_qualified_name(string& qualified_name) const;

  virtual const string&
  get_qualified_name() const;

  void
  set_hash(size_t) const;

  bool
  get_is_in_public_symbol_table() const;

  void
  set_is_in_public_symbol_table(bool);

  location
  get_location() const;

  void
  set_location(const location& l);

  const string&
  get_name() const;

  const string&
  get_qualified_parent_name() const;

  void
  set_name(const string& n);

  const string&
  get_linkage_name() const;

  void
  set_linkage_name(const std::string& m);

  scope_decl*
  get_scope() const;

  visibility
  get_visibility() const;

  void
  set_visibility(visibility v);

  friend bool
  equals(const decl_base&, const decl_base&, change_kind*);

  friend bool
  equals(const var_decl&, const var_decl&, change_kind*);

  friend decl_base_sptr
  add_decl_to_scope(decl_base_sptr dcl, scope_decl* scpe);

  friend void
  remove_decl_from_scope(decl_base_sptr);

  friend decl_base_sptr
  insert_decl_into_scope(decl_base_sptr,
			 vector<shared_ptr<decl_base> >::iterator,
			 scope_decl*);

  friend enum access_specifier
  get_member_access_specifier(const decl_base& d);

  friend enum access_specifier
  get_member_access_specifier(const decl_base_sptr& d);

  friend void
  set_member_access_specifier(decl_base& d,
			      access_specifier a);

  friend bool
  get_member_is_static(const decl_base& d);

  friend bool
  get_member_is_static(const decl_base_sptr& d);

  friend void
  set_member_is_static(const decl_base_sptr& d, bool s);

  friend void
  set_member_is_static(decl_base& d, bool s);

  friend bool
  get_member_function_is_virtual(const function_decl& f);

  friend void
  set_member_function_is_virtual(function_decl&, bool);

  friend class class_decl;
};// end class decl_base

bool
operator==(const decl_base_sptr, const decl_base_sptr);

bool
operator==(const type_base_sptr, const type_base_sptr);

std::ostream&
operator<<(std::ostream&, decl_base::visibility);

std::ostream&
operator<<(std::ostream&, decl_base::binding);

bool
equals(const scope_decl&, const scope_decl&, change_kind*);

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
  struct hash;

  scope_decl(const std::string& name, location locus,
	     visibility	vis = VISIBILITY_DEFAULT)
  : decl_base(name, locus, /*mangled_name=*/name, vis)
  {}

  scope_decl(location l) : decl_base("", l)
  {}

  virtual size_t
  get_hash() const;

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

  const scopes&
  get_member_scopes() const
  {return member_scopes_;}

  bool
  is_empty() const
  {return get_member_decls().empty();}

  bool
  find_iterator_for_member(const decl_base*, declarations::iterator&);

  bool
  find_iterator_for_member(const decl_base_sptr, declarations::iterator&);

  virtual bool
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

bool
operator==(scope_decl_sptr, scope_decl_sptr);

/// Hasher for the @ref scope_decl type.
struct scope_decl::hash
{
  size_t
  operator()(const scope_decl& d) const;

  size_t
  operator()(const scope_decl* d) const;
};

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

bool
equals(const type_base&, const type_base&, change_kind*);

/// An abstraction helper for type declarations
class type_base : public virtual type_or_decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbid this.
  type_base();


  static type_base_sptr
  get_canonical_type_for(type_base_sptr);

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

  friend type_base_sptr
  canonicalize(type_base_sptr);

  type_base_sptr
  get_canonical_type() const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~type_base();

  void
  set_size_in_bits(size_t);

  virtual size_t
  get_size_in_bits() const;

  void
  set_alignment_in_bits(size_t);

  virtual size_t
  get_alignment_in_bits() const;
};//end class type_base

/// Hash functor for instances of @ref type_base.
struct type_base::hash
{
   size_t
   operator()(const type_base& t) const;

  size_t
  operator()(const type_base* t) const;

  size_t
  operator()(const type_base_sptr t) const;
}; // end struct type_base::hash

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

bool
equals(const type_decl&, const type_decl&, change_kind*);

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

  static type_decl_sptr&
  get_void_type_decl();

  static type_decl_sptr&
  get_variadic_parameter_type_decl();

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_decl&) const;

  virtual string
  get_pretty_representation() const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~type_decl();
};// end class type_decl.

bool
equals(const scope_type_decl&, const scope_type_decl&, change_kind*);

bool
operator==(type_decl_sptr, type_decl_sptr);

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

  virtual bool
  traverse(ir_node_visitor&);

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

  virtual string
  get_pretty_representation() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~namespace_decl();
};// end class namespace_decl

bool
equals(const qualified_type_def&, const qualified_type_def&, change_kind*);

typedef shared_ptr<qualified_type_def> qualified_type_def_sptr;

/// The abstraction of a qualified type.
class qualified_type_def : public virtual type_base, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden.
  qualified_type_def();

protected:
  string build_name(bool) const;

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

  qualified_type_def(type_base_sptr type, CV quals, location locus);

  virtual size_t
  get_size_in_bits() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  CV
  get_cv_quals() const;

  void
  set_cv_quals(CV cv_quals);

  string
  get_cv_quals_string_prefix() const;

  type_base_sptr
  get_underlying_type() const;

  virtual void
  get_qualified_name(string& qualified_name) const;

  virtual const string&
  get_qualified_name() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~qualified_type_def();
}; // end class qualified_type_def.

qualified_type_def::CV
operator|(qualified_type_def::CV, qualified_type_def::CV);

std::ostream&
operator<<(std::ostream&, qualified_type_def::CV);

bool
equals(const pointer_type_def&, const pointer_type_def&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref pointer_type_def
typedef shared_ptr<pointer_type_def> pointer_type_def_sptr;

/// The abstraction of a pointer type.
class pointer_type_def : public virtual type_base, public virtual decl_base
{
  type_base_wptr pointed_to_type_;

  // Forbidden.
  pointer_type_def();

public:

  /// A hasher for instances of pointer_type_def
  struct hash;

  pointer_type_def(const type_base_sptr& pointed_to_type, size_t size_in_bits,
		   size_t alignment_in_bits, location locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  const type_base_sptr
  get_pointed_to_type() const;

  virtual void
  get_qualified_name(string&) const;

  virtual const string&
  get_qualified_name() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~pointer_type_def();
}; // end class pointer_type_def

bool
equals(const reference_type_def&, const reference_type_def&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref reference_type_def
typedef shared_ptr<reference_type_def> reference_type_def_sptr;

/// Abstracts a reference type.
class reference_type_def : public virtual type_base, public virtual decl_base
{
  type_base_wptr	pointed_to_type_;
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

  type_base_sptr
  get_pointed_to_type() const;

  bool
  is_lvalue() const;

  virtual void
  get_qualified_name(string& qualified_name) const;

  virtual const string&
  get_qualified_name() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~reference_type_def();
}; // end class reference_type_def

bool
equals(const array_type_def&, const array_type_def&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref array_type_def
typedef shared_ptr<array_type_def> array_type_def_sptr;

/// The abstraction of an array type.
class array_type_def : public virtual type_base, public virtual decl_base
{
private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden.
  array_type_def();

public:

  /// Hasher for intances of array_type_def.
  struct hash;

  class subrange_type;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::subrange
  typedef shared_ptr<subrange_type> subrange_sptr;

  /// Convenience typedef for a vector of @ref subrange_sptr
  typedef std::vector<subrange_sptr> subranges_type;

  /// Abtraction for an array dimension
  class subrange_type
  {
    struct priv;
    typedef shared_ptr<priv> priv_sptr;
    priv_sptr priv_;

    // Forbidden.
    subrange_type();
  public:

    /// Hasher for an instance of array::subrange
    struct hash;

    subrange_type(size_t lower_bound, size_t upper_bound,
		  location loc);

    subrange_type(size_t upper_bound, location loc);

    size_t
    get_upper_bound() const;

    size_t
    get_lower_bound() const;

    void
    set_upper_bound(size_t ub);

    void
    set_lower_bound(size_t lb);

    size_t
    get_length() const;

    bool
    is_infinite() const;

    bool
    operator==(const subrange_type& o) const;

    location
    get_location() const;
  };

  array_type_def(const type_base_sptr type,
		 const std::vector<subrange_sptr>& subs,
		 location locus);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual void
  get_qualified_name(string& qualified_name) const;

  virtual const string&
  get_qualified_name() const;

  const type_base_sptr
  get_element_type() const;

  virtual void
  append_subrange(subrange_sptr sub);

  virtual void
  append_subranges(const std::vector<subrange_sptr>& subs);

  virtual int
  get_dimension_count() const;

   virtual bool
  is_infinite() const;

  virtual string
  get_pretty_representation() const;

  virtual string
  get_subrange_representation() const;

  virtual bool
  traverse(ir_node_visitor& v);

  location
  get_location() const;

  const std::vector<subrange_sptr>&
  get_subranges() const;

  virtual ~array_type_def();

}; // end class array_type_def

bool
equals(const enum_type_decl&, const enum_type_decl&, change_kind*);

/// Convenience typedef for shared pointer on enum_type_decl.
typedef shared_ptr<enum_type_decl> enum_type_decl_sptr;

/// Abstracts a declaration for an enum type.
class enum_type_decl : public virtual type_base, public virtual decl_base
{
public:

  /// A hasher for an enum_type_decl.
  struct hash;

  /// Enumerator Datum.
  class enumerator;

  /// Convenience typedef for a list of @ref enumerator.
  typedef std::vector<enumerator> enumerators;

private:

  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

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
  /// @param enms a list of enumerators for this enum.
  ///
  /// @param mangled_name the mangled name of the enum type.
  ///
  /// @param vis the visibility of instances of this type.
  enum_type_decl(const string& name, location locus,
		 type_base_sptr underlying_type,
		 enumerators& enms, const std::string& mangled_name = "",
		 visibility vis = VISIBILITY_DEFAULT);

  type_base_sptr
  get_underlying_type() const;

  const enumerators&
  get_enumerators() const;

  enumerators&
  get_enumerators();

  virtual string
  get_pretty_representation() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~enum_type_decl();
}; // end class enum_type_decl

bool
operator==(enum_type_decl_sptr l, enum_type_decl_sptr r);
/// The abstraction of an enumerator
class enum_type_decl::enumerator
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;


public:

  enumerator();

  enumerator(const string& name, size_t value);

  enumerator(const enumerator&);

  bool
  operator==(const enumerator& other) const;

  const string&
  get_name() const;

  const string&
  get_qualified_name() const;

  void
  set_name(const string& n);

  size_t
  get_value() const;

  void
  set_value(size_t v);

  enum_type_decl*
  get_enum_type() const;

  void
  set_enum_type(enum_type_decl*);
}; // end class enum_type_def::enumerator

bool
equals(const typedef_decl&, const typedef_decl&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref typedef_decl.
typedef shared_ptr<typedef_decl> typedef_decl_sptr;

/// The abstraction of a typedef declaration.
class typedef_decl : public virtual type_base, public virtual decl_base
{
  type_base_wptr underlying_type_;

  // Forbidden
  typedef_decl();

public:

  /// Hasher for the typedef_decl type.
  struct hash;

  typedef_decl(const string& name, const shared_ptr<type_base> underlying_type,
	       location locus, const std::string& mangled_name = "",
	       visibility vis = VISIBILITY_DEFAULT);

  virtual size_t
  get_size_in_bits() const;

  virtual size_t
  get_alignment_in_bits() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation() const;

  type_base_sptr
  get_underlying_type() const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~typedef_decl();
};// end class typedef_decl

class dm_context_rel;

/// A convenience typedef for a shared pointer to dm_context_rel.
typedef shared_ptr<dm_context_rel> dm_context_rel_sptr;

/// The abstraction for a data member context relationship.  This
/// relates a data member to its parent class.
///
/// The relationship carries properties like the offset of the data
/// member, if applicable.
class dm_context_rel : public context_rel
{
protected:
  bool is_laid_out_;
  size_t offset_in_bits_;

public:
  dm_context_rel()
    : context_rel(),
      is_laid_out_(!is_static_),
      offset_in_bits_(0)
  {}

  dm_context_rel(scope_decl* s,
		 bool is_laid_out,
		 size_t offset_in_bits,
		 access_specifier a,
		 bool is_static)
    : context_rel(s, a, is_static),
      is_laid_out_(is_laid_out),
      offset_in_bits_(offset_in_bits)
  {}

  dm_context_rel(scope_decl* s)
    : context_rel(s),
      is_laid_out_(!is_static_),
      offset_in_bits_(0)
  {}

  bool
  get_is_laid_out() const
  {return is_laid_out_;}

  void
  set_is_laid_out(bool f)
  {is_laid_out_ = f;}

  size_t
  get_offset_in_bits() const
  {return offset_in_bits_;}

  void
  set_offset_in_bits(size_t o)
  {offset_in_bits_ = o;}

  bool
  operator==(const dm_context_rel& o) const
  {
    if (!context_rel::operator==(o))
      return false;

    return (is_laid_out_ == o.is_laid_out_
	    && offset_in_bits_ == o.offset_in_bits_);
  }

  virtual ~dm_context_rel();
};// end class class_decl::dm_context_rel

bool
equals(const var_decl&, const var_decl&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref var_decl
typedef shared_ptr<var_decl> var_decl_sptr;

/// Abstracts a variable declaration.
class var_decl : public virtual decl_base
{
  struct priv;
  shared_ptr<priv> priv_;

  // Forbidden
  var_decl();

  virtual void
  set_scope(scope_decl*);

public:

  /// Hasher for a var_decl type.
  struct hash;

  /// Equality functor to compare pointers to variable_decl.
  struct ptr_equal;

  var_decl(const std::string&		name,
	   shared_ptr<type_base>	type,
	   location			locus,
	   const std::string&		mangled_name,
	   visibility			vis = VISIBILITY_DEFAULT,
	   binding			bind = BINDING_NONE);

  virtual bool
  operator==(const decl_base&) const;

  const type_base_sptr
  get_type() const;

  binding
  get_binding() const;

  void
  set_binding(binding b);

  void
  set_symbol(elf_symbol_sptr sym);

  elf_symbol_sptr
  get_symbol() const;

  var_decl_sptr
  clone() const;

  const string&
  get_id() const;

  virtual size_t
  get_hash() const;

  virtual string
  get_pretty_representation() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~var_decl();

  friend void
  set_data_member_offset(var_decl_sptr m, size_t o);

  friend size_t
  get_data_member_offset(const var_decl_sptr m);

  friend size_t
  get_data_member_offset(const var_decl& m);

  friend void
  set_data_member_is_laid_out(var_decl_sptr m, bool l);

  friend bool
  get_data_member_is_laid_out(const var_decl& m);

  friend bool
  get_data_member_is_laid_out(const var_decl_sptr m);
}; // end class var_decl

bool
equals(const function_decl&, const function_decl&, change_kind*);

/// Convenience typedef for a shared pointer on a @ref function_decl
typedef shared_ptr<function_decl> function_decl_sptr;

/// Abstraction for a function declaration.
class function_decl : public virtual decl_base
{
  struct priv;
  shared_ptr<priv> priv_;

public:
  /// Hasher for function_decl
  struct hash;

  /// Equality functor to compare pointers to function_decl
  struct ptr_equal;

  class parameter;

  /// Convenience typedef for a shared pointer on a @ref
  /// function_decl::parameter
  typedef shared_ptr<parameter> parameter_sptr;

  /// Convenience typedef for a vector of @ref parameter_sptr
  typedef std::vector<parameter_sptr> parameters;

  /// Abtraction for the parameter of a function.
  class parameter;

  function_decl(const std::string& name,
		function_type_sptr function_type,
		bool declared_inline,
		location locus,
		const std::string& mangled_name,
		visibility vis,
		binding bind);

  function_decl(const std::string& name,
		shared_ptr<type_base> fn_type,
		bool declared_inline,
		location locus,
		const std::string& mangled_name = "",
		visibility vis = VISIBILITY_DEFAULT,
		binding bind = BINDING_GLOBAL);

  virtual string
  get_pretty_representation() const;

  string
  get_pretty_representation_of_declarator () const;

  const std::vector<parameter_sptr >&
  get_parameters() const;

  void
  append_parameter(parameter_sptr parm);

  void
  append_parameters(std::vector<parameter_sptr >& parms);

  parameters::const_iterator
  get_first_non_implicit_parm() const;

  const function_type_sptr
  get_type() const;

  const type_base_sptr
  get_return_type() const;

  void
  set_type(function_type_sptr fn_type);

  void
  set_symbol(elf_symbol_sptr sym);

  elf_symbol_sptr
  get_symbol() const;

  bool
  is_declared_inline() const;

  binding
  get_binding() const;

  function_decl_sptr
  clone() const;

  virtual bool
  operator==(const decl_base& o) const;

  /// Return true iff the function takes a variable number of
  /// parameters.
  ///
  /// @return true if the function taks a variable number
  /// of parameters.
  bool
  is_variadic() const;

  virtual size_t
  get_hash() const;

  const string&
  get_id() const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~function_decl();
}; // end class function_decl

bool
equals(const function_decl::parameter&,
       const function_decl::parameter&,
       change_kind*);

/// Abstraction of a function parameter.
class function_decl::parameter : public decl_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:

  /// Hasher for an instance of function::parameter
  struct hash;

  parameter(const type_base_sptr	type,
	    unsigned			index,
	    const std::string&		name,
	    location			loc,
	    bool			variadic_marker = false);

  parameter(const type_base_sptr	type,
	    unsigned			index,
	    const std::string&		name,
	    location			loc,
	    bool			variadic_marker,
	    bool			is_artificial);

  parameter(const type_base_sptr	type,
	    const std::string&		name,
	    location			loc,
	    bool			variadic_marker = false,
	    bool			is_artificial	= false);

  parameter(const type_base_sptr	type,
	    unsigned			index = 0,
	    bool			variadic_marker = false);

  const type_base_sptr
  get_type()const;

  const string
  get_type_name() const;

  const string
  get_type_pretty_representation() const;

  const string
  get_name_id() const;

  unsigned
  get_index() const;

  void
  set_index(unsigned i);

   bool
  get_artificial() const;

  void
  set_artificial(bool f);

  bool
  get_variadic_marker() const;

  bool
  operator==(const parameter& o) const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual size_t
  get_hash() const;

  virtual void
  get_qualified_name(string& qualified_name) const;

  virtual string
  get_pretty_representation() const;
}; // end class function_decl::parameter

bool
operator==(const function_decl::parameter_sptr&,
	   const function_decl::parameter_sptr&);

/// A hashing functor for a function_decl::parameter.
struct function_decl::parameter::hash
{
  size_t
  operator()(const function_decl::parameter&) const;

  size_t
  operator()(const function_decl::parameter*) const;

  size_t
  operator()(const function_decl::parameter_sptr) const;
}; // end struct function_decl::parameter::hash

bool
equals(const function_type&, const function_type&, change_kind*);

/// Abstraction of a function type.
class function_type : public virtual type_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

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

public:

  function_type(type_base_sptr		return_type,
		const parameters&	parms,
		size_t			size_in_bits,
		size_t			alignment_in_bits);

  function_type(type_base_sptr	return_type,
		size_t		size_in_bits,
		size_t		alignment_in_bits);

  function_type(size_t size_in_bits,
		size_t alignment_in_bits);

  type_base_sptr
  get_return_type() const;

  void
  set_return_type(type_base_sptr t);

  const parameters&
  get_parameters() const;

  const parameter_sptr
  get_parm_at_index_from_first_non_implicit_parm(size_t) const;

  void
  set_parameters(const parameters &p);

  void
  append_parameter(parameter_sptr parm);

  bool
  is_variadic() const;

  parameters::const_iterator
  get_first_non_implicit_parm() const;

  virtual bool
  operator==(const type_base&) const;

  virtual string
  get_pretty_representation() const;

  virtual bool
  traverse(ir_node_visitor&);

  virtual ~function_type();
};//end class function_type

/// The hashing functor for @ref function_type.
struct function_type::hash
{
  size_t
  operator()(const function_type& t) const;

  size_t
  operator()(const function_type* t) const;

  size_t
  operator()(const function_type_sptr t) const;
};// end struct function_type::hash

/// Convenience typedef for shared pointer to @ref method_type.
typedef shared_ptr<method_type> method_type_sptr;
/// Abstracts the type of a class member function.
class method_type : public function_type
{
  class_decl_wptr class_type_;

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

  class_decl_sptr
  get_class_type() const
  {return class_decl_sptr(class_type_);}

  void
  set_class_type(shared_ptr<class_decl> t);

  virtual string
  get_pretty_representation() const;

  virtual ~method_type();
};// end class method_type.

/// Convenience typedef for shared pointer to template parameter
typedef shared_ptr<template_parameter> template_parameter_sptr;

/// Convenience typedef for a shared pointer to template_decl
typedef shared_ptr<template_decl> template_decl_sptr;

/// Convenience typedef for a weak pointer to template_decl
typedef weak_ptr<template_decl> template_decl_wptr;

/// The base class of templates.
class template_decl : public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  template_decl();

public:

  /// Hasher.
  struct hash;

  template_decl(const string& name,
		location locus,
		visibility vis = VISIBILITY_DEFAULT);

  void
  add_template_parameter(const template_parameter_sptr p);

  const std::list<template_parameter_sptr>&
  get_template_parameters() const;

  virtual bool
  operator==(const template_decl& o) const;

  virtual ~template_decl();
};//end class template_decl

/// Base class for a template parameter.  Client code should use the
/// more specialized type_template_parameter,
/// non_type_template_parameter and template_template_parameter below.
class template_parameter
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden
  template_parameter();

 public:

  /// Hashers.
  struct hash;
  struct dynamic_hash;
  struct shared_ptr_hash;

  template_parameter(unsigned			index,
		     template_decl_sptr	enclosing_tdecl);

  virtual bool
  operator==(const template_parameter&) const;

  unsigned
  get_index() const;

  const template_decl_sptr
  get_enclosing_template_decl() const;

  bool
  get_hashing_has_started() const;

  void
  set_hashing_has_started(bool f) const;

  virtual ~template_parameter();
};//end class template_parameter

struct template_decl::hash
{
    size_t
    operator()(const template_decl& t) const;
};// end struct template_decl::hash

/// Convenience typedef for a shared pointer to @ref type_tparameter.
typedef shared_ptr<type_tparameter> type_tparameter_sptr;

/// Abstracts a type template parameter.
class type_tparameter : public template_parameter, public virtual type_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  type_tparameter();

public:

  /// Hasher.
  struct hash;

  type_tparameter(unsigned		index,
		  template_decl_sptr	enclosing_tdecl,
		  const std::string&	name,
		  location		locus);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const type_tparameter&) const;

  virtual ~type_tparameter();
};// end class type_tparameter.

/// Convenience typedef for shared pointer to @ref
/// non_type_template_parameter
typedef shared_ptr<non_type_tparameter> non_type_tparameter_sptr;

/// Abstracts non type template parameters.
class non_type_tparameter : public template_parameter, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_base_wptr type_;

  // Forbidden
  non_type_tparameter();

public:
  /// Hasher.
  struct hash;

  non_type_tparameter(unsigned			index,
		      template_decl_sptr	enclosing_tdecl,
		      const std::string&	name,
		      shared_ptr<type_base>	type,
		      location			locus);
  virtual size_t
  get_hash() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  const type_base_sptr
  get_type() const;

  virtual ~non_type_tparameter();
};// end class non_type_tparameter

/// Hasher for the @ref non_type_tparameter type.
struct non_type_tparameter::hash
{
  size_t
  operator()(const non_type_tparameter& t) const;

  size_t
  operator()(const non_type_tparameter* t) const;
};

class template_tparameter;

/// Convenience typedef for a shared_ptr to @ref template_tparameter.
typedef shared_ptr<template_tparameter> template_tparameter_sptr;

/// Abstracts a template template parameter.
class template_tparameter : public type_tparameter, public template_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  // Forbidden
  template_tparameter();

public:

  /// A hasher for instances of template_tparameter
  struct hash;

  template_tparameter(unsigned index,
		      template_decl_sptr enclosing_tdecl,
		      const std::string& name,
		      location locus);

  virtual bool
  operator==(const type_base&) const;

  virtual bool
  operator==(const template_parameter&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual ~template_tparameter();
};

/// Convenience typedef for shared pointer to type_composition
typedef shared_ptr<type_composition> type_composition_sptr;

/// This abstracts a composition of types based on template type
/// parameters.  The result of the composition is a type that can be
/// referred to by a template non-type parameter.  Instances of this
/// type can appear at the same level as template parameters, in the
/// scope of a template_decl.
class type_composition : public template_parameter, public virtual decl_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_composition();

public:
  struct hash;

  type_composition(unsigned		index,
		   template_decl_sptr	tdecl,
		   type_base_sptr	composed_type);

  const type_base_sptr
  get_composed_type() const;

  void
  set_composed_type(type_base_sptr t);

  virtual size_t
  get_hash() const;

  virtual ~type_composition();
};

/// Hasher for the @ref type_composition type.
struct type_composition::hash
{
  size_t
  operator()(const type_composition& t) const;

  size_t
  operator()(const type_composition* t) const;

}; //struct type_composition::hash

/// Convenience typedef for a shared pointer on a @ref function_tdecl
typedef shared_ptr<function_tdecl> function_tdecl_sptr;

/// Abstract a function template declaration.
class function_tdecl : public template_decl, public scope_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  function_tdecl();

public:

  /// Hash functor for function templates.
  struct hash;
  struct shared_ptr_hash;

  function_tdecl(location	locus,
		 visibility	vis = VISIBILITY_DEFAULT,
		 binding	bind = BINDING_NONE);

  function_tdecl(function_decl_sptr	pattern,
		 location		locus,
		 visibility		vis = VISIBILITY_DEFAULT,
		 binding		bind = BINDING_NONE);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  void
  set_pattern(shared_ptr<function_decl> p);

  shared_ptr<function_decl>
  get_pattern() const;

  binding
  get_binding() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~function_tdecl();
}; // end class function_tdecl.

/// Convenience typedef for a shared pointer on a @ref class_tdecl
typedef shared_ptr<class_tdecl> class_tdecl_sptr;

/// Abstract a class template.
class class_tdecl : public template_decl, public scope_decl
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbidden
  class_tdecl();

public:

  /// Hashers.
  struct hash;
  struct shared_ptr_hash;

  class_tdecl(location locus, visibility vis = VISIBILITY_DEFAULT);

  class_tdecl(shared_ptr<class_decl> pattern,
	      location locus, visibility vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const template_decl&) const;

  virtual bool
  operator==(const class_tdecl&) const;

  void
  set_pattern(class_decl_sptr p);

  shared_ptr<class_decl>
  get_pattern() const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~class_tdecl();
};// end class class_tdecl

bool
equals(const class_decl&, const class_decl&, change_kind*);

/// Abstracts a class declaration.
class class_decl : public scope_type_decl
{
  // Forbidden
  class_decl();

public:
  /// Hasher.
  struct hash;

  /// Forward declarations.
  class member_base;
  class base_spec;
  class method_decl;
  class member_function_template;
  class member_class_template;

  /// Convenience typedef
  /// @{
  typedef shared_ptr<base_spec>			base_spec_sptr;
  typedef std::vector<base_spec_sptr>			base_specs;
  typedef std::vector<type_base_sptr>			member_types;
  typedef std::vector<var_decl_sptr>			data_members;
  typedef shared_ptr<method_decl>			method_decl_sptr;
  typedef std::vector<method_decl_sptr>		member_functions;
  typedef shared_ptr<member_function_template>		member_function_template_sptr;
  typedef std::vector<member_function_template_sptr>	member_function_templates;
  typedef shared_ptr<member_class_template>		member_class_template_sptr;
  typedef std::vector<member_class_template_sptr>	member_class_templates;
  /// @}

private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;
protected:

  virtual decl_base_sptr
  add_member_decl(decl_base_sptr);

  virtual decl_base_sptr
  insert_member_decl(decl_base_sptr member, declarations::iterator before);

  virtual void
  remove_member_decl(decl_base_sptr);

public:

  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, bool is_struct,
	     location locus, visibility vis,
	     base_specs& bases, member_types& mbrs,
	     data_members& data_mbrs, member_functions& member_fns);

  class_decl(const std::string& name, size_t size_in_bits,
	     size_t align_in_bits, bool is_struct,
	     location locus, visibility vis);
  class_decl(const std::string& name, bool is_struct,
	     bool is_declaration_only = true);

  virtual string
  get_pretty_representation() const;

  bool
  get_is_declaration_only() const;

  void
  set_is_declaration_only(bool f);

  bool
  is_struct() const;

  void
  set_definition_of_declaration(class_decl_sptr);

  const class_decl_sptr&
  get_definition_of_declaration() const;

  void
  set_earlier_declaration(decl_base_sptr declaration);

  decl_base_sptr
  get_earlier_declaration() const;

  void
  add_base_specifier(shared_ptr<base_spec> b);

  const base_specs&
  get_base_specifiers() const;

  void
  insert_member_type(type_base_sptr t,
		     declarations::iterator before);

  void
  add_member_type(type_base_sptr t);

  type_base_sptr
  add_member_type(type_base_sptr t, access_specifier a);

  void
  remove_member_type(type_base_sptr t);

  const member_types&
  get_member_types() const;

  void
  add_data_member(var_decl_sptr v, access_specifier a,
		  bool is_laid_out, bool is_static,
		  size_t offset_in_bits);

  const data_members&
  get_data_members() const;

  const data_members&
  get_non_static_data_members() const;

  void
  add_member_function(method_decl_sptr f,
		      access_specifier a,
		      bool is_virtual,
		      size_t vtable_offset,
		      bool is_static, bool is_ctor,
		      bool is_dtor, bool is_const);

  const member_functions&
  get_member_functions() const;

  const member_functions&
  get_virtual_mem_fns() const;

  void
  sort_virtual_mem_fns();

  void
  add_member_function_template(shared_ptr<member_function_template>);

  const member_function_templates&
  get_member_function_templates() const;

  void
  add_member_class_template(shared_ptr<member_class_template> m);

  const member_class_templates&
  get_member_class_templates() const;

  bool
  has_no_base_nor_member() const;

  bool
  has_virtual_member_functions() const;

  virtual size_t
  get_hash() const;

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const type_base&) const;

  bool
  operator==(const class_decl&) const;

  virtual bool
  traverse(ir_node_visitor& v);

  virtual ~class_decl();

  friend void
  fixup_virtual_member_function(method_decl_sptr method);

  friend bool
  equals(const class_decl&, const class_decl&, change_kind*);
};// end class class_decl

void
fixup_virtual_member_function(class_decl::method_decl_sptr method);

/// Hasher for the @ref class_decl type
struct class_decl::hash
{
  size_t
  operator()(const class_decl& t) const;

  size_t
  operator()(const class_decl* t) const;
}; // end struct class_decl::hash

enum access_specifier
get_member_access_specifier(const decl_base&);

enum access_specifier
get_member_access_specifier(const decl_base_sptr&);

void
set_member_access_specifier(decl_base&,
			    access_specifier);

void
set_member_access_specifier(const decl_base_sptr&,
			    access_specifier);

std::ostream&
operator<<(std::ostream&, access_specifier);

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

  /// Getter for the access specifier of this member.
  ///
  /// @return the access specifier for this member.
  access_specifier
  get_access_specifier() const
  {return access_;}

  /// Setter for the access specifier of this member.
  ///
  /// @param a the new access specifier.
  void
  set_access_specifier(access_specifier a)
  {access_ = a;}

  /// @return true if the member is static, false otherwise.
  bool
  get_is_static() const
  {return is_static_;}

  /// Set a flag saying if the parameter is static or not.
  ///
  /// @param f set to true if the member is static, false otherwise.
  void
  set_is_static(bool f)
  {is_static_ = f;}

  virtual bool
  operator==(const member_base& o) const;
};// end class class_decl::member_base

bool
equals(const class_decl::base_spec&,
       const class_decl::base_spec&,
       change_kind*);

/// Abstraction of a base specifier in a class declaration.
class class_decl::base_spec : public member_base,
			      public virtual decl_base
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

  const shared_ptr<class_decl>&
  get_base_class() const
  {return base_class_;}

  bool
  get_is_virtual() const
  {return is_virtual_;}

  long
  get_offset_in_bits() const
  {return offset_in_bits_;}

  virtual bool
  operator==(const decl_base&) const;

  virtual bool
  operator==(const member_base&) const;

  virtual size_t
  get_hash() const;

  virtual bool
  traverse(ir_node_visitor&);
};// end class class_decl::base_spec

bool
operator==(const class_decl::base_spec_sptr l,
	   const class_decl::base_spec_sptr r);

class mem_fn_context_rel;

/// A convenience typedef for a shared pointer to @ref
/// mem_fn_context_rel.
typedef shared_ptr<mem_fn_context_rel> mem_fn_context_rel_sptr;

/// Abstraction of a member function context relationship.  This
/// relates a member function to its parent class.
class mem_fn_context_rel : public context_rel
{
protected:
  bool		is_virtual_;
  size_t	vtable_offset_in_bits_;
  bool		is_constructor_;
  bool		is_destructor_;
  bool		is_const_;

public:
  mem_fn_context_rel()
    : context_rel(),
      is_virtual_(false),
      vtable_offset_in_bits_(0),
      is_constructor_(false),
      is_destructor_(false),
      is_const_(false)
  {}

  mem_fn_context_rel(scope_decl* s)
    : context_rel(s),
      is_virtual_(false),
      vtable_offset_in_bits_(0),
      is_constructor_(false),
      is_destructor_(false),
      is_const_(false)
  {}

  mem_fn_context_rel(scope_decl* s,
		     bool is_constructor,
		     bool is_destructor,
		     bool is_const,
		     bool is_virtual,
		     size_t vtable_offset_in_bits,
		     access_specifier access,
		     bool is_static)
    : context_rel(s, access, is_static),
      is_virtual_(is_virtual),
      vtable_offset_in_bits_(vtable_offset_in_bits),
      is_constructor_(is_constructor),
      is_destructor_(is_destructor),
      is_const_(is_const)
  {}

  bool
  is_virtual() const
  {return is_virtual_;}

  void
  is_virtual(bool is_virtual)
  {is_virtual_ = is_virtual;}

  /// Getter for the vtable offset property.
  ///
  /// This is the vtable offset of the member function of this
  /// relation.
  ///
  /// @return the vtable offset property of the relation.
  size_t
  vtable_offset() const
  {return vtable_offset_in_bits_;}

  /// Setter for the vtable offset property.
  ///
  /// This is the vtable offset of the member function of this
  /// relation.
  ///
  /// @partam s the new vtable offset.
  void
  vtable_offset(size_t s)
  {vtable_offset_in_bits_ = s;}

  /// Getter for the 'is-constructor' property.
  ///
  /// This tells if the member function of this relation is a
  /// constructor.
  ///
  /// @return the is-constructor property of the relation.
  bool
  is_constructor() const
  {return is_constructor_;}

  /// Setter for the 'is-constructor' property.
  ///
  /// @param f the new value of the the property.  Is true if this is
  /// for a constructor, false otherwise.
  void
  is_constructor(bool f)
  {is_constructor_ = f;}

  /// Getter for the 'is-destructor' property.
  ///
  /// Tells if the member function of this relation is a destructor.
  ///
  /// @return the is-destructor property of the relation;
  bool
  is_destructor() const
  {return is_destructor_;}

  /// Setter for the 'is-destructor' property.
  ///
  /// @param f the new value of the property.  Is true if this is for
  /// a destructor, false otherwise.
  void
  is_destructor(bool f)
  {is_destructor_ = f;}

  /// Getter for the 'is-const' property.
  ///
  /// Tells if the member function of this relation is a const member
  /// function.
  ///
  /// @return the 'is-const' property of the relation.
  bool
  is_const() const
  {return is_const_;}

  /// Setter for the 'is-const' property.
  ///
  /// @param f the new value of the property.  Is true if this is for
  /// a const entity, false otherwise.
  void
  is_const(bool f)
  {is_const_ = f;}

  virtual ~mem_fn_context_rel();
}; // end class mem_fn_context_rel

/// Abstraction of the declaration of a method. This is an
/// implementation detail for class_decl::member_function.
class class_decl::method_decl : public function_decl
{
  method_decl();

  virtual void
  set_scope(scope_decl*);

public:

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
  {function_decl::set_type(fn_type);}

  friend bool
  get_member_function_is_ctor(const function_decl&);

  friend void
  set_member_function_is_ctor(function_decl&, bool);

  friend void
  set_member_function_is_ctor(const function_decl_sptr&, bool);

  friend bool
  get_member_function_is_dtor(const function_decl&);

  friend void
  set_member_function_is_dtor(function_decl&, bool);

  friend void
  set_member_function_is_dtor(const function_decl_sptr&, bool);

  friend bool
  get_member_function_is_static(const function_decl&);

  friend void
  set_member_function_is_static(const function_decl&, bool);

  friend bool
  get_member_function_is_const(const function_decl&);

  friend void
  set_member_function_is_const(function_decl&, bool);

  friend void
  set_member_function_is_const(const function_decl_sptr&, bool);

  friend size_t
  get_member_function_vtable_offset(const function_decl&);

  friend void
  set_member_function_vtable_offset(function_decl&, size_t);

  friend void
  set_member_function_vtable_offset(const function_decl_sptr&, size_t);

  friend bool
  get_member_function_is_virtual(const function_decl&);

  friend void
  set_member_function_is_virtual(function_decl&, bool);

  virtual ~method_decl();
};// end class class_decl::method_decl

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

  virtual bool
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

  class_tdecl_sptr
  as_class_tdecl() const
  {return class_tmpl_;}

  virtual bool
  operator==(const member_base& o) const;

  virtual bool
  operator==(const member_class_template&) const;

  virtual bool
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

/// A hasher that manages to cache the computed hash and re-use it if
/// it is available.
struct type_base::cached_hash
{
  size_t
  operator() (const type_base* t) const;

  size_t
  operator() (const type_base_sptr t) const;
};

/// A hashing functor for instances and pointers of @ref var_decl.
struct var_decl::hash
{
  size_t
  operator()(const var_decl& t) const;

  size_t
  operator()(const var_decl* t) const;
}; //end struct var_decl::hash

/// A comparison functor for pointers to @ref var_decl.
struct var_decl::ptr_equal
{
  /// Return true if the two instances of @ref var_decl are equal.
  ///
  /// @param l the first variable to compare.
  ///
  /// @param r the second variable to compare.
  ///
  /// @return true if @p l equals @p r.
  bool
  operator()(const var_decl* l, const var_decl* r) const
  {
    if (l == r)
      return true;
    if (!!l != !!r)
      return false;
    return (*l == *r);
  }
};// end struct var_decl::ptr_equal

/// A hashing functor fo instances and pointers of @ref function_decl.
struct function_decl::hash
{
  size_t
  operator()(const function_decl& t) const;

  size_t
  operator()(const function_decl* t) const;
};//end struct function_decl::hash

/// Equality functor for instances of @ref function_decl
struct function_decl::ptr_equal
{
  /// Tests if two pointers to @ref function_decl are equal.
  ///
  /// @param l the first pointer to @ref function_decl to consider in
  /// the comparison.
  ///
  /// @param r the second pointer to @ref function_decl to consider in
  /// the comparison.
  ///
  /// @return true if the two functions @p l and @p r are equal, false
  /// otherwise.
  bool
  operator()(const function_decl* l, const function_decl* r) const
  {
    if (l == r)
      return true;
    if (!!l != !!r)
      return false;
    return (*l == *r);
  }
};// function_decl::ptr_equal

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
/// inherit ir_node_visitor, overload the ir_node_visitor::visit_begin
/// or ir_node_visitor::visit_end method of its choice, and provide
/// and implementation for it.  Note that inside these two methods,the
/// node_visitor_base::visiting() method returns false.

/// That new visitor class would then be passed to e.g,
/// translation_unit::traverse or to the traverse method of any type
/// where the traversal is supposed to start from.
struct ir_node_visitor : public node_visitor_base
{
  virtual bool visit_begin(decl_base*);
  virtual bool visit_end(decl_base*);

  virtual bool visit_begin(scope_decl*);
  virtual bool visit_end(scope_decl*);

  virtual bool visit_begin(type_base*);
  virtual bool visit_end(type_base*);

  virtual bool visit_begin(scope_type_decl*);
  virtual bool visit_end(scope_type_decl*);

  virtual bool visit_begin(type_decl*);
  virtual bool visit_end(type_decl*);

  virtual bool visit_begin(namespace_decl*);
  virtual bool visit_end(namespace_decl*);

  virtual bool visit_begin(qualified_type_def*);
  virtual bool visit_end(qualified_type_def*);

  virtual bool visit_begin(pointer_type_def*);
  virtual bool visit_end(pointer_type_def*);

  virtual bool visit_begin(reference_type_def*);
  virtual bool visit_end(reference_type_def*);

  virtual bool visit_begin(array_type_def*);
  virtual bool visit_end(array_type_def*);

  virtual bool visit_begin(enum_type_decl*);
  virtual bool visit_end(enum_type_decl*);

  virtual bool visit_begin(typedef_decl*);
  virtual bool visit_end(typedef_decl*);

  virtual bool visit_begin(function_type*);
  virtual bool visit_end(function_type*);

  virtual bool visit_begin(var_decl*);
  virtual bool visit_end(var_decl*);

  virtual bool visit_begin(function_decl*);
  virtual bool visit_end(function_decl*);

  virtual bool visit_begin(function_decl::parameter*);
  virtual bool visit_end(function_decl::parameter*);

  virtual bool visit_begin(function_tdecl*);
  virtual bool visit_end(function_tdecl*);

  virtual bool visit_begin(class_tdecl*);
  virtual bool visit_end(class_tdecl*);

  virtual bool visit_begin(class_decl*);
  virtual bool visit_end(class_decl*);

  virtual bool visit_begin(class_decl::base_spec*);
  virtual bool visit_end(class_decl::base_spec*);

  virtual bool visit_begin(class_decl::member_function_template*);
  virtual bool visit_end(class_decl::member_function_template*);

  virtual bool visit_begin(class_decl::member_class_template*);
  virtual bool visit_end(class_decl::member_class_template*);
}; // end struct ir_node_visitor

// Debugging facility
void
fns_to_str(vector<function_decl*>::const_iterator a_begin,
	   vector<function_decl*>::const_iterator a_end,
	   vector<function_decl*>::const_iterator b_begin,
	   vector<function_decl*>::const_iterator b_end,
	   std::ostream& o);

}// end namespace ir
} // end namespace abigail
#endif // __ABG_IR_H__
