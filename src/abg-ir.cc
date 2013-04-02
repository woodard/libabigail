#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <tr1/memory>
#include "abg-ir.h"

using std::string;
using std::tr1::dynamic_pointer_cast;

namespace abigail
{

/// \brief the location of a token represented in its simplest form.
/// Instances of this type are to be stored in a sorted vector, so the
/// type must have proper relational operators.
class expanded_location
{
  string	m_path;
  unsigned	m_line;
  unsigned	m_column;

  expanded_location();

public:

  friend class location_manager;

  expanded_location(const string&	path,
		    unsigned		line,
		    unsigned		column)
    : m_path(path), m_line(line), m_column(column)
  {
  }

  bool
  operator==(const expanded_location& l) const
  {
    return (m_path == l.m_path
	    && m_line == l.m_line
	    && m_column && l.m_column);
  }

  bool
  operator<(const expanded_location& l) const
  {
    if (m_path < l.m_path)
      return true;
    else if (m_path > l.m_path)
      return false;

    if (m_line < l.m_line)
      return true;
    else if (m_line > l.m_line)
      return false;

    return m_column < l.m_column;
  }
};

struct location_manager::priv
{
  // This sorted vector contains the expanded locations of the tokens
  // coming from a given ABI Corpus.  The index of a given expanded
  // location in the table gives us an integer that is used to build
  // instance of location types.
  std::vector<expanded_location> locs;
};

location_manager::location_manager()
{
  m_priv =
    shared_ptr<location_manager::priv>(new location_manager::priv);
}

/// Insert the triplet representing a source locus into our internal
/// vector of location triplet.  Return an instance of location type,
/// built from an integral type that represents the index of the
/// source locus triplet into our source locus table.
///
/// \param file_path the file path of the source locus
/// \param line the line number of the source location
/// \param col the column number of the source location
location
location_manager::create_new_location(const std::string&	file_path,
				      size_t			line,
				      size_t			col)
{
  expanded_location l(file_path, line, col);

  std::vector<expanded_location>::iterator i =
    std::upper_bound(m_priv->locs.begin(),
		     m_priv->locs.end(),
		     l);

  if (i == m_priv->locs.end())
    {
      m_priv->locs.push_back(l);
      i = m_priv->locs.end();
      i--;
    }
  else
    {
      std::vector<expanded_location>::iterator prev = i;
      prev--;
      if (*prev == l)
	i = prev;
      else
	i = m_priv->locs.insert(i, l);
    }
  return location(std::distance (m_priv->locs.begin(), i));
}

/// Given an instance of location type, return the triplet
/// {path,line,column} that represents the source locus.  Note that
/// the location must have been previously created from the function
/// location_manager::expand_location otherwise this function yields
/// unexpected results, including possibly a crash.
///
/// \param location the instance of location type to expand
/// \param path the resulting path of the source locus
/// \param line the resulting line of the source locus
/// \param column the resulting colum of the source locus
void
location_manager::expand_location(const location	location,
				  std::string&		path,
				  unsigned&		line,
				  unsigned&		column) const
{
  expanded_location &l = m_priv->locs[location.m_value];
  path = l.m_path;
  line = l.m_line;
  column = l.m_column;
}

// <Decl definition>
decl_base::decl_base()
{
}

decl_base::decl_base(const std::string&	name,
		     location			locus,
		     visibility vis)
  : m_location(locus),
    m_name(name),
    m_context(0),
    m_visibility(vis)
{
}

decl_base::decl_base(location l)
  : m_location(l),
    m_context(0),
    m_visibility(VISIBILITY_DEFAULT)
{
}

decl_base::decl_base(const decl_base& d)
{
  m_location = d.m_location;
  m_name = d.m_name;
  m_context = d.m_context;
  m_visibility = m_visibility;
}

/// Return true iff the two decls have the same name.
///
/// This function doesn't test if the scopes of the the two decls are
/// equal.
bool
decl_base::operator==(const decl_base& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return get_name() == other.get_name();
}

decl_base::~decl_base()
{
}

/// Setter of the scope of the current decl.
///
/// Note that the decl won't hold a reference on the scope.  It's
/// rather the scope that holds a reference on its members.
void
decl_base::set_scope(scope_decl* scope)
{
  m_context = scope;
}

// </Decl definition>

// <scope_decl definitions>
scope_decl::scope_decl(const std::string&		name,
		       location			locus,
		       visibility vis)
  : decl_base(name, locus, vis)
{
}


scope_decl::scope_decl(location l)
  : decl_base("", l)
{
}

/// Return true iff both scopes have the same names and have the same
/// member decls.
///
/// This function doesn't check for equality of the scopes of its
/// arguments.
bool
scope_decl::operator==(const scope_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  if (static_cast<decl_base>(*this) != static_cast<decl_base>(other))
    return false;

  std::list<shared_ptr<decl_base> >::const_iterator i, j;
  for (i = get_member_decls().begin(), j = other.get_member_decls().begin();
       i != get_member_decls().end() && j != other.get_member_decls().end();
       ++i, ++j)
    if (**i != **j)
      return false;

  if (i != get_member_decls().end() || j != other.get_member_decls().end())
    return false;

  return true;
}

/// Add a member decl to this scope.  Note that user code should not
/// use this, but rather use #add_decl_to_scope.
///
/// \param member the new member decl to add to this scope.
void
scope_decl::add_member_decl(const shared_ptr<decl_base> member)
{
  m_members.push_back(member);
}

const std::list<shared_ptr<decl_base> >&
scope_decl::get_member_decls() const
{
  return m_members;
}

scope_decl::~scope_decl()
{
}

/// Appends a decl to a given scope.
///
/// \param the decl to add append to the scope
///
/// \param the scope to append the decl to
void
add_decl_to_scope(shared_ptr<decl_base> decl,
		  scope_decl*		scope)
{
  if (scope && decl)
    {
      scope->add_member_decl (decl);
      decl->set_scope(scope);
    }
}

// </scope_decl definition>

// <type_base definitions>
type_base::type_base(size_t s = 8, size_t a = 8)
  : m_size_in_bits(s),
    m_alignment_in_bits(a)
{
}

/// Return true iff both type declarations are equal.
///
/// Note that this doesn't test if the scopes of both types are equal.
bool
type_base::operator==(const type_base& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (get_size_in_bits() == other.get_size_in_bits()
	  && get_alignment_in_bits() == other.get_alignment_in_bits());
}

void
type_base::set_size_in_bits(size_t s)
{
  m_size_in_bits = s;
}

size_t
type_base::get_size_in_bits() const
{
  return m_size_in_bits;
}

void
type_base::set_alignment_in_bits(size_t a)
{
  m_alignment_in_bits = a;
}

size_t
type_base::get_alignment_in_bits() const
{
  return m_alignment_in_bits;
}

type_base::~type_base()
{
}

// </type_base definitions>

//<type_decl definitions>

type_decl::type_decl(const std::string&	name,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     location			locus,
		     visibility		vis)

