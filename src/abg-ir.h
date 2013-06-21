// -*- Mode: C++ -*-
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

#ifndef __ABG_IR_H__
#define __ABG_IR_H__

#include <cstddef>
#include <tr1/memory>
#include <list>
#include <vector>
#include <string>
#include <tr1/functional>
#include <typeinfo>
#include <utility> // for std::rel_ops, at least.
#include "abg-hash.h"

using std::tr1::shared_ptr;
using std::tr1::hash;
using std::string;

// Our real stuff
namespace abigail
{

using namespace std::rel_ops; // Pull in relational operators so that
			      // we don't have to define them all here.

class decl_base;
class scope_decl;
class global_scope;
class class_decl;
class translation_unit;

void add_decl_to_scope(shared_ptr<decl_base>,
		       scope_decl*);

void add_decl_to_scope (shared_ptr<decl_base>,
			shared_ptr<scope_decl>);

global_scope* get_global_scope(const shared_ptr<decl_base>);

translation_unit* get_translation_unit(const shared_ptr<decl_base>);

bool is_global_scope(const scope_decl*);

bool is_global_scope(const shared_ptr<scope_decl>);

bool is_at_global_scope(const shared_ptr<decl_base>);

bool is_at_class_scope(const shared_ptr<decl_base>);

bool is_at_template_scope(const shared_ptr<decl_base>);

bool is_template_parameter(const shared_ptr<decl_base>);

bool is_type(const shared_ptr<decl_base>);

bool is_template_parm_composition_type(const shared_ptr<decl_base>);

bool is_template_decl(const shared_ptr<decl_base>);

bool is_function_template_pattern(const shared_ptr<decl_base>);

/// \brief The source location of a token.
///
/// This represents the location of a token coming from a given ABI
/// Corpus.  This location is actually an abstraction of cursor in the
/// table of all the locations of all the tokens of the ABI Corpus.
/// That table is managed by the location_manager type.
class location
{
  location (unsigned v)
    : m_value(v)
  {
  }

public:

  location()
    : m_value(0)
  {
  }

  unsigned
  get_value() const
  {
    return m_value;
  }

  operator bool() const
  {
    return !!m_value;
  }

  bool
  operator==(const location other) const
  {return m_value == other.m_value;}

  bool
  operator<(const location other) const
  {return m_value < other.m_value;}

  friend class location_manager;

private:
  unsigned m_value;
};

/// \brief The entry point to manage locations.
///
/// This type keeps a table of all the locations for tokens of a
/// given ABI Corpus
class location_manager
{
  struct priv;
  shared_ptr<priv> m_priv;

public:

  location_manager();

  location
  create_new_location(const std::string&	file,
		      size_t			line,
		      size_t			column);

  void
  expand_location(const location	location,
		  std::string&		path,
		  unsigned&		line,
		  unsigned&		column) const;
};

/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// into a translation unit.
class translation_unit
{
  // Forbidden
  translation_unit();

public:

  typedef std::list<shared_ptr<decl_base> > decls_type;

  translation_unit(const std::string& path);

  const std::string&
  get_path() const;

  const shared_ptr<global_scope>
  get_global_scope() const;

  location_manager&
  get_loc_mgr();

  const location_manager&
  get_loc_mgr() const;

  bool
  is_empty() const;

private:
  std::string m_path;
  location_manager m_loc_mgr;
  mutable shared_ptr<global_scope> m_global_scope;
};//end class translation_unit

/// \brief The base type of all declarations.
class decl_base
{
  // Forbidden
  decl_base();

  void
  set_scope(scope_decl*);

public:

  enum visibility
  {
    VISIBILITY_NONE,
    VISIBILITY_DEFAULT,
    VISIBILITY_PROTECTED,
    VISIBILITY_HIDDEN,
    VISIBILITY_INTERNAL
  };// end enum visibility

  enum binding
  {
    BINDING_NONE,
    BINDING_LOCAL,
    BINDING_GLOBAL,
    BINDING_WEAK
  };// end enum binding

  decl_base(const std::string&	name,
	    location		locus,
	    const std::string&	mangled_name = "",
	    visibility		vis = VISIBILITY_DEFAULT);

  decl_base(location);

  decl_base(const decl_base&);

  virtual bool
  operator==(const decl_base&) const;

  virtual ~decl_base();

  location
  get_location() const
  {return m_location;}

  void
  set_location(const location& l)
  {m_location = l;}

  const string&
  get_name() const
  {return m_name;}

  void
  set_name(const string& n)
  {m_name = n;}

  const string&
  get_mangled_name() const
  {return m_mangled_name;}

  void
  set_mangled_name(const std::string& m)
  {m_mangled_name = m;}

  scope_decl*
  get_scope() const
  {return m_context;}

  visibility
  get_visibility() const
  {return m_visibility;}

  void
  set_visibility(visibility v)
  {m_visibility = v;}

  friend void
  add_decl_to_scope(shared_ptr<decl_base>,
		    scope_decl*);

private:
  location m_location;
  std::string m_name;
  std::string m_mangled_name;
  scope_decl* m_context;
  visibility m_visibility;
};// end class decl_base


/// \brief A declaration that introduces a scope.
class scope_decl : public virtual decl_base
{
  scope_decl();


