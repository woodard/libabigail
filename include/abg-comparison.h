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

#ifndef __ABG_COMPARISON_H__
#define __ABG_COMPARISON_H__

/// @file

#include <tr1/unordered_map>
#include "abg-corpus.h"
#include "abg-diff-utils.h"

namespace abigail
{

/// @brief utilities to compare abi artifacts
///
/// The main entry points of the namespace are the compute_diff()
/// overloads used to compute the difference between two abi artifacts.
namespace comparison
{

namespace filtering
{
class filter_base;
typedef shared_ptr<filter_base> filter_base_sptr;
typedef std::vector<filter_base_sptr> filters;
}

// Inject types we need into this namespace.
using std::ostream;
using std::vector;
using std::tr1::unordered_map;
using std::pair;

using diff_utils::insertion;
using diff_utils::deletion;
using diff_utils::edit_script;

class diff;

/// Convenience typedef for a shared_ptr for the @ref diff class
typedef shared_ptr<diff> diff_sptr;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref decl_base_sptr.
typedef unordered_map<string, decl_base_sptr> string_decl_base_sptr_map;

/// Convenience typedef for a changed type or decl.  The first element
/// of the pair is the old type/decl and the second is the new one.
typedef pair<decl_base_sptr, decl_base_sptr> changed_type_or_decl;

/// Convenience typedef for a map of string and class_decl::basse_spec_sptr.
typedef unordered_map<string, class_decl::base_spec_sptr> string_base_sptr_map;

/// Convenience typedef for a pair of class_decl::base_spec_sptr.
typedef pair<class_decl::base_spec_sptr,
	     class_decl::base_spec_sptr> changed_base;

/// Convenience typedef for a map of string and changed_base.
typedef unordered_map<string, changed_base> string_changed_base_map;

/// Convenience typedef for a changed function parameter.  The first element of
/// the pair is the old function parm and the second element is the
/// new function parm.
typedef pair<function_decl::parameter_sptr,
	     function_decl::parameter_sptr> changed_parm;

/// Convenience typedef for a map which value is a changed function
/// parameter and which key is the name of the function parameter.
typedef unordered_map<string, changed_parm> string_changed_parm_map;

/// Convenience typedef for a map which value is changed type of decl.
/// The key of the map is the qualified name of the type/decl.
typedef unordered_map<string,
		      changed_type_or_decl> string_changed_type_or_decl_map;

/// Convenience typedef for a map which value is a function
/// parameter.  The key is the name of the function parm.
typedef unordered_map<string, function_decl::parameter_sptr> string_parm_map;

/// Convenience typedef for a map which value is an enumerator.  The
/// key is the name of the enumerator.
typedef unordered_map<string, enum_type_decl::enumerator> string_enumerator_map;

/// Convenience typedef for a changed enumerator.  The first element
/// of the pair is the old enumerator and the second one is the new enumerator.
typedef std::pair<enum_type_decl::enumerator,
		  enum_type_decl::enumerator> changed_enumerator;

/// Convenience typedef for a map which value is a changed enumerator.
/// The key is the name of the changed enumerator.
typedef unordered_map<string, changed_enumerator> string_changed_enumerator_map;

/// Convenience typedef for a map which key is a string and which
/// value is a pointer to @ref decl_base.
typedef unordered_map<string, function_decl*> string_function_ptr_map;

/// Convenience typedef for a pair of pointer to @ref function_decl
/// representing a @ref function_decl change.  The first
/// member of the pair represents the initial function and the second
/// member represents the changed function.
typedef std::pair<function_decl*, function_decl*> changed_function_ptr;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref changed_function_ptr.
typedef unordered_map<string,
		      changed_function_ptr> string_changed_function_ptr_map;

/// Convenience typedef for a pair of class_decl::member_function_sptr
/// representing a changed member function.  The first element of the
/// pair is the initial member function and the second element is the
/// changed one.
typedef pair<class_decl::method_decl_sptr,
	     class_decl::method_decl_sptr> changed_member_function_sptr;

/// Convenience typedef for a hash map of strings and changed member functions.
typedef unordered_map<string,
		      changed_member_function_sptr>
	string_changed_member_function_sptr_map;

/// Convenience typedef for a hash map of strings  and member functions.
typedef unordered_map<string,
		      class_decl::method_decl_sptr>
string_member_function_sptr_map;

/// Convenience typedef for a map which key is a string and which
/// value is a point to @ref var_decl.
typedef unordered_map<string, var_decl*> string_var_ptr_map;

/// Convenience typedef for a pair of pointer to @ref var_decl
/// representing a @ref var_decl change.  The first member of the pair
/// represents the initial variable and the second member represents
/// the changed variable.
typedef std::pair<var_decl*, var_decl*> changed_var_ptr;

/// Convenience typedef for a map which key is a stirng and which
/// value is a @ref changed_var_ptr.
typedef unordered_map<string, changed_var_ptr> string_changed_var_ptr_map;

class diff_context;

/// Convenience typedef for a shared pointer of @ref diff_context.
typedef shared_ptr<diff_context> diff_context_sptr;

class diff_node_visitor;

struct diff_traversable_base;

/// Convenience typedef for shared_ptr on diff_traversable_base.
typedef shared_ptr<diff_traversable_base> diff_traversable_base_sptr;

/// An enum for the different ways to visit a diff tree node.
///
/// This is used by the node traversing code to know when to invoke a
/// visitor on a diff tree node.
enum visiting_kind
{
  /// The default enumerator value of this enum.  It doesn't have any
  /// particular meaning yet.
  NO_VISITING_KIND = 0,
  /// This says that a visitor should be invoked on a tree node
  /// *before* visiting the node's children.
  PRE_VISITING_KIND = 1,
  /// This says that a visotor should be invoked on a tree node
  /// *after* visiting the node's children.
  POST_VISITING_KIND =  1 << 1
};

visiting_kind
operator|(visiting_kind l, visiting_kind r);

///  The base class for the diff classes that are to be traversed.
class diff_traversable_base : public traversable_base
{
public:

