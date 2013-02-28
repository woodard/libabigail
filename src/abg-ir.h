#ifndef __ABL_IR_H__
#define __ABL_IR_H__

#include <cstddef>
#include <tr1/memory>
#include <list>

using std::tr1::shared_ptr;

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
  unsigned m_value;

  location();
  location (unsigned);

public:

  friend class location_manager;
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
  location create_new_location(const std::string&	file,
			       size_t			line,
			       size_t			column);
  void expand_location(const location	location,
		       std::string&	path,
		       unsigned&	line,
		       unsigned&	column);
};

class scope_decl;

/// \brief The base type of all declarations.
class decl
{
  decl();
public:

  decl(const std::string& name,
       const scope_decl& context,
       location locus);
  decl(const decl&);

  location get_location() const;
  void set_location(const location&);

  scope_decl& get_context() const;
};

/// \brief A declaration that introduces a scope.
class scope_decl : public decl
{
  scope_decl();

public:

  scope_decl(const std::string &name,
	     const scope_decl& context,
	     location& locus);
  scope_decl(location&);

  void add_member_decl(const shared_ptr<decl>);
  const std::list<shared_ptr<decl> >& get_member_decls() const;
};

/// The an abstraction helper for type declarations
class type_base
{
public:
  type_base();
  void set_size_in_bits(const size_t);
  size_t get_size_in_bits() const;

  void set_alignment_in_bits(const size_t);
  size_t get_alignment_in_bits() const;
};

/// A basic type declaration that introduces no scope.
class type_decl : public decl, public type_base
{
  type_decl();
public:

  type_decl(const std::string& name,
	    const scope_decl& context,
	    location locus);
  type_decl(const type_decl&);
};

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public type_base
{
  scope_type_decl();
public:

  scope_type_decl(const std::string& name,
		  const scope_decl& context,
		  location locus);
};

} // end namespace abigail
#endif // __ABL_IR_H__
