// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2015 Red Hat, Inc.
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
#include <ostream>
#include "abg-corpus.h"
#include "abg-diff-utils.h"
#include "abg-ini.h"

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

/// Convenience typedef for a vector of @ref diff_sptr.
typedef vector<diff_sptr> diff_sptrs_type;

class decl_diff_base;

/// Convenience typedef for a shared_ptr of @ref decl_diff_base.
typedef shared_ptr<decl_diff_base> decl_diff_base_sptr;

/// Convenience typedef for a vector of @ref decl_diff_base_sptr.
typedef vector<decl_diff_base_sptr> decl_diff_base_sptrs_type;

class type_diff_base;
/// Convenience pointer for a shared pointer to a type_diff_base
typedef shared_ptr<type_diff_base> type_diff_base_sptr;

/// Convenience typedef for a vector of @ref type_diff_base_sptr
typedef vector<type_diff_base_sptr> type_diff_base_sptrs_type;

class function_decl_diff;

/// Convenience typedef for a shared pointer to a @ref function_decl type.
typedef shared_ptr<function_decl_diff> function_decl_diff_sptr;

/// Convenience typedef for a vector of @ref function_decl_diff_sptr
typedef vector<function_decl_diff_sptr> function_decl_diff_sptrs_type;

class fn_parm_diff;

/// Convenience typedef for a shared pointer to a @ref fn_parm_diff
/// type.
typedef shared_ptr<fn_parm_diff> fn_parm_diff_sptr;

class var_diff;

/// Convenience typedef for a shared pointer to a @ref var_diff type.
typedef shared_ptr<var_diff> var_diff_sptr;

/// Convenience typedef for a vector of @ref var_diff_sptr.
typedef vector<var_diff_sptr> var_diff_sptrs_type;

class base_diff;

/// Convenience typedef for a shared pointer to a @ref base_diff type.
typedef shared_ptr<base_diff> base_diff_sptr;

/// Convenience typedef for a vector of @ref base_diff_sptr.
typedef vector<base_diff_sptr> base_diff_sptrs_type;

class class_diff;

/// Convenience typedef for a shared pointer on a @ref class_diff type.
typedef shared_ptr<class_diff> class_diff_sptr;

/// Convenience typedef for a map of pointer values.  The Key is a
/// pointer value and the value is potentially another pointer value
/// associated to the first one.
typedef unordered_map<size_t, size_t> pointer_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref decl_base_sptr.
typedef unordered_map<string, decl_base_sptr> string_decl_base_sptr_map;

/// Convenience typedef for a map which key is an unsigned integer and
/// which value is a @ref decl_base_sptr
typedef unordered_map<unsigned, decl_base_sptr> unsigned_decl_base_sptr_map;

/// Convenience typedef for a map of string and class_decl::basse_spec_sptr.
typedef unordered_map<string, class_decl::base_spec_sptr> string_base_sptr_map;

/// Convenience typedef for a map of string and @ref base_diff_sptr.
typedef unordered_map<string, base_diff_sptr> string_base_diff_sptr_map;

/// Convenience typedef for a map which value is a changed function
/// parameter and which key is the name of the function parameter.
typedef unordered_map<string, fn_parm_diff_sptr> string_fn_parm_diff_sptr_map;

/// Convenience typedef for a map which key is an integer and which
/// value is a changed parameter.
typedef unordered_map<unsigned, fn_parm_diff_sptr>
unsigned_fn_parm_diff_sptr_map;

/// Convenience typedef for a map which key is an integer and which
/// value is a parameter.
typedef unordered_map<unsigned,
		      function_decl::parameter_sptr> unsigned_parm_map;

/// Convenience typedef for a map which value is a
/// type_diff_base_sptr.  The key of the map is the qualified name of
/// the changed type.
typedef unordered_map<string,
		      type_diff_base_sptr> string_type_diff_base_sptr_map;

/// Convenience typedef for a map which value is a
/// decl_diff_base_sptr.  The key of the map is the qualified name of
/// the changed type.
typedef unordered_map<string,
		      decl_diff_base_sptr> string_decl_diff_base_sptr_map;

/// Convenience typedef for a map which value is a diff_sptr.  The key
/// of the map is the qualified name of the changed type.
typedef unordered_map<string, diff_sptr> string_diff_sptr_map;

/// Convenience typedef for a map whose key is a string and whose
/// value is a changed variable of type @ref var_diff_sptr.
typedef unordered_map<string,
		      var_diff_sptr> string_var_diff_sptr_map;


/// Convenience typedef for a map whose key is an unsigned int and
/// whose value is a changed variable of type @ref var_diff_sptr.
typedef unordered_map<unsigned, var_diff_sptr> unsigned_var_diff_sptr_map;

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

/// Convenience typedef for a vector of changed enumerators.
typedef vector<changed_enumerator> changed_enumerators_type;

/// Convenience typedef for a map which value is a changed enumerator.
/// The key is the name of the changed enumerator.
typedef unordered_map<string, changed_enumerator> string_changed_enumerator_map;

/// Convenience typedef for a map which key is a string and which
/// value is a pointer to @ref decl_base.
typedef unordered_map<string, function_decl*> string_function_ptr_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref function_decl_diff_sptr.
typedef unordered_map<string,
		      function_decl_diff_sptr>
				string_function_decl_diff_sptr_map;

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

/// Convenience typedef for a map whose key is a string and whose
/// value is an @ref elf_symbol_sptr.
typedef unordered_map<string, elf_symbol_sptr> string_elf_symbol_map;

/// Convenience typedef for a map which key is a string and which
/// value is a @ref var_diff_sptr.
typedef unordered_map<string, var_diff_sptr> string_var_diff_ptr_map;

class diff_context;

/// Convenience typedef for a shared pointer of @ref diff_context.
typedef shared_ptr<diff_context> diff_context_sptr;

/// Convenience typedef for a weak pointer of @ref diff_context.
typedef weak_ptr<diff_context> diff_context_wptr;

class diff_node_visitor;

struct diff_traversable_base;

/// Convenience typedef for shared_ptr on diff_traversable_base.
typedef shared_ptr<diff_traversable_base> diff_traversable_base_sptr;

/// An enum for the different ways to visit a diff tree node.
///
/// This is used by the node traversing code, to know when to avoid
/// visiting children nodes, for instance.
enum visiting_kind
{
  /// The default enumerator value of this enum.  It doesn't have any
  /// particular meaning yet.
  DEFAULT_VISITING_KIND = 0,

  /// This says that the traversing code should avoid visiting the
  /// children nodes of the current node being visited.
  SKIP_CHILDREN_VISITING_KIND = 1,