  virtual bool
  traverse(diff_node_visitor& v);
}; // end struct diff_traversable_base

/// An enum for the different categories that a diff tree node falls
/// into, regarding the kind of changes it represents.
enum diff_category
{
  /// This means the diff node does not carry any (meaningful) change.
  NO_CHANGE_CATEGORY = 0,
  /// This means the diff node (or at least one of its descendant
  /// nodes) carries access related changes, e.g, a private member
  /// that becomes public.
  ACCESS_CHANGE_CATEGORY = 1,
  /// This means the diff node (or at least one of its descendant
  /// nodes) carries a change that modifies the size of a type or an
  /// offset of a type member.
  SIZE_OR_OFFSET_CHANGE_CATEGORY = 1 << 1,

  /// A special enumerator that is the logical 'or' all the
  /// enumerators above.
  ///
  /// This one must stay the last enumerator.  Please update it each
  /// time you add a new enumerator above.
  EVERYTHING_CATEGORY =
  ACCESS_CHANGE_CATEGORY
  | SIZE_OR_OFFSET_CHANGE_CATEGORY
};

diff_category
operator|(diff_category c1, diff_category c2);

diff_category
operator^(diff_category c1, diff_category c2);

diff_category
operator&(diff_category c1, diff_category c2);

diff_category
operator~(diff_category c);

/// The context of the diff.  This type holds various bits of
/// information that is going to be used throughout the diffing of two
/// entities and the reporting that follows.
class diff_context
{
  struct priv;
  shared_ptr<priv> priv_;

public:
  diff_context();

  diff_sptr
  has_diff_for(const decl_base_sptr first,
	       const decl_base_sptr second) const;

