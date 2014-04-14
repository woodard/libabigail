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

#include "abg-hash.h"
#include "abg-comparison.h"
#include "abg-comp-filter.h"

namespace abigail
{

namespace comparison
{

// Inject types from outside in here.
using std::vector;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;

/// A deleter for shared pointers that ... doesn't delete the object
/// managed by the shared pointer.
struct noop_deleter
{
  template<typename T>
  void
  operator()(T*)
  {}
};

/// Convenience typedef for a pair of decls.
typedef std::pair<const decl_base_sptr, const decl_base_sptr> decls_type;

/// A hashing functor for @ef decls_type.
struct decls_hash
{
  size_t
  operator()(const decls_type& d) const
  {
    size_t h1 = d.first ? d.first->get_hash() : 0;
    size_t h2 = d.second ? d.second->get_hash() : 0;
    return hashing::combine_hashes(h1, h2);
  }
};

/// An equality functor for @ref decls_type.
struct decls_equal
{
  bool
  operator()(const decls_type d1, const decls_type d2) const
  {
    if (d1.first == d2.first && d1.second == d2.second)
      return true;

    if (!!d1.first != !!d2.first
	|| !!d1.second != !!d2.second)
      return false;

    return (*d1.first == *d2.first
	    && *d1.second == *d2.second);
  }
};

/// A convenience typedef for a map of @ref decls_type and diff_sptr.
typedef unordered_map<decls_type, diff_sptr, decls_hash, decls_equal>
decls_diff_map_type;

/// The overloaded or operator for @ref visiting_kind.
visiting_kind
operator|(visiting_kind l, visiting_kind r)
{return static_cast<visiting_kind>(static_cast<unsigned>(l)
				   | static_cast<unsigned>(r));}

#define TRY_PRE_VISIT(v)					\
  do {								\
    if (v.get_visiting_kind() & PRE_VISITING_KIND)		\
      if (!v.visit(this, /*pre=*/true))			\
	return false;						\
  } while (false)

#define TRY_PRE_VISIT_CLASS_DIFF(v)				\
  do {								\
    if (v.get_visiting_kind() & PRE_VISITING_KIND)		\
      if (!v.visit(this, /*pre=*/true))			\
	{							\
	  priv_->traversing_ = false;				\
	  return false;					\
	}							\
  } while (false)


#define TRY_POST_VISIT(v)					\
  do {								\
    if (v.get_visiting_kind() & POST_VISITING_KIND)		\
      if (!v.visit(this, /*pre=*/false))			\
	return false;						\
  } while (false)

#define TRY_POST_VISIT_CLASS_DIFF(v)				\
  do {								\
    if (v.get_visiting_kind() & POST_VISITING_KIND)		\
      if (!v.visit(this, /*pre=*/false))			\
	{							\
	  priv_->traversing_ = false;				\
	  return false;					\
	}							\
  } while (false)

/// Inside the code of a given diff node, traverse a sub-tree of the
/// diff nove and propagate the category of the sub-tree up to the
/// current diff node.
///
/// @param node the sub-tree node of the current diff node to consider.
///
/// @param visitor the visitor used to visit the traversed sub tree
/// nodes.
#define TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(node, visitor)	\
  do {									\
    bool r = node->traverse(visitor);					\
    add_to_category(node->get_category());				\
    if (!r)								\
      return false;							\
  } while (false)

/// Inside the code of a given diff node that is a member of a
/// class_diff node, traverse a sub-tree of the diff nove and
/// propagate the category of the sub-tree up to the current diff
/// node.
///
/// @param node the sub-tree node of the current diff node to consider.
///
/// @param visitor the visitor used to visit the traversed sub tree
/// nodes.
#define TRAVERSE_MEM_DIFF_NODE_AND_PROPAGATE_CATEGORY(node, visitor)	\
  do {									\
    bool r = node->traverse(visitor);					\
    add_to_category(node->get_category());				\
    if (!r)								\
      {								\
	priv_->traversing_ = false;					\
	return false;							\
      }								\
  } while (false)

/// Inside the code of a given diff node that is for member functions
/// of a class_diff node, traverse a sub-tree of the diff nove and
/// propagate the category of the sub-tree up to the current diff
/// node.
///
/// Note that for a diff for member functions, the only categories we
/// want to see propagated to the diff of the enclosing class are
/// categories about changes to vtables.
/// 
/// @param node the sub-tree node of the current diff node to consider.
///
/// @param visitor the visitor used to visit the traversed sub tree
/// nodes.
#define TRAVERSE_MEM_FN_DIFF_NODE_AND_PROPAGATE_CATEGORY(node, visitor)	\
  do {									\
    bool r = node->traverse(visitor);					\
    add_to_category(node->get_category()				\
		    & VIRTUAL_MEMBER_CHANGE_CATEGORY);			\
    if (!r)								\
      {								\
	priv_->traversing_ = false;					\
	return false;							\
      }								\
  } while (false)

/// Inside the traveral code of a diff node, if the node has been
/// traversed already, return immediately.  Otherwise, mark the
/// current node as a 'traversed' node.
#define ENSURE_DIFF_NODE_TRAVERSED_ONCE		\
  do {							\
    if (context()->diff_has_been_traversed(this))	\
      return true;					\
    context()->mark_diff_as_traversed(this);		\
  } while (false)

/// Inside the traveral code of a diff node that is member node of a
/// class_diff node, if the node has been traversed already, return
/// immediately.  Otherwise, mark the current node as a 'traversed'
/// node.
#define ENSURE_MEM_DIFF_NODE_TRAVERSED_ONCE		\
  do {							\
    if (context()->diff_has_been_traversed(node))	\
      {						\
	priv_->traversing_ = false;			\
	return true;					\
      }						\
    context()->mark_diff_as_traversed(node);		\
  } while (false)

/// The default traverse function.
///
/// @return true.
bool
diff_traversable_base::traverse(diff_node_visitor&)
{return true;}

/// The private member (pimpl) for @ref diff_context.
struct diff_context::priv
{
  diff_category			allowed_category_;
  decls_diff_map_type			decls_diff_map;
  vector<filtering::filter_base_sptr>	filters_;
  pointer_map				traversed_diff_nodes_;
  bool					show_stats_only_;
  bool					show_deleted_fns_;
  bool					show_changed_fns_;
  bool					show_added_fns_;
  bool					show_deleted_vars_;
  bool					show_changed_vars_;
  bool					show_added_vars_;

  priv()
    : allowed_category_(EVERYTHING_CATEGORY),
      show_stats_only_(false),
      show_deleted_fns_(true),
      show_changed_fns_(true),
      show_added_fns_(true),
      show_deleted_vars_(true),
      show_changed_vars_(true),
      show_added_vars_(true)
   {}
 };// end struct diff_context::priv

diff_context::diff_context()
  : priv_(new diff_context::priv)
{
  // Setup all the diff output filters we have.
  filtering::filter_base_sptr f;

  f.reset(new filtering::harmless_filter);
  add_diff_filter(f);

  f.reset(new filtering::harmful_filter);
  add_diff_filter(f);
}

/// Tests if the current diff context already has a diff for two decls.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for(const decl_base_sptr first,
			   const decl_base_sptr second) const
{
  decls_diff_map_type::const_iterator i =
    priv_->decls_diff_map.find(std::make_pair(first, second));
  if (i != priv_->decls_diff_map.end())
    return i->second;
  return diff_sptr();
}

/// Tests if the current diff context already has a diff for two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for_types(const type_base_sptr first,
				  const type_base_sptr second) const
{
  return has_diff_for(get_type_declaration(first),
		       get_type_declaration(second));
}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
const diff*
diff_context::has_diff_for(const diff* d) const
{return has_diff_for(d->first_subject(), d->second_subject()).get();}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
diff_sptr
diff_context::has_diff_for(const diff_sptr d) const
{return has_diff_for(d->first_subject(), d->second_subject());}

/// Getter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @return a bitmap that represents the set of categories that the
/// user wants to see reported.
diff_category
diff_context::get_allowed_category() const
{return priv_->allowed_category_;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::set_allowed_category(diff_category c)
{priv_->allowed_category_ = c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function perform a bitwise or between the new set of
/// categories and the current ones, and then sets the current
/// categories to the result of the or.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::switch_categories_on(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ | c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function actually unsets bits from the current categories.
///
/// @param c a bitmap that represents the set of categories to unset
/// from the current categories.
void
diff_context::switch_categories_off(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ & ~c;}

/// Add a diff for two decls to the cache of the current diff_context
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param the diff to add.
void
diff_context::add_diff(decl_base_sptr first,
			decl_base_sptr second,
			diff_sptr d)
{priv_->decls_diff_map[std::make_pair(first, second)] = d;}

/// Test if a diff node has been traversed.
///
/// @param d the diff node to consider.
bool
diff_context::diff_has_been_traversed(const diff* d) const
{
  const diff* canonical = has_diff_for(d);
  if (!canonical)
    canonical = d;

  size_t ptr_value = reinterpret_cast<uintptr_t>(canonical);
  return (priv_->traversed_diff_nodes_.find(ptr_value)
	  != priv_->traversed_diff_nodes_.end());
}

/// Mark a diff node as traversed by a traversing algorithm.
///
/// Subsequent invocations of diff_has_been_traversed() on the diff
/// node will yield true.
void
diff_context::mark_diff_as_traversed(const diff* d)
{
   const diff* canonical = has_diff_for(d);
   if (!canonical)
    canonical = d;

   size_t ptr_value = reinterpret_cast<uintptr_t>(canonical);
   priv_->traversed_diff_nodes_[ptr_value] = true;
}

/// Unmark all the diff nodes that were marked as being traversed.
void
diff_context::forget_traversed_diffs()
{priv_->traversed_diff_nodes_.clear();}

/// Getter for the diff tree nodes filters to apply to diff sub-trees.
///
/// @return the vector of tree filters to apply to diff sub-trees.
const filtering::filters&
diff_context::diff_filters() const
{return priv_->filters_;}

/// Setter for the diff filters to apply to a given diff sub-tree.
///
/// @param f the new diff filter to add to the vector of diff filters
/// to apply to diff sub-trees.
void
diff_context::add_diff_filter(filtering::filter_base_sptr f)
{priv_->filters_.push_back(f);}

/// Apply the diff filters to a given diff sub-tree.
///
/// If the current context is instructed to filter out some categories
/// then this function walks the given sub-tree and categorizes its
/// nodes by using the filters held by the context.
///
/// @param diff the diff sub-tree to apply the filters to.
void
diff_context::maybe_apply_filters(diff_sptr diff)
{
  if (get_allowed_category() == EVERYTHING_CATEGORY)
    return;

  for (filtering::filters::const_iterator i = diff_filters().begin();
       i != diff_filters().end();
       ++i)
    {
      diff->context()->forget_traversed_diffs();
      filtering::apply_filter(*i, diff);
    }
}

/// Set a flag saying if the comparison module should only show the
/// diff stats.
///
/// @param f the flag to set.
void
diff_context::show_stats_only(bool f)
{priv_->show_stats_only_ = f;}

/// Test if the comparison module should only show the diff stats.
///
/// @return true if the comparison module should only show the diff
/// stats, false otherwise.
bool
diff_context::show_stats_only() const
{return priv_->show_stats_only_;}

/// Set a flag saying to show the deleted functions.
///
/// @param f true to show deleted functions.
void
diff_context::show_deleted_fns(bool f)
{priv_->show_deleted_fns_ = f;}

/// @return true if we want to show the deleted functions, false
/// otherwise.
bool
diff_context::show_deleted_fns() const
{return priv_->show_deleted_fns_;}

/// Set a flag saying to show the changed functions.
///
/// @param f true to show the changed functions.
void
diff_context::show_changed_fns(bool f)
{priv_->show_changed_fns_ = f;}

/// @return true if we want to show the changed functions, false otherwise.
bool
diff_context::show_changed_fns() const
{return priv_->show_changed_fns_;}

/// Set a flag saying to show the added functions.
///
/// @param f true to show the added functions.
void
diff_context::show_added_fns(bool f)
{priv_->show_added_fns_ = f;}

/// @return true if we want to show the added functions, false
/// otherwise.
bool
diff_context::show_added_fns() const
{return priv_->show_added_fns_;}

/// Set a flag saying to show the deleted variables.
///
/// @param f true to show the deleted variables.
void
diff_context::show_deleted_vars(bool f)
{priv_->show_deleted_vars_ = f;}

/// @return true if we want to show the deleted variables, false
/// otherwise.
bool
diff_context::show_deleted_vars() const
{return priv_->show_deleted_vars_;}

/// Set a flag saying to show the changed variables.
///
/// @param f true to show the changed variables.
void
diff_context::show_changed_vars(bool f)
{priv_->show_changed_vars_ = f;}

/// @return true if we want to show the changed variables, false otherwise.
bool
diff_context::show_changed_vars() const
{return priv_->show_changed_vars_;}

/// Set a flag saying to show the added variables.
///
/// @param f true to show the added variables.
void
diff_context::show_added_vars(bool f)
{priv_->show_added_vars_ = f;}

/// @return true if we want to show the added variables, false
/// otherwise.
bool
diff_context::show_added_vars() const
{return priv_->show_added_vars_;}
// </diff_context stuff>

// <diff stuff>

/// Test if this diff tree node is to be filtered out for reporting
/// purposes.
///
/// The function tests if the categories of the diff tree node are
/// "forbidden" by the context or not.
///
/// @return true iff the current diff node should NOT be reported.
bool
diff::is_filtered_out() const
{return (get_category() != NO_CHANGE_CATEGORY
	 && !(get_category() & context()->get_allowed_category()));}

/// Test if this diff tree node should be reported.
///
/// @return true iff the current node should be reported.
bool
diff::to_be_reported() const
{
  if (length() && !is_filtered_out())
    return true;
  return false;
}
// </diff stuff>

static bool
report_size_and_alignment_changes(decl_base_sptr	first,
				  decl_base_sptr	second,
				  diff_context_sptr	ctxt,
				  ostream&		out,
				  const string&	indent,
				  bool			nl);

// <distinct_diff stuff>

/// The private data structure for @ref distinct_diff.
struct distinct_diff::priv
{
};// end struct distinct_diff

/// Constructor for @ref distinct_diff.
///
/// Note that the two entities considered for the diff (and passed in
/// parameter) must be of different kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
distinct_diff::distinct_diff(decl_base_sptr first,
			     decl_base_sptr second,
			     diff_context_sptr ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{assert(entities_are_of_distinct_kinds(first, second));}

/// Getter for the first subject of the diff.
///
/// @return the first subject of the diff.
const decl_base_sptr
distinct_diff::first() const
{return first_subject();}

/// Getter for the second subject of the diff.
///
/// @return the second subject of the diff.
const decl_base_sptr
distinct_diff::second() const
{return second_subject();}

/// Test if the two arguments are of different kind, or that are both
/// NULL.
///
/// @param first the first argument to test for similarity in kind.
///
/// @param second the second argument to test for similarity in kind.
///
/// @return true iff the two arguments are of different kind.
bool
distinct_diff::entities_are_of_distinct_kinds(decl_base_sptr first,
					      decl_base_sptr second)
{
  if (!!first != !!second)
    return true;
  if (!first && !second)
    // We do consider diffs of two empty decls as a diff of distinct
    // kinds, for now.
    return true;
  if (first == second)
    return false;

  return typeid(*first.get()) != typeid(*second.get());
}

/// @return 1 if the two subjects of the diff are different, 0
/// otherwise.
unsigned
distinct_diff::length() const
{
  if (first() == second()
      || (first() && second()
	  && *first() == *second()))
    return 0;
  return 1;
}

/// Emit a report about the current diff instance.
///
/// @param out the output stream to send the diff report to.
///
/// @param indent the indentation string to use in the report.
void
distinct_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  decl_base_sptr f = first(), s = second();

  string f_repr = f ? f->get_pretty_representation() : "'void'";
  string s_repr = s ? s->get_pretty_representation() : "'void'";

  out << indent << "entity changed from " << f_repr << " to " << s_repr << "\n";

  type_base_sptr fs = strip_typedef(is_type(f)),
    ss = strip_typedef(is_type(s));

  if (fs && ss
      && !entities_are_of_distinct_kinds(get_type_declaration(fs),
					 get_type_declaration(ss)))
    {
      diff_sptr diff = compute_diff(get_type_declaration(fs),
				    get_type_declaration(ss),
				    context());
      if (diff->length())
	assert(diff->to_be_reported());
      diff->report(out, indent + "  ");
    }
else
  if (!report_size_and_alignment_changes(f, s, context(), out, indent, true))
    out << indent << "but no size changed\n";
  else
    out << "\n";
}

/// Traverse an instance of distinct_diff.
///
/// @param v the visitor invoked on the instance of disting_diff.
///
/// @return true if the whole tree has to be traversed, false
/// otherwise.
bool
distinct_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  type_base_sptr fs = strip_typedef(is_type(first())),
    ss = strip_typedef(is_type(second()));

  decl_base_sptr f = get_type_declaration(fs), s = get_type_declaration(ss);

  if (f && s && !entities_are_of_distinct_kinds(f, s))
    {
      diff_sptr d = compute_diff(f, s, context());
      TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

      TRY_POST_VISIT(v);
    }

  return true;
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
distinct_diff_sptr
compute_diff_for_distinct_kinds(const decl_base_sptr first,
				const decl_base_sptr second,
				diff_context_sptr ctxt)
{
  if (!distinct_diff::entities_are_of_distinct_kinds(first, second))
    return distinct_diff_sptr();

  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      distinct_diff_sptr d = dynamic_pointer_cast<distinct_diff>(dif);
      assert(d);
      return d;
    }
  distinct_diff_sptr result(new distinct_diff(first, second, ctxt));

  ctxt->add_diff(first, second, result);
  return result;
}

/// </distinct_diff stuff>

/// Try to compute a diff on two instances of DiffType representation.
///
/// The function template performs the diff if and only if the decl
/// representations are of a DiffType.
///
/// @tparm DiffType the type of instances to diff.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
///
///@return the diff of the two types @p first and @p second if and
///only if they represent the parametrized type DiffType.  Otherwise,
///returns a NULL pointer value.
template<typename DiffType>
diff_sptr
try_to_diff(const decl_base_sptr first,
	    const decl_base_sptr second,
	    diff_context_sptr ctxt)
{
  if (shared_ptr<DiffType> f =
      dynamic_pointer_cast<DiffType>(first))
    {
      shared_ptr<DiffType> s =
	dynamic_pointer_cast<DiffType>(second);
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}


/// This is a specialization of @ref try_to_diff() template to diff
/// instances of @ref class_decl.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
template<>
diff_sptr
try_to_diff<class_decl>(const decl_base_sptr first,
			const decl_base_sptr second,
			diff_context_sptr ctxt)
{
  if (class_decl_sptr f =
      dynamic_pointer_cast<class_decl>(first))
    {
      class_decl_sptr s = dynamic_pointer_cast<class_decl>(second);
      if (f->get_is_declaration_only())
	{
	  class_decl_sptr f2 = f->get_definition_of_declaration();
	  if (f2)
	    f = f2;
	}
      if (s->get_is_declaration_only())
	{
	  class_decl_sptr s2 = s->get_definition_of_declaration();
	  if (s2)
	    s = s2;
	}
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
static diff_sptr
try_to_diff_distinct_kinds(const decl_base_sptr first,
			   const decl_base_sptr second,
			   diff_context_sptr ctxt)
{return compute_diff_for_distinct_kinds(first, second, ctxt);}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// Whenever a new kind of type decl is supported by abigail, if we
/// want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first type decl to consider for the diff
///
/// @param second the second type decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const decl_base_sptr first,
		       const decl_base_sptr second,
		       diff_context_sptr ctxt)
{
  diff_sptr d;

  const decl_base_sptr f = first;
  const decl_base_sptr s = second;

  ((d = try_to_diff_distinct_kinds(f, s, ctxt))
   ||(d = try_to_diff<type_decl>(f, s, ctxt))
   ||(d = try_to_diff<enum_type_decl>(f, s, ctxt))
   ||(d = try_to_diff<class_decl>(f, s,ctxt))
   ||(d = try_to_diff<pointer_type_def>(f, s, ctxt))
   ||(d = try_to_diff<reference_type_def>(f, s, ctxt))
   ||(d = try_to_diff<qualified_type_def>(f, s, ctxt))
   ||(d = try_to_diff<typedef_decl>(f, s, ctxt)));

  assert(d);

  return d;
}

diff_category
operator|(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   | static_cast<unsigned>(c2));}

diff_category&
operator|=(diff_category& c1, diff_category c2)
{
  c1 = c1 | c2;
  return c1;
}

diff_category
operator^(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   ^ static_cast<unsigned>(c2));}

diff_category
operator&(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   & static_cast<unsigned>(c2));}