  /// This says that the traversing code should not mark visited nodes
  /// as having been traversed.  This is useful, for instance, for
  /// visitors which have debugging purposes.
  DO_NOT_MARK_VISITED_NODES_AS_VISITED = 1 << 1
};

visiting_kind
operator|(visiting_kind l, visiting_kind r);

visiting_kind
operator&(visiting_kind l, visiting_kind r);

visiting_kind
operator~(visiting_kind l);

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
  /// This means the diff node does not carry any (meaningful) change,
  /// or that it carries changes that have not yet been categorized.
  NO_CHANGE_CATEGORY = 0,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries access related changes, e.g, a private member
  /// that becomes public.
  ACCESS_CHANGE_CATEGORY = 1,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries a change involving two compatible types.  For
  /// instance a type and its typedefs.
  COMPATIBLE_TYPE_CHANGE_CATEGORY = 1 << 1,

  /// This means that a diff node in the sub-tree carries a harmless
  /// declaration name change.  This is set only for name changes for
  /// data members and typedefs.
  HARMLESS_DECL_NAME_CHANGE_CATEGORY = 1 << 2,

  /// This means that a diff node in the sub-tree carries an addition
  /// or removal of a non-virtual member function.
  NON_VIRT_MEM_FUN_CHANGE_CATEGORY = 1 << 3,

  /// This means that a diff node in the sub-tree carries an addition
  /// or removal of a static data member.
  STATIC_DATA_MEMBER_CHANGE_CATEGORY = 1 << 4,

  /// This means that a diff node in the sub-tree carries an addition
  /// of enumerator to an enum type.
  HARMLESS_ENUM_CHANGE_CATEGORY = 1 << 5,

  /// This means that a diff node in the sub-tree carries an a symbol
  /// alias change that is harmless.
  HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY = 1 << 6,

  /// This means that a diff node was marked as suppressed by a
  /// user-provided suppression specification.
  SUPPRESSED_CATEGORY = 1 << 7,

  /// This means the diff node (or at least one of its descendant
  /// nodes) carries a change that modifies the size of a type or an
  /// offset of a type member.  Removal or changes of enumerators in a
  /// enum fall in this category too.
  SIZE_OR_OFFSET_CHANGE_CATEGORY = 1 << 8,

  /// This means that a diff node in the sub-tree carries a change to
  /// a vtable.
  VIRTUAL_MEMBER_CHANGE_CATEGORY = 1 << 9,

  /// A diff node in this category is redundant.  That means it's
  /// present as a child of a other nodes in the diff tree.
  REDUNDANT_CATEGORY = 1 << 10,

  /// A special enumerator that is the logical 'or' all the
  /// enumerators above.
  ///
  /// This one must stay the last enumerator.  Please update it each
  /// time you add a new enumerator above.
  EVERYTHING_CATEGORY =
  ACCESS_CHANGE_CATEGORY
  | COMPATIBLE_TYPE_CHANGE_CATEGORY
  | HARMLESS_DECL_NAME_CHANGE_CATEGORY
  | NON_VIRT_MEM_FUN_CHANGE_CATEGORY
  | STATIC_DATA_MEMBER_CHANGE_CATEGORY
  | HARMLESS_ENUM_CHANGE_CATEGORY
  | HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY
  | SUPPRESSED_CATEGORY
  | SIZE_OR_OFFSET_CHANGE_CATEGORY
  | VIRTUAL_MEMBER_CHANGE_CATEGORY
  | REDUNDANT_CATEGORY
};

diff_category
operator|(diff_category c1, diff_category c2);

diff_category&
operator|=(diff_category& c1, diff_category c2);

diff_category&
operator&=(diff_category& c1, diff_category c2);

diff_category
operator^(diff_category c1, diff_category c2);

diff_category
operator&(diff_category c1, diff_category c2);

diff_category
operator~(diff_category c);

ostream&
operator<<(ostream& o, diff_category);

class corpus_diff;

/// A convenience typedef for a shared pointer to @ref corpus_diff.
typedef shared_ptr<corpus_diff> corpus_diff_sptr;
class suppression_base;

/// Convenience typedef for a shared pointer to a @ref suppression.
typedef shared_ptr<suppression_base> suppression_sptr;

/// Convenience typedef for a vector of @ref suppression_sptr
typedef vector<suppression_sptr> suppressions_type;

/// Base type of the suppression specifications types.
///
/// This abstracts a suppression specification.  It's a way to specify
/// how to drop reports about a particular diff node on the floor, if
/// it matches the supppression specification.
class suppression_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  // Forbid default constructor
  suppression_base();

protected:
  priv_sptr priv_;

public:
  suppression_base(const string& label);

  const string
  get_label() const;

  void
  set_label(const string&);

  void
  set_file_name_regex_str(const string& regexp);

  const string&
  get_file_name_regex_str() const;

  void
  set_soname_regex_str(const string& regexp);

  const string&
  get_soname_regex_str() const;

  virtual bool
  suppresses_diff(const diff*) const = 0;

  virtual ~suppression_base();
}; // end class suppression_base

void
read_suppressions(std::istream& input,
		  suppressions_type& suppressions);

void
read_suppressions(const string& file_path,
		  suppressions_type& suppressions);

class type_suppression;

/// Convenience typedef for a shared pointer to type_suppression.
typedef shared_ptr<type_suppression> type_suppression_sptr;

/// Convenience typedef for vector of @ref type_suppression_sptr.
typedef vector<type_suppression_sptr> type_suppressions_type;

/// Abstraction of a type suppression specification.
///
/// Specifies under which condition reports about a type diff node
/// should be dropped on the floor.
class type_suppression : public suppression_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbid this;
  type_suppression();

