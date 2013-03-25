// -*- mode: C++ -*-

#ifndef __ABL_IR_H__
#define __ABL_IR_H__

#include <cstddef>
#include <tr1/memory>
#include <list>
#include <string>
#include <tr1/functional>
#include <typeinfo>

#include "abg-hash.h"

using std::tr1::shared_ptr;
using std::tr1::hash;
using std::string;

// Our real stuff
namespace abigail
{
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

class decl_base;
class scope_decl;

void add_decl_to_scope(shared_ptr<decl_base>,
		       shared_ptr<scope_decl>);

/// \brief The base type of all declarations.
class decl_base
{
  decl_base();

  void
  set_scope(shared_ptr<scope_decl>);

public:

  enum kind
  {
    KIND_DECL,
    KIND_SCOPE_DECL,
    KIND_TYPE_DECL,
    KIND_SCOPE_TYPE_DECL,
    KIND_NAMESPACE_DECL
  };

  enum kind
  what_kind () const;

protected:

  decl_base(kind			what_kind,
	    const std::string&		name,
	    location			locus);

public:

  decl_base(const std::string&		name,
	    location			locus);
  decl_base(location);
  decl_base(const decl_base&);
  virtual ~decl_base();

  location
  get_location() const
  {
    return m_location;
  }

  void
  set_location(const location&l)
  {
    m_location = l;
  }

  const string&
  get_name() const
  {
    return m_name;
  }

  void
  set_name(const string& n)
  {
    m_name = n;
  }

  shared_ptr<scope_decl>
  get_scope() const
  {
    return m_context;
  }

  friend void
  add_decl_to_scope(shared_ptr<decl_base>,
		    shared_ptr<scope_decl>);
private:
  kind m_kind;
  location m_location;
  std::string m_name;
  shared_ptr<scope_decl> m_context;
};

/// \brief A declaration that introduces a scope.
class scope_decl : public decl_base
{
  scope_decl();

  void
  add_member_decl(const shared_ptr<decl_base>);

protected:
  scope_decl(kind				akind,
	     const std::string&		name,
	     location				locus);

public:
  scope_decl(const std::string&		name,
	     location				locus);
  scope_decl(location);

  const std::list<shared_ptr<decl_base> >&
  get_member_decls() const;

  virtual ~scope_decl();

  friend void
  add_decl_to_scope(shared_ptr<decl_base>,
		    shared_ptr<scope_decl>);

private:
  std::list<shared_ptr<decl_base> > m_members;
};

/// \brief Facility to hash instances of decl_base.
struct decl_base_hash
{
  size_t
  operator() (const decl_base& d)
  {
    hash<string> str_hash;
    hash<unsigned> unsigned_hash;

    size_t v = str_hash(typeid(d).name());
    if (!d.get_name().empty())
      v = hashing::combine_hashes(v, str_hash(d.get_name()));
    if (d.get_location())
      v = hashing::combine_hashes(v, unsigned_hash(d.get_location()));

    v = hashing::combine_hashes(v, this->operator()(*d.get_scope()));
    return v;
  }
};//end struct decl_base_hash

/// An abstraction helper for type declarations
class type_base
{
  // Forbid this.
  type_base();

public:

  type_base(size_t s, size_t a);
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
};

/// A basic type declaration that introduces no scope.
class type_decl : public decl_base, public type_base
{
  // Forbidden.
  type_decl();

protected:

  type_decl(kind			akind,
	    const std::string&		name,
	    size_t			size_in_bits,
	    size_t			alignment_in_bits,
	    location			locus);

public:

  type_decl(const std::string&			name,
	    size_t				size_in_bits,
	    size_t				alignment_in_bits,
	    location				locus);

  virtual ~type_decl();

};

/// Facility to hash instance of type_decl
struct type_decl_hash
{
  size_t
  operator()(const type_decl& t)
  {
    decl_base_hash decl_hash;
    hash<size_t> size_t_hash;

    size_t v = decl_hash(static_cast<type_decl>(t));
    v = hashing::combine_hashes(v, size_t_hash(t.get_size_in_bits()));
    v = hashing::combine_hashes(v, size_t_hash(t.get_alignment_in_bits()));

    return v;
  }
};//end struct type_decl_hash

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public type_base
{
  scope_type_decl();

protected:

  scope_type_decl(kind				akind,
		  const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  location			locus);
public:

  scope_type_decl(const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  location			locus);

  virtual ~scope_type_decl();
};

class namespace_decl : public scope_decl
{
public:

  namespace_decl(const std::string& name,
		 location locus);

  virtual ~namespace_decl();
};

} // end namespace abigail
#endif // __ABL_IR_H__