diff_category
operator~(diff_category c)
{return static_cast<diff_category>(~static_cast<unsigned>(c));}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// @param first the first construct to consider for the diff
///
/// @param second the second construct to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const type_base_sptr first,
		       const type_base_sptr second,
		       diff_context_sptr ctxt)
{
  diff_sptr d;

  decl_base_sptr f = dynamic_pointer_cast<decl_base>(first);
  decl_base_sptr s = dynamic_pointer_cast<decl_base>(second);

  if (d = ctxt->has_diff_for(f, s))
    ;
  else
    {
      d = compute_diff_for_types(f, s, ctxt);
      ctxt->add_diff(f, s, d);
    }

  return d;
}

/// Compute the difference between two decls.
///
/// The function consider every possible decls known to libabigail and
/// runs the appropriate diff function on them.
///
/// Whenever a new kind of non-type decl is supported by abigail, if
/// we want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first decl to consider for the diff
///
/// @param second the second decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.
static diff_sptr
compute_diff_for_decls(const decl_base_sptr first,
		       const decl_base_sptr second,
		       diff_context_sptr ctxt)
{

  diff_sptr d;

  ((d = try_to_diff_distinct_kinds(first, second, ctxt))
   || (d = try_to_diff<function_decl>(first, second, ctxt))
   || (d = try_to_diff<var_decl>(first, second, ctxt)));

   assert(d);

  return d;
}

/// Compute the difference between two decls.  The decls can represent
/// either type declarations, or non-type declaration.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff could not be
/// computed.
diff_sptr
compute_diff(const decl_base_sptr	first,
	     const decl_base_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (!first || !second)
    return diff_sptr();

  diff_sptr d;
  if (is_type(first) && is_type(second))
    d = compute_diff_for_types(first, second, ctxt);
  else
    d = compute_diff_for_decls(first, second, ctxt);

  return d;
}

/// Compute the difference between two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff couldn't be
/// computed.
diff_sptr
compute_diff(const type_base_sptr	first,
	     const type_base_sptr	second,
	     diff_context_sptr		ctxt)
{
    if (!first || !second)
      return diff_sptr();

  decl_base_sptr f = get_type_declaration(first),
    s = get_type_declaration(second);
  diff_sptr d = compute_diff_for_types(f,s, ctxt);

  return d;
}

/// Return the length of the diff between two instances of @ref decl_base
///
/// @param first the first instance of @ref decl_base to consider.
///
/// @param second the second instance of @ref decl_base to consider.
///
/// @return the length of the differences between @p first and @p second.
static unsigned
diff_length_of_decl_bases(decl_base_sptr first, decl_base_sptr second)
{
  unsigned l = 0;

  if (first->get_name() != second->get_name())
    ++l;
  if (first->get_visibility() != second->get_visibility())
    ++l;
  return l;
}

/// Return the length of the diff between two instances of @ref type_base
///
/// @param first the first instance of @ref type_base to consider.
///
/// @param second the second instance of @ref type_base to consider.
///
/// @return the length of the differences between @p first and @p second.
static unsigned
diff_length_of_type_bases(type_base_sptr first, type_base_sptr second)
{
  unsigned l = 0;

  if (first->get_size_in_bits() != second->get_size_in_bits())
    ++l;
  if (first->get_alignment_in_bits() != second->get_alignment_in_bits())
    ++l;

  return l;
}

static bool
maybe_report_diff_for_member(decl_base_sptr	decl1,
			     decl_base_sptr	decl2,
			     diff_context_sptr	ctxt,
			     ostream&		out,
			     const string&	indent);

/// Stream a string representation for a member function.
///
/// @param mem_fn the member function to stream
///
/// @param out the output stream to send the representation to
static void
represent(class_decl::method_decl_sptr mem_fn, ostream& out)
{
  if (!mem_fn || !is_member_function(mem_fn))
    return;

  class_decl::method_decl_sptr meth =
    dynamic_pointer_cast<class_decl::method_decl>(mem_fn);
  assert(meth);

  out << "'" << mem_fn->get_pretty_representation() << "'";
  if (member_function_is_virtual(mem_fn))
    out << ", virtual at voffset "
	<< get_member_function_vtable_offset(mem_fn)
	<< "/"
	<< meth->get_type()->get_class_type()->get_virtual_mem_fns().size();
}

/// Stream a string representation for a data member.
///
/// @param d the data member to stream
///
/// @param out the output stream to send the representation to
static void
represent_data_member(var_decl_sptr d, ostream& out)
{
  if (!is_data_member(d) || !get_data_member_is_laid_out(d))
    return;

  out << "'" << d->get_pretty_representation() << "'"
      << ", at offset "
      << get_data_member_offset(d)
      << " (in bits)\n";
}

/// Represent the changes that happened on two versions of a given
/// class data member.
///
/// @param o the older version of the data member.
///
/// @param n the newer version of the data member.
///
/// @param ctxt the diff context to use.
///
/// @param out the output stream to send the representation to.
///
/// @param indent the indentation string to use for the change report.
static void
represent(var_decl_sptr	o,
	  var_decl_sptr	n,
	  diff_context_sptr	ctxt,
	  ostream&		out,
	  const string&	indent = "")
{
  diff_sptr diff = compute_diff_for_decls(o, n, ctxt);
  if (!diff->to_be_reported())
    return;

  bool emitted = false;
  string name1 = o->get_qualified_name();
  string name2 = n->get_qualified_name();
  string pretty_representation = o->get_pretty_representation();

  if (ctxt->get_allowed_category() & DECL_NAME_CHANGE_CATEGORY
      && name1 !=  name2)
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "name changed to '" << name2 << "'";
      emitted = true;
    }

  if (get_data_member_is_laid_out(o)
      != get_data_member_is_laid_out(n))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      if (get_data_member_is_laid_out(o))
	out << "is no more laid out";
      else
	out << "now becomes laid out";
      emitted = true;
    }
  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (get_data_member_offset(o)
	  != get_data_member_offset(n)))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "offset changed from "
	  << get_data_member_offset(o)
	  << " to " << get_data_member_offset(n);
      emitted = true;
    }
  if (o->get_binding() != n->get_binding())
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "elf binding changed from " << o->get_binding()
	  << " to " << n->get_binding();
      emitted = true;
    }
  if (o->get_visibility() != n->get_visibility())
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "visibility changed from " << o->get_visibility()
	  << " to " << n->get_visibility();
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(o)
	  != get_member_access_specifier(n)))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";

      out << "access changed from '"
	  << get_member_access_specifier(o)
	  << "' to '"
	  << get_member_access_specifier(n) << "'";
      emitted = true;
    }
  if (get_member_is_static(o)
      != get_member_is_static(n))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";

      if (get_member_is_static(o))
	out << "is no more static";
      else
	out << "now becomes static";
      emitted = true;
    }
  if (*o->get_type() != *n->get_type())
    {
      diff_sptr d = compute_diff_for_types(o->get_type(),
					   n->get_type(),
					   ctxt);
      if (d->to_be_reported())
	{
	  if (!emitted)
	    out << indent << "type of '" << pretty_representation << "' changed:\n";
	  else
	    out << "\n" << indent << "and its type '"
		<< get_type_declaration(o->get_type())->get_pretty_representation()
		<< "' changed:\n";
	  if (d->currently_reporting())
	    out << indent << "  details are being reported\n";
	  else if (d->reported_once())
	    out << indent << "  details were reported earlier\n";
	  else
	    d->report(out, indent + "  ");
	  emitted = false;
	}
    }
  if (emitted)
    out << "\n";
}

/// Report the size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
///
/// @param nl whether to start the first report line with a new line.
///
/// @return true iff something was reported.
static bool
report_size_and_alignment_changes(decl_base_sptr	first,
				  decl_base_sptr	second,
				  diff_context_sptr	ctxt,
				  ostream&		out,
				  const string&	indent,
				  bool			nl)
{
  type_base_sptr f = dynamic_pointer_cast<type_base>(first),
    s = dynamic_pointer_cast<type_base>(second);

  if (!s || !f)
    return false;

  bool n = false;
  unsigned fs = f->get_size_in_bits(), ss = s->get_size_in_bits(),
    fa = f->get_alignment_in_bits(), sa = s->get_alignment_in_bits();

  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (fs != ss))
    {
      if (nl)
	out << "\n";
      out << indent << "size changed from " << fs << " to " << ss << " bits";
      n = true;
    }
  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (fa != sa))
    {
      if (n)
	out << "\n";
      out << indent
	  << "alignment changed from " << fa << " to " << sa << " bits";
      n = true;
    }

  if (n)
    return true;
  return false;
}

/// Report the name, size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
///
/// @param nl whether to start the first report line with a new line.
///
/// @return true iff something was reported.
static bool
report_name_size_and_alignment_changes(decl_base_sptr		first,
				       decl_base_sptr		second,
				       diff_context_sptr	ctxt,
				       ostream&		out,
				       const string&		indent,
				       bool			nl)
{
  string fn = first->get_qualified_name(),
    sn = second->get_qualified_name();

  if (fn != sn
      && ctxt->get_allowed_category() & DECL_NAME_CHANGE_CATEGORY)
    {
      if (nl)
	out << "\n";
      out << indent << "name changed from '"
	  << fn << "' to '" << sn << "'";
      nl = true;
    }

  nl |= report_size_and_alignment_changes(first, second, ctxt,
					  out, indent, nl);
  return nl;
}

/// Represent the kind of difference we want report_mem_header() to
/// report.
enum diff_kind
{
  del_kind,
  ins_kind,
  subtype_change_kind,
  change_kind
};

/// Output the header preceding the the report for
/// insertion/deletion/change of a part of a class.  This is a
/// subroutine of class_diff::report.
///
/// @param out the output stream to output the report to.
///
/// @param number the number of insertion/deletion to refer to in the
/// header.
///
/// @param k the kind of diff (insertion/deletion/change) we want the
/// head to introduce.
///
/// @param section_name the name of the sub-part of the class to
/// report about.
///
/// @param indent the string to use as indentation prefix in the
/// header.
static void
report_mem_header(ostream& out,
		  size_t number,
		  size_t num_filtered,
		  diff_kind k,
		  const string& section_name,
		  const string& indent)
{
  size_t net_number = number - num_filtered;
  string change;
  char colon_or_semi_colon = ':';

  switch (k)
    {
    case del_kind:
      change = (number > 1) ? "deletions" : "deletion";
      break;
    case ins_kind:
      change = (number > 1) ? "insertions" : "insertion";
      break;
    case subtype_change_kind:
    case change_kind:
      change = (number > 1) ? "changes" : "change";
      break;
    }

  if (net_number == 0)
    {
      out << indent << "no " << section_name << " " << change;
      colon_or_semi_colon = ';';
    }
  else if (net_number == 1)
    out << indent << "1 " << section_name << " " << change;
  else
    out << indent << net_number << " " << section_name
	<< " " << change;

  if (num_filtered)
    out << " (" << num_filtered << " filtered)";
  out << colon_or_semi_colon << '\n';
}

// <pointer_type_def stuff>
struct pointer_diff::priv
{
  diff_sptr underlying_type_diff_;
};//end struct pointer_diff::priv

// <var_diff stuff>

/// The internal type for the impl idiom implementation of @ref
/// var_diff.
struct var_diff::priv
{
  diff_sptr type_diff_;
};//end struct var_diff

/// Constructor for @ref var_diff.
///
/// @param first the first instance of @ref var_decl to consider in
/// the diff.
///
/// @param second the second instance of @ref var_decl to consider in
/// the diff.
///
/// @param type_diff the diff between types of the instances of
/// var_decl.
///
/// @param ctxt the diff context to use.
var_diff::var_diff(var_decl_sptr	first,
		   var_decl_sptr	second,
		   diff_sptr		type_diff,
		   diff_context_sptr	ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{priv_->type_diff_ = type_diff;}

/// Getter for the first @ref var_decl of the diff.
///
/// @return the first @ref var_decl of the diff.
var_decl_sptr
var_diff::first_var() const
{return dynamic_pointer_cast<var_decl>(first_subject());}

/// Getter for the second @ref var_decl of the diff.
///
/// @return the second @ref var_decl of the diff.
var_decl_sptr
var_diff::second_var() const
{return dynamic_pointer_cast<var_decl>(second_subject());}

/// Getter for the diff of the types of the instances of @ref
/// var_decl.
///
/// @return the diff of the types of the instances of @ref var_decl.
diff_sptr
var_diff::type_diff() const
{return priv_->type_diff_;}

/// Compute and return the length of the current diff.
///
/// @return the length of the current diff.
unsigned
var_diff::length() const
{return *first_var() != *second_var();}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
var_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  decl_base_sptr first = first_var(), second = second_var();
  string n = first->get_pretty_representation();

  if (report_name_size_and_alignment_changes(first, second,
					     context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second, context(), out, indent);

  if (diff_sptr d = type_diff())
    {
      if (d->to_be_reported())
	{
	  out << indent << "type of variable changed:\n";
	  d->report(out, indent + " ");
	}
    }
}

/// Traverse the current instance of var_diff.
///
/// @param v the visitor invoked on the current instance of var_diff.
///
/// @return true if the whole tree is to be traversed, false
/// otherwise.
bool
var_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between two instances of @ref var_decl.
///
/// @param first the first @ref var_decl to consider for the diff.
///
/// @param second the second @ref var_decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref var_decl.
var_diff_sptr
compute_diff(const var_decl_sptr	first,
	     const var_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr di = ctxt->has_diff_for(first, second))
    {
      var_diff_sptr d = dynamic_pointer_cast<var_diff>(di);
      assert(d);
      return d;
    }

  diff_sptr type_diff = compute_diff(first->get_type(),
				     second->get_type(),
				     ctxt);
  var_diff_sptr d(new var_diff(first, second, type_diff, ctxt));
  ctxt->add_diff(first, second, d);

  return d;
}

// </var_diff stuff>

/// Report the differences in access specifiers and static-ness for
/// class members.
///
/// @param decl1 the first class member to consider.
///
/// @param decl2 the second class member to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the indentation string to use for the report.
///
/// @return true if something was reported, false otherwise.
static bool
maybe_report_diff_for_member(decl_base_sptr	decl1,
			     decl_base_sptr	decl2,
			     diff_context_sptr	ctxt,
			     ostream&		out,
			     const string&	indent)