public:

  /// The kind of the type the current type suppression is supposed to
  /// be about.
  enum type_kind
  {
    UNKNOWN_TYPE_KIND,
    CLASS_TYPE_KIND,
    STRUCT_TYPE_KIND,
    UNION_TYPE_KIND,
    ENUM_TYPE_KIND,
    ARRAY_TYPE_KIND,
    TYPEDEF_TYPE_KIND,
    BUILTIN_TYPE_KIND
  }; // end enum type_kind

  /// The different ways through which the type diff has been reached.
  enum reach_kind
  {
    /// The type diff has been reached (from a function or variable
    /// change) directly.
    DIRECT_REACH_KIND = 0,

    /// The type diff has been reached (from a function or variable
    /// change) through a pointer.
    POINTER_REACH_KIND,

    /// The type diff has been reached (from a function or variable
    /// change) through a reference; you know, like a c++ reference..
    REFERENCE_REACH_KIND,

    /// The type diff has been reached (from a function or variable
    /// change) through either a reference or a pointer.
    REFERENCE_OR_POINTER_REACH_KIND
  }; // end enum reach_kind

  class insertion_range;
  /// A convenience typedef for a shared pointer to @ref
  /// insertion_range.
  typedef shared_ptr<insertion_range> insertion_range_sptr;
  /// A convenience typedef for a vector of @ref insertion_range_sptr.
  typedef vector<insertion_range_sptr> insertion_ranges;

  type_suppression(const string& label,
		   const string& type_name_regexp,
		   const string& type_name);

  virtual ~type_suppression();

  void
  set_type_name_regex_str(const string& name_regex_str);

  const string&
  get_type_name_regex_str() const;

  void
  set_type_name(const string& name);

  const string&
  get_type_name() const;

  bool
  get_consider_type_kind() const;

  void
  set_consider_type_kind(bool f);

  void
  set_type_kind(type_kind k);

  type_kind
  get_type_kind() const;

  bool
  get_consider_reach_kind() const;

  void
  set_consider_reach_kind(bool f);

  reach_kind
  get_reach_kind() const;

  void
  set_reach_kind(reach_kind k);

  void
  set_data_member_insertion_ranges(const insertion_ranges& r);

  const insertion_ranges&
  get_data_member_insertion_ranges() const;

  insertion_ranges&
  get_data_member_insertion_ranges();

  const vector<string>&
  get_source_locations_to_keep() const;

  void
  set_source_locations_to_keep(const vector<string>&);

  const string&
  get_source_location_to_keep_regex_str() const;

  void
  set_source_location_to_keep_regex_str(const string&);

  virtual bool
  suppresses_diff(const diff* diff) const;

  bool
  suppresses_type(const type_base_sptr type,
		  const diff_context_sptr ctxt) const;
}; // end type_suppression

type_suppression_sptr
is_type_suppression(const suppression_sptr);

/// The abstraction of a range of offsets in which a member of a type
/// might get inserted.
class type_suppression::insertion_range
{
public:

  class boundary;
  class integer_boundary;
  class fn_call_expr_boundary;

  /// Convenience typedef for a shared_ptr to @ref boundary
  typedef shared_ptr<boundary> boundary_sptr;

  /// Convenience typedef for a shared_ptr to a @ref integer_boundary
  typedef shared_ptr<integer_boundary> integer_boundary_sptr;

  /// Convenience typedef for a shared_ptr to a @ref
  /// fn_call_expr_boundary
  typedef shared_ptr<fn_call_expr_boundary> fn_call_expr_boundary_sptr;

private:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  insertion_range();

  insertion_range(boundary_sptr begin, boundary_sptr end);

  boundary_sptr
  begin() const;

 boundary_sptr
  end() const;

  static insertion_range::integer_boundary_sptr
  create_integer_boundary(int value);

  static insertion_range::fn_call_expr_boundary_sptr
  create_fn_call_expr_boundary(ini::function_call_expr_sptr);

  static insertion_range::fn_call_expr_boundary_sptr
  create_fn_call_expr_boundary(const string&);

  static bool
  eval_boundary(boundary_sptr	boundary,
		class_decl_sptr context,
		ssize_t&	value);
}; // end class insertion_range

type_suppression::insertion_range::integer_boundary_sptr
is_integer_boundary(type_suppression::insertion_range::boundary_sptr);

type_suppression::insertion_range::fn_call_expr_boundary_sptr
is_fn_call_expr_boundary(type_suppression::insertion_range::boundary_sptr);

/// The abstraction of the boundary of an @ref insertion_range, in the
/// context of a @ref type_suppression
class type_suppression::insertion_range::boundary
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  boundary();
  virtual ~boundary();
};// end class type_suppression::insertion_range::boundary

/// An @ref insertion_range boundary that is expressed as an integer
/// value.  That integer value is usually a bit offset.
class type_suppression::insertion_range::integer_boundary
  : public type_suppression::insertion_range::boundary
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  integer_boundary();

public:
  integer_boundary(int value);
  int as_integer() const;
  operator int() const;
  ~integer_boundary();
}; //end class type_suppression::insertion_range::integer_boundary

/// An @ref insertion_range boundary that is expressed as function
/// call expression.  The (integer) value of that expression is
/// usually a bit offset.
class type_suppression::insertion_range::fn_call_expr_boundary
  : public type_suppression::insertion_range::boundary
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  fn_call_expr_boundary();

public:
  fn_call_expr_boundary(ini::function_call_expr_sptr expr);
  ini::function_call_expr_sptr as_function_call_expr() const;
  operator ini::function_call_expr_sptr () const;
  ~fn_call_expr_boundary();
}; //end class type_suppression::insertion_range::fn_call_expr_boundary

class function_suppression;

/// Convenience typedef for a shared pointer to function_suppression.
typedef shared_ptr<function_suppression> function_suppression_sptr;

/// Convenience typedef for a vector of @ref function_suppression_sptr.
typedef vector<function_suppression_sptr> function_suppressions_type;

/// Abstraction of a function suppression specification.
///
/// Specifies under which condition reports about a @ref
/// function_decl_diff diff node should be dropped on the floor for
/// the purpose of reporting.
class function_suppression : public suppression_base
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbid this.
  function_suppression();