  void
  add_member_decl(const shared_ptr<decl_base>);

public:
  scope_decl(const std::string& name,
	     location		locus,
	     visibility	vis = VISIBILITY_DEFAULT)
    : decl_base(name, locus, /*mangled_name=*/name, vis)
  {}


  scope_decl(location l)
    : decl_base("", l)
  {}

  virtual bool
  operator==(const scope_decl&) const;

  const std::list<shared_ptr<decl_base> >&
  get_member_decls() const
  {return m_members;}

  const std::list<shared_ptr<scope_decl> >&
  get_member_scopes() const
  {return m_member_scopes;}

  bool
  is_empty() const
  {return get_member_decls().empty();}

  virtual ~scope_decl();

  friend void
  add_decl_to_scope(shared_ptr<decl_base>,
		    scope_decl*);

private:
  std::list<shared_ptr<decl_base> > m_members;
  std::list<shared_ptr<scope_decl> > m_member_scopes;
};// end class scope_decl.

/// \brief Facility to hash instances of decl_base.
struct decl_base_hash
{
  size_t
  operator() (const decl_base& d) const;
};//end struct decl_base_hash

/// This abstracts the global scope of a given translation unit.
///
/// Only one instance of this class must be present in a given
/// translation_unit.  That instance is implicitely created the first
/// time translatin_unit::get_global_scope is invoked.
class global_scope : public scope_decl
{
  global_scope(translation_unit *tu)
    : decl_base("", location()),
      scope_decl("", location()),
      m_translation_unit(tu)
  {
  }

public:

  friend class translation_unit;

  translation_unit*
  get_translation_unit() const
  {return m_translation_unit;}

  virtual ~global_scope();

private:
  translation_unit* m_translation_unit;
};// end class global_scope;

/// An abstraction helper for type declarations
class type_base
{
  // Forbid this.
  type_base();

public:

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

private:

  size_t m_size_in_bits;
  size_t m_alignment_in_bits;
};//class type_base;

/// A hasher for type_base types.
struct type_base_hash
{
  size_t
  operator()(const type_base& t) const;
};//end struct type_base_hash

/// A hasher for types.  It gets the dynamic type of the current
/// instance of type and hashes it accordingly.  Note that the hashing
/// function of this hasher must be updated each time a new kind of
/// type is added to the IR.
struct dynamic_type_hash
{
  size_t
  operator()(const type_base* t) const;
};//end struct dynamic_type_hash

/// A hasher for shared_ptr<type_base> that will hash it based on the
/// runtime type of the type pointed to.
struct type_shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<type_base> t) const
  {
    return dynamic_type_hash()(t.get());
  }
};//end struct type_shared_ptr_hash

/// A predicate for deep equality of instances of
/// shared_ptr<type_base>
struct type_shared_ptr_equal
{
  bool
  operator()(const shared_ptr<type_base>l,
	     const shared_ptr<type_base>r) const
  {
    if (l != r)
      return false;

    if (l)
      return *l == *r;

    return true;
  }
};//end struct type_shared_ptr_equal

/// A basic type declaration that introduces no scope.
class type_decl : public virtual decl_base, public virtual type_base
{
  // Forbidden.
  type_decl();

public:

  type_decl(const std::string&	name,
	    size_t		size_in_bits,
	    size_t		alignment_in_bits,
	    location		locus,
	    const std::string&	mangled_name = "",
	    visibility		vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const type_decl&) const;

  virtual ~type_decl();
};// class type_decl

/// Facility to hash instance of type_decl
struct type_decl_hash
{
  size_t
  operator()(const type_decl& t) const;
};//end struct type_decl_hash

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public virtual type_base
{
  scope_type_decl();

public:

  scope_type_decl(const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  location			locus,
		  visibility			vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const scope_type_decl&) const;

  virtual ~scope_type_decl();
};

/// Hasher for instances of scope_type_decl
struct scope_type_decl_hash
{
  size_t
  operator()(const scope_type_decl& t) const;

};//end struct scope_type_decl_hash

/// The abstraction of a namespace declaration
class namespace_decl : public scope_decl
{
public:

  namespace_decl(const std::string&	name,
		 location		locus,
		 visibility		vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const namespace_decl&) const;

  virtual ~namespace_decl();
};//end class namespace_decl

/// The abstraction of a qualified type.
class qualified_type_def : public virtual type_base, public virtual decl_base
{

  // Forbidden.
  qualified_type_def();
public:
  /// Bit field values representing the cv qualifiers of the
  /// underlying type.
  enum CV
  {
    CV_NONE = 0,
    CV_CONST = 1,
    CV_VOLATILE = 1 << 1,
    CV_RESTRICT = 1 << 2
  };

  qualified_type_def(shared_ptr<type_base>	underlying_type,
		      CV			quals,
		      location			locus);

  virtual bool
  operator==(const qualified_type_def&) const;

  char
  get_cv_quals() const;

  void
  set_cv_quals(char cv_quals);

  const shared_ptr<type_base>
  get_underlying_type() const;

  virtual ~qualified_type_def();

private:
  char m_cv_quals;
  shared_ptr<type_base> m_underlying_type;
};//end class qualified_type_def

qualified_type_def::CV operator| (qualified_type_def::CV,
				  qualified_type_def::CV);

/// A Hasher for instances of qualified_type_def
struct qualified_type_def_hash
{
  size_t
  operator()(const qualified_type_def& t) const;
};//end struct qualified_type_def_hash