{
  bool reported = false;
  if (!is_member_decl(decl1) || !is_member_decl(decl2))
    return reported;

  string decl1_repr = decl1->get_pretty_representation(),
    decl2_repr = decl2->get_pretty_representation();

  if (get_member_is_static(decl1) != get_member_is_static(decl2))
    {
      bool lost = get_member_is_static(decl1);
      out << indent << "'" << decl1_repr << "' ";
      if (lost)
	out << "became non-static";
      else
	out << "became static";
      out << "\n";
      reported = true;
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(decl1)
	  != get_member_access_specifier(decl2)))
    {
      out << indent << "'" << decl1_repr << "' access changed from '"
	  << get_member_access_specifier(decl1)
	  << "' to '"
	  << get_member_access_specifier(decl2)
	  << "'\n";
      reported = true;
    }
  return reported;
}

/// Constructor for a pointer_diff.
///
/// @param first the first pointer to consider for the diff.
///
/// @param second the secon pointer to consider for the diff.
///
/// @param ctxt the diff context to use.
pointer_diff::pointer_diff(pointer_type_def_sptr	first,
			   pointer_type_def_sptr	second,
			   diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{}

/// Getter for the first subject of a pointer diff
///
/// @return the first pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::first_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(first_subject());}

/// Getter for the second subject of a pointer diff
///
/// @return the second pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::second_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(second_subject());}

/// Getter for the length of this diff.
///
/// @return the length of this diff.
unsigned
pointer_diff::length() const
{return underlying_type_diff()
    ? underlying_type_diff()->length()
    : 0;}

/// Getter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @return the diff between the pointed-to types.
diff_sptr
pointer_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @param d the new diff between the pointed-to types of the pointers
/// of this diff.
void
pointer_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
pointer_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      string name = d->first_subject()->get_pretty_representation();
      if (diff_sptr d2 = context()->has_diff_for(d))
	{
	  if (d2->currently_reporting())
	    out << indent << "pointed to type '"
		<< name
		<< "' changed; details are being reported\n";
	  else if (d2->reported_once())
	    out << indent << "pointed to type '"
		<< name
		<< "' changed, as reported earlier\n";
	  else
	    {
	      out << indent
		  << "in pointed to type '"
		  << name
		  << "':\n";
	      d->report(out, indent + "  ");
	    }
	}
      else
	{
	  out << indent
	      << "in pointed to type '"
	      << name
	      << "':\n";
	      d->report(out, indent + "  ");
	}
    }
}

/// Traverse the current instance of pointer_diff.
///
/// @param v the visitor to invoke on each node traversed.
///
/// @return true if the entire sub-tree was visisted, false otherwise.
bool
pointer_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;

  TRY_PRE_VISIT(v);

  if (diff_sptr d = underlying_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between between two pointers.
///
/// @param first the pointer to consider for the diff.
///
/// @param second the pointer to consider for the diff.
///
/// @return the resulting diff between the two pointers.
///
/// @param ctxt the diff context to use.
pointer_diff_sptr
compute_diff(pointer_type_def_sptr	first,
	     pointer_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      pointer_diff_sptr d = dynamic_pointer_cast<pointer_diff>(dif);
      assert(d);
      return d;
    }

  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  pointer_diff_sptr result(new pointer_diff(first, second, ctxt));
  result->underlying_type_diff(d);
  ctxt->add_diff(first, second, result);

  return result;
}

// </pointer_type_def>

// <reference_type_def>
struct reference_diff::priv
{
  diff_sptr underlying_type_diff_;
};//end struct reference_diff::priv

/// Constructor for reference_diff
///
/// @param first the first reference_type of the diff.
///
/// @param second the second reference_type of the diff.
///
/// @param ctxt the diff context to use.
reference_diff::reference_diff(const reference_type_def_sptr	first,
			       const reference_type_def_sptr	second,
			       diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)

{}

/// Getter for the first reference of the diff.
///
/// @return the first reference of the diff.
reference_type_def_sptr
reference_diff::first_reference() const
{return dynamic_pointer_cast<reference_type_def>(first_subject());}


/// Getter for the second reference of the diff.
///
/// @return for the second reference of the diff.
reference_type_def_sptr
reference_diff::second_reference() const
{return dynamic_pointer_cast<reference_type_def>(second_subject());}


/// Getter for the diff between the two referred-to types.
///
/// @return the diff between the two referred-to types.
const diff_sptr&
reference_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two referred-to types.
///
/// @param d the new diff betweend the two referred-to types.
diff_sptr&
reference_diff::underlying_type_diff(diff_sptr d)
{
  priv_->underlying_type_diff_ = d;
  return priv_->underlying_type_diff_;
}

/// Getter of the length of the diff.
///
/// @return the length of the diff.
unsigned
reference_diff::length() const
{return underlying_type_diff()
    ? underlying_type_diff()->length()
    : 0;}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
reference_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      string name = d->first_subject()->get_pretty_representation();
      if (diff_sptr d2 = context()->has_diff_for(d))
	{
	  if (d2->currently_reporting())
	    out << indent << "referenced type '"
		<< name
		<< "' changed; details are being reported\n";
	  else if (d2->reported_once())
	    out << indent << "referenced type '"
		<< name
		<< "' changed, as reported earlier\n";
	  else
	    {
	      out << indent
		  << "in referenced type '"
		  << name
		  << "':\n";
	      d->report(out, indent + "  ");
	    }
	}
      else
	{
	  out << indent
	      << "in referenced type '"
	      << name
	      << "':\n";
	  d->report(out, indent + "  ");
	}
    }
}

/// Traverse the diff sub-tree under the current instance of
/// reference_diff.
///
/// @param v the visitor to invoke on each node diff node.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
reference_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;

  TRY_PRE_VISIT(v);

  if (diff_sptr d = underlying_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between two references.
///
/// @param first the first reference to consider for the diff.
///
/// @param second the second reference to consider for the diff.
///
/// @param ctxt the diff context to use.
reference_diff_sptr
compute_diff(reference_type_def_sptr	first,
	     reference_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      reference_diff_sptr d = dynamic_pointer_cast<reference_diff>(dif);
      return d;
    }

  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  reference_diff_sptr result(new reference_diff(first, second, ctxt));
  result->underlying_type_diff(d);
  ctxt->add_diff(first, second, result);

  return result;

}
// </reference_type_def>

// <qualified_type_diff stuff>

struct qualified_type_diff::priv
{
  diff_sptr underlying_type_diff;
};// end struct qualified_type_diff::priv


/// Constructor for qualified_type_diff.
///
/// @param first the first qualified type of the diff.
///
/// @param second the second qualified type of the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff::qualified_type_diff(qualified_type_def_sptr	first,
					 qualified_type_def_sptr	second,
					 diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{
}

/// Getter for the first qualified type of the diff.
///
/// @return the first qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::first_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(first_subject());}

/// Getter for the second qualified type of the diff.
///
/// @return the second qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::second_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(second_subject());}

/// Getter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
diff_sptr
qualified_type_diff::underlying_type_diff() const
{return priv_->underlying_type_diff;}

/// Setter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
void
qualified_type_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff = d;}

/// Return the length of the diff, or zero if the two qualified types
/// are equal.
///
/// @return the length of the diff, or zero if the two qualified types
/// are equal.
unsigned
qualified_type_diff::length() const
{
  unsigned l = 0;
  char fcv = first_qualified_type()->get_cv_quals(),
    scv = second_qualified_type()->get_cv_quals();

  if (fcv != scv)
    {
      if ((fcv & qualified_type_def::CV_CONST)
	  != (scv & qualified_type_def::CV_CONST))
	++l;
      if ((fcv & qualified_type_def::CV_VOLATILE)
	  != (scv & qualified_type_def::CV_RESTRICT))
	++l;
      if ((fcv & qualified_type_def::CV_RESTRICT)
	  != (scv & qualified_type_def::CV_RESTRICT))
	++l;
    }

  return (underlying_type_diff()
	  ? underlying_type_diff()->length() + l
	  : l);
}

/// Return the first underlying type that is not a qualified type.
/// @param t the qualified type to consider.
///
/// @return the first underlying type that is not a qualified type, or
/// NULL if t is NULL.
static type_base_sptr
get_leaf_type(qualified_type_def_sptr t)
{
  if (!t)
    return type_base_sptr();

  type_base_sptr ut = t->get_underlying_type();
  qualified_type_def_sptr qut = dynamic_pointer_cast<qualified_type_def>(ut);

  if (!qut)
    return ut;
  return get_leaf_type(qut);
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize to.
///
/// @param indent the string to use to indent the lines of the report.
void
qualified_type_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string fname = first_qualified_type()->get_pretty_representation(),
    sname = second_qualified_type()->get_pretty_representation();

  if (diff_sptr d = context()->has_diff_for(first_qualified_type(),
					    second_qualified_type()))
    {
      if (d->currently_reporting())
	{
	  out << indent << " details are being reported\n";
	  return;
	}
      else if (d->reported_once())
	{
	  out << indent << " details were reported earlier\n";
	  return;
	}
    }


  if (fname != sname)
    {
      out << indent << "'" << fname << "' changed to '" << sname << "'\n";
      return;
    }

  type_base_sptr flt = get_leaf_type(first_qualified_type()),
    slt = get_leaf_type(second_qualified_type());
  string fltname = get_type_declaration(flt)->get_pretty_representation(),
    sltname = get_type_declaration(slt)->get_pretty_representation();

  diff_sptr d = compute_diff_for_types(flt, slt, context());
  if (diff_sptr d = context()->has_diff_for_types(flt, slt))
    {
      if (d->currently_reporting())
	out << indent << "unqualified underlying type "
	    << fltname << " changed; details are being reported\n";
      else if (d->reported_once())
	out << indent << "unqualified underlying type "
	    << fltname << " changed, as reported earlier\n";
      else
	{
	  out << indent
	      << "in unqualified underlying type '"
	      << fltname << "':\n";
	  d->report(out, indent + "  ");
	}
    }
  else
    {
      out << indent << "in unqualified underlying type '" << fltname << "':\n";
      d->report(out, indent + "  ");
    }
}

/// Traverse the diff sub-tree under the current instance of
/// qualified_type_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true is the traversing has to keep going, false otherwise.
bool
qualified_type_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = underlying_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between two qualified types.
///
/// @param first the first qualified type to consider for the diff.
///
/// @param second the second qualified type to consider for the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr	first,
	     const qualified_type_def_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      qualified_type_diff_sptr d =
	dynamic_pointer_cast<qualified_type_diff>(dif);
      assert(d);
      return d;
    }

  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  qualified_type_diff_sptr result(new qualified_type_diff(first, second, ctxt));
  result->underlying_type_diff(d);
  ctxt->add_diff(first, second, result);

  return result;
}

// </qualified_type_diff stuff>

// <enum_diff stuff>
struct enum_diff::priv
{
  diff_sptr underlying_type_diff_;
  edit_script enumerators_changes_;
  string_enumerator_map deleted_enumerators_;
  string_enumerator_map inserted_enumerators_;
  string_changed_enumerator_map changed_enumerators_;
};//end struct enum_diff::priv

/// Clear the lookup tables useful for reporting an enum_diff.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
enum_diff::clear_lookup_tables()
{
  priv_->deleted_enumerators_.clear();
  priv_->inserted_enumerators_.clear();
  priv_->changed_enumerators_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
enum_diff::lookup_tables_empty() const
{
  return (priv_->deleted_enumerators_.empty()
	  && priv_->inserted_enumerators_.empty()
	  && priv_->changed_enumerators_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
enum_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script e = priv_->enumerators_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	const enum_type_decl::enumerator& n =
	  first_enum()->get_enumerators()[i];
	const string& name = n.get_name();
	assert(priv_->deleted_enumerators_.find(n.get_name())
	       == priv_->deleted_enumerators_.end());
	priv_->deleted_enumerators_[name] = n;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    const enum_type_decl::enumerator& n =
	      second_enum()->get_enumerators()[i];
	    const string& name = n.get_name();
	    assert(priv_->inserted_enumerators_.find(n.get_name())
		   == priv_->inserted_enumerators_.end());
	    string_enumerator_map::const_iterator j =
	      priv_->deleted_enumerators_.find(name);
	    if (j == priv_->deleted_enumerators_.end())
	      priv_->inserted_enumerators_[name] = n;
	    else
	      {
		if (j->second != n)
		  priv_->changed_enumerators_[j->first] =
		    std::make_pair(j->second, n);
		priv_->deleted_enumerators_.erase(j);
	      }
	  }
      }
  }
}

/// Constructor for enum_diff.
///
/// @param first the first enum type of the diff.
///
/// @param second the second enum type of the diff.
///
/// @param underlying_type_diff the diff of the two underlying types
/// of the two enum types.
///
/// @param ctxt the diff context to use.
enum_diff::enum_diff(const enum_type_decl_sptr	first,
		     const enum_type_decl_sptr	second,
		     const diff_sptr		underlying_type_diff,
		     const diff_context_sptr	ctxt)
  : diff(first, second,ctxt),
    priv_(new priv)
{priv_->underlying_type_diff_ = underlying_type_diff;}

/// @return the first enum of the diff.
const enum_type_decl_sptr
enum_diff::first_enum() const
{return dynamic_pointer_cast<enum_type_decl>(first_subject());}

/// @return the second enum of the diff.
const enum_type_decl_sptr
enum_diff::second_enum() const
{return dynamic_pointer_cast<enum_type_decl>(second_subject());}

/// @return the diff of the two underlying enum types.
diff_sptr
enum_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// @return a map of the enumerators that were deleted.
const string_enumerator_map&
enum_diff::deleted_enumerators() const
{return priv_->deleted_enumerators_;}

/// @return a map of the enumerators that were inserted
const string_enumerator_map&
enum_diff::inserted_enumerators() const
{return priv_->inserted_enumerators_;}

/// @return a map the enumerators that were changed
const string_changed_enumerator_map&
enum_diff::changed_enumerators() const
{return priv_->changed_enumerators_;}

/// @return the length of the diff.
unsigned
enum_diff::length() const
{
  unsigned a, b;
  a = underlying_type_diff() ? underlying_type_diff()->length() : 0;
  b = priv_->enumerators_changes_.length();
  return a + b;
}