public:

  class parameter_spec;

  /// Convenience typedef for shared_ptr of @ref parameter_spec.
  typedef shared_ptr<parameter_spec> parameter_spec_sptr;

  /// Convenience typedef for vector of @ref parameter_spec_sptr.
  typedef vector<parameter_spec_sptr> parameter_specs_type;

  /// The kind of change the current function suppression should apply
  /// to.
  enum change_kind
  {
    UNDEFINED_CHANGE_KIND,
    /// A change in a sub-type of the function.
    FUNCTION_SUBTYPE_CHANGE_KIND = 1,
    /// The function was added to the second second subject of the
    /// diff.
    ADDED_FUNCTION_CHANGE_KIND = 1 << 1,
    /// The function was deleted from the second subject of the diff.
    DELETED_FUNCTION_CHANGE_KIND = 1 << 2,
    /// This represents all the changes possibly described by this
    /// enum.  It's a logical 'OR' of all the change enumerators
    /// above.
    ALL_CHANGE_KIND = (FUNCTION_SUBTYPE_CHANGE_KIND
		       | ADDED_FUNCTION_CHANGE_KIND
		       | DELETED_FUNCTION_CHANGE_KIND)
  };

  function_suppression(const string&		label,
		       const string&		name,
		       const string&		name_regex,
		       const string&		return_type_name,
		       const string&		return_type_regex,
		       parameter_specs_type&	parm_specs,
		       const string&		symbol_name,
		       const string&		symbol_name_regex,
		       const string&		symbol_version,
		       const string&		symbol_version_regex_str);

  virtual ~function_suppression();

  static change_kind
  parse_change_kind(const string&);

  change_kind
  get_change_kind() const;

  void
  set_change_kind(change_kind k);

  const string&
  get_function_name() const;

  void
  set_function_name(const string&);

  const string&
  get_function_name_regex_str() const;

  void
  set_function_name_regex_str(const string&);

  const string&
  get_return_type_name() const;

  void
  set_return_type_name(const string&);

  const string&
  get_return_type_regex_str() const;

  void
  set_return_type_regex_str(const string& r);

  const parameter_specs_type&
  get_parameter_specs() const;

  void
  set_parameter_specs(parameter_specs_type&);

  void
  append_parameter_specs(const parameter_spec_sptr);

  const string&
  get_symbol_name() const;

  void
  set_symbol_name(const string& n);

  const string&
  get_symbol_name_regex_str() const;

  void
  set_symbol_name_regex_str(const string&);

  const string&
  get_symbol_version() const;

  void
  set_symbol_version(const string&);

  const string&
  get_symbol_version_regex_str() const;

  void
  set_symbol_version_regex_str(const string&);

  bool
  get_allow_other_aliases() const;

  void
  set_allow_other_aliases(bool f);

  virtual bool
  suppresses_diff(const diff* diff) const;

  bool
  suppresses_function(const function_decl* fn,
		      change_kind k,
		      const diff_context_sptr ctxt) const;

  bool
  suppresses_function(const function_decl_sptr fn,
		      change_kind k,
		      const diff_context_sptr ctxt) const;

  bool
  suppresses_function_symbol(const elf_symbol* sym,
			     change_kind k,
			     const diff_context_sptr ctxt);

  bool
  suppresses_function_symbol(const elf_symbol_sptr sym,
			     change_kind k,
			     const diff_context_sptr ctxt);
}; // end class function_suppression.

function_suppression_sptr
is_function_suppression(const suppression_sptr);

function_suppression::change_kind
operator&(function_suppression::change_kind l,
	  function_suppression::change_kind r);

function_suppression::change_kind
operator|(function_suppression::change_kind l,
	  function_suppression::change_kind r);

/// Abstraction of the specification of a function parameter in a
/// function suppression specification.
class function_suppression::parameter_spec
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  friend class function_suppression;

  priv_sptr priv_;

  // Forbid this.
  parameter_spec();

public:
  parameter_spec(size_t index,
		 const string& type_name,
		 const string& type_name_regex);

  size_t
  get_index() const;

  void
  set_index(size_t);

  const string&
  get_parameter_type_name() const;

  void
  set_parameter_type_name(const string&);

  const string&
  get_parameter_type_name_regex_str() const;

  void
  set_parameter_type_name_regex_str(const string&);
};// end class function_suppression::parameter_spec

class variable_suppression;

/// A convenience typedef for a shared pointer to @ref
/// variable_suppression.
typedef shared_ptr<variable_suppression> variable_suppression_sptr;

/// A convenience typedef for a vector of @ref
/// variable_suppression_sptr.
typedef vector<variable_suppression_sptr> variable_suppressions_type;

/// The abstraction of a variable suppression specification.
///
/// It specifies under which condition reports about a @ref var_diff
/// diff node should be dropped on the floor for the purpose of
/// reporting.
class variable_suppression : public suppression_base
{
public:


  /// The kind of change the current variable suppression should apply
  /// to.
  enum change_kind
  {
    UNDEFINED_CHANGE_KIND,
    /// A change in a sub-type of the variable.
    VARIABLE_SUBTYPE_CHANGE_KIND = 1,
    /// The variable was added to the second second subject of the
    /// diff.
    ADDED_VARIABLE_CHANGE_KIND = 1 << 1,
    /// The variable was deleted from the second subject of the diff.
    DELETED_VARIABLE_CHANGE_KIND = 1 << 2,
    /// This represents all the changes possibly described by this
    /// enum.  It's a logical 'OR' of all the change enumerators
    /// above.
    ALL_CHANGE_KIND = (VARIABLE_SUBTYPE_CHANGE_KIND
		       | ADDED_VARIABLE_CHANGE_KIND
		       | DELETED_VARIABLE_CHANGE_KIND)
  };

private:
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

public:
  variable_suppression(const string& label,
		       const string& name,
		       const string& name_regex_str,
		       const string& symbol_name,
		       const string& symbol_name_regex_str,
		       const string& symbol_version,
		       const string& symbol_version_regex_str,
		       const string& type_name,
		       const string& type_name_regex_str);

  virtual ~variable_suppression();

  static change_kind
  parse_change_kind(const string&);

  change_kind
  get_change_kind() const;

  void
  set_change_kind(change_kind k);

  const string&
  get_name() const;

  void
  set_name(const string&);

  const string&
  get_name_regex_str() const;

  void
  set_name_regex_str(const string&);

  const string&
  get_symbol_name() const;

  void
  set_symbol_name(const string&);

  const string&
  get_symbol_name_regex_str() const;

  void
  set_symbol_name_regex_str(const string&);

  const string&
  get_symbol_version() const;

  void
  set_symbol_version(const string&);

  const string&
  get_symbol_version_regex_str() const;

  void
  set_symbol_version_regex_str(const string&);

  const string&
  get_type_name() const;

  void
  set_type_name(const string&);

  const string&
  get_type_name_regex_str() const;

  void
  set_type_name_regex_str(const string&);

  bool
  suppresses_diff(const diff* d) const;

  bool
  suppresses_variable(const var_decl* fn,
		      change_kind k,
		      const diff_context_sptr cxt) const;

  bool
  suppresses_variable(const var_decl_sptr fn,
		      change_kind k,
		      const diff_context_sptr cxt) const;

  bool
  suppresses_variable_symbol(const elf_symbol* sym,
			     change_kind k,
			     const diff_context_sptr cxt) const;

  bool
  suppresses_variable_symbol(const elf_symbol_sptr fn,
			     change_kind k,
			     const diff_context_sptr cxt) const;
}; // end class variable_suppression

variable_suppression_sptr
is_variable_suppression(const suppression_sptr);

variable_suppression::change_kind
operator&(variable_suppression::change_kind l,
	  variable_suppression::change_kind r);

variable_suppression::change_kind
operator|(variable_suppression::change_kind l,
	  variable_suppression::change_kind r);

