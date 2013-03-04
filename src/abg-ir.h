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
  location (unsigned);
public:

  location();
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

protected:
  enum kind
  {
    KIND_DECL,
    KIND_SCOPE_DECL,
    KIND_TYPE_DECL,
    KIND_SCOPE_TYPE_DECL,
    KIND_NAMESPACE_DECL
  };

  enum kind what_decl_kind () const;

  decl(kind				what_kind,
       const std::string&		name,
       const shared_ptr<scope_decl>	context,
       location			locus);

public:
  decl(const std::string&		name,
       const shared_ptr<scope_decl>	context,
       location			locus);
  decl(location);
  decl(const decl&);

  location get_location() const;
  void set_location(const location&);

  shared_ptr<scope_decl> get_context() const;

private:
  kind m_kind;
  location m_location;
  std::string m_name;
  shared_ptr<scope_decl> m_context;
};

/// \brief A declaration that introduces a scope.
class scope_decl : public decl
{
  scope_decl();

protected:
  scope_decl(kind				akind,
	     const std::string&		name,
	     const shared_ptr<scope_decl>	context,
	     location				locus);

public:
  scope_decl(const std::string&		name,
	     const shared_ptr<scope_decl>	context,
	     location				locus);
  scope_decl(location);

  void add_member_decl(const shared_ptr<decl>);
  const std::list<shared_ptr<decl> >& get_member_decls() const;

private:
  std::list<shared_ptr<decl> > m_members;
};

/// An abstraction helper for type declarations
class type_base
{
  // Forbid this.
  type_base();

public:

  type_base(size_t s, size_t a);

  void set_size_in_bits(size_t);
  size_t get_size_in_bits() const;

  void set_alignment_in_bits(size_t);
  size_t get_alignment_in_bits() const;

private:

  size_t m_size_in_bits;
  size_t m_alignment_in_bits;
};

/// A basic type declaration that introduces no scope.
class type_decl : public decl, public type_base
{
  // Forbidden.
  type_decl();

protected:

  type_decl(kind				akind,
	    const std::string&			name,
	    size_t				size_in_bits,
	    size_t				alignment_in_bits,
	    const shared_ptr<scope_decl>	context,
	    location				locus);

public:

  type_decl(const std::string&			name,
	    size_t				size_in_bits,
	    size_t				alignment_in_bits,
	    const shared_ptr<scope_decl>	context,
	    location				locus);
};

/// A type that introduces a scope.
class scope_type_decl : public scope_decl, public type_base
{
  scope_type_decl();

protected:

  scope_type_decl(kind				akind,
		  const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  const shared_ptr<scope_decl>	context,
		  location			locus);
public:

  scope_type_decl(const std::string&		name,
		  size_t			size_in_bits,
		  size_t			alignment_in_bits,
		  const shared_ptr<scope_decl>	context,
		  location			locus);
};

class namespace_decl : public scope_decl
{
public:

  namespace_decl(const std::string& name,
		 const shared_ptr<namespace_decl> context,
		 location locus);
};

} // end namespace abigail
#endif // __ABL_IR_H__