  : decl_base(name, locus, vis),
    type_base(size_in_bits, alignment_in_bits)
{
}

/// Return true if both types equals.
///
/// Note that this does not check the scopes of any of the types.
bool
type_decl::operator==(const type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<decl_base>(*this) == other
	  && static_cast<type_base>(*this) == other);
}

type_decl::~type_decl()
{
}

//</type_decl definitions>

// <scope_type_decl definitions>

scope_type_decl::scope_type_decl(const std::string&		name,
				 size_t			size_in_bits,
				 size_t			alignment_in_bits,
				 location			locus)
  :scope_decl(name, locus),
   type_base(size_in_bits, alignment_in_bits)
{
}

/// Return true iff both scope types are equal.
///
/// Note that this function does not consider the scope of the scope
/// types themselves.
bool
scope_type_decl::operator==(const scope_type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<scope_decl>(*this) == other
	  && static_cast<type_base>(*this) == other);
}

scope_type_decl::~scope_type_decl()
{
}

// </scope_type_decl definitions>

// <namespace_decl>
namespace_decl::namespace_decl(const std::string& name,
			       location locus)
  : scope_decl(name, locus)
{
}

/// Return true iff both namespaces and their members are equal.
///
/// Note that this function does not check if the scope of these
/// namespaces are equal.
bool
namespace_decl::operator==(const namespace_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return (static_cast<scope_decl>(*this) == other);
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

// <qualified_type_def>

/// Constructor of the qualified_type_def
///
/// \param type the underlying type
///
/// \params quals a bitfield representing the const/volatile qualifiers
///
/// \param locus the location of the qualified type definition
qualified_type_def::qualified_type_def(shared_ptr<type_base>	type,
				       CV			quals,
				       location		locus)
  : type_base(type->get_size_in_bits(),
	      type->get_alignment_in_bits()),
    decl_base("", locus,
	      dynamic_pointer_cast<decl_base>(type)->get_visibility()),
    m_cv_quals(quals),
    m_underlying_type(type)
{
  if (quals & qualified_type_def::CV_CONST)
    set_name(get_name() + "const ");
  if (quals & qualified_type_def::CV_VOLATILE)
    set_name(get_name() + "volatile ");
  set_name(get_name() + dynamic_pointer_cast<decl_base>(type)->get_name());
}

/// Return true iff both qualified types are equal.
///
/// Note that this function does not check for equality of the scopes.
bool
qualified_type_def::operator==(const qualified_type_def& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other)
      || get_cv_quals() != other.get_cv_quals())
    return false;

  return *get_underlying_type() == *other.get_underlying_type();
}