/// The context of the diff.  This type holds various bits of
/// information that is going to be used throughout the diffing of two
/// entities and the reporting that follows.
class diff_context
{
  struct priv;
  shared_ptr<priv> priv_;

  diff_sptr
  has_diff_for(const type_or_decl_base_sptr first,
	       const type_or_decl_base_sptr second) const;

  diff_sptr
  has_diff_for_types(const type_base_sptr first,
		     const type_base_sptr second) const;

  const diff*
  has_diff_for(const diff* d) const;

  diff_sptr
  has_diff_for(const diff_sptr d) const;

  void
  add_diff(const type_or_decl_base_sptr first,
	   const type_or_decl_base_sptr second,
	   const diff_sptr d);

  void
  add_diff(const diff_sptr d);

  void
  add_diff(const diff* d);

  void
  set_canonical_diff_for(const type_or_decl_base_sptr first,
			 const type_or_decl_base_sptr second,
			 const diff_sptr);

  diff_sptr
  set_or_get_canonical_diff_for(const type_or_decl_base_sptr first,
				const type_or_decl_base_sptr second,
				const diff_sptr canonical_diff);

public:
  diff_context();

  void
  set_corpora(const corpus_sptr corp1,
	      const corpus_sptr corp2);

  const corpus_sptr
  get_first_corpus() const;

  const corpus_sptr
  get_second_corpus() const;

  diff_sptr
  get_canonical_diff_for(const type_or_decl_base_sptr first,
			 const type_or_decl_base_sptr second) const;

  diff_sptr
  get_canonical_diff_for(const diff_sptr d) const;

  void
  initialize_canonical_diff(const diff_sptr diff);

  diff*
  diff_has_been_visited(const diff*) const;

  diff_sptr
  diff_has_been_visited(const diff_sptr) const;

  void
  mark_diff_as_visited(const diff*);

  void
  forget_visited_diffs();

  void
  mark_last_diff_visited_per_class_of_equivalence(const diff*);

  void
  clear_last_diffs_visited_per_class_of_equivalence();

  const diff*
  get_last_visited_diff_of_class_of_equivalence(const diff*);

  void
  forbid_visiting_a_node_twice(bool f);

  bool
  visiting_a_node_twice_is_forbidden() const;

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
  maybe_apply_filters(diff_sptr diff);

  void
  maybe_apply_filters(corpus_diff_sptr diff);

  suppressions_type&
  suppressions() const;

  void
  add_suppression(const suppression_sptr suppr);

  void
  add_suppressions(const suppressions_type& supprs);

  void
  show_stats_only(bool f);

  bool
  show_stats_only() const;

  void
  show_soname_change(bool f);

  bool
  show_soname_change() const;

  void
  show_architecture_change(bool f);

  bool
  show_architecture_change() const;

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

  bool
  show_linkage_names() const;

  void
  show_linkage_names(bool f);

  bool
  show_redundant_changes() const;

  void
  show_redundant_changes(bool f);

  bool
  show_symbols_unreferenced_by_debug_info() const;

  void
  show_symbols_unreferenced_by_debug_info(bool f);

  bool
  show_added_symbols_unreferenced_by_debug_info() const;

  void
  show_added_symbols_unreferenced_by_debug_info(bool f);

  void
  default_output_stream(ostream*);

  ostream*
  default_output_stream();

  void
  error_output_stream(ostream*);

  ostream*
  error_output_stream() const;

  bool
  dump_diff_tree() const;

  void
  dump_diff_tree(bool f);

  void
  do_dump_diff_tree(const diff_sptr) const;

  void
  do_dump_diff_tree(const corpus_diff_sptr) const;

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second,
	       diff_context_sptr	ctxt);
};//end struct diff_context.

/// The abstraction of a change between two ABI artifacts.
///
/// Please read more about the @ref DiffNode "IR" of the comparison
/// engine to learn more about this.
///
/// This type encapsulates an edit script (a set of insertions and
/// deletions) for two constructs that are to be diff'ed.  The two
/// constructs are called the "subjects" of the diff.
class diff : public diff_traversable_base
{
  friend class diff_context;

  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  // Forbidden
  diff();

protected:
  priv_sptr priv_;

  diff(type_or_decl_base_sptr first_subject,
       type_or_decl_base_sptr second_subject);

  diff(type_or_decl_base_sptr	first_subject,
       type_or_decl_base_sptr	second_subject,
       diff_context_sptr	ctxt);

  void
  begin_traversing();

  void
  end_traversing();

  virtual void
  finish_diff_type();

  void
  set_canonical_diff(diff *);

public:
  type_or_decl_base_sptr
  first_subject() const;

  type_or_decl_base_sptr
  second_subject() const;

  const vector<diff_sptr>&
  children_nodes() const;

  const diff*
  parent_node() const;

  diff* get_canonical_diff() const;

  bool
  is_traversing() const;

  void
  append_child_node(diff_sptr);

  const diff_context_sptr
  context() const;

  void
  context(diff_context_sptr c);

  bool
  currently_reporting() const;

  void
  currently_reporting(bool f) const;

  bool
  reported_once() const;

  void
  reported_once(bool f) const;

  diff_category
  get_category() const;

  diff_category
  get_local_category() const;

  diff_category
  add_to_category(diff_category c);

  diff_category
  add_to_local_category(diff_category c);

  void
  add_to_local_and_inherited_categories(diff_category c);

  diff_category
  remove_from_category(diff_category c);

  diff_category
  remove_from_local_category(diff_category c);

  void
  set_category(diff_category c);

  void
  set_local_category(diff_category c);

  bool
  is_filtered_out() const;

  bool
  is_filtered_out_wrt_non_inherited_categories() const;

  bool
  is_suppressed() const;

  bool
  to_be_reported() const;

  bool
  has_local_changes_to_be_reported() const;

  virtual const string&
  get_pretty_representation() const;

  virtual void
  chain_into_hierarchy();

  /// Pure interface to get the length of the changes encapsulated by
  /// this diff.  A length of zero means that the current instance of
  /// @ref diff doesn't carry any change.
  ///
  /// This is to be implemented by all descendants of this type.
  virtual bool
  has_changes() const = 0;

  /// Pure interface to know if the current instance of @diff carries
  /// a local change.  A local change is a change that is on the @ref
  /// diff object itself, as opposed to a change that is carried by
  /// some of its children nodes.
  ///
  /// This is to be implemented by all descendants of this type.
  virtual bool
  has_local_changes() const = 0;

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

  virtual bool
  traverse(diff_node_visitor& v);
};// end class diff

diff_sptr
compute_diff(const decl_base_sptr,
	     const decl_base_sptr,
	     diff_context_sptr ctxt);