  diff_sptr
  has_diff_for_types(const type_base_sptr first,
		     const type_base_sptr second) const;

  diff_sptr
  has_diff_for(const diff_sptr d) const;

  void
  add_diff(const decl_base_sptr first,
	   const decl_base_sptr second,
	   diff_sptr d);

  diff_category
  get_allowed_category() const;

  void
  set_allowed_category(diff_category c);

  void
  switch_categories_on(diff_category c);

  void
  switch_categories_off(diff_category c);

  const filtering::filters&
  diff_filters() const;

  void
  add_diff_filter(filtering::filter_base_sptr);

  void
  maybe_apply_filters(diff_sptr dif);

  void
  show_stats_only(bool f);

  bool
  show_stats_only() const;

  void
  show_deleted_fns(bool f);

  bool
  show_deleted_fns() const;

  void
  show_changed_fns(bool f);

  bool
  show_changed_fns() const;

  void
  show_added_fns(bool f);

  bool
  show_added_fns() const;

  void
  show_deleted_vars(bool f);

  bool
  show_deleted_vars() const;

  void
  show_changed_vars(bool f);

  bool
  show_changed_vars() const;

  void
  show_added_vars(bool f);

  bool
  show_added_vars() const;
};//end struct diff_context.

/// This type encapsulates an edit script (a set of insertions and
/// deletions) for two constructs that are to be diff'ed.  The two
/// constructs are called the "subjects" of the diff.
class diff : public diff_traversable_base
{
  diff*		parent_;
  decl_base_sptr	first_subject_;
  decl_base_sptr	second_subject_;
  diff_context_sptr	ctxt_;
  diff_category	category_;
  mutable bool		reported_once_;
  mutable bool		currently_reporting_;

  // Forbidden
  diff();

protected:
  diff(decl_base_sptr first_subject,
       decl_base_sptr second_subject)
    : parent_(0),
      first_subject_(first_subject),
      second_subject_(second_subject),
      category_(NO_CHANGE_CATEGORY),
      reported_once_(false),
      currently_reporting_(false)
  {}

diff(decl_base_sptr	first_subject,
     decl_base_sptr	second_subject,
     diff_context_sptr	ctxt)
    : parent_(0),
      first_subject_(first_subject),
      second_subject_(second_subject),
      ctxt_(ctxt),
      category_(NO_CHANGE_CATEGORY),
      reported_once_(false),
      currently_reporting_(false)
  {}

  diff(diff*			parent,
       decl_base_sptr		first_subject,
       decl_base_sptr		second_subject,
       diff_context_sptr	ctxt)
    : parent_(parent),
      first_subject_(first_subject),
      second_subject_(second_subject),
      ctxt_(ctxt),
      category_(NO_CHANGE_CATEGORY),
      reported_once_(false),
      currently_reporting_(false)
  {}

public:

  /// Getter of the parent diff node of this one.
  ///
  /// Return the parent diff node if any, null otherwise.
  diff*
  get_parent() const
  {return parent_;}

  /// Setter for the parent diff node of this one.
  ///
  /// @param parent the parent diff node.
  void
  set_parent(diff* parent)
  {parent_ = parent;}

  /// Getter of the first subject of the diff.
  ///
  /// @return the first subject of the diff.
  decl_base_sptr
  first_subject() const
  {return first_subject_;}

  /// Getter of the second subject of the diff.
  ///
  /// @return the second subject of the diff.
  decl_base_sptr
  second_subject() const
  {return second_subject_;}

  /// Getter of the context of the current diff.
  ///
  /// @return the context of the current diff.
  const diff_context_sptr
  context() const
  {return ctxt_;}

  /// Setter of the context of the current diff.
  ///
  /// @param c the new context to set.
  void
  context(diff_context_sptr c)
  {ctxt_ = c;}

  /// Tests if we are currently in the middle of emitting a report for
  /// this diff.
  ///
  /// @return true if we are currently emitting a report for the
  /// current diff, false otherwise.
  bool
  currently_reporting() const
  {return currently_reporting_;}