/// The destructor of the qualified type
qualified_type_def::~qualified_type_def()
{
}

/// Getter of the const/volatile qualifier bit field
char
qualified_type_def::get_cv_quals() const
{
  return m_cv_quals;
}

/// Setter of the const/value qualifiers bit field
void
qualified_type_def::set_cv_quals(char cv_quals)
{
  m_cv_quals = cv_quals;
}

/// Getter of the underlying type
const shared_ptr<type_base>
qualified_type_def::get_underlying_type() const
{
  return m_underlying_type;
}

// </qualified_type_def>

/// A hashing function for type declarations.
///
/// This function gets the dynamic type of the actualy type
/// declaration and calls the right hashing function for that type.
///
/// Note that each time a new type declaration kind is added to the
/// system, this function needs to be updated.
///
/// \param t a pointer to the type declaration to be hashed
///
/// \return the resulting hash
size_t
dynamic_type_hash::operator()(const type_base* t) const
{
  if (const type_decl* d = dynamic_cast<const type_decl*> (t))
    return type_decl_hash()(*d);
  if (const scope_type_decl* d = dynamic_cast<const scope_type_decl*>(t))
    return scope_type_decl_hash()(*d);
  if (const qualified_type_def* d = dynamic_cast<const qualified_type_def*>(t))
    return qualified_type_def_hash()(*d);
  if (const pointer_type_def* d = dynamic_cast<const pointer_type_def*>(t))
    return pointer_type_def_hash()(*d);
  if (const reference_type_def* d = dynamic_cast<const reference_type_def*>(t))
    return reference_type_def_hash()(*d);
  if (const enum_type_decl* d = dynamic_cast<const enum_type_decl*>(t))
    return enum_type_decl_hash()(*d);
  if (const typedef_decl* d = dynamic_cast<const typedef_decl*>(t))
    return typedef_decl_hash()(*d);

  // Poor man's fallback case.
  return type_base_hash()(*t);
}

//<pointer_type_def definitions>

