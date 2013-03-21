#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <typeinfo>

#include "abg-ir.h"
#include "abg-hash.h"

using std::string;

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
  : m_kind(KIND_DECL)
{
}

decl_base::decl_base(const std::string&	name,
		     shared_ptr<scope_decl>	context,
		     location			locus)
  :m_kind(KIND_DECL),
   m_location(locus),
   m_name(name),
   m_context(context)
{
  if (m_context)
    m_context->add_member_decl(shared_ptr<decl_base>(this));
}

decl_base::decl_base(kind				what_kind,
		     const std::string&		name,
		     const shared_ptr<scope_decl>	context,
		     location				locus)
  : m_kind(what_kind),
    m_location(locus),
    m_name(name),
    m_context(context)
{
    if (m_context)
      m_context->add_member_decl(shared_ptr<decl_base>(this));
}

decl_base::decl_base(location l)
  :m_kind(KIND_DECL),
   m_location(l)
{
}

decl_base::decl_base(const decl_base& d)
{
  m_kind = d.m_kind;
  m_location = d.m_location;
  m_name = d.m_name;
  m_context = d.m_context;
}

enum decl_base::kind
decl_base::what_kind () const
{
  return m_kind;
}

decl_base::~decl_base()
{
}

// </Decl definition>

// <scope_decl definitions>
scope_decl::scope_decl(const std::string&		name,
		       const shared_ptr<scope_decl>	context,
		       location			locus)
  : decl_base(KIND_SCOPE_DECL, name, context, locus)
{
}

scope_decl::scope_decl(kind				akind,
		       const std::string&		name,
		       const shared_ptr<scope_decl>	context,
		       location			locus)
  : decl_base(akind, name, context, locus)
{
}

scope_decl::scope_decl(location l)
  : decl_base(KIND_SCOPE_DECL, "", shared_ptr<scope_decl>(), l)
{
}

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

// </scope_decl definition>

// <type_base definitions>
type_base::type_base(size_t s = 8, size_t a = 8)
  : m_size_in_bits(s),
    m_alignment_in_bits(a)
{
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
		     shared_ptr<scope_decl>	context,
		     location			locus)
  : decl_base(KIND_TYPE_DECL, name, context, locus),
    type_base(size_in_bits, alignment_in_bits)
{
}

type_decl::type_decl(kind			akind,
		     const std::string&	name,
		     size_t			size_in_bits,
		     size_t			alignment_in_bits,
		     shared_ptr<scope_decl>	context,
		     location			locus)
  :    decl_base(akind, name, context, locus),
       type_base(size_in_bits, alignment_in_bits)
{
}

type_decl::~type_decl()
{
}

//</type_decl definitions>

// <scope_type_decl definitions>

scope_type_decl::scope_type_decl(const std::string&		name,
				 size_t			size_in_bits,
				 size_t			alignment_in_bits,
				 const shared_ptr<scope_decl>	context,
				 location			locus)
  :scope_decl(KIND_SCOPE_TYPE_DECL, name, context, locus),
   type_base(size_in_bits, alignment_in_bits)
{
}

scope_type_decl::scope_type_decl(kind				akind,
				 const std::string&		name,
				 size_t			size_in_bits,
				 size_t			alignment_in_bits,
				 const shared_ptr<scope_decl>	context,
				 location			locus)
  : scope_decl(akind, name, context, locus),
    type_base(size_in_bits, alignment_in_bits)
{
}

scope_type_decl::~scope_type_decl()
{
}

// </scope_type_decl definitions>

// <namespace_decl>
namespace_decl::namespace_decl(const std::string& name,
			       const shared_ptr<namespace_decl> context,
			       location locus)
  : scope_decl(KIND_NAMESPACE_DECL, name, context, locus)
{
}

namespace_decl::~namespace_decl()
{
}

// </namespace_decl>

}//end namespace abigail