diff_sptr
compute_diff(const type_base_sptr,
	     const type_base_sptr,
	     diff_context_sptr ctxt);

/// The base class of diff between types.
class type_diff_base : public diff
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  type_diff_base();

protected:
  type_diff_base(type_base_sptr	first_subject,
		 type_base_sptr	second_subject,
		 diff_context_sptr	ctxt);

public:

  virtual bool
  has_local_changes() const = 0;

  virtual ~type_diff_base();
};// end class type_diff_base

/// The base class of diff between decls.
class decl_diff_base : public diff
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

protected:
  decl_diff_base(decl_base_sptr	first_subject,
		 decl_base_sptr	second_subject,
		 diff_context_sptr	ctxt);

public:

  virtual bool
  has_local_changes() const = 0;

  virtual ~decl_diff_base();
};// end class decl_diff_base

string
get_pretty_representation(diff*);

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
  distinct_diff(type_or_decl_base_sptr first,
		type_or_decl_base_sptr second,
		diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:

  const type_or_decl_base_sptr
  first() const;

  const type_or_decl_base_sptr
  second() const;

  const diff_sptr
  compatible_child_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  static bool
  entities_are_of_distinct_kinds(type_or_decl_base_sptr first,
				 type_or_decl_base_sptr second);

  friend distinct_diff_sptr
  compute_diff_for_distinct_kinds(const type_or_decl_base_sptr first,
				  const type_or_decl_base_sptr second,
				  diff_context_sptr ctxt);
};// end class distinct_types_diff

distinct_diff_sptr
compute_diff_for_distinct_kinds(const type_or_decl_base_sptr,
				const type_or_decl_base_sptr,
				diff_context_sptr ctxt);

/// Abstracts a diff between two instances of @ref var_decl
class var_diff : public decl_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  var_diff(var_decl_sptr first,
	   var_decl_sptr second,
	   diff_sptr type_diff,
	   diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  var_decl_sptr
  first_var() const;

  var_decl_sptr
  second_var() const;

  diff_sptr
  type_diff() const;

  virtual void
  chain_into_hierarchy();

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual const string&
  get_pretty_representation() const;

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
class pointer_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  pointer_diff(pointer_type_def_sptr	first,
	       pointer_type_def_sptr	second,
	       diff_sptr		underlying_type_diff,
	       diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const pointer_type_def_sptr
  first_pointer() const;

  const pointer_type_def_sptr
  second_pointer() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

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
class reference_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  reference_diff(const reference_type_def_sptr	first,
		 const reference_type_def_sptr	second,
		 diff_sptr			underlying,
		 diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  reference_type_def_sptr
  first_reference() const;

  reference_type_def_sptr
  second_reference() const;

  const diff_sptr&
  underlying_type_diff() const;

  diff_sptr&
  underlying_type_diff(diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend reference_diff_sptr
  compute_diff(reference_type_def_sptr first,
	       reference_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class reference_diff

reference_diff_sptr
compute_diff(reference_type_def_sptr first,
	     reference_type_def_sptr second,
	     diff_context_sptr ctxt);

class array_diff;

/// Convenience typedef for a shared pointer on a @ref
/// array_diff type.
typedef shared_ptr<array_diff> array_diff_sptr;

/// The abstraction of a diff between two arrays.
class array_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  array_diff(const array_type_def_sptr	first,
	     const array_type_def_sptr	second,
	     diff_sptr			element_type_diff,
	     diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const array_type_def_sptr
  first_array() const;

  const array_type_def_sptr
  second_array() const;

  const diff_sptr&
  element_type_diff() const;

  void
  element_type_diff(diff_sptr);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend array_diff_sptr
  compute_diff(array_type_def_sptr first,
	       array_type_def_sptr second,
	       diff_context_sptr ctxt);
};// end class array_diff

array_diff_sptr
compute_diff(array_type_def_sptr first,
	     array_type_def_sptr second,
	     diff_context_sptr ctxt);

class qualified_type_diff;
typedef class shared_ptr<qualified_type_diff> qualified_type_diff_sptr;

/// Abstraction of a diff between two qualified types.
class qualified_type_diff : public type_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  qualified_type_diff(qualified_type_def_sptr	first,
		      qualified_type_def_sptr	second,
		      diff_sptr		underling,
		      diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  const qualified_type_def_sptr
  first_qualified_type() const;

  const qualified_type_def_sptr
  second_qualified_type() const;

  diff_sptr
  underlying_type_diff() const;

  void
  underlying_type_diff(const diff_sptr);

  diff_sptr
  leaf_underlying_type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

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
class enum_diff : public type_diff_base
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

  virtual void
  finish_diff_type();

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

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend enum_diff_sptr
  compute_diff(const enum_type_decl_sptr first,
	       const enum_type_decl_sptr second,
	       diff_context_sptr ctxt);
};//end class enum_diff;

enum_diff_sptr
compute_diff(const enum_type_decl_sptr,
	     const enum_type_decl_sptr,
	     diff_context_sptr);

/// This type abstracts changes for a class_decl.
class class_diff : public type_diff_base
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

  virtual void
  finish_diff_type();

public:
  //TODO: add change of the name of the type.

  virtual ~class_diff();

  class_decl_sptr
  first_class_decl() const;

  class_decl_sptr
  second_class_decl() const;

  const edit_script&
  base_changes() const;

  edit_script&
  base_changes();

  const string_base_sptr_map&
  deleted_bases() const;

  const string_base_sptr_map&
  inserted_bases() const;

  const base_diff_sptrs_type&
  changed_bases();

  const edit_script&
  member_types_changes() const;

  edit_script&
  member_types_changes();

  const edit_script&
  data_members_changes() const;

  edit_script&
  data_members_changes();

  const string_decl_base_sptr_map&
  inserted_data_members() const;

  const string_decl_base_sptr_map&
  deleted_data_members() const;

  const edit_script&
  member_fns_changes() const;

  edit_script&
  member_fns_changes();

  const function_decl_diff_sptrs_type&
  changed_member_fns() const;

  const string_member_function_sptr_map&
  deleted_member_fns() const;

  const string_member_function_sptr_map&
  inserted_member_fns() const;

  const edit_script&
  member_fn_tmpls_changes() const;

  edit_script&
  member_fn_tmpls_changes();

  const edit_script&
  member_class_tmpls_changes() const;

  edit_script&
  member_class_tmpls_changes();

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second,
	       diff_context_sptr	ctxt);
};// end class_diff

class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt);

/// An abstraction of a diff between two instances of class_decl::base_spec.
class base_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

protected:
  base_diff(class_decl::base_spec_sptr	first,
	    class_decl::base_spec_sptr	second,
	    class_diff_sptr		underlying,
	    diff_context_sptr		ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  class_decl::base_spec_sptr
  first_base() const;

  class_decl::base_spec_sptr
  second_base() const;

  const class_diff_sptr
  get_underlying_class_diff() const;

  void
  set_underlying_class_diff(class_diff_sptr d);

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();

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

  virtual void
  finish_diff_type();

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

  const diff_sptrs_type&
  changed_types() const;

  const diff_sptrs_type&
  changed_decls() const;

  const string_decl_base_sptr_map&
  removed_types() const;

  const string_decl_base_sptr_map&
  removed_decls() const;

  const string_decl_base_sptr_map&
  added_types() const;

  const string_decl_base_sptr_map&
  added_decls() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
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

/// Abstraction of a diff between two function parameters.
class fn_parm_diff : public decl_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  virtual void
  finish_diff_type();

  fn_parm_diff(const function_decl::parameter_sptr	first,
	       const function_decl::parameter_sptr	second,
	       diff_context_sptr			ctxt);

public:
  friend fn_parm_diff_sptr
  compute_diff(const function_decl::parameter_sptr	first,
	       const function_decl::parameter_sptr	second,
	       diff_context_sptr			ctxt);

  const function_decl::parameter_sptr
  first_parameter() const;

  const function_decl::parameter_sptr
  second_parameter() const;

  diff_sptr
  get_type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
}; // end class fn_parm_diff

fn_parm_diff_sptr
compute_diff(const function_decl::parameter_sptr	first,
	     const function_decl::parameter_sptr	second,
	     diff_context_sptr				ctxt);

class function_type_diff;

/// A convenience typedef for a shared pointer to @ref
/// function_type_type_diff
typedef shared_ptr<function_type_diff> function_type_diff_sptr;

/// Abstraction of a diff between two function types.
class function_type_diff: public type_diff_base
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

  void
  ensure_lookup_tables_populated();

  const function_decl::parameter_sptr
  deleted_parameter_at(int i) const;

  const function_decl::parameter_sptr
  inserted_parameter_at(int i) const;

protected:
  function_type_diff(const function_type_sptr	first,
		     const function_type_sptr	second,
		     diff_context_sptr		ctxt);

  virtual void
  finish_diff_type();

public:
  friend function_type_diff_sptr
  compute_diff(const function_type_sptr	first,
	       const function_type_sptr	second,
	       diff_context_sptr		ctxt);

  const function_type_sptr
  first_function_type() const;

  const function_type_sptr
  second_function_type() const;

  const diff_sptr
  return_type_diff() const;

  const string_fn_parm_diff_sptr_map&
  subtype_changed_parms() const;

  const string_parm_map&
  removed_parms() const;

  const string_parm_map&
  added_parms() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
};// end class function_type_diff

function_type_diff_sptr
compute_diff(const function_type_sptr	first,
	     const function_type_sptr	second,
	     diff_context_sptr		ctxt);

/// Abstraction of a diff between two function_decl.
class function_decl_diff : public decl_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

  void
  ensure_lookup_tables_populated();


protected:
  function_decl_diff(const function_decl_sptr	first,
		     const function_decl_sptr	second,
		     diff_context_sptr		ctxt);

  virtual void
  finish_diff_type();

public:

friend function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

  const function_decl_sptr
  first_function_decl() const;

  const function_decl_sptr
  second_function_decl() const;

  const function_type_diff_sptr
  type_diff() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
}; // end class function_decl_diff