  /// Sets a flag saying if we are currently in the middle of emitting
  /// a report for this diff.
  ///
  /// @param f true if we are currently emitting a report for the
  /// current diff, false otherwise.
  void
  currently_reporting(bool f) const
  {currently_reporting_ = f;}

  /// Tests if a report has already been emitted for the current diff.
  ///
  /// @return true if a report has already been emitted for the
  /// current diff, false otherwise.
  bool
  reported_once() const
  {return reported_once_;}

  /// Sets a flag saying if a report has already been emitted for the
  /// current diff.
  ///
  /// @param f true if a repot has already been emitted for the
  /// current diff, false otherwise.
  void
  reported_once(bool f) const
  {reported_once_ = f;}

  /// Getter for the category of the current diff tree node.
  ///
  /// @return the category of the current diff tree node.
  diff_category
  get_category() const
  {return category_;}

  /// Adds the current diff tree node to an additional set of
  /// categories.
  ///
  /// @param c a bit-map representing the set of categories to add the
  /// current diff tree node to.
  ///
  /// @return the resulting bit-map representing the categories this
  /// current diff tree node belongs to.
  diff_category
  add_to_category(diff_category c)
  {
    category_ = category_ | c;
    return category_;
  }

  bool
  is_filtered_out() const;

  bool
  to_be_reported() const;

  /// Pure interface to get the length of the changes
  /// encapsulated by this diff.  This is to be implemented by all
  /// descendants of this class.
  virtual unsigned
  length() const = 0;

  /// Pure interface to report the diff in a serialized form that is
  /// legible for the user.
  ///
  /// Note that the serializd report has to leave one empty line at
  /// the end of its content.
  ///
  /// @param out the output stream to serialize the report to.
  ///
  /// @param indent the indentation string to use.
  virtual void
  report(ostream& out, const string& indent = "") const = 0;
};// end class diff

diff_sptr
compute_diff(const decl_base_sptr,
	     const decl_base_sptr,
	     diff_context_sptr ctxt);

diff_sptr
compute_diff(const type_base_sptr,
	     const type_base_sptr,
	     diff_context_sptr ctxt);

class distinct_diff;

/// Convenience typedef for a shared pointer to distinct_types_diff
typedef shared_ptr<distinct_diff> distinct_diff_sptr;

/// An abstraction of a diff between entities that are of a different
/// kind (disctinct).
class distinct_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  distinct_diff(decl_base_sptr first,
		decl_base_sptr second,
		diff_context_sptr ctxt = diff_context_sptr());
public:

  const decl_base_sptr
  first() const;

  const decl_base_sptr
  second() const;

  virtual unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  static bool
  entities_are_of_distinct_kinds(decl_base_sptr first,
				 decl_base_sptr second);

  friend distinct_diff_sptr
  compute_diff_for_distinct_kinds(const decl_base_sptr first,
				  const decl_base_sptr second,
				  diff_context_sptr ctxt);

  virtual bool
  traverse(diff_node_visitor& v);
};// end class distinct_types_diff

distinct_diff_sptr
compute_diff_for_distinct_kinds(const decl_base_sptr,
				const decl_base_sptr,
				diff_context_sptr ctxt);

class var_diff;

/// Convenience typedef for a shared pointer to var_diff.
typedef shared_ptr<var_diff> var_diff_sptr;

/// Abstracts a diff between two instances of @ref var_decl
class var_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  var_diff(var_decl_sptr first,
	   var_decl_sptr second,
	   diff_sptr type_diff,
	   diff_context_sptr ctxt = diff_context_sptr());

public:
  var_decl_sptr
  first_var() const;

  var_decl_sptr
  second_var() const;

  diff_sptr
  type_diff() const;

  virtual unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend var_diff_sptr
  compute_diff(const var_decl_sptr	first,
	       const var_decl_sptr	second,
	       diff_context_sptr	ctxt);
};// end class var_diff