pointer_type_def::pointer_type_def(shared_ptr<type_base>&	pointed_to,
				   size_t			size_in_bits,
				   size_t			align_in_bits,
				   location			locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus,
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    m_pointed_to_type(pointed_to)
{
}

/// Return true iff both instances of pointer_type_def are equal.
///
/// Note that this function does not check for the scopes of the this
/// types.
bool
pointer_type_def::operator==(const pointer_type_def& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return *get_pointed_to_type() == *other.get_pointed_to_type();
}

shared_ptr<type_base>
pointer_type_def::get_pointed_to_type() const
{
  return m_pointed_to_type;
}

pointer_type_def::~pointer_type_def()
{
}

// </pointer_type_def definitions>

// <reference_type_def definitions>

reference_type_def::reference_type_def(shared_ptr<type_base>&	pointed_to,
				       bool			lvalue,
				       size_t			size_in_bits,
				       size_t			align_in_bits,
				       location		locus)
  : type_base(size_in_bits, align_in_bits),
    decl_base("", locus,
	      dynamic_pointer_cast<decl_base>(pointed_to)->get_visibility()),
    m_pointed_to_type(pointed_to),
    m_is_lvalue(lvalue)
{
}

bool
reference_type_def::operator==(const reference_type_def& other) const
{
    // Runtime types must be equal.
  if (typeid(*this) != typeid(other))
    return false;

  return *get_pointed_to_type() == *other.get_pointed_to_type();
}

shared_ptr<type_base>
reference_type_def::get_pointed_to_type() const
{
  return m_pointed_to_type;
}

bool
reference_type_def::is_lvalue() const
{
  return m_is_lvalue;
}

reference_type_def::~reference_type_def()
{
}

// </reference_type_def definitions>

/// Constructor of an enum type declaration.
///
/// \param name the name of the enum
///
/// \param locus the locus at which the enum appears in the source
/// code.
///
/// \param underlying_type the underlying type of the enum
///
/// \param enumerators a list of enumerators for this enum.
enum_type_decl::enum_type_decl(const string&			name,
			       location			locus,
			       shared_ptr<type_base>		underlying_type,
			       const std::list<enumerator>&	enumerators,
			       visibility			vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, vis),
    m_underlying_type(underlying_type),
    m_enumerators(enumerators)
  {
  }

/// Return the underlying type of the enum.
shared_ptr<type_base>
enum_type_decl::get_underlying_type() const
{
  return m_underlying_type;
}

/// Return the list of enumerators of the enum.
const std::list<enum_type_decl::enumerator>&
enum_type_decl::get_enumerators() const
{
  return m_enumerators;
}

/// Destructor for the enum type declaration.
enum_type_decl::~enum_type_decl()
{
}

/// Equality operator.
///
/// \param other the other enum to test against.
///
/// \return true iff other is equals the current instance of enum type
/// decl.
bool
enum_type_decl::operator==(const enum_type_decl& other) const
{
  // Runtime types must be equal.
  if (typeid(*this) != typeid(other)
      || *get_underlying_type() != *other.get_underlying_type())
    return false;

  std::list<enumerator>::const_iterator i, j;
  for (i = get_enumerators().begin(), j = other.get_enumerators().begin();
       i != get_enumerators().end() && j != other.get_enumerators().end();
       ++i, ++j)
    if (*i != *j)
      return false;

  if (i != get_enumerators().end() || j != other.get_enumerators().end())
    return false;

  return true;
}

// <typedef_decl definitions>

/// Constructor of the typedef_decl type.
///
/// \param name the name of the typedef.
///
/// \param underlying_type the underlying type of the typedef.
///
/// \param locus the source location of the typedef declaration.
typedef_decl::typedef_decl(const string&		name,
			   const shared_ptr<type_base>	underlying_type,
			   location			locus,
			   visibility vis)
  : type_base(underlying_type->get_size_in_bits(),
	      underlying_type->get_alignment_in_bits()),
    decl_base(name, locus, vis),
    m_underlying_type(underlying_type)
{
}

/// Equality operator
///
/// \param other the other typedef_decl to test against.
bool
typedef_decl::operator==(const typedef_decl& other) const
{
  return (typeid(*this) == typeid(other)
	  && get_name() == other.get_name()
	  && *get_underlying_type() == *other.get_underlying_type());
}

/// Getter of the underlying type of the typedef.
///
/// \return the underlying_type.
shared_ptr<type_base>
typedef_decl::get_underlying_type() const
{
  return m_underlying_type;
}

/// Destructor of the typedef_decl.
typedef_decl::~typedef_decl()
{
}

// </typedef_decl definitions>
}//end namespace abigail
