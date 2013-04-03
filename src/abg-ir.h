// -*- mode: C++ -*-
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

bool is_decl_at_global_scope(const shared_ptr<decl_base>);

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
};

/// \brief A declaration that introduces a scope.
class scope_decl : public decl_base
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
	     const std::string& mangled_name = "",
	     visibility	vis = VISIBILITY_DEFAULT)
    : decl_base(name, locus, mangled_name, vis)
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
};

/// \brief Facility to hash instances of decl_base.
struct decl_base_hash
{
  size_t
  operator() (const decl_base& d) const
  {
    hash<string> str_hash;
    hash<unsigned> unsigned_hash;

    size_t v = str_hash(typeid(d).name());
    if (!d.get_name().empty())
      v = hashing::combine_hashes(v, str_hash(d.get_name()));
    if (d.get_location())
      v = hashing::combine_hashes(v, unsigned_hash(d.get_location()));

    if (d.get_scope())
      v = hashing::combine_hashes(v, this->operator()(*d.get_scope()));
    return v;
  }
};//end struct decl_base_hash

/// This abstracts the global scope of a given translation unit.
///
/// Only one instance of this class must be present in a given
/// translation_unit.  That instance is implicitely created the first
/// time translatin_unit::get_global_scope is invoked.
class global_scope : public scope_decl
{
  global_scope()
    : scope_decl("", location()),
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
  operator()(const type_base& t) const
  {
    hash<size_t> size_t_hash;
    hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, size_t_hash(t.get_size_in_bits()));
    v = hashing::combine_hashes(v, size_t_hash(t.get_alignment_in_bits()));
    return v;
  }
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
class type_decl : public decl_base, public type_base
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
  operator()(const type_decl& t) const
  {
    decl_base_hash decl_hash;
    type_base_hash type_hash;
    hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_hash(t));

    return v;
  }
};//end struct type_decl_hash

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public type_base
{
  scope_type_decl();

public:

  scope_type_decl(const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  location			locus,
		  const std::string&		mangled_name = "",
		  visibility			visiblity = VISIBILITY_DEFAULT);

  virtual bool
  operator==(const scope_type_decl&) const;

  virtual ~scope_type_decl();
};

/// Hasher for instances of scope_type_decl
struct scope_type_decl_hash
{
  size_t
  operator()(const scope_type_decl& t) const
  {
    decl_base_hash decl_hash;
    type_base_hash type_hash;
    hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_hash(t));

    return v;
  }
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
class qualified_type_def : public type_base, public decl_base
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
  operator()(const qualified_type_def& t) const
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
};//end struct qualified_type_def_hash

/// The abstraction of a pointer type.
class pointer_type_def : public type_base, public decl_base
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
  operator()(const pointer_type_def& t) const
  {
    hash<string> str_hash;
    type_base_hash type_hash;
    decl_base_hash decl_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, decl_hash(t));
    v = hashing::combine_hashes(v, type_hash(t));
    return v;
  }
};// end struct pointer_type_def_hash

/// Abstracts a reference type.
class reference_type_def : public type_base, public decl_base
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
  operator() (const reference_type_def& t)
  {
    hash<string> str_hash;
    type_base_hash type_hash;
    decl_base_hash decl_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, str_hash(t.is_lvalue()
					    ? "lvalue"
					    : "rvalue"));
    v = hashing::combine_hashes(v, type_hash(t));
    v = hashing::combine_hashes(v, decl_hash(t));
    return v;
  }
};//end struct reference_type_def_hash

/// Abstracts a declaration for an enum type.
class enum_type_decl: public type_base, public decl_base
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
  operator()(const enum_type_decl& t) const
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
};//end struct enum_type_decl_hash

/// The abstraction of a typedef declaration.
class typedef_decl: public type_base, public decl_base
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
  operator()(const typedef_decl& t) const
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
};// end struct typedef_decl_hash

/// Abstracts a variable declaration.
class var_decl : public decl_base
{
  // Forbidden
  var_decl();

public:

  var_decl(const std::string&		name,
	   shared_ptr<type_base>&	type,
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

} // end namespace abigail
#endif // __ABG_IR_H__