/// The abstraction of a pointer type.
class pointer_type_def : public virtual type_base,
			 public virtual decl_base
{
  // Forbidden.
  pointer_type_def();

public:

  pointer_type_def(shared_ptr<type_base>&	pointed_to_type,
		   size_t			size_in_bits,
		   size_t			alignment_in_bits,
		   location			locus);

  virtual bool
  operator==(const pointer_type_def&) const;

  shared_ptr<type_base>
  get_pointed_to_type() const;

  virtual ~pointer_type_def();

private:
  shared_ptr<type_base> m_pointed_to_type;
};//end class pointer_type_def

/// A hasher for instances of pointer_type_def
struct pointer_type_def_hash
{
  size_t
  operator()(const pointer_type_def& t) const;
};// end struct pointer_type_def_hash

/// Abstracts a reference type.
class reference_type_def : public virtual type_base,
			   public virtual decl_base
{
  // Forbidden.
  reference_type_def();

public:
  reference_type_def(shared_ptr<type_base>&	pointed_to_type,
		     bool			lvalue,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     location			locus);

  virtual bool
  operator==(const reference_type_def&) const;

  shared_ptr<type_base>
  get_pointed_to_type() const;

  bool
  is_lvalue() const;

  virtual ~reference_type_def();

private:
  shared_ptr<type_base> m_pointed_to_type;
  bool m_is_lvalue;
};//end class reference_type_def

/// Hasher for intances of reference_type_def.
struct reference_type_def_hash
{
  size_t
  operator()(const reference_type_def& t);
};//end struct reference_type_def_hash

/// Abstracts a declaration for an enum type.
class enum_type_decl: public virtual type_base,
		      public virtual decl_base
{
  // Forbidden
  enum_type_decl();

public:

  class enumerator
  {
    //Forbidden
    enumerator();

  public:

    enumerator(const string& name,
	       size_t value)
      :m_name(name),
       m_value(value)
    {
    }

    bool
    operator==(const enumerator& other) const
    {
      return (get_name() == other.get_name()
	      && get_value() == other.get_value());
    }
    const string&
    get_name() const
    {return m_name;}

    void
    set_name(const string& n)
    {m_name = n;}

    size_t
    get_value() const
    {return m_value;}

    void
    set_value(size_t v)
    {m_value=v;}

  private:
    string m_name;
    size_t m_value;
  };//end struct enumerator

  enum_type_decl(const string&			name,
		 location			locus,
		 shared_ptr<type_base>		underlying_type,
		 const std::list<enumerator>&	enumerators,
		 const std::string&		mangled_name = "",
		 visibility			vis = VISIBILITY_DEFAULT);

  shared_ptr<type_base>
  get_underlying_type() const;

  const std::list<enumerator>&
  get_enumerators() const;

  virtual bool
  operator==(const enum_type_decl&) const;

  virtual ~enum_type_decl();

private:
  shared_ptr<type_base> m_underlying_type;
  std::list<enumerator> m_enumerators;
};// end class enum_type_decl

/// A hasher for an enum_type_decl.
struct enum_type_decl_hash
{
  size_t
  operator()(const enum_type_decl& t) const;

};//end struct enum_type_decl_hash

/// The abstraction of a typedef declaration.
class typedef_decl: public virtual type_base,
		    public virtual decl_base
{
  // Forbidden
  typedef_decl();

public:

  typedef_decl(const string&			name,
	       const shared_ptr<type_base>	underlying_type,
	       location				locus,
	       const std::string&		mangled_name = "",
	       visibility			vis = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const typedef_decl&) const;

  shared_ptr<type_base>
  get_underlying_type() const;

  virtual ~typedef_decl();

private:
  shared_ptr<type_base> m_underlying_type;
};//end class typedef_decl

/// Hasher for the typedef_decl type.
struct typedef_decl_hash
{
  size_t
  operator()(const typedef_decl& t) const;

};// end struct typedef_decl_hash

/// Abstracts a variable declaration.
class var_decl : public virtual decl_base
{
  // Forbidden
  var_decl();

public:

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
  {return m_type;}

  binding
  get_binding() const
  {return m_binding;}

  void
  set_binding(binding b)
  {m_binding = b;}

  virtual ~var_decl();

private:
  shared_ptr<type_base> m_type;
  binding m_binding;
};// end class var_decl

/// Hasher for a var_decl type.
struct var_decl_hash
{
  size_t
  operator()(const var_decl& t) const;

};// end struct var_decl_hash

class function_type;

/// Abstraction for a function declaration.
class function_decl: public virtual decl_base
{
public:

  /// Abtraction for the parameter of a function.
  class parameter
  {
  public:

    parameter(const shared_ptr<type_base> type,
	      const std::string& name,
	      location loc,
	      bool variadic_marker = false)
      : m_type(type),
	m_name(name),
	m_location(loc),
	m_variadic_marker (variadic_marker)
    {}

    parameter(const shared_ptr<type_base> type,
	      bool variadic_marker = false)
      : m_type(type),
	m_variadic_marker (variadic_marker)
    {}

    const shared_ptr<type_base>
    get_type()const
    {return m_type;}

    const shared_ptr<type_base>
    get_type()
    {return m_type;}

    const std::string&
    get_name() const
    {return m_name;}

    location
    get_location() const
    {return m_location;}