/// Report the differences between the two enums.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
enum_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string name = first_enum()->get_pretty_representation();

  enum_type_decl_sptr first = first_enum(), second = second_enum();

  if (report_name_size_and_alignment_changes(first, second, context(),
					     out, indent,
					     /*start_with_num_line=*/false))
    out << "\n";
  maybe_report_diff_for_member(first, second, context(), out, indent);

  // name
  if (first->get_name() != second->get_name()
      && context()->get_allowed_category() & DECL_NAME_CHANGE_CATEGORY)
    out << indent << "enum name changed from '"
	<< first->get_qualified_name() << "' to '"
	<< second->get_qualified_name() << "'\n";

  //underlying type
  underlying_type_diff()->report(out, indent);

  //report deletions/insertions/change of enumerators
  unsigned numdels = deleted_enumerators().size();
  unsigned numins = inserted_enumerators().size();
  unsigned numchanges = changed_enumerators().size();

  if (numdels)
    {
      report_mem_header(out, numdels, 0, del_kind, "enumerator", indent);
      for (string_enumerator_map::const_iterator i =
	     deleted_enumerators().begin();
	   i != deleted_enumerators().end();
	   ++i)
	{
	  if (changed_enumerators().find(i->first)
	      != changed_enumerators().end())
	    continue;
	  if (i != deleted_enumerators().begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->second.get_qualified_name(first)
	      << "' value '"
	      << i->second.get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numins)
    {
      report_mem_header(out, numins, 0, ins_kind, "enumerator", indent);
      for (string_enumerator_map::const_iterator i =
	     inserted_enumerators().begin();
	   i != inserted_enumerators().end();
	   ++i)
	{
	  if (changed_enumerators().find(i->first)
	      != changed_enumerators().end())
	    continue;
	  if (i != inserted_enumerators().begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->second.get_qualified_name(second)
	      << "' value '"
	      << i->second.get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numchanges)
    {
      report_mem_header(out, numchanges, 0, change_kind, "enumerator", indent);
      for (string_changed_enumerator_map::const_iterator i =
	     changed_enumerators().begin();
	   i != changed_enumerators().end();
	   ++i)
	{
	  if (i != changed_enumerators().begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->second.first.get_qualified_name(first)
	      << "' from value '"
	      << i->second.first.get_value() << "' to '"
	      << i->second.second.get_value() << "'";
	}
      out << "\n\n";
    }
}

/// Traverse the diff sub-tree under the current instance of
/// enum_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
enum_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = underlying_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the set of changes between two instances of @ref
/// enum_type_decl.
///
/// @param first a pointer to the first enum_type_decl to consider.
///
/// @param second a pointer to the second enum_type_decl to consider.
///
/// @return the resulting diff of the two enums @p first and @p
/// second.
///
/// @param ctxt the diff context to use.
enum_diff_sptr
compute_diff(const enum_type_decl_sptr first,
	     const enum_type_decl_sptr second,
	     diff_context_sptr ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      enum_diff_sptr d = dynamic_pointer_cast<enum_diff>(dif);
      assert(d);
      return d;
    }

  diff_sptr ud = compute_diff_for_types(first->get_underlying_type(),
					second->get_underlying_type(),
					ctxt);
  enum_diff_sptr d(new enum_diff(first, second, ud, ctxt));

  compute_diff(first->get_enumerators().begin(),
	       first->get_enumerators().end(),
	       second->get_enumerators().begin(),
	       second->get_enumerators().end(),
	       d->priv_->enumerators_changes_);

  d->ensure_lookup_tables_populated();

  ctxt->add_diff(first, second, d);

  return d;
}
// </enum_diff stuff>

//<class_diff stuff>

struct class_diff::priv
{
  bool traversing_;
  edit_script base_changes_;
  edit_script member_types_changes_;
  edit_script data_members_changes_;
  edit_script member_fns_changes_;
  edit_script member_fn_tmpls_changes_;
  edit_script member_class_tmpls_changes_;

  string_base_sptr_map deleted_bases_;
  string_base_sptr_map inserted_bases_;
  string_changed_base_map changed_bases_;
  string_decl_base_sptr_map deleted_member_types_;
  string_decl_base_sptr_map inserted_member_types_;
  string_changed_type_or_decl_map changed_member_types_;
  string_decl_base_sptr_map deleted_data_members_;
  unsigned_decl_base_sptr_map deleted_dm_by_offset_;
  string_decl_base_sptr_map inserted_data_members_;
  unsigned_decl_base_sptr_map inserted_dm_by_offset_;
  string_changed_type_or_decl_map subtype_changed_dm_;
  unsigned_changed_type_or_decl_map changed_dm_;
  string_member_function_sptr_map deleted_member_functions_;
  string_member_function_sptr_map inserted_member_functions_;
  string_changed_member_function_sptr_map changed_member_functions_;
  string_decl_base_sptr_map deleted_member_class_tmpls_;
  string_decl_base_sptr_map inserted_member_class_tmpls_;
  string_changed_type_or_decl_map changed_member_class_tmpls_;

  class_decl::base_spec_sptr
  base_has_changed(class_decl::base_spec_sptr) const;

  decl_base_sptr
  member_type_has_changed(decl_base_sptr) const;

  decl_base_sptr
  subtype_changed_dm(decl_base_sptr) const;

  decl_base_sptr
  member_class_tmpl_has_changed(decl_base_sptr) const;

  size_t
  count_filtered_bases(const diff_context_sptr&);

  size_t
  count_filtered_subtype_changed_dm(const diff_context_sptr&);

  size_t
  count_filtered_changed_dm(const diff_context_sptr&);

  size_t
  count_filtered_changed_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_inserted_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_deleted_mem_fns(const diff_context_sptr&);

  priv()
    : traversing_(false)
  {}
};//end struct class_diff::priv

/// Clear the lookup tables useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
class_diff::clear_lookup_tables(void)
{
  priv_->deleted_bases_.clear();
  priv_->inserted_bases_.clear();
  priv_->changed_bases_.clear();
  priv_->deleted_member_types_.clear();
  priv_->inserted_member_types_.clear();
  priv_->changed_member_types_.clear();
  priv_->deleted_data_members_.clear();
  priv_->inserted_data_members_.clear();
  priv_->subtype_changed_dm_.clear();
  priv_->deleted_member_functions_.clear();
  priv_->inserted_member_functions_.clear();
  priv_->changed_member_functions_.clear();
  priv_->deleted_member_class_tmpls_.clear();
  priv_->inserted_member_class_tmpls_.clear();
  priv_->changed_member_class_tmpls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
class_diff::lookup_tables_empty(void) const
{
  return (priv_->deleted_bases_.empty()
	  && priv_->inserted_bases_.empty()
	  && priv_->changed_bases_.empty()
	  && priv_->deleted_member_types_.empty()
	  && priv_->inserted_member_types_.empty()
	  && priv_->changed_member_types_.empty()
	  && priv_->deleted_data_members_.empty()
	  && priv_->inserted_data_members_.empty()
	  && priv_->subtype_changed_dm_.empty()
	  && priv_->inserted_member_functions_.empty()
	  && priv_->deleted_member_functions_.empty()
	  && priv_->changed_member_functions_.empty()
	  && priv_->deleted_member_class_tmpls_.empty()
	  && priv_->inserted_member_class_tmpls_.empty()
	  && priv_->changed_member_class_tmpls_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
class_diff::ensure_lookup_tables_populated(void) const
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script& e = priv_->base_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	class_decl::base_spec_sptr b =
	  first_class_decl()->get_base_specifiers()[i];
	string qname = b->get_base_class()->get_qualified_name();
	assert(priv_->deleted_bases_.find(qname)
	       == priv_->deleted_bases_.end());
	priv_->deleted_bases_[qname] = b;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    class_decl::base_spec_sptr b =
	      second_class_decl()->get_base_specifiers()[i];
	    string qname = b->get_base_class()->get_qualified_name();
	    assert(priv_->inserted_bases_.find(qname)
		   == priv_->inserted_bases_.end());
	    string_base_sptr_map::const_iterator j =
	      priv_->deleted_bases_.find(qname);
	    if (j != priv_->deleted_bases_.end())
	      {
		if (*j->second != *b)
		  priv_->changed_bases_[qname] =
		    std::make_pair(j->second, b);
		priv_->deleted_bases_.erase(j);
	      }
	    else
	      priv_->inserted_bases_[qname] = b;
	  }
      }
  }

  {
    edit_script& e = priv_->member_types_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  get_type_declaration(first_class_decl()->get_member_types()[i]);
	class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(d);
	if (klass_decl && klass_decl->get_is_declaration_only())
	  continue;
	string qname = d->get_qualified_name();
	priv_->deleted_member_types_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      get_type_declaration(second_class_decl()->get_member_types()[i]);
	    class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(d);
	    if (klass_decl && klass_decl->get_is_declaration_only())
	      continue;
	    string qname = d->get_qualified_name();
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_types_.find(qname);
	    if (j != priv_->deleted_member_types_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[qname] =
		    std::make_pair(j->second, d);
		priv_->deleted_member_types_.erase(j);
	      }
	    else
	      priv_->inserted_member_types_[qname] = d;
	  }
      }
  }

  {
    edit_script& e = priv_->data_members_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d = first_class_decl()->get_data_members()[i];
	string qname = d->get_qualified_name();
	assert(priv_->deleted_data_members_.find(qname)
	       == priv_->deleted_data_members_.end());
	priv_->deleted_data_members_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d = second_class_decl()->get_data_members()[i];
	    string qname = d->get_qualified_name();
	    assert(priv_->inserted_data_members_.find(qname)
		   == priv_->inserted_data_members_.end());
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_data_members_.find(qname);
	    if (j != priv_->deleted_data_members_.end())
	      {
		if (*j->second != *d)
		  priv_->subtype_changed_dm_[qname]=
		    std::make_pair(j->second, d);
		priv_->deleted_data_members_.erase(j);
	      }
	    else
	      priv_->inserted_data_members_[qname] = d;
	  }
      }

    // Now detect when a data member is deleted from offset N and
    // another one is added to offset N.  In that case, we want to be
    // able to say that the data member at offset N changed.
    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_data_members_.begin();
	 i != priv_->deleted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->deleted_dm_by_offset_[offset] = i->second;
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_data_members_.begin();
	 i != priv_->inserted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->inserted_dm_by_offset_[offset] = i->second;
      }

    for (unsigned_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_dm_by_offset_.begin();
	 i != priv_->inserted_dm_by_offset_.end();
	 ++i)
      {
	unsigned_decl_base_sptr_map::const_iterator j =
	  priv_->deleted_dm_by_offset_.find(i->first);
	if (j != priv_->deleted_dm_by_offset_.end())
	  priv_->changed_dm_[i->first] = std::make_pair(j->second, i->second);
      }

    for (unsigned_changed_type_or_decl_map::const_iterator i =
	   priv_->changed_dm_.begin();
	 i != priv_->changed_dm_.end();
	 ++i)
      {
	priv_->deleted_dm_by_offset_.erase(i->first);
	priv_->inserted_dm_by_offset_.erase(i->first);
	priv_->deleted_data_members_.erase
	  (i->second.first->get_qualified_name());
	priv_->inserted_data_members_.erase
	  (i->second.second->get_qualified_name());
      }
  }

  {
    edit_script& e = priv_->member_fns_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	class_decl::method_decl_sptr mem_fn =
	  first_class_decl()->get_virtual_mem_fns()[i];
	string name = mem_fn->get_mangled_name();
	if (name.empty())
	  name = mem_fn->get_pretty_representation();
	assert(!name.empty());
	if (priv_->deleted_member_functions_.find(name)
	    != priv_->deleted_member_functions_.end())
	  continue;
	priv_->deleted_member_functions_[name] = mem_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;

	    class_decl::method_decl_sptr mem_fn =
	      second_class_decl()->get_virtual_mem_fns()[i];
	    string name = mem_fn->get_mangled_name();
	    if (name.empty())
	      name = mem_fn->get_pretty_representation();
	    assert(!name.empty());
	    if (priv_->inserted_member_functions_.find(name)
		!= priv_->inserted_member_functions_.end())
	      continue;
	    string_member_function_sptr_map::const_iterator j =
	      priv_->deleted_member_functions_.find(name);
	    if (j != priv_->deleted_member_functions_.end())
	      {
		if (*j->second != *mem_fn)
		  priv_->changed_member_functions_[name] =
		    std::make_pair(j->second, mem_fn);
		priv_->deleted_member_functions_.erase(j);
	      }
	    else
	      priv_->inserted_member_functions_[name] = mem_fn;
	  }
      }
  }

  {
    edit_script& e = priv_->member_class_tmpls_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  first_class_decl()->get_member_class_templates()[i]->
	  as_class_tdecl();
	string qname = d->get_qualified_name();
	assert(priv_->deleted_member_class_tmpls_.find(qname)
	       == priv_->deleted_member_class_tmpls_.end());
	priv_->deleted_member_class_tmpls_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      second_class_decl()->get_member_class_templates()[i]->
	      as_class_tdecl();
	    string qname = d->get_qualified_name();
	    assert(priv_->inserted_member_class_tmpls_.find(qname)
		   == priv_->inserted_member_class_tmpls_.end());
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_class_tmpls_.find(qname);
	    if (j != priv_->deleted_member_class_tmpls_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[qname]=
		    std::make_pair(j->second, d);
		priv_->deleted_member_class_tmpls_.erase(j);
	      }
	    else
	      priv_->inserted_member_class_tmpls_[qname] = d;
	  }
      }
  }
}

/// Test whether a given base class has changed.  A base class has
/// changed if it's in both in deleted *and* inserted bases.
///
///@param d the declaration for the base class to consider.
///
/// @return the new base class if the given base class has changed, or
/// NULL if it hasn't.
class_decl::base_spec_sptr
class_diff::priv::base_has_changed(class_decl::base_spec_sptr d) const
{
  string qname = d->get_base_class()->get_qualified_name();
  string_changed_base_map::const_iterator it =
    changed_bases_.find(qname);

  return (it == changed_bases_.end())
    ? class_decl::base_spec_sptr()
    : it->second.second;

}

/// Test whether a given member type has changed.
///
/// @param d the declaration for the member type to consider.
///
/// @return the new member type if the given member type has changed,
/// or NULL if it hasn't.
decl_base_sptr
class_diff::priv::member_type_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_changed_type_or_decl_map::const_iterator it =
    changed_member_types_.find(qname);

  return ((it == changed_member_types_.end())
	  ? decl_base_sptr()
	  : it->second.second);
}

/// Test whether a given data member has changed.
///
/// @param d the declaration for the data member to consider.
///
/// @return the new data member if the given data member has changed,
/// or NULL if if hasn't.
decl_base_sptr
class_diff::priv::subtype_changed_dm(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_changed_type_or_decl_map::const_iterator it =
    subtype_changed_dm_.find(qname);

  return ((it == subtype_changed_dm_.end())
	  ? decl_base_sptr()
	  : it->second.second);
}

/// Test whether a given member class template has changed.
///
/// @param d the declaration for the given member class template to consider.
///
/// @return the new member class template if the given one has
/// changed, or NULL if it hasn't.
decl_base_sptr
class_diff::priv::member_class_tmpl_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_changed_type_or_decl_map::const_iterator it =
    changed_member_class_tmpls_.find(qname);

  return ((it == changed_member_class_tmpls_.end())
	  ? decl_base_sptr()
	  : it->second.second);
}