var_diff_sptr
compute_diff(const var_decl_sptr, const var_decl_sptr, diff_context_sptr);

class pointer_diff;
/// Convenience typedef for a shared pointer on a @ref
/// pointer_diff type.
typedef shared_ptr<pointer_diff> pointer_diff_sptr;

/// The abstraction of a diff between two pointers.
class pointer_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  pointer_diff(pointer_type_def_sptr	first,
	       pointer_type_def_sptr	second,
	       diff_context_sptr	ctxt = diff_context_sptr());

public:
  const pointer_type_def_sptr
  first_pointer() const;

  const pointer_type_def_sptr
  second_pointer() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend pointer_diff_sptr
  compute_diff(pointer_type_def_sptr	first,
	       pointer_type_def_sptr	second,
	       diff_context_sptr	ctxt);
};// end class pointer_diff

pointer_diff_sptr
compute_diff(pointer_type_def_sptr first,
	     pointer_type_def_sptr second,
	     diff_context_sptr ctxt);

class reference_diff;

/// Convenience typedef for a shared pointer on a @ref
/// reference_diff type.
typedef shared_ptr<reference_diff> reference_diff_sptr;

/// The abstraction of a diff between two references.
class reference_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  reference_diff(const reference_type_def_sptr	first,
		 const reference_type_def_sptr	second,
		 diff_context_sptr		ctxt = diff_context_sptr());

public:
  reference_type_def_sptr
  first_reference() const;

  reference_type_def_sptr
  second_reference() const;

  const diff_sptr&
  underlying_type_diff() const;

  diff_sptr&
  underlying_type_diff(diff_sptr);

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend reference_diff_sptr
  compute_diff(reference_type_def_sptr first,
	       reference_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class reference_diff

reference_diff_sptr
compute_diff(reference_type_def_sptr first,
	     reference_type_def_sptr second,
	     diff_context_sptr ctxt);

class qualified_type_diff;
typedef class shared_ptr<qualified_type_diff> qualified_type_diff_sptr;

/// Abstraction of a diff between two qualified types.
class qualified_type_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  qualified_type_diff(qualified_type_def_sptr	first,
		      qualified_type_def_sptr	second,
		      diff_context_sptr	ctxt = diff_context_sptr());

public:
  const qualified_type_def_sptr
  first_qualified_type() const;

  const qualified_type_def_sptr
  second_qualified_type() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend qualified_type_diff_sptr
  compute_diff(const qualified_type_def_sptr first,
	       const qualified_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class qualified_type_diff.

qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr first,
	     const qualified_type_def_sptr second,
	     diff_context_sptr ctxt);

class enum_diff;
typedef shared_ptr<enum_diff> enum_diff_sptr;

/// Abstraction of a diff between two enums.
class enum_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  void
  clear_lookup_tables();

  bool
  lookup_tables_empty() const;

  void
  ensure_lookup_tables_populated();

protected:
  enum_diff(const enum_type_decl_sptr,
	    const enum_type_decl_sptr,
	    const diff_sptr,
	    diff_context_sptr ctxt = diff_context_sptr());

public:
  const enum_type_decl_sptr
  first_enum() const;

  const enum_type_decl_sptr
  second_enum() const;

  diff_sptr
  underlying_type_diff() const;

  const string_enumerator_map&
  deleted_enumerators() const;

  const string_enumerator_map&
  inserted_enumerators() const;

  const string_changed_enumerator_map&
  changed_enumerators() const;

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend enum_diff_sptr
  compute_diff(const enum_type_decl_sptr first,
	       const enum_type_decl_sptr second,
	       diff_context_sptr ctxt);
};//end class enum_diff;

enum_diff_sptr
compute_diff(const enum_type_decl_sptr,
	     const enum_type_decl_sptr,
	     diff_context_sptr);

class class_diff;