    bool
    operator==(const parameter& o) const
    {return *get_type() == *o.get_type();}

    bool
    get_variadic_marker () const
    {return m_variadic_marker;}

  private:
    shared_ptr<type_base> m_type;
    std::string m_name;
    location m_location;
    bool m_variadic_marker;
  };// end class function::parameter

  /// Hasher for an instance of function::parameter
  struct parameter_hash
  {
    size_t
    operator()(const parameter& p) const;
  };//end struct parameter_hash

  function_decl
  (const std::string&			name,
   const std::vector<shared_ptr<parameter> >& parms,
   shared_ptr<type_base>		return_type,
   size_t				ftype_size_in_bits,
   size_t				ftype_align_in_bits,
   bool				declared_inline,
   location				locus,
   const std::string&			mangled_name = "",
   visibility				vis = VISIBILITY_DEFAULT,
   binding				bind = BINDING_GLOBAL);

  function_decl
  (const std::string&			name,
   shared_ptr<function_type>		function_type,
   bool				declared_inline,
   location				locus,
   const std::string&			mangled_name = "",
   visibility				vis = VISIBILITY_DEFAULT,
   binding				bind = BINDING_GLOBAL)
    : decl_base(name, locus, mangled_name, vis),
      m_type(function_type),
      m_declared_inline(declared_inline),
      m_binding(bind)
  {}

  function_decl
  (const std::string&			name,
   shared_ptr<type_base>		function_type,
   bool				declared_inline,
   location				locus,
   const std::string&			mangled_name = "",
   visibility				vis = VISIBILITY_DEFAULT,
   binding				bind = BINDING_GLOBAL);

  const std::vector<shared_ptr<parameter> >&
  get_parameters() const;

  void
  append_parameter(shared_ptr<parameter> parm);

  void
  append_parameters(std::vector<shared_ptr<parameter> >& parms);

  const shared_ptr<function_type>
  get_type() const;

  const shared_ptr<type_base>
  get_return_type() const;

  void
  set_type(shared_ptr<function_type> fn_type)
  {m_type = fn_type;}

  bool
  is_declared_inline() const
  {return m_declared_inline;}

  binding
  get_binding() const
  {return m_binding;}

  virtual bool
  operator==(const function_decl& o) const;

  /// Return true iff the function takes a variable number of
  /// parameters.
  ///
  /// \return true if the function taks a variable number
  /// of parameters.
  bool
  is_variadic() const
  {return (!get_parameters().empty()
	   && get_parameters().back()->get_variadic_marker());}

  virtual ~function_decl();

protected:
  shared_ptr<function_type> m_type;
private:
  bool m_declared_inline;
  decl_base::binding m_binding;
};// end class function_decl

/// Hasher for function_decl
struct function_decl_hash
{
  size_t
  operator()(const function_decl& t) const;

};// end function_decl_hash

/// Abstraction of a function type.
class function_type : public virtual type_base
{
  function_type ();

public:

  /// The most straightforward constructor for the the function_type
  /// class.
  ///
  /// \param return_type the return type of the function type.
  ///
  /// \param parms the list of parameters of the function type.
  /// Stricto sensu, we just need a list of types; we are using a list
  /// of parameters (where each parameter also carries the name of the
  /// parameter and its source location) to try and provide better
  /// diagnostics whenever it makes sense.  If it appears that this
  /// wasts too many resources, we can fall back to taking just a
  /// vector of types here.
  ///
  /// \param size_in_bits the size of this type, in bits.
  ///
  /// \param alignment_in_bits the alignment of this type, in bits.
  ///
  /// \param size_in_bits the size of this type.
  function_type(shared_ptr<type_base> return_type,
		const std::vector<shared_ptr<function_decl::parameter> >& parms,
		size_t size_in_bits,
		size_t alignment_in_bits)
    : type_base(size_in_bits, alignment_in_bits),
      m_return_type(return_type),
      m_parms(parms)
  {}

  /// A constructor for a function_type that takes no parameters.
  ///
  /// \param return_type the return type of this function_type.
  ///
  /// \param size_in_bits the size of this type, in bits.
  ///
  /// \param alignment_in_bits the alignment of this type, in bits.
  function_type(shared_ptr<type_base> return_type,
		size_t size_in_bits,
		size_t alignment_in_bits)
    : type_base(size_in_bits, alignment_in_bits),
      m_return_type (return_type)
  {}

  /// A constructor for a function_type that takes no parameter and
  /// that has no return_type yet.  These missing parts can (and must)
  /// be added later.
  ///
  /// \param size_in_bits the size of this type, in bits.
  ///
  /// \param alignment_in_bits the alignment of this type, in bits.
  function_type(size_t size_in_bits,
		size_t alignment_in_bits)
    : type_base(size_in_bits, alignment_in_bits)
  {}

  const shared_ptr<type_base>
  get_return_type() const
  {return m_return_type;}

  void
  set_return_type(shared_ptr<type_base> t)
  {m_return_type = t;}

  const std::vector<shared_ptr<function_decl::parameter> >&
  get_parameters() const
  {return m_parms;}

  std::vector<shared_ptr<function_decl::parameter> >&
  get_parameters()
  {return m_parms;}

  void
  set_parameters(const std::vector<shared_ptr<function_decl::parameter> > &p)
  {m_parms = p;}

