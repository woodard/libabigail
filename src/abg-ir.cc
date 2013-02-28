#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>

#include "abg-ir.h"

using std::string;

namespace abigail
{

location::location()
  : m_value(0)
{
}

location::location (unsigned value)
  : m_value(value)
{
}

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

  unsigned opaque = 0;
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
				  unsigned&		column)
{
  expanded_location &l = m_priv->locs[location.m_value];
  path = l.m_path;
  line = l.m_line;
  column = l.m_column;
}

}//end namespace abigail
