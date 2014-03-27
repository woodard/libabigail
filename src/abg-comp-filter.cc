// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains definitions of diff objects filtering
/// facilities.

#include "abg-comp-filter.h"

namespace abigail
{
namespace comparison
{
namespace filtering
{

/// Walk and categorize the nodes of a diff sub-tree.
///
/// @param filter the filter invoked on each node of the walked
/// sub-tree.
///
/// @param d the diff sub-tree node to start the walk from.
void
apply_filter(filter_base& filter, diff_sptr d)
{
  filter.set_visiting_kind(PRE_VISITING_KIND | POST_VISITING_KIND);
  d->traverse(filter);
}

/// Walk and categorize the nodes of a diff sub-tree.
///
/// @param filter the filter invoked on each node of the walked
/// sub-tree.
///
/// @param d the diff sub-tree node to start the walk from.
void
apply_filter(filter_base_sptr filter, diff_sptr d)
{apply_filter(*filter, d);}

/// The visiting code of the harmless_filter.
///
/// @param d the diff node being visited.
///
/// @param pre this is true iff the node is being visited *before* the
/// children nodes of @p d.
///
/// @return true iff the traversal shall keep going after the
/// completion of this function.
bool
harmless_filter::visit(diff* d, bool pre)
{
  if (pre)
    {
      decl_base_sptr f = d->first_subject(),
	s = d->second_subject();

      if (!is_member_decl(f)
	  && !is_member_decl(s))
	return true;

      access_specifier fa = get_member_access_specifier(f),
	sa = get_member_access_specifier(s);

      if (sa != fa)
	d->add_to_category(ACCESS_CHANGED_CATEGORY);
      // Add non-virtual member function deletions and changes due to
      // details that are being reported or got reported earlier.
    }

  // Propagate the categorization to the parent nodes.
  if (d->get_parent())
    d->get_parent()->add_to_category(d->get_category()
				     & ACCESS_CHANGED_CATEGORY);

  return true;
}

/// The visiting code of the harmful_filter.
///
/// @param d the diff node being visited.
///
/// @param pre this is true iff the node is being visited *before* the
/// children nodes of @p d.
///
/// @return true iff the traversal shall keep going after the
/// completion of this function.
bool
harmful_filter::visit(diff* d, bool pre)
{
  if (pre)
    {
      decl_base_sptr f = d->first_subject(),
	s = d->second_subject();
      bool size_changed = false;
      type_base_sptr tf, ts;

      if (is_type(f) && is_type(s))
	{
	  tf= is_type(f), ts = is_type(s);
	  if (tf->get_size_in_bits() != ts->get_size_in_bits())
	    size_changed = true;
	}

      if (size_changed)
	d->add_to_category(SIZE_CHANGED_CATEGORY);
    }

  // Propagate the categorization to the parent nodes.
  if (d->get_parent())
    d->get_parent()->add_to_category(d->get_category() & SIZE_CHANGED_CATEGORY);

  return true;
}

} // end namespace filtering
} // end namespace comparison
} // end namespace abigail
