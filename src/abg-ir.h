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
class translation_unit;

void add_decl_to_scope(shared_ptr<decl_base>,
		       scope_decl*);

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
    BINDING_WEAK,
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

  /// Add a member decl to this scope.  Note that user code should not
  /// use this, but rather use #add_decl_to_scope.
  ///
  /// \param member the new member decl to add to this scope.
  void
  add_member_decl(const shared_ptr<decl_base> member)
  {m_members.push_back(member);}

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

  bool
  is_empty() const
  {return get_member_decls().empty();}

  virtual ~scope_decl();

  friend void
  add_decl_to_scope(shared_ptr<decl_base>,
		    scope_decl*);

private:
  std::list<shared_ptr<decl_base> > m_members;
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
  global_scope()
    : decl_base("", location()),
      scope_decl("", location()),
      m_translation_unit(0)
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
    CV_VOLATILE = 1 << 1
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

/// Abstraction for a function declaration.
class function_decl: public virtual decl_base
{
public:

  /// Abtraction for the parameter of a function.
  class parameter
  {
  public:

    parameter(const shared_ptr<type_base> type,
	      std::string name,
	      location loc)
      : m_type(type),
	m_name(name),
	m_location(loc)
    {}

    const shared_ptr<type_base>
    get_type()const
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

  private:
    shared_ptr<type_base> m_type;
    std::string m_name;
    location m_location;
  };// end class function::parameter

  /// Hasher for an instance of function::parameter
  struct parameter_hash
  {
    size_t
    operator()(const parameter& p) const
    {
      type_shared_ptr_hash hash_type_ptr;
      return hash_type_ptr(p.get_type());
    }
  };//end struct parameter_hash

  function_decl
  (const std::string&			name,
   std::list<shared_ptr<parameter> >	parms,
   shared_ptr<type_base>		return_type,
   bool				declared_inline,
   location				locus,
   const std::string&			mangled_name = "",
   visibility				vis = VISIBILITY_DEFAULT,
   binding				bind = BINDING_GLOBAL)
    : decl_base(name, locus, mangled_name, vis),
      m_parms(parms),
      m_return_type(return_type),
      m_declared_inline(declared_inline),
      m_binding(bind)
  {}

  const std::list<shared_ptr<parameter> >&
  get_parameters() const
  {return m_parms;}

  void
  add_parameter(shared_ptr<parameter> parm)
  {m_parms.push_back(parm);}

  void
  add_parameters(std::list<shared_ptr<parameter> >parms)
  {
    for (std::list<shared_ptr<parameter> >::const_iterator i = parms.begin();
	 i != parms.end();
	 ++i)
      m_parms.push_back(*i);
  }

  const shared_ptr<type_base>
  get_return_type() const
  {return m_return_type;}

  void
  set_return_type(shared_ptr<type_base> t)
  {m_return_type = t;}

  bool
  is_declared_inline() const
  {return m_declared_inline;}

  binding
  get_binding() const
  {return m_binding;}

  virtual bool
  operator==(const function_decl& o) const;

  virtual ~function_decl();

private:
  std::list<shared_ptr<parameter> > m_parms;
  shared_ptr<type_base> m_return_type;
  bool m_declared_inline;
  decl_base::binding m_binding;
};// end class function_decl

/// Hasher for function_decl
struct function_decl_hash
{
  size_t
  operator()(const function_decl& t) const;

};// end function_decl_hash

/// Abstracts a class declaration.
class class_decl : public scope_type_decl
{
  // Forbidden
  class_decl();

public:

  enum access_specifier
  {
    private_access,
    protected_access,
    public_access,
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

    member(access_specifier a)
      : m_access(a)
    {}

    access_specifier
    get_access_specifier() const
    {return m_access;}

    bool
    operator==(const member& o)
    {return get_access_specifier() == o.get_access_specifier();}

  private:
    enum access_specifier m_access;
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
		size_t offset_in_bits)
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
	member(access),
	m_is_laid_out(is_laid_out),
	m_is_static(is_static),
	m_offset_in_bits(offset_in_bits)
    {}

    data_member(const std::string&	name,
		shared_ptr<type_base>&	type,
		access_specifier access,
		location		locus,
		const std::string&	mangled_name,
		visibility		vis,
		binding		bind,
		bool			is_laid_out,
		bool			is_static,
		size_t			offset_in_bits)
      : decl_base(name, locus, name, vis),
	var_decl(name, type, locus, mangled_name, vis, bind),
	member(access),
	m_is_laid_out(is_laid_out),
	m_is_static(is_static),
	m_offset_in_bits(offset_in_bits)
    {}

    bool
    is_laid_out() const
    {return m_is_laid_out;}