/// Count the number of bases classes whose changes got filtered out.
///
/// @param ctxt the context to use to determine the filtering settings
/// from the user.
///
/// @return the number of bases classes whose changes got filtered
/// out.
size_t
class_diff::priv::count_filtered_bases(const diff_context_sptr& ctxt)
{
  size_t num_filtered = 0;
  for (string_changed_base_map::const_iterator i = changed_bases_.begin();
       i != changed_bases_.end();
       ++i)
    {
      class_decl::base_spec_sptr o =
	dynamic_pointer_cast<class_decl::base_spec>(i->second.first);
      class_decl::base_spec_sptr n =
	dynamic_pointer_cast<class_decl::base_spec>(i->second.second);
      diff_sptr diff = compute_diff(o, n, ctxt);
      ctxt->maybe_apply_filters(diff);
      if (diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;
}

/// Count the number of data members whose changes got filtered out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of data members whose changes got filtered out.
size_t
class_diff::priv::count_filtered_subtype_changed_dm(const diff_context_sptr& ctxt)
{
  size_t num_filtered= 0;
  for (string_changed_type_or_decl_map::const_iterator i =
	 subtype_changed_dm_.begin();
       i != subtype_changed_dm_.end();
       ++i)
    {
      var_decl_sptr o =
	dynamic_pointer_cast<var_decl>(i->second.first);
      var_decl_sptr n =
	dynamic_pointer_cast<var_decl>(i->second.second);
      diff_sptr diff = compute_diff_for_decls(o, n, ctxt);
      ctxt->maybe_apply_filters(diff);
      if (diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;
}

/// Count the number of member functions whose changes got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of member functions whose changes got filtered
/// out.
size_t
class_diff::priv::count_filtered_changed_mem_fns(const diff_context_sptr& ctxt)
{
  if (ctxt->get_allowed_category() & NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
    return 0;

  size_t count = 0;
  for (string_changed_member_function_sptr_map::const_iterator i =
	 changed_member_functions_.begin();
       i != changed_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f = i->second.first,
	s = i->second.second;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->is_filtered_out()
	  || (!member_function_is_virtual(i->second.first)
	      && !member_function_is_virtual(i->second.second)))
	++count;
    }

  return count;
}

/// Count the number of inserted member functions whose got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of inserted member functions whose got filtered
/// out.
size_t
class_diff::priv::count_filtered_inserted_mem_fns(const diff_context_sptr& ctxt)
{
  if (ctxt->get_allowed_category() & NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
    return 0;

  size_t count = 0;
  for (string_member_function_sptr_map::const_iterator i =
	 inserted_member_functions_.begin();
       i != inserted_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f = i->second,
	s = i->second;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->is_filtered_out()
	  || !member_function_is_virtual(i->second))
	++count;
    }

  return count;
}

/// Count the number of deleted member functions whose got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of deleted member functions whose got filtered
/// out.
size_t
class_diff::priv::count_filtered_deleted_mem_fns(const diff_context_sptr& ctxt)
{
    if (ctxt->get_allowed_category()
      & NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
    return 0;

    size_t count = 0;
  for (string_member_function_sptr_map::const_iterator i =
	 deleted_member_functions_.begin();
       i != deleted_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f = i->second,
	s = i->second;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->is_filtered_out()
	  || !member_function_is_virtual(i->second))
	++count;
    }

  return count;
}

/// Count the number of data member offsets that have changed.
///
/// @param ctxt the diff context to use.
size_t
class_diff::priv::count_filtered_changed_dm(const diff_context_sptr& ctxt)
{
  size_t num_filtered= 0;
  for (unsigned_changed_type_or_decl_map::const_iterator i =
	 changed_dm_.begin();
       i != changed_dm_.end();
       ++i)
    {
      var_decl_sptr o =
	dynamic_pointer_cast<var_decl>(i->second.first);
      var_decl_sptr n =
	dynamic_pointer_cast<var_decl>(i->second.second);
      diff_sptr diff = compute_diff(o, n, ctxt);
      ctxt->maybe_apply_filters(diff);
      if (diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;

}

/// Constructor of class_diff
///
/// @param first_scope the first class of the diff.
///
/// @param second_scope the second class of the diff.
///
/// @param ctxt the diff context to use.
class_diff::class_diff(shared_ptr<class_decl>	first_scope,
		       shared_ptr<class_decl>	second_scope,
		       diff_context_sptr	ctxt)
  : diff(first_scope, second_scope, ctxt),
    priv_(new priv)
{}

/// Getter for the length of the diff.
///
/// @return the length of the diff.
unsigned
class_diff::length() const
{return (*first_class_decl() != *second_class_decl());}

/// @return the first class invoveld in the diff.
shared_ptr<class_decl>
class_diff::first_class_decl() const
{return dynamic_pointer_cast<class_decl>(first_subject());}

/// Getter of the second class involved in the diff.
///
/// @return the second class invoveld in the diff
shared_ptr<class_decl>
class_diff::second_class_decl() const
{return dynamic_pointer_cast<class_decl>(second_subject());}

/// @return the edit script of the bases of the two classes.
const edit_script&
class_diff::base_changes() const
{return priv_->base_changes_;}

/// @return the edit script of the bases of the two classes.
edit_script&
class_diff::base_changes()
{return priv_->base_changes_;}

/// @return the edit script of the member types of the two classes.
const edit_script&
class_diff::member_types_changes() const
{return priv_->member_types_changes_;}

/// @return the edit script of the member types of the two classes.
edit_script&
class_diff::member_types_changes()
{return priv_->member_types_changes_;}

/// @return the edit script of the data members of the two classes.
const edit_script&
class_diff::data_members_changes() const
{return priv_->data_members_changes_;}

/// @return the edit script of the data members of the two classes.
edit_script&
class_diff::data_members_changes()
{return priv_->data_members_changes_;}

const string_decl_base_sptr_map&
class_diff::inserted_data_members() const
{return priv_->inserted_data_members_;}

const string_decl_base_sptr_map&
class_diff::deleted_data_members() const
{return priv_->deleted_data_members_;}

/// @return the edit script of the member functions of the two
/// classes.
const edit_script&
class_diff::member_fns_changes() const
{return priv_->member_fns_changes_;}

const string_changed_member_function_sptr_map&
class_diff::changed_member_fns() const
{return priv_->changed_member_functions_;}

/// @return the edit script of the member functions of the two
/// classes.
edit_script&
class_diff::member_fns_changes()
{return priv_->member_fns_changes_;}

/// @return a map of member functions that got deleted.
const string_member_function_sptr_map&
class_diff::deleted_member_fns() const
{return priv_->deleted_member_functions_;}

/// @return a map of member functions that got inserted.
const string_member_function_sptr_map&
class_diff::inserted_member_fns() const
{return priv_->inserted_member_functions_;}

///@return the edit script of the member function templates of the two
///classes.
const edit_script&
class_diff::member_fn_tmpls_changes() const
{return priv_->member_fn_tmpls_changes_;}

/// @return the edit script of the member function templates of the
/// two classes.
edit_script&
class_diff::member_fn_tmpls_changes()
{return priv_->member_fn_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// classes.
const edit_script&
class_diff::member_class_tmpls_changes() const
{return priv_->member_class_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// classes.
edit_script&
class_diff::member_class_tmpls_changes()
{return priv_->member_class_tmpls_changes_;}

/// Produce a basic report about the changes between two class_decl.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
class_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string name = first_subject()->get_pretty_representation();

  if (diff_sptr d = context()->has_diff_for(first_subject(),
					    second_subject()))
    {
      if (d->currently_reporting())
	{
	  out << indent << "details are being reported\n";
	  return;
	}
      else if (d->reported_once())
	{
	  out << indent << "details were reported earlier\n";
	  return;
	}
    }

  currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = first_class_decl(),
    second = second_class_decl();

  if (report_name_size_and_alignment_changes(first, second, context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second, context(), out, indent);

  // bases classes
  if (base_changes())
    {
      // Report deletions.
      int numdels = priv_->deleted_bases_.size();
      size_t numchanges = priv_->changed_bases_.size();

      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "base class", indent);

	  for (string_base_sptr_map::const_iterator i
		 = priv_->deleted_bases_.begin();
	       i != priv_->deleted_bases_.end();
	       ++i)
	    {
	      if (i != priv_->deleted_bases_.begin())
		out << "\n";

	      class_decl::base_spec_sptr base = i->second;

	      if ( priv_->base_has_changed(base))
		continue;
	      out << indent << "  "
		  << base->get_base_class()->get_qualified_name();
	    }
	  out << "\n\n";
	}

      // Report changes.
      size_t num_filtered = priv_->count_filtered_bases(context());
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered, change_kind,
			    "base class", indent);
	  for (string_changed_base_map::const_iterator it =
		 priv_->changed_bases_.begin();
	       it != priv_->changed_bases_.end();
	       ++it)
	    {
	      class_decl::base_spec_sptr o =
		dynamic_pointer_cast<class_decl::base_spec>(it->second.first);
	      class_decl::base_spec_sptr n =
		dynamic_pointer_cast<class_decl::base_spec>(it->second.second);
	      out << indent << "  '"
		  << o->get_base_class()->get_pretty_representation()
		  << "' changed:\n";
	      diff_sptr dif = compute_diff(o, n, context());
	      dif->report(out, indent + "    ");
	    }
	  out << "\n";
	}

      //Report insertions.
      int numins = priv_->inserted_bases_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "base class", indent);

	  bool emitted = false;
	  for (string_base_sptr_map::const_iterator i =
		 priv_->inserted_bases_.begin();
	       i != priv_->inserted_bases_.end();
	       ++i)
	    {
	      class_decl_sptr b = i->second->get_base_class();
	      if (emitted)
		out << "\n";
	      out << indent << b->get_qualified_name();
	      emitted = true;
	    }
	  out << "\n";
	}
    }

  // member types
  if (const edit_script& e = member_types_changes())
    {
      int numchanges = priv_->changed_member_types_.size();
      int numdels = priv_->deleted_member_types_.size();

      // report deletions
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "member type", indent);

	  for (string_decl_base_sptr_map::const_iterator i =
		 priv_->deleted_member_types_.begin();
	       i != priv_->deleted_member_types_.end();
	       ++i)
	    {
	      if (i != priv_->deleted_member_types_.begin())
		out << "\n";
	      decl_base_sptr mem_type = i->second;
	      out << indent << "  '"
		  << mem_type->get_pretty_representation()
		  << "'";
	    }
	  out << "\n\n";
	}
      // report changes
      if (numchanges)
	{
	  report_mem_header(out, numchanges, 0, change_kind,
			    "member type", indent);

	  for (string_changed_type_or_decl_map::const_iterator it =
		 priv_->changed_member_types_.begin();
	       it != priv_->changed_member_types_.end();
	       ++it)
	    {
	      decl_base_sptr o = it->second.first;
	      decl_base_sptr n = it->second.second;
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed:\n";
	      diff_sptr dif = compute_diff_for_types(o, n, context());
	      dif->report(out, indent + "    ");
	    }
	  out << "\n";
	}

      // report insertions
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;

      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "member type", indent);

	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      type_base_sptr mem_type;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  if (emitted)
		    out << "\n";
		  mem_type = second->get_member_types()[*j];
		  if (!priv_->
		      member_type_has_changed(get_type_declaration(mem_type)))
		    {
		      out << indent << "  '"
			  << get_type_declaration(mem_type)->
			get_pretty_representation()
			  << "'";
		      emitted = true;
		    }
		}
	    }
	  out << "\n\n";
	}
    }

  // data members
  if (data_members_changes())
    {
      // report deletions
      int numdels = priv_->deleted_data_members_.size();
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "data member", indent);
	  bool emitted = false;
	  for (string_decl_base_sptr_map::const_iterator i =
		 priv_->deleted_data_members_.begin();
	       i != priv_->deleted_data_members_.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(i->second);
	      assert(data_mem);
	      out << indent << "  ";
	      represent_data_member(data_mem, out);
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      // report change
      size_t numchanges = priv_->subtype_changed_dm_.size();
      size_t num_filtered = priv_->count_filtered_subtype_changed_dm(context());
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    subtype_change_kind, "data member", indent);

	  for (string_changed_type_or_decl_map::const_iterator it =
		 priv_->subtype_changed_dm_.begin();
	       it != priv_->subtype_changed_dm_.end();
	       ++it)
	    {
	      var_decl_sptr o =
		dynamic_pointer_cast<var_decl>(it->second.first);
	      var_decl_sptr n =
		dynamic_pointer_cast<var_decl>(it->second.second);
	      represent(o, n, context(), out, indent + " ");
	    }
	  out << "\n";
	}

      numchanges = priv_->changed_dm_.size();
      num_filtered = priv_->count_filtered_changed_dm(context());
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    change_kind, "data member", indent);
	  for (unsigned_changed_type_or_decl_map::const_iterator it =
		 priv_->changed_dm_.begin();
	       it != priv_->changed_dm_.end();
	       ++it)
	    {
	      var_decl_sptr o =
		dynamic_pointer_cast<var_decl>(it->second.first);
	      var_decl_sptr n =
		dynamic_pointer_cast<var_decl>(it->second.second);
	      represent(o, n, context(), out, indent + " ");
	    }
	  out << "\n";
	}

      //report insertions
      int numins = priv_->inserted_data_members_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "data member", indent);
	  bool emitted = false;
	  for (string_decl_base_sptr_map::const_iterator i =
		 priv_->inserted_data_members_.begin();
	       i != priv_->inserted_data_members_.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(i->second);
	      assert(data_mem);
	      if (emitted)
		out << "\n";
	      out << indent << "  ";
	      represent_data_member(data_mem, out);
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}
    }

  // member functions
  if (member_fns_changes())
    {
      // report deletions
      int numdels = priv_->deleted_member_functions_.size();
      size_t num_filtered = priv_->count_filtered_deleted_mem_fns(context());
      size_t net_num_dels = numdels - num_filtered;
      if (numdels)
	report_mem_header(out, numdels, num_filtered, del_kind,
			  "member function", indent);
      for (string_member_function_sptr_map::const_iterator i =
	     priv_->deleted_member_functions_.begin();
	   i != priv_->deleted_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !member_function_is_virtual(i->second))
	    continue;

	  if (i != priv_->deleted_member_functions_.begin())
	    out << "\n";
	  class_decl::method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(mem_fun, out);
	}
      if (net_num_dels)
	out << "\n";

      // report insertions;
      int numins = priv_->inserted_member_functions_.size();
      num_filtered = priv_->count_filtered_inserted_mem_fns(context());
      if (numins)
	report_mem_header(out, numins, num_filtered, ins_kind,
			  "member function", indent);
      bool emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     priv_->inserted_member_functions_.begin();
	   i != priv_->inserted_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !member_function_is_virtual(i->second))
	    continue;

	  if (i != priv_->inserted_member_functions_.begin())
	    out << "\n";
	  class_decl::method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report member function with sub-types changes
      int numchanges = priv_->changed_member_functions_.size();
      num_filtered = priv_->count_filtered_changed_mem_fns(context());
      if (numchanges)
	report_mem_header(out, numchanges, num_filtered, change_kind,
			  "member function", indent);
      for (string_changed_member_function_sptr_map::const_iterator i =
	     priv_->changed_member_functions_.begin();
	   i != priv_->changed_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !member_function_is_virtual(i->second.first)
	      && !member_function_is_virtual(i->second.second))
	    continue;

	  class_decl::method_decl_sptr f = i->second.first;
	  class_decl::method_decl_sptr s = i->second.second;
	  diff_sptr diff = compute_diff_for_decls(f, s, context());
	  if (!diff || !diff->to_be_reported())
	    continue;

	  string repr = f->get_pretty_representation();
	  if (i !=priv_->changed_member_functions_.begin())
	    out << "\n";
	  out << indent << "  '" << repr << "' has some sub-type changes:\n";
	  diff->report(out, indent + "    ");
	  emitted = true;
	}
      if (numchanges)
	out << "\n";
    }

  // member function templates
  if (const edit_script& e = member_fn_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member function template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  class_decl::member_function_template_sptr mem_fn_tmpl =
	    first->get_member_function_templates()[i->index()];
	  out << indent << "  '"
	      << mem_fn_tmpl->as_function_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member function template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  class_decl::member_function_template_sptr mem_fn_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_fn_tmpl = second->get_member_function_templates()[*j];
	      out << indent << "  '"
		  << mem_fn_tmpl->as_function_tdecl()->
		get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }

  // member class templates.
  if (const edit_script& e = member_class_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member class template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  class_decl::member_class_template_sptr mem_cls_tmpl =
	    first->get_member_class_templates()[i->index()];
	  out << indent << "  '"
	      << mem_cls_tmpl->as_class_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member class template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  class_decl::member_class_template_sptr mem_cls_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_cls_tmpl = second->get_member_class_templates()[*j];
	      out << indent << "  '"
		  << mem_cls_tmpl->as_class_tdecl()
		->get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }

  currently_reporting(false);
  reported_once(true);
}