/// Convenience typedef for a shared pointer on a @ref class_diff type.
typedef shared_ptr<class_diff> class_diff_sptr;

/// This type abstracts changes for a class_decl.
class class_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

  void
  clear_lookup_tables(void);

  bool
  lookup_tables_empty(void) const;

  void
  ensure_lookup_tables_populated(void) const;

protected:
  class_diff(class_decl_sptr first_scope,
	     class_decl_sptr second_scope,
	     diff_context_sptr ctxt = diff_context_sptr());

public:
  //TODO: add change of the name of the type.

  shared_ptr<class_decl>
  first_class_decl() const;

  shared_ptr<class_decl>
  second_class_decl() const;

  const edit_script&
  base_changes() const;

  edit_script&
  base_changes();

  const edit_script&
  member_types_changes() const;

  edit_script&
  member_types_changes();

  const edit_script&
  data_members_changes() const;

  edit_script&
  data_members_changes();

  const edit_script&
  member_fns_changes() const;

  edit_script&
  member_fns_changes();

  const edit_script&
  member_fn_tmpls_changes() const;

  edit_script&
  member_fn_tmpls_changes();

  const edit_script&
  member_class_tmpls_changes() const;

  edit_script&
  member_class_tmpls_changes();

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second,
	       diff_context_sptr	ctxt);
};// end class_diff

class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt);

class base_diff;

/// Convenience typedef for a shared pointer to base_diff.
typedef shared_ptr<base_diff> base_diff_sptr;

/// An abstraction of a diff between two instances of base_diff.
class base_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  base_diff(class_decl::base_spec_sptr	first,
	    class_decl::base_spec_sptr	second,
	    diff_context_sptr		ctxt = diff_context_sptr());

public:
  class_decl::base_spec_sptr
  first_base() const;

  class_decl::base_spec_sptr
  second_base() const;

  const class_diff_sptr
  get_underlying_class_diff() const;

  void
  set_underlying_class_diff(class_diff_sptr d);

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend base_diff_sptr
  compute_diff(const class_decl::base_spec_sptr first,
	       const class_decl::base_spec_sptr second,
	       diff_context_sptr		ctxt);
};// end class base_diff

base_diff_sptr
compute_diff(const class_decl::base_spec_sptr first,
	     const class_decl::base_spec_sptr second,
	     diff_context_sptr		ctxt);

class scope_diff;

/// Convenience typedef for a shared pointer on a @ref scope_diff.
typedef shared_ptr<scope_diff> scope_diff_sptr;

/// An abstractions of the changes between two scopes.
class scope_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

  bool
  lookup_tables_empty() const;

  void
  clear_lookup_tables();

  void
  ensure_lookup_tables_populated();

protected:
  scope_diff(scope_decl_sptr first_scope,
	     scope_decl_sptr second_scope,
	     diff_context_sptr ctxt = diff_context_sptr());

public:

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr	first,
	       const scope_decl_sptr	second,
	       scope_diff_sptr		d,
	       diff_context_sptr	ctxt);

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr	first_scope,
	       const scope_decl_sptr	second_scope,
	       diff_context_sptr	ctxt);

  const scope_decl_sptr
  first_scope() const;

  const scope_decl_sptr
  second_scope() const;

  const edit_script&
  member_changes() const;

  edit_script&
  member_changes();

  const decl_base_sptr
  deleted_member_at(unsigned index) const;

  const decl_base_sptr
  deleted_member_at(vector<deletion>::const_iterator) const;

  const decl_base_sptr
  inserted_member_at(unsigned i);

  const decl_base_sptr
  inserted_member_at(vector<unsigned>::const_iterator i);

  const string_changed_type_or_decl_map&
  changed_types() const;

  const string_changed_type_or_decl_map&
  changed_decls() const;

  const string_decl_base_sptr_map&
  removed_types() const;

  const string_decl_base_sptr_map&
  removed_decls() const;

  const string_decl_base_sptr_map&
  added_types() const;

  const string_decl_base_sptr_map&
  added_decls() const;

  virtual unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);
};// end class scope_diff