  void
  append_parameter(shared_ptr<function_decl::parameter> parm)
  {m_parms.push_back (parm);}

  bool
  is_variadic() const
  {return !m_parms.empty() && m_parms.back()->get_variadic_marker();}

  bool
  operator==(const function_type&) const;

  virtual ~function_type();

private:
  shared_ptr<type_base> m_return_type;

protected:
  std::vector<shared_ptr<function_decl::parameter> > m_parms;
};// end class function_type

/// Hasher for an instance of function_type
struct function_type_hash
{
  size_t
  operator()(const function_type& t) const;
}; // end struct function_type_hash

/// Abstracts the type of a class member function.
class method_type : public function_type
{
  method_type();

public:

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
  {return m_class_type;}

  void
  set_class_type(shared_ptr<class_decl>);

  virtual ~method_type();

private:
  shared_ptr<class_decl> m_class_type;
};// end class method_type

/// Hasher for intances of method_type
struct method_type_hash
{
  size_t
  operator()(const method_type& t)const;
};//end struct method_type_hash

class template_parameter;
class template_type_parameter;
class template_non_type_parameter;
class template_template_parameter;

/// The base class of templates.
class template_decl
{

public:

  template_decl()
  {}

  void
  add_template_parameter(shared_ptr<template_parameter> p)
  {m_parms.push_back(p);}

  const std::list<shared_ptr<template_parameter> >&
  get_template_parameters() const
  {return m_parms;}

  virtual bool
  operator==(const template_decl& o) const;

  virtual ~template_decl();

private:
  std::list<shared_ptr<template_parameter> > m_parms;
};// end class template_decl

struct template_decl_hash
{
  size_t
  operator()(const template_decl&) const;

};//end struct template_decl_hash

/// Base class for a template parameter.  Client code should use the
/// more specialized type_template_parameter,
/// non_type_template_parameter and template_template_parameter below.
class template_parameter
{
  // Forbidden
  template_parameter();

 public:

  template_parameter(unsigned index)
    : m_index(index)
  {}

  virtual bool
  operator==(const template_parameter&) const;

  unsigned
  get_index() const
  {return m_index;}

  virtual ~template_parameter();

private:
  unsigned m_index;
};//end class template_parameter

struct template_parameter_hash
{
  size_t
  operator()(const template_parameter& t) const;
};//end class template_parameter_hash

struct dynamic_template_parameter_hash
{
  size_t
  operator()(const template_parameter*) const;
};//end struct dynamic_template_parameter_hash

struct template_parameter_shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<template_parameter> t) const
  {
    return dynamic_template_parameter_hash()(t.get());
  }
};// end struct template_parameter_shared_ptr_hash

/// Abstracts a type template parameter.
class template_type_parameter : public template_parameter,
				public virtual type_decl
{
  // Forbidden
  template_type_parameter();

public:
  template_type_parameter(unsigned index,
			  const std::string& name,
			  location locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus),
      template_parameter(index)
  {}

  virtual bool
  operator==(const template_type_parameter&) const;

  virtual ~template_type_parameter();
};//end class template_type_parameter

struct template_type_parameter_hash
{
  size_t
  operator()(const template_type_parameter& t) const;
};//end struct template_type_parameter_hash

/// Abstracts non type template parameters.
class template_non_type_parameter : public template_parameter,
				    public virtual decl_base
{
  // Forbidden
  template_non_type_parameter();

public:

  template_non_type_parameter(unsigned index,
			      const std::string& name,
			      shared_ptr<type_base> type,
			      location locus)
    : decl_base(name, locus, ""),
      template_parameter(index),
      m_type(type)
  {}

  virtual bool
  operator==(const template_non_type_parameter&) const;


  shared_ptr<type_base>
  get_type() const
  {return m_type;}

  virtual ~template_non_type_parameter();

private:
  shared_ptr<type_base> m_type;
};// class template_non_type_parameter

struct template_non_type_parameter_hash
{
  size_t
  operator()(const template_non_type_parameter& t) const;
};// end struct template_non_type_parameter_hash

/// Abstracts a template template parameter.
class template_template_parameter : public template_type_parameter,
				    public template_decl
{
  // Forbidden
  template_template_parameter();

public:

  template_template_parameter(unsigned index,
			      const std::string& name,
			      location locus)
    : decl_base(name, locus),
      type_base(0, 0),
      type_decl(name, 0, 0, locus, name, VISIBILITY_DEFAULT),
      template_type_parameter(index, name, locus)
  {}

  virtual bool
  operator==(const template_template_parameter& o) const;

  virtual ~template_template_parameter();
};//end class template_template_parameter

/// A hasher for instances of template_template_parameter
struct template_template_parameter_hash
{
  size_t
  operator()(const template_template_parameter& t) const;
};// end struct template_template_parameter_hash

/// This abstracts a composition of types based on template type
/// parameters.  The result of the composition is a type that can be
/// referred to by a template non-type parameter.  Instances of this
/// type can appear at the same level as template parameters, in the
/// scope of a template_decl.
class tmpl_parm_type_composition : public template_parameter,
				   public virtual decl_base
{
  tmpl_parm_type_composition();

public:
  tmpl_parm_type_composition(unsigned			index,
			     shared_ptr<type_base>	composed_type);

  shared_ptr<type_base>
  get_composed_type() const
  {return m_type;}

  void
  set_composed_type(shared_ptr<type_base> t)
  {m_type = t;}


  virtual ~tmpl_parm_type_composition();

private:
  shared_ptr<type_base> m_type;
};// end class tmpl_parm_type_composition

