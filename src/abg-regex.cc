// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2020 Red Hat, Inc.
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

/// @file
///
/// Some specialization for shared pointer utility templates.
///

#include <sstream>

#include "abg-regex.h"
#include "abg-sptr-utils.h"

namespace abigail
{

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>(regex_t *p)
{return regex::regex_t_sptr(p, regex::regex_t_deleter());}

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>()
{return sptr_utils::build_sptr(new regex_t);}

namespace regex
{

/// Generate a regex pattern equivalent to testing set membership.
///
/// A string will match the resulting pattern regex, if and only if it
/// was present in the vector.
///
/// @param strs a vector of strings
///
/// @return a regex pattern
std::string
generate_from_strings(const std::vector<std::string>& strs)
{
  if (strs.empty())
    // This cute-looking regex does not match any string.
    return "^_^";
  std::ostringstream os;
  std::vector<std::string>::const_iterator i = strs.begin();
  os << "^(" << *i++;
  while (i != strs.end())
    os << "|" << *i++;
  os << ")$";
  return os.str();
}

}//end namespace regex

}//end namespace abigail