scope_diff_sptr
compute_diff(const scope_decl_sptr first,
	     const scope_decl_sptr second,
	     scope_diff_sptr d,
	     diff_context_sptr ctxt);

scope_diff_sptr
compute_diff(const scope_decl_sptr first_scope,
	     const scope_decl_sptr second_scope,
	     diff_context_sptr ctxt);

class function_decl_diff;

/// Convenience typedef for a shared pointer on a @ref function_decl type.
typedef shared_ptr<function_decl_diff> function_decl_diff_sptr;

/// Abstraction of a diff between two function_decl.
class function_decl_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

  void
  ensure_lookup_tables_populated();

  const function_decl::parameter_sptr
  deleted_parameter_at(int i) const;

  const function_decl::parameter_sptr
  inserted_parameter_at(int i) const;

protected:
  function_decl_diff(const function_decl_sptr	first,
		     const function_decl_sptr	second,
		     diff_context_sptr		ctxt);

public:
friend function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

  const function_decl_sptr
  first_function_decl() const;

  const function_decl_sptr
  second_function_decl() const;

  const diff_sptr
  return_type_diff() const;

  const string_changed_parm_map&
  changed_parms() const;

  const string_parm_map&
  removed_parms() const;

  const string_parm_map&
  added_parms() const;

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);
}; // end class function_decl_diff

function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

class type_decl_diff;

/// Convenience typedef for a shared pointer on a @ref type_decl_diff type.
typedef shared_ptr<type_decl_diff> type_decl_diff_sptr;

/// Abstraction of a diff between two basic type declarations.
class type_decl_diff : public diff
{
  type_decl_diff();

protected:
  type_decl_diff(const type_decl_sptr first,
		 const type_decl_sptr second,
		 diff_context_sptr ctxt = diff_context_sptr());

public:
  friend type_decl_diff_sptr
  compute_diff(const type_decl_sptr	first,
	       const type_decl_sptr	second,
	       diff_context_sptr	ctxt);

  const type_decl_sptr
  first_type_decl() const;

  const type_decl_sptr
  second_type_decl() const;

  unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);
};// end type_decl_diff

type_decl_diff_sptr
compute_diff(const type_decl_sptr,
	     const type_decl_sptr,
	     diff_context_sptr);

class typedef_diff;

/// Convenience typedef for a shared pointer on a typedef_diff type.
typedef shared_ptr<typedef_diff> typedef_diff_sptr;

/// Abstraction of a diff between two typedef_decl.
class typedef_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

  typedef_diff();

protected:
  typedef_diff(const typedef_decl_sptr	first,
	       const typedef_decl_sptr	second,
	       diff_context_sptr	ctxt = diff_context_sptr());

public:
  friend typedef_diff_sptr
  compute_diff(const typedef_decl_sptr	first,
	       const typedef_decl_sptr	second,
	       diff_context_sptr	ctxt);

  const typedef_decl_sptr
  first_typedef_decl() const;

  const typedef_decl_sptr
  second_typedef_decl() const;

  const diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual unsigned
  length() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);
};// end class typedef_diff

typedef_diff_sptr
compute_diff(const typedef_decl_sptr,
	     const typedef_decl_sptr,
	     diff_context_sptr ctxt);

class translation_unit_diff;

/// Convenience typedef for a shared pointer on a
/// @ref translation_unit_diff type.
typedef shared_ptr<translation_unit_diff> translation_unit_diff_sptr;

/// An abstraction of a diff between two translation units.
class translation_unit_diff : public scope_diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  translation_unit_diff(translation_unit_sptr	first,
			translation_unit_sptr	second,
			diff_context_sptr	ctxt = diff_context_sptr());