/// Abstract a function template declaration.
class function_template_decl : public template_decl, public scope_decl
{
  // Forbidden
  function_template_decl();

public:

  function_template_decl(location	locus,
			 visibility	vis = VISIBILITY_DEFAULT,
			 binding	bind	    = BINDING_NONE)
    : decl_base("", locus, "", vis),
      scope_decl("", locus),
      m_binding(bind)
  {}

  function_template_decl(shared_ptr<function_decl> pattern,
			 location locus,
			 visibility vis = VISIBILITY_DEFAULT,
			 binding bind = BINDING_NONE)
    : decl_base(pattern->get_name(), locus,
		pattern->get_name(), vis),
      scope_decl(pattern->get_name(), locus),
      m_binding(bind)
  {set_pattern(pattern);}

  virtual bool
  operator==(const function_template_decl&) const;

  void
  set_pattern(shared_ptr<function_decl> p)
  {
    m_pattern = p;
    add_decl_to_scope(p, this);
    set_name(p->get_name());
  }

  shared_ptr<function_decl>
  get_pattern() const
  {return m_pattern;}

  binding
  get_binding() const
  {return m_binding;}

  virtual ~function_template_decl();

private:
  shared_ptr<function_decl> m_pattern;
  binding m_binding;
};//end class function_template_decl

/// Hashing functor for pointer to a function template.
struct fn_tmpl_shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<function_template_decl>) const;
};//end struct fn_tmpl_shared_ptr_hash

/// Hash functor for function templates
struct function_template_decl_hash
{
  size_t
  operator()(const function_template_decl&) const;
};// end struct function_template_decl_hash

/// Abstract a class template.
class class_template_decl : public template_decl, public scope_decl
{
  // Forbidden
  class_template_decl();

public:
  class_template_decl(location locus,
		      visibility vis = VISIBILITY_DEFAULT)
    : decl_base("", locus, "", vis),
      scope_decl("", locus)
  {}

  /// Constructor for the class_template_decl type.
  ///
  /// \param the pattern of the class template.  This must NOT be a
  /// null pointer.  If you really this to be null, please use the
  /// constructor above instead.
  ///
  /// \param the source location of the declaration of the type.
  ///
  /// \param the visibility of the instances of class instantiated
  /// from this template.
  class_template_decl(shared_ptr<class_decl> pattern,
		      location locus,
		      visibility vis = VISIBILITY_DEFAULT);


  virtual bool
  operator==(const class_template_decl&) const;

  void
  set_pattern(shared_ptr<class_decl> p);

  shared_ptr<class_decl>
  get_pattern() const
  {return m_pattern;}


  virtual ~class_template_decl();

private:
  shared_ptr<class_decl> m_pattern;
};// end class class_template_decl

struct class_template_decl_hash
{
  size_t
  operator()(const class_template_decl&) const;
};// end struct class_template_decl_hash

struct class_tmpl_shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<class_template_decl>) const;
};// end struct class_tmpl_shared_ptr_hash

/// Abstracts a class declaration.
class class_decl : public scope_type_decl
{
  // Forbidden
  class_decl();

public:

  enum access_specifier
  {
    no_access,
    private_access,
    protected_access,
    public_access
  };//end enum access_specifier

  /// The base class for member types, data members and member
  /// functions.  Its purpose is to mainly to carry the access
  /// specifier (and possibly other properties that might be shared by
  /// all class members) for the member.
  class member
  {
    // Forbidden
    member();
  public:

    member(access_specifier a,
	   bool is_static = false)
      : m_access(a),
	m_is_static(is_static)
    {}

    access_specifier
    get_access_specifier() const
    {return m_access;}

    bool
    is_static() const
    {return m_is_static;}

    bool
    operator==(const member& o) const
    {
      return (get_access_specifier() == o.get_access_specifier()
	      && is_static() == o.is_static());
    }

  private:
    enum access_specifier m_access;
    bool m_is_static;
  };//end class member.

  /// Hasher for a class_decl::member
  struct member_hash
  {
    size_t
    operator()(const member& m) const
    {
      hash<int> hash_int;
      return hash_int(m.get_access_specifier());
    }
  };// struct member_hash

  /// Abstracts a member type declaration.
  class member_type : public member, public virtual decl_base
  {
    //Forbidden
    member_type();

  public:

    member_type(shared_ptr<type_base> t,
		access_specifier access)
      : decl_base("", location()),
	member(access),
	m_type(t)
    {
    }

    bool
    operator==(const member_type& o) const
    {
      return (*as_type() == *o.as_type()
	      && static_cast<member>(*this) == o);
    }

    operator shared_ptr<type_base>() const
    {return m_type;}

    shared_ptr<type_base>
    as_type() const
    {return m_type;}

  private:
    shared_ptr<type_base> m_type;
  };//end class member_type

  /// A hash functor for instances class_decl::member_type.
  struct member_type_hash
  {
    size_t
    operator()(const member_type& t)const;
  };// end struct member_type_hash

  /// Abstraction of a base specifier in a class declaration.
  class base_spec : public member
  {

    // Forbidden
    base_spec();