function_decl_diff_sptr
compute_diff(const function_decl_sptr	first,
	     const function_decl_sptr	second,
	     diff_context_sptr		ctxt);

class type_decl_diff;

/// Convenience typedef for a shared pointer on a @ref type_decl_diff type.
typedef shared_ptr<type_decl_diff> type_decl_diff_sptr;

/// Abstraction of a diff between two basic type declarations.
class type_decl_diff : public type_diff_base
{
  type_decl_diff();

protected:
  type_decl_diff(const type_decl_sptr first,
		 const type_decl_sptr second,
		 diff_context_sptr ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

public:
  friend type_decl_diff_sptr
  compute_diff(const type_decl_sptr	first,
	       const type_decl_sptr	second,
	       diff_context_sptr	ctxt);

  const type_decl_sptr
  first_type_decl() const;

  const type_decl_sptr
  second_type_decl() const;

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;
};// end type_decl_diff

type_decl_diff_sptr
compute_diff(const type_decl_sptr,
	     const type_decl_sptr,
	     diff_context_sptr);

class typedef_diff;

/// Convenience typedef for a shared pointer on a typedef_diff type.
typedef shared_ptr<typedef_diff> typedef_diff_sptr;

/// Abstraction of a diff between two typedef_decl.
class typedef_diff : public type_diff_base
{
  struct priv;
  shared_ptr<priv> priv_;

  typedef_diff();

protected:
  typedef_diff(const typedef_decl_sptr	first,
	       const typedef_decl_sptr	second,
	       const diff_sptr		underlying_type_diff,
	       diff_context_sptr	ctxt = diff_context_sptr());

  virtual void
  finish_diff_type();

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

  virtual const string&
  get_pretty_representation() const;

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream&, const string& indent = "") const;

  virtual void
  chain_into_hierarchy();
};// end class typedef_diff

typedef_diff_sptr
compute_diff(const typedef_decl_sptr,
	     const typedef_decl_sptr,
	     diff_context_sptr ctxt);

const diff*
get_typedef_diff_underlying_type_diff(const diff* diff);

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

  virtual bool
  has_changes() const;

  virtual bool
  has_local_changes() const;

  virtual void
  report(ostream& out, const string& indent = "") const;
};//end class translation_unit_diff

translation_unit_diff_sptr
compute_diff(const translation_unit_sptr first,
	     const translation_unit_sptr second,
	     diff_context_sptr ctxt = diff_context_sptr());

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

  void
  finish_diff_type();