/// Traverse the diff sub-tree under the current instance of class_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
class_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT_CLASS_DIFF(v);

  if (priv_->traversing_)
    return true;

  priv_->traversing_ = true;

  // base class changes.
  for (string_changed_base_map::const_iterator i =
	 priv_->changed_bases_.begin();
       i != priv_->changed_bases_.end();
       ++i)
    if (diff_sptr d = compute_diff(i->second.first,
				   i->second.second,
				   context()))
      TRAVERSE_MEM_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  // data member changes
  for (string_changed_type_or_decl_map::const_iterator i =
	 priv_->subtype_changed_dm_.begin();
       i != priv_->subtype_changed_dm_.end();
       ++i)
    if (diff_sptr d = compute_diff_for_decls(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_MEM_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  for (unsigned_changed_type_or_decl_map::const_iterator i =
	 priv_->changed_dm_.begin();
       i != priv_->changed_dm_.end();
       ++i)
    if (diff_sptr d = compute_diff_for_decls(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_MEM_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  // member types changes
  for (string_changed_type_or_decl_map::const_iterator i =
	 priv_->changed_member_types_.begin();
       i != priv_->changed_member_types_.end();
       ++i)
    if (diff_sptr d = compute_diff_for_types(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_MEM_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  // member function changes
  for (string_changed_member_function_sptr_map::const_iterator i =
	 priv_->changed_member_functions_.begin();
       i != priv_->changed_member_functions_.end();
       ++i)
    if (diff_sptr d = compute_diff_for_decls(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_MEM_FN_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT_CLASS_DIFF(v);

  priv_->traversing_ = false;
  return true;
}

/// Compute the set of changes between two instances of class_decl.
///
/// @param first the first class_decl to consider.
///
/// @param second the second class_decl to consider.
///
/// @return changes the resulting changes.
///
/// @param ctxt the diff context to use.
class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  class_decl_sptr f = look_through_decl_only_class(first),
    s = look_through_decl_only_class(second);

   if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      class_diff_sptr d = dynamic_pointer_cast<class_diff>(dif);
      assert(d);
      return d;
    }

  class_diff_sptr changes(new class_diff(f, s, ctxt));

  // Compare base specs
  compute_diff(f->get_base_specifiers().begin(),
	       f->get_base_specifiers().end(),
	       s->get_base_specifiers().begin(),
	       s->get_base_specifiers().end(),
	       changes->base_changes());

  // Do *not* compare member types because it generates lots of noise
  // and I doubt it's really useful.
#if 0
  compute_diff(f->get_member_types().begin(),
	       f->get_member_types().end(),
	       s->get_member_types().begin(),
	       s->get_member_types().end(),
	       changes->member_types_changes());
#endif

  // Compare data member
  compute_diff(f->get_data_members().begin(),
	       f->get_data_members().end(),
	       s->get_data_members().begin(),
	       s->get_data_members().end(),
	       changes->data_members_changes());

  // Compare virtual member functions
  compute_diff(f->get_virtual_mem_fns().begin(),
	       f->get_virtual_mem_fns().end(),
	       s->get_virtual_mem_fns().begin(),
	       s->get_virtual_mem_fns().end(),
	       changes->member_fns_changes());

  // Compare member function templates
  compute_diff(f->get_member_function_templates().begin(),
	       f->get_member_function_templates().end(),
	       s->get_member_function_templates().begin(),
	       s->get_member_function_templates().end(),
	       changes->member_fn_tmpls_changes());

  // Likewise, do not compare member class templates
#if 0
  compute_diff(f->get_member_class_templates().begin(),
	       f->get_member_class_templates().end(),
	       s->get_member_class_templates().begin(),
	       s->get_member_class_templates().end(),
	       changes->member_class_tmpls_changes());
#endif

  changes->ensure_lookup_tables_populated();

  ctxt->add_diff(first, second, changes);
  ctxt->add_diff(f, s, changes);

  return changes;
}

//</class_diff stuff>

// <base_diff stuff>
struct base_diff::priv
{
  class_diff_sptr underlying_class_diff_;
}; // end struct base_diff::priv

/// @param first the first base spec to consider.
///
/// @param second the second base spec to consider.
///
/// @param ctxt the context of the diff.
base_diff::base_diff(class_decl::base_spec_sptr first,
		     class_decl::base_spec_sptr second,
		     diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{
}

/// Getter for the first base spec of the diff object.
///
/// @return the first base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::first_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(first_subject());}

/// Getter for the second base spec of the diff object.
///
/// @return the second base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::second_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(second_subject());}

/// Getter for the diff object for the diff of the underlying base
/// classes.
///
/// @return the diff object for the diff of the underlying base
/// classes.
const class_diff_sptr
base_diff::get_underlying_class_diff() const
{return priv_->underlying_class_diff_;}

/// Setter for the diff object for the diff of the underlyng base
/// classes.
///
/// @param d the new diff object for the diff of the underlying base
/// classes.
void
base_diff::set_underlying_class_diff(class_diff_sptr d)
{priv_->underlying_class_diff_ = d;}

/// Getter for the length of the diff.
///
/// @return the length of the diff.
unsigned
base_diff::length() const
{return *first_base() != *second_base();}

/// Generates a report for the current instance of base_diff.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
base_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  class_decl::base_spec_sptr f = first_base(), s = second_base();
  string repr = f->get_base_class()->get_pretty_representation();
  bool emitted = false;

  if (f->get_is_static() != s->get_is_static())
    {
      if (f->get_is_static())
	out << indent << "is no more static";
      else
	out << indent << "now becomes static";
      emitted = true;
    }

  if ((context()->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (f->get_access_specifier() != s->get_access_specifier()))
    {
      if (emitted)
	out << ", ";

      out << "has access changed from '"
	  << f->get_access_specifier()
	  << "' to '"
	  << s->get_access_specifier()
	  << "'";

      emitted = true;
    }

  if (class_diff_sptr d = get_underlying_class_diff())
    {
      if (d->to_be_reported())
	{
	  if (emitted)
	    out << "\n";
	  d->report(out, indent);
	}
    }
}

/// Traverse the diff sub-tree under the current instance of base_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
base_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (class_diff_sptr d = get_underlying_class_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Constructs the diff object representing a diff between two base
/// class specifications.
///
/// @param first the first base class specification.
///
/// @param second the second base class specification.
///
/// @param ctxt the content of the diff.
///
/// @return the resulting diff object.
base_diff_sptr
compute_diff(const class_decl::base_spec_sptr	first,
	     const class_decl::base_spec_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      base_diff_sptr d = dynamic_pointer_cast<base_diff>(dif);
      assert(d);
      return d;
    }

  class_diff_sptr cl = compute_diff(first->get_base_class(),
				    second->get_base_class(),
				    ctxt);
  base_diff_sptr changes(new base_diff(first, second, ctxt));
  changes->set_underlying_class_diff(cl);

  ctxt->add_diff(first, second, changes);

  return changes;
}

// </base_diff stuff>

//<scope_diff stuff>
struct scope_diff::priv
{
  // The edit script built by the function compute_diff.
  edit_script member_changes_;

  // Below are the useful lookup tables.
  //
  // If you add a new lookup table, please update member functions
  // clear_lookup_tables, lookup_tables_empty and
  // ensure_lookup_tables_built.

  // The deleted/inserted types/decls.  These basically map what is
  // inside the member_changes_ data member.  Note that for instance,
  // a given type T might be deleted from the first scope and added to
  // the second scope again; this means that the type was *changed*.
  string_decl_base_sptr_map deleted_types_;
  string_decl_base_sptr_map deleted_decls_;
  string_decl_base_sptr_map inserted_types_;
  string_decl_base_sptr_map inserted_decls_;

  // The changed types/decls lookup tables.
  //
  // These lookup tables are populated from the lookup tables above.
  //
  // Note that the value stored in each of these tables is a pair
  // containing the old decl/type and the new one.  That way it is
  // easy to run a diff between the old decl/type and the new one.
  //
  // A changed type/decl is one that has been deleted from the first
  // scope and that has been inserted into the second scope.
  string_changed_type_or_decl_map changed_types_;
  string_changed_type_or_decl_map changed_decls_;

  // The removed types/decls lookup tables.
  //
  // A removed type/decl is one that has been deleted from the first
  // scope and that has *NOT* been inserted into it again.
  string_decl_base_sptr_map removed_types_;
  string_decl_base_sptr_map removed_decls_;

  // The added types/decls lookup tables.
  //
  // An added type/decl is one that has been inserted to the first
  // scope but that has not been deleted from it.
  string_decl_base_sptr_map added_types_;
  string_decl_base_sptr_map added_decls_;
};//end struct scope_diff::priv

/// Clear the lookup tables that are useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed.
void
scope_diff::clear_lookup_tables()
{
  priv_->deleted_types_.clear();
  priv_->deleted_decls_.clear();
  priv_->inserted_types_.clear();
  priv_->inserted_decls_.clear();
  priv_->changed_types_.clear();
  priv_->changed_decls_.clear();
  priv_->removed_types_.clear();
  priv_->removed_decls_.clear();
  priv_->added_types_.clear();
  priv_->added_decls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// This function must be updated each time a lookup table is added or
/// removed.
///
/// @return true iff all the lookup tables are empty.
bool
scope_diff::lookup_tables_empty() const
{
  return (priv_->deleted_types_.empty()
	  && priv_->deleted_decls_.empty()
	  && priv_->inserted_types_.empty()
	  && priv_->inserted_decls_.empty()
	  && priv_->changed_types_.empty()
	  && priv_->changed_decls_.empty()
	  && priv_->removed_types_.empty()
	  && priv_->removed_decls_.empty()
	  && priv_->added_types_.empty()
	  && priv_->added_decls_.empty());
}

/// If the lookup tables are not yet built, walk the member_changes_
/// member and fill the lookup tables.
void
scope_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  edit_script& e = priv_->member_changes_;

  // Populate deleted types & decls lookup tables.
  for (vector<deletion>::const_iterator i = e.deletions().begin();
       i != e.deletions().end();
       ++i)
    {
      decl_base_sptr decl = deleted_member_at(i);
      string qname = decl->get_qualified_name();
      if (is_type(decl))
	{
	  class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(decl);
	  if (klass_decl && klass_decl->get_is_declaration_only())
	    continue;

	  assert(priv_->deleted_types_.find(qname)
		 == priv_->deleted_types_.end());
	  priv_->deleted_types_[qname] = decl;
	}
      else
	{
	  assert(priv_->deleted_decls_.find(qname)
		 == priv_->deleted_decls_.end());
	  priv_->deleted_decls_[qname] = decl;
	}
    }

  // Populate inserted types & decls lookup tables.
  for (vector<insertion>::const_iterator it = e.insertions().begin();
       it != e.insertions().end();
       ++it)
    {
      for (vector<unsigned>::const_iterator i = it->inserted_indexes().begin();
	   i != it->inserted_indexes().end();
	   ++i)
	{
	  decl_base_sptr decl = inserted_member_at(i);
	  string qname = decl->get_qualified_name();
	  if (is_type(decl))
	    {
	      class_decl_sptr klass_decl =
		dynamic_pointer_cast<class_decl>(decl);
	      if (klass_decl && klass_decl->get_is_declaration_only())
		continue;

	      assert(priv_->inserted_types_.find(qname)
		     == priv_->inserted_types_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_types_.find(qname);
	      if (j != priv_->deleted_types_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_types_[qname] =
		      std::make_pair(j->second, decl);
		  priv_->deleted_types_.erase(j);
		}
	      else
		priv_->inserted_types_[qname] = decl;
	    }
	  else
	    {
	      assert(priv_->inserted_decls_.find(qname)
		     == priv_->inserted_decls_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_decls_.find(qname);
	      if (j != priv_->deleted_decls_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_decls_[qname] =
		      std::make_pair(j->second, decl);
		  priv_->deleted_decls_.erase(j);
		}
	      else
		priv_->inserted_decls_[qname] = decl;
	    }
	}
    }

  // Populate removed types/decls lookup tables
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_types_.find(i->first);
      if (r == priv_->inserted_types_.end())
	priv_->removed_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_decls_.find(i->first);
      if (r == priv_->inserted_decls_.end())
	priv_->removed_decls_[i->first] = i->second;
    }

  // Populate added types/decls.
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_types_.begin();
       i != priv_->inserted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_types_.find(i->first);
      if (r == priv_->deleted_types_.end())
	priv_->added_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_decls_.begin();
       i != priv_->inserted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_decls_.find(i->first);
      if (r == priv_->deleted_decls_.end())
	priv_->added_decls_[i->first] = i->second;
    }
}

/// Constructor for scope_diff
///
/// @param first_scope the first scope to consider for the diff.
///
/// @param second_scope the second scope to consider for the diff.
///
/// @param ctxt the diff context to use.
scope_diff::scope_diff(scope_decl_sptr first_scope,
		       scope_decl_sptr second_scope,
		       diff_context_sptr ctxt)
  : diff(first_scope, second_scope, ctxt),
    priv_(new priv)
{}

/// Getter for the first scope of the diff.
///
/// @return the first scope of the diff.
const scope_decl_sptr
scope_diff::first_scope() const
{return dynamic_pointer_cast<scope_decl>(first_subject());}

/// Getter for the second scope of the diff.
///
/// @return the second scope of the diff.
const scope_decl_sptr
scope_diff::second_scope() const
{return dynamic_pointer_cast<scope_decl>(second_subject());}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
const edit_script&
scope_diff::member_changes() const
{return priv_->member_changes_;}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
edit_script&
scope_diff::member_changes()
{return priv_->member_changes_;}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member that is reported
/// (in the edit script) as deleted at a given index.
///
/// @param i the index (in the edit script) of an element of the first
/// scope that has been reported as being delete.
///
/// @return the scope member that has been reported by the edit script
/// as being deleted at index i.
const decl_base_sptr
scope_diff::deleted_member_at(unsigned i) const
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(first_subject());
 return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the first scope
/// of this diff instance) that is reported (in the edit script) as
/// deleted at a given iterator.
///
/// @param i the iterator of an element of the first scope that has
/// been reported as being delete.
///
/// @return the scope member of the first scope of this diff that has
/// been reported by the edit script as being deleted at iterator i.
const decl_base_sptr
scope_diff::deleted_member_at(vector<deletion>::const_iterator i) const
{return deleted_member_at(i->index());}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given index.
///
/// @param i the index of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from index i.
const decl_base_sptr
scope_diff::inserted_member_at(unsigned i)
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(second_subject());
  return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given iterator.
///
/// @param i the iterator of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from iterator i.
const decl_base_sptr
scope_diff::inserted_member_at(vector<unsigned>::const_iterator i)
{return inserted_member_at(*i);}

/// @return a map containing the types which content has changed from
/// the first scope to the other.
const string_changed_type_or_decl_map&
scope_diff::changed_types() const
{return priv_->changed_types_;}

/// @return a map containing the decls which content has changed from
/// the first scope to the other.
const string_changed_type_or_decl_map&
scope_diff::changed_decls() const
{return priv_->changed_decls_;}

const string_decl_base_sptr_map&
scope_diff::removed_types() const
{return priv_->removed_types_;}

const string_decl_base_sptr_map&
scope_diff::removed_decls() const
{return priv_->removed_decls_;}

const string_decl_base_sptr_map&
scope_diff::added_types() const
{return priv_->added_types_;}

const string_decl_base_sptr_map&
scope_diff::added_decls() const
{return priv_->added_decls_;}

/// @return the length of the diff.
unsigned
scope_diff::length() const
{
  // TODO: add the number of really removed/added stuff.
  return changed_types().size() + changed_decls().size();
}

/// Report the changes of one scope against another.
///
/// @param out the out stream to report the changes to.
///
/// @param indent the string to use for indentation.
void
scope_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  // Report changed types.
  unsigned num_changed_types = changed_types().size();
  if (num_changed_types == 0)
    ;
  else if (num_changed_types == 1)
    out << indent << "1 changed type:\n";
  else
    out << indent << num_changed_types << " changed types:\n";

  for (string_changed_type_or_decl_map::const_iterator i =
	 changed_types().begin();
       i != changed_types().end();
       ++i)
    {
      out << indent << "  '"
	  << i->second.first->get_pretty_representation()
	  << "' changed:\n";

      diff_sptr diff = compute_diff_for_types(i->second.first,
					      i->second.second,
					      context());
      if (diff)
	diff->report(out, indent + "    ");
    }

  // Report changed decls
  unsigned num_changed_decls = changed_decls().size();
  if (num_changed_decls == 0)
    ;
  else if (num_changed_decls == 1)
    out << indent << "1 changed declaration:\n";
  else
    out << indent << num_changed_decls << " changed declarations:\n";

  for (string_changed_type_or_decl_map::const_iterator i=
	 changed_decls().begin();
       i != changed_decls().end ();
       ++i)
    {
      out << indent << "  '"
	  << i->second.first->get_pretty_representation()
	  << "' was changed to '"
	  << i->second.second->get_pretty_representation()
	  << "':\n";
      diff_sptr diff = compute_diff_for_decls(i->second.first,
					      i->second.second,
					      context());
      if (diff)
	diff->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (priv_->deleted_types_.size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (priv_->deleted_decls_.size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_types_.begin();
       i != priv_->inserted_types_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_decls_.begin();
       i != priv_->inserted_decls_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// Traverse the diff sub-tree under the current instance of scope_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
scope_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  for (string_changed_type_or_decl_map::const_iterator i =
	 changed_types().begin();
       i != changed_types().end();
       ++i)
    if (diff_sptr d = compute_diff_for_types(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  for (string_changed_type_or_decl_map::const_iterator i =
	 changed_decls().begin();
       i != changed_decls().end();
       ++i)
    if (diff_sptr d = compute_diff_for_decls(i->second.first,
					     i->second.second,
					     context()))
      TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between two scopes.
///
/// @param first the first scope to consider in computing the diff.
///
/// @param second the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param d a pointer to the diff object to populate with the
/// computed diff.
///
/// @return return the populated \a d parameter passed to this
/// function.
///
/// @param ctxt the diff context to use.
scope_diff_sptr
compute_diff(const scope_decl_sptr	first,
	     const scope_decl_sptr	second,
	     scope_diff_sptr		d,
	     diff_context_sptr		ctxt)
{
  assert(d->first_scope() == first && d->second_scope() == second);

  compute_diff(first->get_member_decls().begin(),
	       first->get_member_decls().end(),
	       second->get_member_decls().begin(),
	       second->get_member_decls().end(),
	       d->member_changes());

  d->ensure_lookup_tables_populated();
  d->context(ctxt);

  ctxt->add_diff(first, second, d);

  return d;
}

/// Compute the diff between two scopes.
///
/// @param first_scope the first scope to consider in computing the diff.
///
/// @param second_scope the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param ctxt the diff context to use.
///
/// @return return the resulting diff
scope_diff_sptr
compute_diff(const scope_decl_sptr	first_scope,
	     const scope_decl_sptr	second_scope,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first_scope, second_scope))
    {
      scope_diff_sptr d = dynamic_pointer_cast<scope_diff>(dif);
      assert(d);
      return d;
    }

  scope_diff_sptr d(new scope_diff(first_scope, second_scope, ctxt));
  return compute_diff(first_scope, second_scope, d, ctxt);
}

//</scope_diff stuff>

// <function_decl_diff stuff>
struct function_decl_diff::priv
{
  enum Flags
  {
    NO_FLAG = 0,
    IS_DECLARED_INLINE_FLAG = 1,
    IS_NOT_DECLARED_INLINE_FLAG = 1 << 1,
    BINDING_NONE_FLAG = 1 << 2,
    BINDING_LOCAL_FLAG = 1 << 3,
    BINDING_GLOBAL_FLAG = 1 << 4,
    BINDING_WEAK_FLAG = 1 << 5
  };// end enum Flags


  diff_sptr	return_type_diff_;
  edit_script	parm_changes_;
  vector<char>	first_fn_flags_;
  vector<char>	second_fn_flags_;
  edit_script	fn_flags_changes_;

  // useful lookup tables.
  string_parm_map		deleted_parms_;
  string_parm_map		added_parms_;
  string_changed_parm_map	subtype_changed_parms_;
  unsigned_parm_map		deleted_parms_by_id_;
  unsigned_parm_map		added_parms_by_id_;
  unsigned_changed_parm_map	changed_parms_by_id_;

  Flags
  fn_is_declared_inline_to_flag(function_decl_sptr f) const
  {
    return (f->is_declared_inline()
	    ? IS_DECLARED_INLINE_FLAG
	    : IS_NOT_DECLARED_INLINE_FLAG);
  }

  Flags
  fn_binding_to_flag(function_decl_sptr f) const
  {
    decl_base::binding b = f->get_binding();
    Flags result = NO_FLAG;
    switch (b)
      {
      case decl_base::BINDING_NONE:
	result = BINDING_NONE_FLAG;
	break;
      case decl_base::BINDING_LOCAL:
	  result = BINDING_LOCAL_FLAG;
	  break;
      case decl_base::BINDING_GLOBAL:
	  result = BINDING_GLOBAL_FLAG;
	  break;
      case decl_base::BINDING_WEAK:
	result = BINDING_WEAK_FLAG;
	break;
      }
    return result;
  }

};// end struct function_decl_diff::priv

/// Getter for a parameter at a given index (in the sequence of
/// parameters of the first function of the diff) marked deleted in
/// the edit script.
///
/// @param i the index of the parameter in the sequence of parameters
/// of the first function considered by the current function_decl_diff
/// object.
///
/// @return the parameter at index
const function_decl::parameter_sptr
function_decl_diff::deleted_parameter_at(int i) const
{return first_function_decl()->get_parameters()[i];}

/// Getter for a parameter at a given index (in the sequence of
/// parameters of the second function of the diff) marked inserted in
/// the edit script.
///
/// @param i the index of the parameter in the sequence of parameters
/// of the second function considered by the current
/// function_decl_diff object.
const function_decl::parameter_sptr
function_decl_diff::inserted_parameter_at(int i) const
{return second_function_decl()->get_parameters()[i];}


/// Build the lookup tables of the diff, if necessary.
void
function_decl_diff::ensure_lookup_tables_populated()
{
  string parm_name;
  function_decl::parameter_sptr parm;
  for (vector<deletion>::const_iterator i =
	 priv_->parm_changes_.deletions().begin();
       i != priv_->parm_changes_.deletions().end();
       ++i)
    {
      parm = *(first_function_decl()->get_first_non_implicit_parm()
	       + i->index());
      parm_name = parm->get_name_id();
      // If for a reason the type name is empty we want to know and
      // fix that.
      assert(!parm_name.empty());
      priv_->deleted_parms_[parm_name] = parm;
      priv_->deleted_parms_by_id_[parm->get_index()] = parm;
    }

  for (vector<insertion>::const_iterator i =
	 priv_->parm_changes_.insertions().begin();
       i != priv_->parm_changes_.insertions().end();
       ++i)
    {
      for (vector<unsigned>::const_iterator j =
	     i->inserted_indexes().begin();
	   j != i->inserted_indexes().end();
	   ++j)
	{
	  parm = *(second_function_decl()->get_first_non_implicit_parm() + *j);
	  parm_name = parm->get_name_id();
	  // If for a reason the type name is empty we want to know and
	  // fix that.
	  assert(!parm_name.empty());
	  {
	    string_parm_map::const_iterator k =
	      priv_->deleted_parms_.find(parm_name);
	    if (k != priv_->deleted_parms_.end())
	      {
		if (*k->second != *parm)
		  priv_->subtype_changed_parms_[parm_name] =
		    std::make_pair(k->second, parm);
		priv_->deleted_parms_.erase(parm_name);
	      }
	    else
	      priv_->added_parms_[parm_name] = parm;
	  }
	  {
	    unsigned_parm_map::const_iterator k =
	      priv_->deleted_parms_by_id_.find(parm->get_index());
	    if (k != priv_->deleted_parms_by_id_.end())
	      {
		if (*k->second != *parm
		    && (k->second->get_name_id() != parm_name))
		  priv_->changed_parms_by_id_[parm->get_index()] =
		    std::make_pair(k->second, parm);
		priv_->added_parms_.erase(parm_name);
		priv_->deleted_parms_.erase(k->second->get_name_id());
		priv_->deleted_parms_by_id_.erase(parm->get_index());
	      }
	    else
	      priv_->added_parms_by_id_[parm->get_index()] = parm;
	  }
	}
    }
}

/// Constructor for function_decl_diff
///
/// @param first the first function considered by the diff.
///
/// @param second the second function considered by the diff.
function_decl_diff::function_decl_diff(const function_decl_sptr first,
				       const function_decl_sptr second,
				       diff_context_sptr	ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{
  priv_->first_fn_flags_.push_back
    (priv_->fn_is_declared_inline_to_flag(first_function_decl()));
  priv_->first_fn_flags_.push_back
    (priv_->fn_binding_to_flag(first_function_decl()));

  priv_->second_fn_flags_.push_back
    (priv_->fn_is_declared_inline_to_flag(second_function_decl()));
  priv_->second_fn_flags_.push_back
    (priv_->fn_binding_to_flag(second_function_decl()));
}

/// @return the first function considered by the diff.
const function_decl_sptr
function_decl_diff::first_function_decl() const
{return dynamic_pointer_cast<function_decl>(first_subject());}

/// @return the second function considered by the diff.
const function_decl_sptr
function_decl_diff::second_function_decl() const
{return dynamic_pointer_cast<function_decl>(second_subject());}

/// Accessor for the diff of the return types of the two functions.
///
/// @return the diff of the return types.
const diff_sptr
function_decl_diff::return_type_diff() const
{return priv_->return_type_diff_;}

/// @return a map of the parameters whose type got changed.  The key
/// of the map is the name of the type.
const string_changed_parm_map&
function_decl_diff::subtype_changed_parms() const
{return priv_->subtype_changed_parms_;}

/// @return a map of parameters that got removed.
const string_parm_map&
function_decl_diff::removed_parms() const
{return priv_->deleted_parms_;}

/// @return a map of parameters that got added.
const string_parm_map&
function_decl_diff::added_parms() const
{return priv_->added_parms_;}

/// @return the length of the changes of the function.
unsigned
function_decl_diff::length() const
{return *first_function_decl() != *second_function_decl();}

/// Serialize a report of the changes encapsulated in the current
/// instance of function_decl_diff over to an output stream.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
function_decl_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  maybe_report_diff_for_member(first_function_decl(),
			       second_function_decl(),
			       context(), out, indent);

  string qn1 = first_function_decl()->get_qualified_name(),
    qn2 = second_function_decl()->get_qualified_name(),
    mn1 = first_function_decl()->get_mangled_name(),
    mn2 = second_function_decl()->get_mangled_name();

  if (qn1 != qn2 && mn1 != mn2)
    {
      string frep1 = first_function_decl()->get_pretty_representation(),
	frep2 = second_function_decl()->get_pretty_representation();
      out << indent << "'" << frep1
	  << "' is different from '"
	  << frep2 << "'\n";
      return;
    }

  // Report about return type differences.
  if (priv_->return_type_diff_ && priv_->return_type_diff_->to_be_reported())
    {
      out << indent << "return type changed:\n";
      priv_->return_type_diff_->report(out, indent + "  ");
    }

  // Hmmh, the above was quick.  Now report about function
  // parameters; this shouldn't as straightforward.
  //
  // Report about the parameter types that have changed sub-types.
  for (string_changed_parm_map::const_iterator i =
	 priv_->subtype_changed_parms_.begin();
       i != priv_->subtype_changed_parms_.end();
       ++i)
    {
      diff_sptr d = compute_diff_for_types(i->second.first->get_type(),
					   i->second.second->get_type(),
					   context());
      if (d)
	{
	  if (d->to_be_reported())
	    {
	      out << indent
		  << "parameter " << i->second.first->get_index()
		  << " of type '"
		  << i->second.first->get_type_pretty_representation()
		  << "' has sub-type changes:\n";
	      d->report(out, indent + "  ");
	    }
	}
    }
  // Report about parameters that have changed, while staying
  // compatible -- otherwise they would have changed the mangled name
  // of the function and the function would have been reported as
  // removed.
  for (unsigned_changed_parm_map::const_iterator i =
	 priv_->changed_parms_by_id_.begin();
       i != priv_->changed_parms_by_id_.end();
       ++i)
    {
      diff_sptr d = compute_diff_for_types(i->second.first->get_type(),
					   i->second.second->get_type(),
					   context());
      if (d)
	{
	  if (d->to_be_reported())
	    {
	      out << indent
		  << "parameter " << i->second.first->get_index()
		  << " of type '"
		  << i->second.first->get_type_pretty_representation()
		  << "' changed:\n";
	      d->report(out, indent + "  ");
	    }
	}
    }

  // Report about the parameters that got removed.
  bool emitted = false;
  for (string_parm_map::const_iterator i = priv_->deleted_parms_.begin();
       i != priv_->deleted_parms_.end();
       ++i)
    {
      out << indent << "parameter " << i->second->get_index()
	  << " of type '" << i->second->get_type_pretty_representation()
	  << "' was removed\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  // Report about the parameters that got added
  emitted = false;
  for (string_parm_map::const_iterator i = priv_->added_parms_.begin();
       i != priv_->added_parms_.end();
       ++i)
    {
      out << indent << "parameter " << i->second->get_index()
	  << " of type '" << i->second->get_type_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// Traverse the diff sub-tree under the current instance of
/// function_decl_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
function_decl_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = return_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  for (string_changed_parm_map::const_iterator i =
	 subtype_changed_parms().begin();
       i != subtype_changed_parms().end();
       ++i)
    if (diff_sptr d = compute_diff_for_types(i->second.first->get_type(),
					     i->second.second->get_type(),
					     context()))
      TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  for (unsigned_changed_parm_map::const_iterator i =
	 priv_->changed_parms_by_id_.begin();
       i != priv_->changed_parms_by_id_.end();
       ++i)
    if (diff_sptr d = compute_diff_for_types(i->second.first->get_type(),
					     i->second.second->get_type(),
					     context()))
      TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute the diff between two function_decl.
///
/// @param first the first function_decl to consider for the diff
///
/// @param second the second function_decl to consider for the diff
///
/// @param ctxt the diff context to use.
///
/// @return the computed diff
function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second,
	     diff_context_sptr ctxt)
{
  if (!first || !second)
    {
      // TODO: implement this for either first or second being NULL.
      return function_decl_diff_sptr();
    }

  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      function_decl_diff_sptr d = dynamic_pointer_cast<function_decl_diff>(dif);
      assert(d);
      return d;
    }

  function_decl_diff_sptr result(new function_decl_diff(first, second, ctxt));

  result->priv_->return_type_diff_ =
    compute_diff_for_types(first->get_return_type(),
			   second->get_return_type(),
			   ctxt);

  diff_utils::compute_diff(first->get_first_non_implicit_parm(),
			   first->get_parameters().end(),
			   second->get_first_non_implicit_parm(),
			   second->get_parameters().end(),
			   result->priv_->parm_changes_);

  diff_utils::compute_diff(result->priv_->first_fn_flags_.begin(),
			   result->priv_->first_fn_flags_.end(),
			   result->priv_->second_fn_flags_.begin(),
			   result->priv_->second_fn_flags_.end(),
			   result->priv_->fn_flags_changes_);

  result->ensure_lookup_tables_populated();

  ctxt->add_diff(first, second, result);

  return result;
}

// </function_decl_diff stuff>

// <type_decl_diff stuff>

/// Constructor for type_decl_diff.
type_decl_diff::type_decl_diff(const type_decl_sptr first,
			       const type_decl_sptr second,
			       diff_context_sptr ctxt)
  : diff(first, second, ctxt)
{}

/// Getter for the first subject of the type_decl_diff.
///
/// @return the first type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::first_type_decl() const
{return dynamic_pointer_cast<type_decl>(first_subject());}

/// Getter for the second subject of the type_decl_diff.
///
/// @return the second type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::second_type_decl() const
{return dynamic_pointer_cast<type_decl>(second_subject());}

/// Getter for the length of the diff.
///
/// @return 0 if the two type_decl are equal, 1 otherwise.
unsigned
type_decl_diff::length() const
{
  type_base_sptr f = is_type(first_type_decl()),
    s = is_type(second_type_decl());
  assert(f && s);

  return (diff_length_of_decl_bases(first_type_decl(),
				    second_type_decl())
	  + diff_length_of_type_bases(f, s));
}

/// Ouputs a report of the differences between of the two type_decl
/// involved in the type_decl_diff.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the string to use for indentatino indent.
void
type_decl_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  type_decl_sptr f = first_type_decl(), s = second_type_decl();

  string name = f->get_pretty_representation();

  bool n = report_name_size_and_alignment_changes(f, s, context(),
						  out, indent,
						  /*new line=*/false);

  if (f->get_visibility() != s->get_visibility())
    {
      if (n)
	out << "\n";
      out << indent
	  << "visibility changed from '"
	  << f->get_visibility() << "' to '" << s->get_visibility();
      n = true;
    }

  if (f->get_mangled_name() != s->get_mangled_name())
    {
      if (n)
	out << "\n";
      out << indent
	  << "mangled name changed from '"
	  << f->get_mangled_name() << "' to "
	  << s->get_mangled_name();
      n = true;
    }

  if (n)
    out << "\n";
}

/// Traverse (visit) the current instance of type_decl_diff node.
///
/// @param v the visitor to invoke on the node.
///
/// @return true if the current traversing has to keep going, false
/// otherwise.
bool
type_decl_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);
  return true;
}
/// Compute a diff between two type_decl.
///
/// This function doesn't actually compute a diff.  As a type_decl is
/// very simple (unlike compound constructs like function_decl or
/// class_decl) it's easy to just compare the components of the
/// type_decl to know what has changed.  Thus this function just
/// builds and return a type_decl_diff object.  The
/// type_decl_diff::report function will just compare the components
/// of the the two type_decl and display where and how they differ.
///
/// @param first a pointer to the first type_decl to
/// consider.
///
/// @param second a pointer to the second type_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the resulting type_decl_diff.
type_decl_diff_sptr
compute_diff(const type_decl_sptr	first,
	     const type_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      type_decl_diff_sptr d = dynamic_pointer_cast<type_decl_diff>(dif);
      assert(d);
      return d;
    }

  type_decl_diff_sptr result(new type_decl_diff(first, second, ctxt));

  // We don't need to actually compute a diff here as a type_decl
  // doesn't have complicated sub-components.  type_decl_diff::report
  // just walks the members of the type_decls and display information
  // about the ones that have changed.  On a similar note,
  // type_decl_diff::length returns 0 if the two type_decls are equal,
  // and 1 otherwise.

  ctxt->add_diff(first, second, result);

  return result;
}

// </type_decl_diff stuff>

// <typedef_diff stuff>

struct typedef_diff::priv
{
  diff_sptr underlying_type_diff_;
};//end struct typedef_diff::priv

/// Constructor for typedef_diff.
typedef_diff::typedef_diff(const typedef_decl_sptr	first,
			   const typedef_decl_sptr	second,
			   diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{}

/// Getter for the firt typedef_decl involved in the diff.
///
/// @return the first subject of the diff.
const typedef_decl_sptr
typedef_diff::first_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(first_subject());}

/// Getter for the second typedef_decl involved in the diff.
///
/// @return the second subject of the diff.
const typedef_decl_sptr
typedef_diff::second_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(second_subject());}

/// Getter for the diff between the two underlying types of the
/// typedefs.
///
/// @return the diff object reprensenting the difference between the
/// two underlying types of the typedefs.
const diff_sptr
typedef_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two underlying types of the
/// typedefs.
///
/// @param d the new diff object reprensenting the difference between
/// the two underlying types of the typedefs.
void
typedef_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// Getter of the length of the diff between the two typedefs.
///
/// @return 0 if the two typedefs are equal, or an integer
/// representing the length of the difference.
unsigned
typedef_diff::length() const
{
  decl_base_sptr second = second_typedef_decl();
  return !(*first_typedef_decl() == *second);
}

/// Reports the difference between the two subjects of the diff in a
/// serialized form.
///
/// @param out the output stream to emit the repot to.
///
/// @param indent the indentation string to use.
void
typedef_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  bool emit_nl = false;
  typedef_decl_sptr f = first_typedef_decl(), s = second_typedef_decl();

  if (diff_sptr d = context()->has_diff_for(f, s))
    {
      if (d->currently_reporting())
	{
	  out << indent << " details are being reported\n";
	  return;
	}
      else if (d->reported_once())
	{
	  out << indent << " details were reported earlier\n";
	  return;
	}
    }

  maybe_report_diff_for_member(f, s, context(), out, indent);

  if (f->get_name() != s->get_name()
      && context()->get_allowed_category() & DECL_NAME_CHANGE_CATEGORY)
    {
      out << indent << "typedef name changed from "
	  << f->get_qualified_name()
	  << " to "
	  << s->get_qualified_name()
	  << "\n";
      emit_nl = true;
    }

  diff_sptr d = underlying_type_diff();
  if (d && d->to_be_reported())
    {
      if (diff_sptr d2 = context()->has_diff_for(d))
	{
	  if (d2->currently_reporting())
	    out << indent << "underlying type '"
		<< d->first_subject()->get_pretty_representation()
		<< "' changed; details are being reported\n";
	  else if (d2->reported_once())
	    out << indent << "underlying type '"
		<< d->first_subject()->get_pretty_representation()
		<< "' changed, as reported earlier\n";
	  else
	    {
	      out << indent << "underlying type '"
		  << d->first_subject()->get_pretty_representation()
		  << "' changed:\n";
	      d->report(out, indent + "  ");
	    }
	}
      else
	{
	  out << indent << "underlying type changed:\n";
	  d->report(out, indent + "  ");
	}
      emit_nl = false;
    }

  if (emit_nl)
    out << "\n";
}

/// Traverse the diff sub-tree under the current instance of typedef_diff.
///
/// @param v the visitor to invoke on the diff nodes of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
typedef_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = underlying_type_diff())
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;
}

/// Compute a diff between two typedef_decl.
///
/// @param first a pointer to the first typedef_decl to consider.
///
/// @param second a pointer to the second typedef_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the the resulting typedef_diff.
typedef_diff_sptr
compute_diff(const typedef_decl_sptr	first,
	     const typedef_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (diff_sptr dif = ctxt->has_diff_for(first, second))
    {
      typedef_diff_sptr d = dynamic_pointer_cast<typedef_diff>(dif);
      assert(d);
      return d;
    }

  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  typedef_diff_sptr result(new typedef_diff(first, second, ctxt));
  result->underlying_type_diff(d);

  ctxt->add_diff(first, second, result);

  return result;
}

// </typedef_diff stuff>

// <translation_unit_diff stuff>

struct translation_unit_diff::priv
{
  translation_unit_sptr first_;
  translation_unit_sptr second_;

  priv(translation_unit_sptr f, translation_unit_sptr s)
    : first_(f), second_(s)
  {}
};//end struct translation_unit_diff::priv

/// Constructor for translation_unit_diff.
///
/// @param first the first translation unit to consider for this diff.
///
/// @param second the second translation unit to consider for this diff.
translation_unit_diff::translation_unit_diff(translation_unit_sptr first,
					     translation_unit_sptr second,
					     diff_context_sptr ctxt)
  : scope_diff(first->get_global_scope(), second->get_global_scope(), ctxt),
    priv_(new priv(first, second))
{
}

/// Getter for the first translation unit of this diff.
///
/// @return the first translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::first_translation_unit() const
{return priv_->first_;}

/// Getter for the second translation unit of this diff.
///
/// @return the second translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::second_translation_unit() const
{return priv_->second_;}

/// @return the length of this diff.
unsigned
translation_unit_diff::length() const
{return scope_diff::length();}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
translation_unit_diff::report(ostream& out, const string& indent) const
{scope_diff::report(out, indent);}

/// Traverse the diff sub-tree under the current instance of
/// translation_unit_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going, false otherwise.
bool
translation_unit_diff::traverse(diff_node_visitor& v)
{
  ENSURE_DIFF_NODE_TRAVERSED_ONCE;
  TRY_PRE_VISIT(v);

  if (diff_sptr d = compute_diff(first_translation_unit(),
				 second_translation_unit(),
				 context()))
    TRAVERSE_DIFF_NODE_AND_PROPAGATE_CATEGORY(d, v);

  TRY_POST_VISIT(v);

  return true;

}

/// Compute the diff between two translation_units.
///
/// @param first the first translation_unit to consider.
///
/// @param second the second translation_unit to consider.
///
/// @param ctxt the diff context to use.  If null, this function will
/// create a new context and set to the diff object returned.
///
/// @return the newly created diff object.
translation_unit_diff_sptr
compute_diff(const translation_unit_sptr	first,
	     const translation_unit_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (!ctxt)
    ctxt.reset(new diff_context);

  // TODO: handle first or second having empty contents.
  translation_unit_diff_sptr tu_diff(new translation_unit_diff(first, second,
							       ctxt));
  scope_diff_sptr sc_diff = dynamic_pointer_cast<scope_diff>(tu_diff);

  compute_diff(static_pointer_cast<scope_decl>(first->get_global_scope()),
	       static_pointer_cast<scope_decl>(second->get_global_scope()),
	       sc_diff,
	       ctxt);

  return tu_diff;
}

// </translation_unit_diff stuff>

// <corpus stuff>
struct corpus_diff::priv
{
  diff_context_sptr			ctxt_;
  corpus_sptr				first_;
  corpus_sptr				second_;
  edit_script				fns_edit_script_;
  edit_script				vars_edit_script_;
  string_function_ptr_map		deleted_fns_;
  string_function_ptr_map		added_fns_;
  string_changed_function_ptr_map	changed_fns_;
  string_var_ptr_map			deleted_vars_;
  string_var_ptr_map			added_vars_;
  string_changed_var_ptr_map		changed_vars_;

  bool
  lookup_tables_empty() const;

  void
  clear_lookup_tables();

  void
  ensure_lookup_tables_populated();

  struct diff_stats
  {
    size_t num_func_removed;
    size_t num_func_added;
    size_t num_func_changed;
    size_t num_func_filtered_out;
    size_t num_vars_removed;
    size_t num_vars_added;
    size_t num_vars_changed;
    size_t num_vars_filtered_out;

    diff_stats()
      : num_func_removed(0),
	num_func_added(0),
	num_func_changed(0),
	num_func_filtered_out(0),
	num_vars_removed(0),
	num_vars_added(0),
	num_vars_changed(0),
	num_vars_filtered_out(0)
    {}
  };

  void
  apply_filters_and_compute_diff_stats(diff_stats&);

  void
  emit_diff_stats(diff_stats&	stats,
		  ostream&	out,
		  const string& indent);
}; // end corpus::priv

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
corpus_diff::priv::lookup_tables_empty() const
{
  return (deleted_fns_.empty()
	  && added_fns_.empty()
	  && changed_fns_.empty()
	  && deleted_vars_.empty()
	  && added_vars_.empty()
	  && changed_vars_.empty());
}

/// Clear the lookup tables useful for reporting an enum_diff.
void
corpus_diff::priv::clear_lookup_tables()
{
  deleted_fns_.clear();
  added_fns_.clear();
  changed_fns_.clear();
  deleted_vars_.clear();
  added_vars_.clear();
  changed_vars_.clear();
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
corpus_diff::priv::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script& e = fns_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_functions().size());

	function_decl* deleted_fn = first_->get_functions()[i];
	string n = deleted_fn->get_mangled_name();
	if (n.empty())
	  n = deleted_fn->get_pretty_representation();
	assert(!n.empty());
	assert(deleted_fns_.find(n) == deleted_fns_.end());
	deleted_fns_[n] = deleted_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    function_decl* added_fn = second_->get_functions()[i];
	    string n = added_fn->get_mangled_name();
	    if (n.empty())
	      n = added_fn->get_pretty_representation();
	    assert(!n.empty());
	    assert(added_fns_.find(n) == added_fns_.end());
	    string_function_ptr_map::const_iterator j =
	      deleted_fns_.find(n);
	    if (j == deleted_fns_.end())
	      {
		// It can happen that an old dwarf producer might not
		// have emitted the mangled name of the first diff
		// subject.  Int hat case, we need to try to use the
		// function synthetic signature here.
		// TODO: also query the underlying elf file's .dynsym
		// symbol table to see if the symbol is present in the
		// first diff subject before for real.
		if (!added_fn->get_mangled_name().empty())
		  j = deleted_fns_.find(added_fn->get_pretty_representation());
	      }
	    if (j != deleted_fns_.end())
	      {
		if (*j->second != *added_fn)
		  changed_fns_[j->first] = std::make_pair(j->second,
							  added_fn);
		deleted_fns_.erase(j);
	      }
	    else
	      added_fns_[n] = added_fn;
	  }
      }
  }

  {
    edit_script& e = vars_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_variables().size());

	var_decl* deleted_var = first_->get_variables()[i];
	string n = deleted_var->get_mangled_name();
	if (n.empty())
	  n = deleted_var->get_pretty_representation();
	assert(!n.empty());
	assert(deleted_vars_.find(n) == deleted_vars_.end());
	deleted_vars_[n] = deleted_var;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    var_decl* added_var = second_->get_variables()[i];
	    string n = added_var->get_mangled_name();
	    if (n.empty())
	      n = added_var->get_name();
	    assert(!n.empty());
	    assert(added_vars_.find(n) == added_vars_.end());
	    string_var_ptr_map::const_iterator j =
	      deleted_vars_.find(n);
	    if (j != deleted_vars_.end())
	      {
		if (*j->second != *added_var)
		  changed_vars_[n] = std::make_pair(j->second, added_var);
		deleted_vars_.erase(j);
	      }
	    else
	      added_vars_[n] = added_var;
	  }
      }
  }
}

