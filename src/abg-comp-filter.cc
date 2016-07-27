// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2016 Red Hat, Inc.
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

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-comp-filter.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{
namespace comparison
{
namespace filtering
{

using std::tr1::dynamic_pointer_cast;

/// Walk the diff sub-trees of a a @ref corpus_diff and apply a filter
/// to the nodes visted.  The filter categorizes each node, assigning
/// it into one or several categories.
///
/// @param filter the filter to apply to the diff nodes
///
/// @param d the corpus diff to apply the filter to.
void
apply_filter(filter_base& filter, corpus_diff_sptr d)
{
  bool s = d->context()->visiting_a_node_twice_is_forbidden();
  d->context()->forbid_visiting_a_node_twice(false);
  d->traverse(filter);
  d->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a diff sub-tree and apply a filter to the nodes visted.  The
/// filter categorizes each node, assigning it into one or several
/// categories.
///
/// Note that this function makes sure to avoid visiting a node (or
/// any other node equivalent to it) more than once.  This helps avoid
/// infinite loops for diff trees that involve type changes that
/// reference themselves.
///
/// @param filter the filter to apply to the nodes of the sub-tree.
///
/// @param d the diff sub-tree to walk and apply the filter to.
void
apply_filter(filter_base& filter, diff_sptr d)
{
  bool s = d->context()->visiting_a_node_twice_is_forbidden();
  d->context()->forbid_visiting_a_node_twice(true);
  d->context()->forget_visited_diffs();
  d->traverse(filter);
  d->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a diff sub-tree and apply a filter to the nodes visted.  The
/// filter categorizes each node, assigning it into one or several
/// categories.
///
/// Note that this function makes sure to avoid visiting a node (or
/// any other node equivalent to it) more than once.  This helps avoid
/// infinite loops for diff trees that involve type changes that
/// reference themselves.
///
/// @param filter the filter to apply to the nodes of the sub-tree.
///
/// @param d the diff sub-tree to walk and apply the filter to.
void
apply_filter(filter_base_sptr filter, diff_sptr d)
{apply_filter(*filter, d);}

/// Test if there is a class that is declaration-only among the two
/// classes in parameter.
///
/// @param class1 the first class to consider.
///
/// @param class2 the second class to consider.
///
/// @return true if either classes are declaration-only, false
/// otherwise.
static bool
there_is_a_decl_only_class(const class_decl_sptr& class1,
			   const class_decl_sptr& class2)
{
  if ((class1 && class1->get_is_declaration_only())
      || (class2 && class2->get_is_declaration_only()))
    return true;
  return false;
}

/// Test if the diff involves a declaration-only class.
///
/// @param diff the class diff to consider.
///
/// @return true iff the diff involves a declaration-only class.
static bool
diff_involves_decl_only_class(const class_diff* diff)
{
  if (diff && there_is_a_decl_only_class(diff->first_class_decl(),
					 diff->second_class_decl()))
    return true;
  return false;
}

/// Tests if the size of a given type changed.
///
/// @param f the first version of the type to consider.
///
/// @param s the second version of the type to consider.
///
/// @return true if the type size changed, false otherwise.
static bool
type_size_changed(const type_base_sptr f, const type_base_sptr s)
{
  if (!f || !s
      || f->get_size_in_bits() == 0
      || s->get_size_in_bits() == 0
      || there_is_a_decl_only_class(is_compatible_with_class_type(f),
				    is_compatible_with_class_type(s)))
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
type_size_changed(const decl_base_sptr f, const decl_base_sptr s)
{return type_size_changed(is_type(f), is_type(s));}

/// Test if a given type diff node carries a type size change.
///
/// @param diff the diff tree node to test.
///
/// @return true if @p diff carries a type size change.
static bool
has_type_size_change(const diff* diff)
{
  if (!diff)
    return false;

  type_base_sptr f = is_type(diff->first_subject()),
    s = is_type(diff->second_subject());

  if (!f || !s)
    return false;

  return type_size_changed(f, s);
}
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

/// Test if there was a function name change, but there there was no
/// change in name of the underlying symbol.  IOW, if the name of a
/// function changed, but the symbol of the new function is equal to
/// the symbol of the old one, or is equal to an alians of the symbol
/// of the old function.
///
/// @param f the first function to consider.
///
/// @param s the second function to consider.
///
/// @return true if the test is positive, false otherwise.
static bool
function_name_changed_but_not_symbol(const function_decl_sptr f,
				     const function_decl_sptr s)
{
  if (!f || !s)
    return false;
  string fn = f->get_qualified_name(),
    sn = s->get_qualified_name();

  if (fn != sn)
    {
      elf_symbol_sptr fs = f->get_symbol(), ss = s->get_symbol();
      if (fs == ss)
	return true;
      for (elf_symbol_sptr s = fs->get_next_alias();
	   s && !s->is_main_symbol();
	   s = s->get_next_alias())
	if (*s == *ss)
	  return true;
    }
  return false;
}

/// Test if the current diff tree node carries a function name change,
/// in which there there was no change in the name of the underlying
/// symbol.  IOW, if the name of a function changed, but the symbol of
/// the new function is equal to the symbol of the old one, or is
/// equal to an alians of the symbol of the old function.
///
/// @param diff the diff tree node to consider.
///
/// @return true if the test is positive, false otherwise.
static bool
function_name_changed_but_not_symbol(const diff* diff)
{
  if (const function_decl_diff* d =
      dynamic_cast<const function_decl_diff*>(diff))
    return function_name_changed_but_not_symbol(d->first_function_decl(),
						d->second_function_decl());
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

/// Test if the size of a non-static data member changed accross two
/// versions.
///
/// @param f the first version of the non-static data member.
///
/// @param s the second version of the non-static data member.
static bool
non_static_data_member_type_size_changed(decl_base_sptr f, decl_base_sptr s)
{
  if (!is_member_decl(f)
      || !is_member_decl(s))
    return false;

  var_decl_sptr fv = dynamic_pointer_cast<var_decl>(f),
    sv = dynamic_pointer_cast<var_decl>(s);
  if (!fv
      || !sv
      || get_member_is_static(fv)
      || get_member_is_static(sv))
    return false;

  return type_size_changed(fv->get_type(), sv->get_type());
}

/// Test if the size of a static data member changed accross two
/// versions.
///
/// @param f the first version of the static data member.
///
/// @param s the second version of the static data member.
static bool
static_data_member_type_size_changed(decl_base_sptr f, decl_base_sptr s)
{
  if (!is_member_decl(f)
      || !is_member_decl(s))
    return false;

  var_decl_sptr fv = dynamic_pointer_cast<var_decl>(f),
    sv = dynamic_pointer_cast<var_decl>(s);
  if (!fv
      || !sv
      || !get_member_is_static(fv)
      || !get_member_is_static(sv))
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
  if ((d1 && d2)
      && (d1 != d2)
      && types_are_compatible(d1, d2))
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
    d1_name = d1->get_qualified_name();
  if (d2)
    d2_name = d2->get_qualified_name();

  return d1_name != d2_name;
}

/// Test if two decls represents a harmless name change.
///
/// For now, a harmless name change is a name change for a typedef,
/// enum or data member.
///
/// @param f the first decl to consider in the comparison.
///
/// @param s the second decl to consider in the comparison.
///
/// @return true iff decl @p s represents a harmless change over @p f.
bool
has_harmless_name_change(decl_base_sptr f, decl_base_sptr s)
{
  return (decl_name_changed(f, s)
	  && ((is_typedef(f) && is_typedef(s))
	      || (is_data_member(f) && is_data_member(s))
	      || (is_enum_type(f) && is_enum_type(s))));
}

/// Test if a class_diff node has non-static members added or
/// removed.
///
/// @param diff the diff node to consider.
///
/// @return true iff the class_diff node has non-static members added
/// or removed.
static bool
non_static_data_member_added_or_removed(const class_diff* diff)
{
  if (diff && !diff_involves_decl_only_class(diff))
    {
      for (string_decl_base_sptr_map::const_iterator i =
	     diff->inserted_data_members().begin();
	   i != diff->inserted_data_members().end();
	   ++i)
	if (!get_member_is_static(i->second))
	  return true;

      for (string_decl_base_sptr_map::const_iterator i =
	     diff->deleted_data_members().begin();
	   i != diff->deleted_data_members().end();
	   ++i)
	if (!get_member_is_static(i->second))
	  return true;
    }

  return false;
}

/// Test if a class_diff node has members added or removed.
///
/// @param diff the diff node to consider.
///
/// @return true iff the class_diff node has members added or removed.
static bool
non_static_data_member_added_or_removed(const diff* diff)
{
  return non_static_data_member_added_or_removed
    (dynamic_cast<const class_diff*>(diff));
}

/// Test if a class_diff node has static members added or removed.
///
/// @param diff the diff node to consider.
///
/// @return true iff the class_diff node has static members added
/// or removed.
static bool
static_data_member_added_or_removed(const class_diff* diff)
{
  if (diff && !diff_involves_decl_only_class(diff))
    {
      for (string_decl_base_sptr_map::const_iterator i =
	     diff->inserted_data_members().begin();
	   i != diff->inserted_data_members().end();
	   ++i)
	if (get_member_is_static(i->second))
	  return true;

      for (string_decl_base_sptr_map::const_iterator i =
	     diff->deleted_data_members().begin();
	   i != diff->deleted_data_members().end();
	   ++i)
	if (get_member_is_static(i->second))
	  return true;
    }

  return false;
}

/// Test if a class_diff node has a harmless "One Definition Rule"
/// violation that will cause a diagnostic rule.
///
/// The conditions this function looks for are:
///
///  1/ The two subject of the diff must be canonically different
///
///  2/ The two subjects of the diff must be structurally equal
///
///  3/ The canonical types of the subjects of the diff must be
///  structurally different.
///
/// These conditions makes the diff node appears as it carries changes
/// (because of a ODR glitch present in the binary), but the glitch
/// has no effect on the structural equality of the subjects of the
/// diff.  If we do not detect these conditions, we'd end up with a
/// diagnostic glitch where the reporter thinks there is an ABI change
/// (because of the canonical difference), but then it fails to give
/// any detail about it, because there is no structural change.
///
/// @param diff the diff node to consider.
///
/// @return true iff the the diff node has a harmless "One Definition
/// Rule" violation that cause an empty false positive.
static bool
class_diff_has_harmless_odr_violation_change(const diff* dif)
{
  class_diff* d =  dynamic_cast<class_diff*>(const_cast<diff*>(dif));
  if (!d || !d->has_changes())
    return false;

  class_decl_sptr first = d->first_class_decl();
  class_decl_sptr second = d->second_class_decl();

  if (equals(*first, *second, 0))
    {
      class_decl_sptr fc = is_class_type(first->get_canonical_type());
      class_decl_sptr sc = is_class_type(second->get_canonical_type());

      if (!equals(*fc, *sc, 0))
	return true;
    }

  return false;
}

/// Test if a class_diff node has static members added or
/// removed.
///
/// @param diff the diff node to consider.
///
/// @return true iff the class_diff node has static members added
/// or removed.
static bool
static_data_member_added_or_removed(const diff* diff)
{
  return static_data_member_added_or_removed
    (dynamic_cast<const class_diff*>(diff));
}

/// Test if the class_diff node has a change involving virtual member
/// functions.
///
/// That means whether there is an added, removed or changed virtual
/// member function.
///
/// @param diff the class_diff node to consider.
///
/// @return true iff the class_diff node contains changes involving
/// virtual member functions.
static bool
has_virtual_mem_fn_change(const class_diff* diff)
{
  if (!diff || diff_involves_decl_only_class(diff))
    return false;

  for (string_member_function_sptr_map::const_iterator i =
	 diff->deleted_member_fns().begin();
       i != diff->deleted_member_fns().end();
       ++i)
    {
      if (get_member_function_is_virtual(i->second))
	{
	  // Do not consider a virtual function that got deleted from
	  // an offset and re-inserted at the same offset as a
	  // "virtual member function change".
	  string_member_function_sptr_map::const_iterator j =
	    diff->inserted_member_fns().find(i->first);
	  if (j != diff->inserted_member_fns().end()
	      && (get_member_function_vtable_offset(i->second)
		  == get_member_function_vtable_offset(j->second)))
	    continue;

	  return true;
	}
    }

  for (string_member_function_sptr_map::const_iterator i =
	 diff->inserted_member_fns().begin();
       i != diff->inserted_member_fns().end();
       ++i)
    {
      if (get_member_function_is_virtual(i->second))
	{
	  // Do not consider a virtual function that got deleted from
	  // an offset and re-inserted at the same offset as a
	  // "virtual member function change".
	  string_member_function_sptr_map::const_iterator j =
	    diff->deleted_member_fns().find(i->first);
	  if (j != diff->deleted_member_fns().end()
	      && (get_member_function_vtable_offset(i->second)
		  == get_member_function_vtable_offset(j->second)))
	    continue;

	  return true;
	}
    }

  for (function_decl_diff_sptrs_type::const_iterator i =
	 diff->changed_member_fns().begin();
       i != diff->changed_member_fns().end();
       ++i)
    if (get_member_function_is_virtual((*i)->first_function_decl())
	|| get_member_function_is_virtual((*i)->second_function_decl()))
      {
	if (get_member_function_vtable_offset((*i)->first_function_decl())
	    == get_member_function_vtable_offset((*i)->second_function_decl()))
	  continue;

	return true;
      }

  return false;
}

/// Test if the function_decl_diff node has a change involving virtual
/// member functions.
///
/// That means whether there is an added, removed or changed virtual
/// member function.
///
/// @param diff the function_decl_diff node to consider.
///
/// @return true iff the function_decl_diff node contains changes
/// involving virtual member functions.
bool
has_virtual_mem_fn_change(const function_decl_diff* diff)
{
  if (!diff)
    return false;

  function_decl_sptr ff = diff->first_function_decl(),
    sf = diff->second_function_decl();

  if (!is_member_function(ff)
      || !is_member_function(sf))
    return false;

  bool ff_is_virtual = get_member_function_is_virtual(ff),
    sf_is_virtual = get_member_function_is_virtual(sf);

  if (ff_is_virtual != sf_is_virtual)
    return true;

  size_t ff_vtable_offset = get_member_function_vtable_offset(ff),
    sf_vtable_offset = get_member_function_vtable_offset(sf);

  if (ff_vtable_offset != sf_vtable_offset)
    return true;

  return false;
}

/// Test if the class_diff node has a change involving virtual member
/// functions.
///
/// That means whether there is an added, removed or changed virtual
/// member function.
///
/// @param diff the class_diff node to consider.
///
/// @return true iff the class_diff node contains changes involving
/// virtual member functions.
static bool
has_virtual_mem_fn_change(const diff* diff)
{
  return (has_virtual_mem_fn_change(dynamic_cast<const class_diff*>(diff))
	  || has_virtual_mem_fn_change(dynamic_cast<const function_decl_diff*>(diff)));
}

/// Test if the class_diff has changes to non virtual member
/// functions.
///
///@param diff the class_diff nod e to consider.
///
/// @retrurn iff the class_diff node has changes to non virtual member
/// functions.
static bool
has_non_virtual_mem_fn_change(const class_diff* diff)
{
  if (!diff || diff_involves_decl_only_class(diff))
    return false;

  for (string_member_function_sptr_map::const_iterator i =
	 diff->deleted_member_fns().begin();
       i != diff->deleted_member_fns().end();
       ++i)
    if (!get_member_function_is_virtual(i->second))
      return true;

  for (string_member_function_sptr_map::const_iterator i =
	 diff->inserted_member_fns().begin();
       i != diff->inserted_member_fns().end();
       ++i)
    if (!get_member_function_is_virtual(i->second))
      return true;

  for (function_decl_diff_sptrs_type::const_iterator i =
	 diff->changed_member_fns().begin();
       i != diff->changed_member_fns().end();
       ++i)
    if(!get_member_function_is_virtual((*i)->first_function_decl())
       && !get_member_function_is_virtual((*i)->second_function_decl()))
      return true;

  return false;
}

/// Test if the class_diff has changes to non virtual member
/// functions.
///
///@param diff the class_diff nod e to consider.
///
/// @retrurn iff the class_diff node has changes to non virtual member
/// functions.
static bool
has_non_virtual_mem_fn_change(const diff* diff)
{return has_non_virtual_mem_fn_change(dynamic_cast<const class_diff*>(diff));}

/// Test if a class_diff carries base classes adding or removals.
///
/// @param diff the class_diff to consider.
///
/// @return true iff @p diff carries base classes adding or removals.
static bool
base_classes_added_or_removed(const class_diff* diff)
{
  if (!diff)
    return false;
  return diff->deleted_bases().size() || diff->inserted_bases().size();
}

/// Test if a class_diff carries base classes adding or removals.
///
/// @param diff the class_diff to consider.
///
/// @return true iff @p diff carries base classes adding or removals.
static bool
base_classes_added_or_removed(const diff* diff)
{return base_classes_added_or_removed(dynamic_cast<const class_diff*>(diff));}

/// Test if an enum_diff carries an enumerator insertion.
///
/// @param diff the enum_diff to consider.
///
/// @return true iff @p diff carries an enumerator insertion.
static bool
has_enumerator_insertion(const diff* diff)
{
  if (const enum_diff* d = dynamic_cast<const enum_diff*>(diff))
    return !d->inserted_enumerators().empty();
  return false;
}

/// Test if an enum_diff carries an enumerator removal.
///
/// @param diff the enum_diff to consider.
///
/// @return true iff @p diff carries an enumerator removal or change.
static bool
has_enumerator_removal_or_change(const diff* diff)
{
  if (const enum_diff* d = dynamic_cast<const enum_diff*>(diff))
    return (!d->deleted_enumerators().empty()
	    || !d->changed_enumerators().empty());
  return false;
}

/// Test if an enum_diff carries a harmful change.
///
/// @param diff the enum_diff to consider.
///
/// @return true iff @p diff carries a harmful change.
static bool
has_harmful_enum_change(const diff* diff)
{
  if (const enum_diff* d = dynamic_cast<const enum_diff*>(diff))
    return (has_enumerator_removal_or_change(d)
	    || has_type_size_change(d));
  return false;
}

/// The visiting code of the harmless_filter.
///
/// Detect if the changes carried by a given diff node are deemed
/// harmless and do categorize the diff node accordingly.
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
  if (!d->has_changes())
    return true;

  diff_category category = NO_CHANGE_CATEGORY;

  if (pre)
    {
      decl_base_sptr f = is_decl(d->first_subject()),
	s = is_decl(d->second_subject());

      if (access_changed(f, s))
	category |= ACCESS_CHANGE_CATEGORY;

      if (is_compatible_change(f, s))
	category |= COMPATIBLE_TYPE_CHANGE_CATEGORY;

      if (has_harmless_name_change(f, s)
	  || class_diff_has_harmless_odr_violation_change(d))
	category |= HARMLESS_DECL_NAME_CHANGE_CATEGORY;

      if (has_non_virtual_mem_fn_change(d))
	category |= NON_VIRT_MEM_FUN_CHANGE_CATEGORY;

      if (static_data_member_added_or_removed(d)
	  || static_data_member_type_size_changed(f, s))
	category |= STATIC_DATA_MEMBER_CHANGE_CATEGORY;

      if (has_enumerator_insertion(d)
	  && !has_harmful_enum_change(d))
	category |= HARMLESS_ENUM_CHANGE_CATEGORY;

      if (function_name_changed_but_not_symbol(d))
	category |= HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY;

      if (category)
	{
	  d->add_to_local_and_inherited_categories(category);
	  // Also update the category of the canonical node.
	  if (diff * canonical = d->get_canonical_diff())
	    canonical->add_to_local_and_inherited_categories(category);
	}
    }

  return true;
}

/// Part of the visiting code of the harmless_filter.
///
/// This function is called after the visiting of a given diff node.
/// Note that when this function is called, the visiting might not
/// have taken place *if* the node (or an equivalent node) has already
/// been visited.
///
/// @param d the diff node that has either been visited or skipped
/// (because it has already been visited during this traversing).
void
harmless_filter::visit_end(diff* d)
{
  if (d->context()->diff_has_been_visited(d))
    {
      // This node or one of its equivalent node has already been
      // visited.  That means at this moment, harmless_filter::visit()
      // has *not* been called prior to this
      // harmless_filter::visit_end() is called.  In other words, only
      // harmless_filter::visit_begin() and
      // harmless_filter::visit_end() are called.
      //
      // So let's update the category of this diff node from its
      // canonical node.
      diff* canonical = d->get_canonical_diff();
      if (canonical)
	d->add_to_local_and_inherited_categories(canonical->get_local_category());
    }
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

  if (!d->has_changes())
    return true;

  if (pre)
    {
      decl_base_sptr f = is_decl(d->first_subject()),
	s = is_decl(d->second_subject());

      // Detect size or offset changes as well as data member addition
      // or removal.
      //
      // TODO: be more specific -- not all size changes are harmful.
      if (type_size_changed(f, s)
	  || data_member_offset_changed(f, s)
	  || non_static_data_member_type_size_changed(f, s)
	  || non_static_data_member_added_or_removed(d)
	  || base_classes_added_or_removed(d)
	  || has_harmful_enum_change(d))
	category |= SIZE_OR_OFFSET_CHANGE_CATEGORY;

      if (has_virtual_mem_fn_change(d))
	category |= VIRTUAL_MEMBER_CHANGE_CATEGORY;

      if (category)
	{
	  d->add_to_local_and_inherited_categories(category);
	  // Update the category of the canonical diff node too.
	  if (diff * canonical = d->get_canonical_diff())
	    canonical->add_to_local_and_inherited_categories(category);
	}
    }

  return true;
}

/// Part of the visiting code of the harmless_filter.
///
/// This function is called after the visiting of a given diff node.
/// Note that when this function is called, the visiting might not
/// have taken place *if* the node (or an equivalent node) has already
/// been visited.
///
/// @param d the diff node that has either been visited or skipped
/// (because it has already been visited during this traversing).
void
harmful_filter::visit_end(diff* d)
{
  if (d->context()->diff_has_been_visited(d))
    {
      // This node or one of its equivalent node has already been
      // visited.  That means at this moment, harmful_filter::visit()
      // has *not* been called prior to this
      // harmful_filter::visit_end() is called.  In other words, only
      // harmful_filter::visit_begin() and harmful_filter::visit_end()
      // are called.
      //
      // So let's update the category of this diff node from it's
      // cnanonical node.
      diff* canonical = d->get_canonical_diff();
      if (canonical)
	d->add_to_local_and_inherited_categories(canonical->get_local_category());
    }
}

} // end namespace filtering
} // end namespace comparison
} // end namespace abigail