public:

  class diff_stats;

  /// A convenience typedef for a shared pointer to @ref diff_stats
  typedef shared_ptr<diff_stats> diff_stats_sptr;

  corpus_sptr
  first_corpus() const;

  corpus_sptr
  second_corpus() const;

  const vector<diff_sptr>&
  children_nodes() const;

  void
  append_child_node(diff_sptr);

  edit_script&
  function_changes() const;

  edit_script&
  variable_changes() const;

  bool
  soname_changed() const;

  bool
  architecture_changed() const;

  const string_function_ptr_map&
  deleted_functions() const;

  const string_function_ptr_map&
  added_functions();

  const string_function_decl_diff_sptr_map&
  changed_functions();

  const function_decl_diff_sptrs_type&
  changed_functions_sorted();

  const string_var_ptr_map&
  deleted_variables() const;

  const string_var_ptr_map&
  added_variables() const;

  const string_var_diff_sptr_map&
  changed_variables();

  const var_diff_sptrs_type&
  changed_variables_sorted();

  const string_elf_symbol_map&
  deleted_unrefed_function_symbols() const;

  const string_elf_symbol_map&
  added_unrefed_function_symbols() const;

  const string_elf_symbol_map&
  deleted_unrefed_variable_symbols() const;

  const string_elf_symbol_map&
  added_unrefed_variable_symbols() const;

  const diff_context_sptr
  context() const;

  const string&
  get_pretty_representation() const;

  bool
  has_changes() const;

  bool
  has_incompatible_changes() const;

  bool
  has_net_subtype_changes() const;

  bool
  has_net_changes() const;

  const diff_stats&
  apply_filters_and_suppressions_before_reporting();

  virtual void
  report(ostream& out, const string& indent = "") const;

  virtual bool
  traverse(diff_node_visitor& v);

  virtual void
  chain_into_hierarchy();

  friend corpus_diff_sptr
  compute_diff(const corpus_sptr f,
	       const corpus_sptr s,
	       diff_context_sptr ctxt = diff_context_sptr());

  friend void
  apply_suppressions(const corpus_diff* diff_tree);
}; // end class corpus_diff

corpus_diff_sptr
compute_diff(const corpus_sptr,
	     const corpus_sptr,
	     diff_context_sptr);

/// This is a document class that aims to capture statistics about the
/// changes carried by a @ref corpus_diff type.
///
/// Its values are populated by the member function
/// corpus_diff::apply_filters_and_suppressions_before_reporting()
class corpus_diff::diff_stats
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  diff_stats();

public:

  diff_stats(diff_context_sptr);

  size_t num_func_removed() const;
  void num_func_removed(size_t);

  size_t num_removed_func_filtered_out() const;
  void num_removed_func_filtered_out(size_t);

  size_t net_num_func_removed() const;

  size_t num_func_added() const;
  void num_func_added(size_t);

  size_t num_added_func_filtered_out() const;
  void num_added_func_filtered_out(size_t);

  size_t net_num_func_added() const;

  size_t num_func_changed() const;
  void num_func_changed(size_t);

  size_t num_changed_func_filtered_out() const;
  void num_changed_func_filtered_out(size_t);

  size_t net_num_func_changed() const;

  size_t num_vars_removed() const;
  void num_vars_removed(size_t);

  size_t num_removed_vars_filtered_out() const;
  void num_removed_vars_filtered_out(size_t) const;

  size_t net_num_vars_removed() const;

  size_t num_vars_added() const;
  void num_vars_added(size_t);

  size_t num_added_vars_filtered_out() const;
  void num_added_vars_filtered_out(size_t);

  size_t net_num_vars_added() const;

  size_t num_vars_changed() const;
  void num_vars_changed(size_t);

  size_t num_changed_vars_filtered_out() const;
  void num_changed_vars_filtered_out(size_t);

  size_t net_num_vars_changed() const;

  size_t num_func_syms_removed() const;
  void num_func_syms_removed(size_t);

  size_t num_removed_func_syms_filtered_out() const;
  void num_removed_func_syms_filtered_out(size_t);

  size_t num_func_syms_added() const;
  void num_func_syms_added(size_t);

  size_t num_added_func_syms_filtered_out() const;
  void num_added_func_syms_filtered_out(size_t);

  size_t net_num_removed_func_syms() const;
  size_t net_num_added_func_syms() const;

  size_t num_var_syms_removed() const;
  void num_var_syms_removed(size_t);

  size_t num_removed_var_syms_filtered_out() const;
  void num_removed_var_syms_filtered_out(size_t);

  size_t num_var_syms_added() const;
  void num_var_syms_added(size_t);

  size_t num_added_var_syms_filtered_out() const;
  void num_added_var_syms_filtered_out(size_t);

  size_t net_num_removed_var_syms() const;
  size_t net_num_added_var_syms() const;
}; // end class corpus_diff::diff_stats

/// The base class for the node visitors.  These are the types used to
/// visit each node traversed by the diff_traversable_base::traverse() method.
class diff_node_visitor : public node_visitor_base
{
protected:
  visiting_kind visiting_kind_;

public:

  /// Default constructor of the @ref diff_node_visitor type.
  diff_node_visitor()
    : visiting_kind_()
  {}

  /// Constructor of the @ref diff_node_visitor type.
  ///
  /// @param k how the visiting has to be performed.
  diff_node_visitor(visiting_kind k)
    : visiting_kind_(k)
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

  virtual void
  visit_begin(diff*);

  virtual void
  visit_begin(corpus_diff*);

  virtual void
  visit_end(diff*);

  virtual void
  visit_end(corpus_diff*);

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

void
propagate_categories(diff* diff_tree);

void
propagate_categories(diff_sptr diff_tree);

void
propagate_categories(corpus_diff* diff_tree);

void
propagate_categories(corpus_diff_sptr diff_tree);

void
apply_suppressions(diff* diff_tree);

void
apply_suppressions(const corpus_diff* diff_tree);

void
apply_suppressions(diff_sptr diff_tree);

void
apply_suppressions(corpus_diff_sptr diff_tree);

void
print_diff_tree(diff* diff_tree, std::ostream&);

void
print_diff_tree(corpus_diff* diff_tree,
		std::ostream&);

void
print_diff_tree(diff_sptr diff_tree,
		std::ostream&);

void
print_diff_tree(corpus_diff_sptr diff_tree,
		std::ostream&);

void
categorize_redundancy(diff* diff_tree);

void
categorize_redundancy(diff_sptr diff_tree);

void
categorize_redundancy(corpus_diff* diff_tree);

void
categorize_redundancy(corpus_diff_sptr diff_tree);

void
clear_redundancy_categorization(diff* diff_tree);

void
clear_redundancy_categorization(diff_sptr diff_tree);

void
clear_redundancy_categorization(corpus_diff* diff_tree);

void
clear_redundancy_categorization(corpus_diff_sptr diff_tree);

void
apply_filters(corpus_diff_sptr diff_tree);

bool
is_diff_of_variadic_parameter_type(const diff*);

bool
is_diff_of_variadic_parameter_type(const diff_sptr&);

bool
is_diff_of_variadic_parameter(const diff*);

bool
is_diff_of_variadic_parameter(const diff_sptr&);
}// end namespace comparison

}// end namespace abigail

#endif //__ABG_COMPARISON_H__