/// Compute the diff stats.
///
/// To know the number of functions that got filtered out, this
/// function applies the categorizing filters to the diff sub-trees of
/// each function changes diff, prior to calculating the stats.
///
/// @param num_removed the number of removed functions.
///
/// @param num_added the number of added functions.
///
/// @param num_changed the number of changed functions.
///
/// @param num_filtered_out the number of changed functions that are
/// got filtered out from the report
void
corpus_diff::priv::apply_filters_and_compute_diff_stats(diff_stats& stat)

{
  stat.num_func_removed = deleted_fns_.size();
  stat.num_func_added = added_fns_.size();
  stat.num_func_changed = changed_fns_.size();

  stat.num_vars_removed = deleted_vars_.size();
  stat.num_vars_added = added_vars_.size();
  stat.num_vars_changed = changed_vars_.size();

  // Calculate number of filtered functions & variables.
  diff_sptr diff;
  for (string_changed_function_ptr_map::const_iterator i =
	 changed_fns_.begin();
       i != changed_fns_.end();
       ++i)
    {
      function_decl_sptr f(i->second.first, noop_deleter());
      function_decl_sptr s(i->second.second, noop_deleter());
      diff = compute_diff(f, s, ctxt_);
      ctxt_->maybe_apply_filters(diff);
      if (diff->is_filtered_out())
	++stat.num_func_filtered_out;
    }

  for (string_changed_var_ptr_map::const_iterator i = changed_vars_.begin();
       i != changed_vars_.end();
       ++i)
    {
      var_decl_sptr f(i->second.first, noop_deleter());
      var_decl_sptr s(i->second.second, noop_deleter());
      diff = compute_diff_for_decls(f, s, ctxt_);
      ctxt_->maybe_apply_filters(diff);
      if (diff->is_filtered_out())
	++stat.num_vars_filtered_out;
    }
}