    bool
    is_static() const
    {return m_is_static;}

    size_t
    get_offset_in_bits() const
    {return m_offset_in_bits;}

    bool
    operator==(const data_member& other) const
    {
      return (is_laid_out() == other.is_laid_out()
	      && is_static() == other.is_static()
	      && get_offset_in_bits() == other.get_offset_in_bits()
	      && static_cast<var_decl>(*this) ==other
	      && static_cast<member>(*this) == other);
    }

  private:
    bool m_is_laid_out;
    bool m_is_static;
    size_t m_offset_in_bits;
  };// end class data_member

  /// Hasher for a data_member.
  struct data_member_hash
  {
    size_t
    operator()(data_member& t);
  };// end struct data_member_hash

  /// Abstracts a member function declaration in a class declaration.
  class member_function : public function_decl, public member
  {
    // Forbidden
    member_function();

  public:

    member_function
    (const std::string&		name,
     std::list<shared_ptr<parameter> >	parms,
     shared_ptr<type_base>		return_type,
     access_specifier			access,
     bool				declared_inline,
     location				locus,
     const std::string&		mangled_name,
     visibility			vis,
     binding				bind,
     size_t				vtable_offset_in_bits,
     bool				is_static,
     bool				is_constructor,
     bool				is_destructor,
     bool				is_const)
      : decl_base(name, locus, name, vis),
	function_decl(name, parms, return_type, declared_inline,
		      locus, mangled_name, vis, bind),
      member(access),
      m_vtable_offset_in_bits(vtable_offset_in_bits),
      m_is_static(is_static),
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
		    bool			is_const)
      : decl_base(fn->get_name(), fn->get_location(),
		  fn->get_mangled_name(), fn->get_visibility()),
	function_decl(fn->get_name(),
		      fn->get_parameters(),
		      fn->get_return_type(),
		      fn->is_declared_inline(),
		      fn->get_location(),
		      fn->get_mangled_name(),
		      fn->get_visibility(),
		      fn->get_binding()),
	member(access),
	m_vtable_offset_in_bits(vtable_offset_in_bits),
      m_is_static(is_static),
      m_is_constructor(is_constructor),
      m_is_destructor(is_destructor),
      m_is_const(is_const)
    {}

    size_t
    get_vtable_offset_in_bits() const
    {return m_vtable_offset_in_bits;}

    bool
    is_static() const
    {return m_is_static;}

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
	      && is_static() == o.is_static()
	      && is_constructor() == o.is_constructor()
	      && is_destructor() == o.is_destructor()
	      && is_const() == o.is_const()
	      && static_cast<member>(*this) == o
	      && static_cast<function_decl>(*this) == o);
    }

  private:
    size_t m_vtable_offset_in_bits;
    bool m_is_static;
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

  class_decl(const std::string&				name,
	     size_t						size_in_bits,
	     size_t						align_in_bits,
	     location						locus,
	     visibility					vis,
	     const std::list<shared_ptr<base_spec> >&		bases,
	     const std::list<shared_ptr<member_type> >&	member_types,
	     const std::list<shared_ptr<data_member> >&	data_members,
	     const std::list<shared_ptr<member_function> >&	member_fns)
    : decl_base(name, locus, name, vis),
      type_base(size_in_bits, align_in_bits),
      scope_type_decl(name, size_in_bits, align_in_bits, locus, vis),
    m_bases(bases),
    m_member_types(member_types),
    m_data_members(data_members),
    m_member_functions(member_fns)
  {}

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

  virtual bool
  operator==(const class_decl&) const;

  virtual ~class_decl();

private:
  std::list<shared_ptr<base_spec> > m_bases;
  std::list<shared_ptr<member_type> > m_member_types;
  std::list<shared_ptr<data_member> > m_data_members;
  std::list<shared_ptr<member_function> > m_member_functions;
};// end class class_decl

/// Hasher for the class_decl type
struct class_decl_hash
{
  size_t operator()(const class_decl& t) const;

};//end struct class_decl_hash


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

class function_template_decl : public template_decl, public scope_decl
{
  // Forbidden
  function_template_decl();

public:

  function_template_decl(location		locus,
			 visibility vis = VISIBILITY_DEFAULT,
			 binding bind = BINDING_NONE)
    : decl_base("", locus, "",  vis),
      scope_decl("", locus),
      m_binding(bind)
  {}

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

struct function_template_decl_hash
{
  size_t
  operator()(const function_template_decl&) const;
};// end struct function_template_decl_hash

struct fn_tmpl_shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<function_template_decl>) const;
};//end struct fn_tmpl_shared_ptr_hash
} // end namespace abigail
#endif // __ABG_IR_H__
