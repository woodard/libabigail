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

using std::tr1::dynamic_pointer_cast;

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

/// Tests if the size of a given type changed.
///
/// @param f the first version of the type to consider.
///
/// @param s the second version of the type to consider.
///
/// @return true if the type size changed, false otherwise.
static bool
type_size_changed(type_base_sptr f, type_base_sptr s)
{
  if (!f || !s)
    return false;

  return f->get_size_in_bits() != s->get_size_in_bits();
}

/// Tests if the size of a given type changed.
///
/// @param f the declaration of the first version of the type to
/// consider.
///
/// @param s the declaration of the second version of the type to
/// consider.
///
/// @return true if the type size changed, false otherwise.
static bool
type_size_changed(decl_base_sptr f, decl_base_sptr s)
{return type_size_changed(is_type(f), is_type(s));}

/// Tests if the access specifiers for a member declaration changed.
///
/// @param f the declaration for the first version of the member
/// declaration to consider.
///
/// @param s the declaration for the second version of the member
/// delcaration to consider.
///
/// @return true iff the access specifier changed.
static bool
access_changed(decl_base_sptr f, decl_base_sptr s)
{
  if (!is_member_decl(f)
      || !is_member_decl(s))
    return false;

  access_specifier fa = get_member_access_specifier(f),
    sa = get_member_access_specifier(s);

  if (sa != fa)
    return true;

  return false;
}

/// Tests if the offset of a given data member changed.
///
/// @param f the declaration for the first version of the data member to
/// consider.
///
/// @param s the declaration for the second version of the data member
/// to consider.
///
/// @return true iff the offset of the data member changed.
static bool
data_member_offset_changed(decl_base_sptr f, decl_base_sptr s)
{
  if (!is_member_decl(f)
      || !is_member_decl(s))
    return false;

  var_decl_sptr v0 = dynamic_pointer_cast<var_decl>(f),
    v1 = dynamic_pointer_cast<var_decl>(s);
  if (!v0 || !v1)
    return false;

  if (get_data_member_offset(v0) != get_data_member_offset(v1))
    return true;

  return false;
}

/// Test if the size of a data member changed accross two versions.
///
/// @param f the first version of the data member.
///
/// @param s the second version of the data member.
static bool
data_member_type_size_changed(decl_base_sptr f, decl_base_sptr s)
{
  if (!is_member_decl(f)
      || !is_member_decl(s))
    return false;

  var_decl_sptr fv = dynamic_pointer_cast<var_decl>(f),
    sv = dynamic_pointer_cast<var_decl>(s);
  if (!fv || !sv)
    return false;

  return type_size_changed(fv->get_type(), sv->get_type());
}

/// Test if two types are different but compatible.
///
/// @param d1 the declaration of the first type to consider.
///
/// @param d2 the declaration of the second type to consider.
///
/// @return true if d1 and d2 are different but compatible.
static bool
is_compatible_change(decl_base_sptr d1, decl_base_sptr d2)
{
  if (is_typedef(d1) || is_typedef(d2))
    if ((d1 != d2) && types_are_compatible(d1, d2))
      return true;

  return false;
}

/// Test if two decls have different names.
///
/// @param d1 the first declaration to consider.
///
/// @param d2 the second declaration to consider.
///
/// @return true if d1 and d2 have different names.
static bool
decl_name_changed(decl_base_sptr d1, decl_base_sptr d2)
{
  string d1_name, d2_name;

  if (d1)
    d1_name = d1->get_name();
  if (d2)
    d2_name = d2->get_name();

  return d1_name != d2_name;
}

static bool
data_member_added_or_removed(const class_diff* diff)
{return (diff && (diff->inserted_data_members().size()
		  || diff->deleted_data_members().size()));}

static bool
data_member_added_or_removed(const diff* diff)
{return data_member_added_or_removed(dynamic_cast<const class_diff*>(diff));}

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
  diff_category category = NO_CHANGE_CATEGORY;

  if (pre)
    {
      decl_base_sptr f = d->first_subject(),
	s = d->second_subject();

      if (access_changed(f, s))
	category |= ACCESS_CHANGE_CATEGORY;

      if (is_compatible_change(f, s))
	category |= COMPATIBLE_TYPE_CHANGE_CATEGORY;

      if (decl_name_changed(f, s))
	category |= DECL_NAME_CHANGE_CATEGORY;

      if (category)
	d->add_to_category(category);
    }

  // Propagate the categorization to the parent nodes.
  if (d->get_parent())
    d->get_parent()->add_to_category(d->get_category());

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
  diff_category category = NO_CHANGE_CATEGORY;

  if (pre)
    {
      decl_base_sptr f = d->first_subject(),
	s = d->second_subject();

      // Detect size or offset changes.  For now, all of them are
      // considered harmful.
      //
      // TODO: be more specific -- not all size changes are harmful.
      if (type_size_changed(f, s)
	  || data_member_offset_changed(f, s))
	{
	  class_decl_sptr cl1 = is_class_type(f),
	    cl2 = is_class_type(s);
	  if ((cl1 && cl1->get_is_declaration_only())
	      || (cl2 && cl2->get_is_declaration_only()))
	    // But do not compare a declaration-only class to another
	    // one for size changes.
	    ;
	  else
	    category |= SIZE_OR_OFFSET_CHANGE_CATEGORY;
	}

      /// If a data member got added or remove, consider it as a "size
      /// of offset change" as well.
      if (data_member_added_or_removed(d))
	category |= SIZE_OR_OFFSET_CHANGE_CATEGORY;

      if (data_member_type_size_changed(f, s))
	category |= SIZE_OR_OFFSET_CHANGE_CATEGORY;

      if (category)
	d->add_to_category(category);
    }

  // Propagate the categorization to the parent nodes.
  if (d->get_parent())
    d->get_parent()->add_to_category(d->get_category());

  return true;
}

} // end namespace filtering
} // end namespace comparison
} // end namespace abigail