/// Emit the summary of the functions & variables that got
/// removed/changed/added.
///
/// @param out the output stream to emit the stats to.
///
/// @param indent the indentation string to use in the summary.
void
corpus_diff::priv::emit_diff_stats(diff_stats&		s,
				   ostream&		out,
				   const string&	indent)
{
  /// Report added/removed/changed functions.
  size_t total = s.num_func_removed + s.num_func_added +
    s.num_func_changed - s.num_func_filtered_out;

  // function changes summary
  out << indent << "Functions changes summary: ";
  out << s.num_func_removed << " Removed, ";
  out << s.num_func_changed - s.num_func_filtered_out << " Changed";
  if (s.num_func_filtered_out)
    out << " (" << s.num_func_filtered_out << " filtered out)";
  out << ", ";
  out << s.num_func_added << " Added ";
  if (total <= 1)
    out << "function\n";
  else
    out << "functions\n";

  total = s.num_vars_removed + s.num_vars_added +
    s.num_vars_changed - s.num_vars_filtered_out;

  // variables changes summary
  out << indent << "Variables changes summary: ";
  out << s.num_vars_removed << " Removed, ";
  out << s.num_vars_changed - s.num_vars_filtered_out<< " Changed";
  if (s.num_vars_filtered_out)
    out << " (" << s.num_vars_filtered_out << " filtered out)";
  out << ", ";
  out << s.num_vars_added << " Added ";
  if (total <= 1)
    out << "variable\n";
  else
    out << "variables\n";
}

/// Constructor for @ref corpus_diff.
///
/// @param first the first corpus of the diff.
///
/// @param second the second corpus of the diff.
///
/// @param ctxt the diff context to use.
corpus_diff::corpus_diff(corpus_sptr first,
			 corpus_sptr second,
			 diff_context_sptr ctxt)
  : priv_(new priv)
{
  priv_->first_ = first;
  priv_->second_ = second;
  priv_->ctxt_ = ctxt;
}

/// @return the first corpus of the diff.
corpus_sptr
corpus_diff::first_corpus() const
{return priv_->first_;}

/// @return the second corpus of the diff.
corpus_sptr
corpus_diff::second_corpus() const
{return priv_->second_;}

/// @return the bare edit script of the functions changed as recorded
/// by the diff.
edit_script&
corpus_diff::function_changes() const
{return priv_->fns_edit_script_;}

/// @return the bare edit script of the variables changed as recorded
/// by the diff.
edit_script&
corpus_diff::variable_changes() const
{return priv_->vars_edit_script_;}

/// Getter for the deleted functions of the diff.
///
/// @return the the deleted functions of the diff.
const string_function_ptr_map&
corpus_diff::deleted_functions() const
{return priv_->deleted_fns_;}

/// Getter for the added functions of the diff.
///
/// @return the added functions of the diff.
const string_function_ptr_map&
corpus_diff::added_functions()
{return priv_->added_fns_;}

/// Getter for the functions which signature didn't change, but which
/// do have some indirect changes in their parms.
///
/// @return functions which signature didn't change, but which
/// do have some indirect changes in their parms.
const string_changed_function_ptr_map&
corpus_diff::changed_functions()
{return priv_->changed_fns_;}

/// Getter of the diff context of this diff
///
/// @return the diff context for this diff.
const diff_context_sptr
corpus_diff::context() const
{return priv_->ctxt_;}

/// @return the length of the changes as recorded by the diff.
unsigned
corpus_diff::length() const
{
  return (priv_->deleted_fns_.size()
	  + priv_->added_fns_.size()
	  - priv_->changed_fns_.size());
}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
corpus_diff::report(ostream& out, const string& indent) const
{
  size_t total = 0, removed = 0, added = 0;
  priv::diff_stats s;

  /// Report added/removed/changed functions.
  priv_->apply_filters_and_compute_diff_stats(s);
  total = s.num_func_removed + s.num_func_added +
    s.num_func_changed - s.num_func_filtered_out;
  const unsigned large_num = 100;

  priv_->emit_diff_stats(s, out, indent);
  if (context()->show_stats_only())
    return;
  out << "\n";

  if (context()->show_deleted_fns())
    {
      if (s.num_func_removed == 1)
	out << indent << "1 Removed function:\n\n";
      else if (s.num_func_removed > 1)
	out << indent << s.num_func_removed << " Removed functions:\n\n";

      for (string_function_ptr_map::const_iterator i =
	     priv_->deleted_fns_.begin();
	   i != priv_->deleted_fns_.end();
	   ++i)
	{
	  out << indent
	      << "  '";
	  if (total > large_num)
	    out << "[D] ";
	  out << i->second->get_pretty_representation()
	      << "\n";
	  ++removed;
	}
      if (removed)
	out << "\n";
    }

  if (context()->show_changed_fns())
    {
      size_t num_changed = s.num_func_changed - s.num_func_filtered_out;
      if (num_changed == 1)
	out << indent << "1 function with some indirect sub-type change:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " functions with some indirect sub-type change:\n\n";

      for (string_changed_function_ptr_map::const_iterator i =
	     priv_->changed_fns_.begin();
	   i != priv_->changed_fns_.end();
	   ++i)
	{
	  function_decl_sptr f(i->second.first, noop_deleter());
	  function_decl_sptr s(i->second.second, noop_deleter());

	  diff_sptr diff = compute_diff(f, s, context());
	  if (!diff)
	    continue;

	  if (diff->to_be_reported())
	    {
	      out << indent << "  [C]'"
		  << i->second.first->get_pretty_representation()
		  << "' has some indirect sub-type changes:\n";
	      diff->report(out, indent + "    ");
	    }
	  }
      out << "\n";
    }

  if (context()->show_added_fns())
    {
      if (s.num_func_added == 1)
	out << indent << "1 Added function:\n";
      else if (s.num_func_added > 1)
	out << indent << s.num_func_added
	    << " Added functions:\n\n";
      for (string_function_ptr_map::const_iterator i =
	     priv_->added_fns_.begin();
	   i != priv_->added_fns_.end();
	   ++i)
	{
	  out
	    << indent
	    << "  ";
	  if (total > large_num)
	    out << "[A] ";
	  out << "'"
	      << i->second->get_pretty_representation()
	      << "'\n";
	  ++added;
	}
      if (added)
	out << "\n";
    }

 // Report added/removed/changed variables.
  total = s.num_vars_removed + s.num_vars_added +
    s.num_vars_changed - s.num_vars_filtered_out;

  if (context()->show_deleted_vars())
    {
      if (s.num_vars_removed == 1)
	out << indent << "1 Deleted variable:\n";
      else if (s.num_vars_removed > 1)
	out << indent << s.num_vars_removed
	    << " Deleted variables:\n\n";
      string n;
      for (string_var_ptr_map::const_iterator i =
	     priv_->deleted_vars_.begin();
	   i != priv_->deleted_vars_.end();
	   ++i)
	{
	  if (!i->second->get_mangled_name().empty())
	    n = demangle_cplus_mangled_name(i->second->get_mangled_name());
	  else
	    n = i->second->get_pretty_representation();
	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[D] ";
	  out << "'"
	      << n
	      << "'\n";
	  ++removed;
	}
      if (removed)
	out << "\n";
    }

  if (context()->show_changed_vars())
    {
      size_t num_changed = s.num_vars_changed - s.num_vars_filtered_out;
      if (num_changed == 1)
	out << indent << "1 Changed variable:\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " Changed variables:\n\n";
      string n1, n2;
      for (string_changed_var_ptr_map::const_iterator i =
	     priv_->changed_vars_.begin();
	   i != priv_->changed_vars_.end();
	   ++i)
	{
	  var_decl_sptr f(i->second.first, noop_deleter());
	  var_decl_sptr s(i->second.second, noop_deleter());
	  diff_sptr diff = compute_diff_for_decls(f, s, context());
	  if (!diff || !diff->to_be_reported())
	    continue;

	  if (!f->get_mangled_name().empty())
	    n1 = demangle_cplus_mangled_name(f->get_mangled_name());
	  else
	    n1 = f->get_pretty_representation();

	  if (!s->get_mangled_name().empty())
	    n2 = demangle_cplus_mangled_name(s->get_mangled_name());
	  else
	    n2 = s->get_pretty_representation();

	  out << indent << "  '"
	      << n1
	      << "' was changed to '"
	      << n2
	      << "':\n";
	  diff->report(out, indent + "    ");
	}
      if (num_changed)
	out << "\n";
    }

  if (context()->show_added_vars())
    {
      if (s.num_vars_added == 1)
	out << indent << "1 Added variable:\n";
      else if (s.num_vars_added > 1)
	out << indent << s.num_vars_added
	    << " Added variables:\n";
      string n;
      for (string_var_ptr_map::const_iterator i =
	     priv_->added_vars_.begin();
	   i != priv_->added_vars_.end();
	   ++i)
	{
	  if (!i->second->get_mangled_name().empty())
	    n = demangle_cplus_mangled_name(i->second->get_mangled_name());
	  else
	    n = i->second->get_pretty_representation();
	  out << indent
	      << "  '";
	  if (total > large_num)
	    out << "[A] ";
	  out << n
	      << "\n";
	  ++added;
	}
      if (added)
	out << "\n";
    }
}

/// Traverse the diff sub-tree under the current instance corpus_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going on, false otherwise.
bool
corpus_diff::traverse(diff_node_visitor& v)
{
  if (!v.visit(this, true))
    return false;

  for (string_changed_function_ptr_map::const_iterator i =
	 changed_functions().begin();
       i != changed_functions().end();
       ++i)
    {
      function_decl_sptr f(i->second.first, noop_deleter());
      function_decl_sptr s(i->second.second, noop_deleter());

      if (diff_sptr d = compute_diff_for_decls(f,s, context()))
	if (!d->traverse(v))
	  return false;
    }

  return true;
}
/// Compute the diff between two instances fo the @ref corpus
///
/// @param f the first @ref corpus to consider for the diff.
///
/// @param s the second @ref corpus to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref corpus.
corpus_diff_sptr
compute_diff(const corpus_sptr	f,
	     const corpus_sptr	s,
	     diff_context_sptr	ctxt)
{
  typedef corpus::functions::const_iterator fns_it_type;
  typedef corpus::variables::const_iterator vars_it_type;
  typedef diff_utils::deep_ptr_eq_functor eq_type;

  if (!ctxt)
    ctxt.reset(new diff_context);

  corpus_diff_sptr r(new corpus_diff(f, s, ctxt));

  diff_utils::compute_diff<fns_it_type, eq_type>(f->get_functions().begin(),
						 f->get_functions().end(),
						 s->get_functions().begin(),
						 s->get_functions().end(),
						 r->priv_->fns_edit_script_);

  struct var_eq_type
  {
    bool
    operator()(const var_decl* first,
	       const var_decl* second)
    {
      string n1, n2;
      if (!first->get_mangled_name().empty())
	n1 = first->get_mangled_name();
      if (n1.empty())
	{
	  n1 = first->get_pretty_representation();
	  n2 = second->get_pretty_representation();
	  assert(!n2.empty());
	}
      assert(!n1.empty());

      if (n2.empty())
	{
	  n2 = second->get_mangled_name();
	  if (n2.empty())
	    {
	      n2 = second->get_pretty_representation();
	      n1 = second->get_pretty_representation();
	      assert(!n1.empty());
	    }
	}
      assert(!n2.empty());

      if (n1 != n2)
	return false;
      if (*first->get_type() != *second->get_type())
	return false;
      return true;
    }
  };

  diff_utils::compute_diff<vars_it_type, var_eq_type>
    (f->get_variables().begin(), f->get_variables().end(),
     s->get_variables().begin(), s->get_variables().end(),
     r->priv_->vars_edit_script_);
  r->priv_->ensure_lookup_tables_populated();

  return r;
}
// </corpus stuff>

// <diff_node_visitor stuff>

/// Default visitor implementation
///
/// @return true
bool
diff_node_visitor::visit(diff*, bool)
{return true;}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(distinct_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(var_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(pointer_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(reference_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(qualified_type_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(enum_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(class_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(base_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(scope_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(function_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(type_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(typedef_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(translation_unit_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(corpus_diff*, bool)
{return true;}

// </diff_node_visitor stuff>

}// end namespace comparison
} // end namespace abigail