  public:

    base_spec(shared_ptr<class_decl> base,
	      access_specifier a)
      : member(a),
	m_base_class(base)
    {}

    const shared_ptr<class_decl>
    get_base_class() const
    {return m_base_class;}

    bool
    operator==(const base_spec& other) const
    {
      return (static_cast<member>(*this) == other
	      && *get_base_class() == *other.get_base_class());
    }

  private:
    shared_ptr<class_decl> m_base_class;
  };// end class base_spec

  /// A hashing functor for instances of class_decl::base_spec.
  struct base_spec_hash
  {
    size_t
    operator()(const base_spec& t) const;
  };// end struct base_spec_hash

  /// Abstract a data member declaration in a class declaration.
  class data_member : public var_decl, public member
  {
    // Forbidden
    data_member();

  public:

    data_member(shared_ptr<var_decl> data_member,
		access_specifier access,
		bool is_laid_out,
		bool is_static,
		size_t offset_in_bits);

    data_member(const std::string&	name,
		shared_ptr<type_base>&	type,
		access_specifier access,
		location		locus,
		const std::string&	mangled_name,
		visibility		vis,
		binding		bind,
		bool			is_laid_out,
		bool			is_static,
		size_t			offset_in_bits);

    bool
    is_laid_out() const
    {return m_is_laid_out;}

    size_t
    get_offset_in_bits() const
    {return m_offset_in_bits;}

    bool
    operator==(const data_member& other) const
    {
      return (is_laid_out() == other.is_laid_out()
	      && get_offset_in_bits() == other.get_offset_in_bits()
	      && static_cast<var_decl>(*this) ==other
	      && static_cast<member>(*this) == other);
    }

    virtual ~data_member();

  private:
    bool m_is_laid_out;
    size_t m_offset_in_bits;
  };// end class data_member

  /// Hasher for a data_member.
  struct data_member_hash
  {
    size_t
    operator()(data_member& t);
  };// end struct data_member_hash


  /// Abstraction of the declaration of a method. This is an
  /// implementation detail for class_decl::member_function.
  class method_decl : public function_decl
  {
    method_decl();

  public:

    method_decl(const std::string&			name,
		const std::vector<shared_ptr<parameter> >& parms,
		shared_ptr<type_base>		return_type,
		shared_ptr<class_decl>		class_type,
		size_t				ftype_size_in_bits,
		size_t				ftype_align_in_bits,
		bool				declared_inline,
		location				locus,
		const std::string&			mangled_name = "",
		visibility				vis = VISIBILITY_DEFAULT,
		binding				bind = BINDING_GLOBAL);

    method_decl(const std::string&	name,
		shared_ptr<method_type> type,
		bool			declared_inline,
		location		locus,
		const std::string&	mangled_name = "",
		visibility		vis	     = VISIBILITY_DEFAULT,
		binding		bind	     = BINDING_GLOBAL);

    method_decl(const std::string&		name,
		shared_ptr<function_type>	type,
		bool				declared_inline,
		location			locus,
		const std::string&		mangled_name = "",
		visibility			vis	     = VISIBILITY_DEFAULT,
		binding			bind	     = BINDING_GLOBAL);

    method_decl(const std::string&	name,
		shared_ptr<type_base>	type,
		bool			declared_inline,
		location		locus,
		const std::string&	mangled_name = "",
		visibility		vis	     = VISIBILITY_DEFAULT,
		binding		bind	     = BINDING_GLOBAL);

    const shared_ptr<method_type>
    get_type() const;

    void
    set_type(shared_ptr<method_type> fn_type)
    {function_decl::set_type(fn_type);}

    virtual ~method_decl();
  };//end class method_decl;

  /// Abstracts a member function declaration in a class declaration.
  class member_function : public method_decl, public member
  {
    // Forbidden
    member_function();

  public:

    member_function
    (const std::string&	name,
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
      member(access, is_static),
      m_vtable_offset_in_bits(vtable_offset_in_bits),
      m_is_constructor(is_constructor),
      m_is_destructor(is_destructor),
      m_is_const(is_const)
    {}

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
      member(access, is_static),
      m_vtable_offset_in_bits(vtable_offset_in_bits),
      m_is_constructor(is_constructor),
      m_is_destructor(is_destructor),
      m_is_const(is_const)
    {}

    member_function(shared_ptr<function_decl>	fn,
		    access_specifier		access,
		    size_t			vtable_offset_in_bits,
		    bool			is_static,
		    bool			is_constructor,
		    bool			is_destructor,
		    bool			is_const);

    size_t
    get_vtable_offset_in_bits() const
    {return m_vtable_offset_in_bits;}

    bool
    is_constructor() const
    {return m_is_constructor;}

    bool
    is_destructor() const
    {return m_is_destructor;}

    bool
    is_const() const
    {return m_is_const;}

    bool
    operator==(const member_function& o) const
    {
      return (get_vtable_offset_in_bits() == o.get_vtable_offset_in_bits()
	      && is_constructor() == o.is_constructor()
	      && is_destructor() == o.is_destructor()
	      && is_const() == o.is_const()
	      && static_cast<member>(*this) == o
	      && static_cast<function_decl>(*this) == o);
    }

  private:
    size_t m_vtable_offset_in_bits;
    bool m_is_constructor;
    bool m_is_destructor;
    bool m_is_const;
  };// end class member_function