public:

  const translation_unit_sptr
  first_translation_unit() const;

  const translation_unit_sptr
  second_translation_unit() const;

  friend translation_unit_diff_sptr
  compute_diff(const translation_unit_sptr	first,
	       const translation_unit_sptr	second,
	       diff_context_sptr		ctxt);

  virtual unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);
};//end class translation_unit_diff

translation_unit_diff_sptr
compute_diff(const translation_unit_sptr first,
	     const translation_unit_sptr second,
	     diff_context_sptr ctxt = diff_context_sptr());

class corpus_diff;

/// A convenience typedef for a shared pointer to @ref corpus_diff.
typedef shared_ptr<corpus_diff> corpus_diff_sptr;

/// An abstraction of a diff between between two abi corpus.
class corpus_diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  corpus_diff(corpus_sptr	first,
	      corpus_sptr	second,
	      diff_context_sptr ctxt = diff_context_sptr());

public:

  corpus_sptr
  first_corpus() const;

  corpus_sptr
  second_corpus() const;

  edit_script&
  function_changes() const;

  edit_script&
  variable_changes() const;

  const string_function_ptr_map&
  deleted_functions() const;

  const string_function_ptr_map&
  added_functions();

  const string_changed_function_ptr_map&
  changed_functions();

  const diff_context_sptr
  context() const;

  unsigned
  length() const;

  void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  friend corpus_diff_sptr
  compute_diff(const corpus_sptr f,
	       const corpus_sptr s,
	       diff_context_sptr ctxt = diff_context_sptr());
}; // end class corpus_diff

corpus_diff_sptr
compute_diff(const corpus_sptr,
	     const corpus_sptr,
	     diff_context_sptr);

/// The base class for the node visitors.  These are the types used to
/// visit each node traversed by the diff_traversable_base::traverse() method.
class diff_node_visitor : public node_visitor_base
{
protected:
  visiting_kind visiting_kind_;

public:

  diff_node_visitor()
    : visiting_kind_(PRE_VISITING_KIND)
  {}

  /// Getter for the visiting policy of the traversing code while
  /// invoking this visitor.
  ///
  /// @return the visiting policy used by the traversing code when
  /// invoking this visitor.
  visiting_kind
  get_visiting_kind() const
  {return visiting_kind_;}

  /// Setter for the visiting policy of the traversing code while
  /// invoking this visitor.
  ///
  /// @param v a bit map representing the new visiting policy used by
  /// the traversing code when invoking this visitor.
  void
  set_visiting_kind(visiting_kind v)
  {visiting_kind_ = v;}

  /// Setter for the visiting policy of the traversing code while
  /// invoking this visitor.  This one makes a logical or between the
  /// current policy and the bitmap given in argument and assigns the
  /// current policy to the result.
  ///
  /// @param v a bitmap representing the visiting policy to or with
  /// the current policy.
  void
  or_visiting_kind(visiting_kind v)
  {visiting_kind_ = visiting_kind_ | v;}

  virtual bool
  visit(diff*, bool);

  virtual bool
  visit(distinct_diff*, bool);

  virtual bool
  visit(var_diff*, bool);

  virtual bool
  visit(pointer_diff*, bool);

  virtual bool
  visit(reference_diff*, bool);

  virtual bool
  visit(qualified_type_diff*, bool);

  virtual bool
  visit(enum_diff*, bool);

  virtual bool
  visit(class_diff*, bool);

  virtual bool
  visit(base_diff*, bool);

  virtual bool
  visit(scope_diff*, bool);

  virtual bool
  visit(function_decl_diff*, bool);

  virtual bool
  visit(type_decl_diff*, bool);

  virtual bool
  visit(typedef_diff*, bool);

  virtual bool
  visit(translation_unit_diff*, bool);

  virtual bool
  visit(corpus_diff*, bool);
}; // end struct diff_node_visitor
}// end namespace comparison

}// end namespace abigail

#endif //__ABG_COMPARISON_H__