  /// A hashing functor for instances of class_decl::member_function.
  struct member_function_hash
  {
    size_t
    operator()(const member_function& t) const;
  };// end struct member_function_hash

  /// Abstract a member function template.
  class member_function_template : public member
  {
    // Forbiden
    member_function_template();

  public:
    member_function_template
    (shared_ptr<function_template_decl> f,
     access_specifier			access,
     bool				is_static,
     bool				is_constructor,
     bool				is_const)
      : member(access, is_static),
	m_is_constructor(is_constructor),
	m_is_const(is_const),
	m_fn_tmpl(f)
    {}

    bool
    is_constructor() const
    {return m_is_constructor;}

    bool
    is_const() const
    {return m_is_const;}

    operator const function_template_decl& () const
    {return *m_fn_tmpl;}

    shared_ptr<function_template_decl>
    as_function_template_decl() const
    {return m_fn_tmpl;}

    bool
    operator==(const member_function_template& o) const;

  private:
    bool m_is_constructor;
    bool m_is_const;
    shared_ptr<function_template_decl> m_fn_tmpl;
  };//end class member_function_template

  struct member_function_template_hash
  {
    size_t
    operator()(const member_function_template&) const;
  };// end struct member_function_template_hash

  /// Abstracts a member class template template
  class member_class_template : public member
  {
    // Forbidden
    member_class_template();

  public:
    member_class_template(shared_ptr<class_template_decl>c,
			  access_specifier access,
			  bool is_static)
      : member(access, is_static),
	m_class_tmpl(c)
    {}

    operator const class_template_decl& () const
    {return *m_class_tmpl;}

    shared_ptr<class_template_decl>
    as_class_template_decl() const
    {return m_class_tmpl;}

    bool
    operator==(const member_class_template& o) const;

  private:
    shared_ptr<class_template_decl> m_class_tmpl;
  };// end class member_class_template

  /// A hashing functor for instances of member_class_template.
  struct member_class_template_hash
  {
    size_t
    operator()(member_class_template&) const;
  };// end struct member_class_template_hash

  typedef std::list<shared_ptr<base_spec> > base_specs_type;
  typedef std::list<shared_ptr<member_type> > member_types_type;
  typedef std::list<shared_ptr<data_member> > data_members_type;
  typedef std::list<shared_ptr<member_function> > member_functions_type;
  typedef std::list<shared_ptr<member_function_template> >
  member_function_templates_type;
  typedef std::list<shared_ptr<member_class_template> >
  member_class_templates_type;

  class_decl(const std::string&				name,
	     size_t						size_in_bits,
	     size_t						align_in_bits,
	     location						locus,
	     visibility					vis,
	     std::list<shared_ptr<base_spec> >&		bases,
	     std::list<shared_ptr<member_type> >&	member_types,
	     std::list<shared_ptr<data_member> >&	data_members,
	     std::list<shared_ptr<member_function> >&	member_fns);

  class_decl(const std::string& name,
	     size_t		size_in_bits,
	     size_t		align_in_bits,
	     location		locus,
	     visibility	vis);

  class_decl(const std::string& name,
	     bool is_declaration_only = true);

  bool
  hashing_started() const
  {return m_hashing_started;}

  void
  hashing_started(bool b) const
  {m_hashing_started = b;}

  bool
  is_declaration_only() const
  {return m_is_declaration_only;}

  void
  set_earlier_declaration(shared_ptr<class_decl> declaration);

  void
  set_earlier_declaration(shared_ptr<type_base> declaration);

  shared_ptr<class_decl>
  get_earlier_declaration() const
  {return m_declaration;}

  void
  add_base_specifier(shared_ptr<base_spec> b)
  {m_bases.push_back(b);}

  const std::list<shared_ptr<base_spec> >&
  get_base_specifiers() const
  {return m_bases;}

  void
  add_member_type(shared_ptr<member_type>t);

  const std::list<shared_ptr<member_type> >&
  get_member_types() const
  {return m_member_types;}

  void
  add_data_member(shared_ptr<data_member> m);

  const std::list<shared_ptr<data_member> >&
  get_data_members() const
  {return m_data_members;}

  void
  add_member_function(shared_ptr<member_function> m);

  const std::list<shared_ptr<member_function> >&
  get_member_functions() const
  {return m_member_functions;}

  void
  add_member_function_template(shared_ptr<member_function_template>);

  const member_function_templates_type&
  get_member_function_templates() const
  {return m_member_function_templates;}

  void
  add_member_class_template(shared_ptr<member_class_template>);

  const member_class_templates_type&
  get_member_class_templates() const
  {return m_member_class_templates;}

  virtual bool
  operator==(const class_decl&) const;

  virtual ~class_decl();

private:
  mutable bool				m_hashing_started;
  shared_ptr<class_decl>		m_declaration;
  bool					m_is_declaration_only;
  base_specs_type			m_bases;
  member_types_type			m_member_types;
  data_members_type			m_data_members;
  member_functions_type		m_member_functions;
  member_function_templates_type	m_member_function_templates;
  member_class_templates_type		m_member_class_templates;
};// end class class_decl

/// Hasher for the class_decl type
struct class_decl_hash
{
  size_t operator()(const class_decl& t) const;

};//end struct class_decl_hash

} // end namespace abigail
#endif // __ABG_IR_H__
