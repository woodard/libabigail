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

#include <tr1/unordered_map>
#include "abg-ir.h"
#include "abg-diff-utils.h"

namespace abigail
{

/// @brief utilities to compare abi artifacts
///
/// The main entry points of the namespace are the compute_diff()
/// overloads used to compute the difference between two abi artifacts.
namespace comparison
{

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

/// This type encapsulates an edit script (a set of insertions and
/// deletions) for two constructs that are to be diff'ed.  The two
/// constructs are called the "subjects" of the diff.
class diff
{
  decl_base_sptr first_subject_;
  decl_base_sptr second_subject_;

protected:
  diff(decl_base_sptr first_subject,
       decl_base_sptr second_subject)
    : first_subject_(first_subject),
      second_subject_(second_subject)
  {}

public:

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
  /// @param the output stream to serialize the report to.
  ///
  /// @param indent the indentation string to use.
  virtual void
  report(ostream& out, const string& indent = "") const = 0;
};// end class diff

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
  pointer_diff(pointer_type_def_sptr first,
	       pointer_type_def_sptr second);

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

  friend pointer_diff_sptr
  compute_diff(pointer_type_def_sptr first,
	       pointer_type_def_sptr second);
};// end class pointer_diff

pointer_diff_sptr
compute_diff(pointer_type_def_sptr first,
	     pointer_type_def_sptr second);

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
  reference_diff(const reference_type_def_sptr first,
		 const reference_type_def_sptr second);

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

  friend reference_diff_sptr
  compute_diff(reference_type_def_sptr first,
	       reference_type_def_sptr second);
};// end class reference_diff

reference_diff_sptr
compute_diff(reference_type_def_sptr first,
	     reference_type_def_sptr second);

class qualified_type_diff;
typedef class shared_ptr<qualified_type_diff> qualified_type_diff_sptr;

/// Abstraction of a diff between two qualified types.
class qualified_type_diff : public diff
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;
  priv_sptr priv_;

protected:
  qualified_type_diff(qualified_type_def_sptr first,
		      qualified_type_def_sptr second);

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

  friend qualified_type_diff_sptr
  compute_diff(const qualified_type_def_sptr,
	       const qualified_type_def_sptr);
};// end class qualified_type_diff.

qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr,
	     const qualified_type_def_sptr);

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
	    const diff_sptr);

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

  friend enum_diff_sptr
  compute_diff(const enum_type_decl_sptr,
	       const enum_type_decl_sptr);
};//end class enum_diff;

enum_diff_sptr
compute_diff(const enum_type_decl_sptr,
	     const enum_type_decl_sptr);

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
  class_diff(class_decl_sptr first_subject,
		  class_decl_sptr second_subject);

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

  friend class_diff_sptr
  compute_diff(const class_decl_sptr	first,
	       const class_decl_sptr	second);
};// end class_diff

class_diff_sptr
compute_diff(const class_decl_sptr first,
	     const class_decl_sptr  second);

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
	     scope_decl_sptr second_scope);

public:

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr,
	       const scope_decl_sptr,
	       scope_diff_sptr);

  friend scope_diff_sptr
  compute_diff(const scope_decl_sptr first_scope,
	       const scope_decl_sptr second_scope);

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
  report(ostream&, const string& indent = "") const;
};// end class scope_diff

scope_diff_sptr
compute_diff(const scope_decl_sptr first_scope,
	     const scope_decl_sptr second_scope,
	     scope_diff_sptr d);

scope_diff_sptr
compute_diff(const scope_decl_sptr first_scope,
	     const scope_decl_sptr second_scope);

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
  function_decl_diff(const function_decl_sptr first,
		     const function_decl_sptr second);

public:
friend function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second);

  const function_decl_sptr
  first_function_decl() const;

  const function_decl_sptr
  second_function_decl() const;

  const diff_sptr
  return_diff() const;

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
}; // end class function_decl_diff

function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second);

class type_decl_diff;

/// Convenience typedef for a shared pointer on a @ref type_decl_diff type.
typedef shared_ptr<type_decl_diff> type_decl_diff_sptr;

/// Abstraction of a diff between two basic type declarations.
class type_decl_diff : public diff
{
  type_decl_diff();

protected:
  type_decl_diff(const type_decl_sptr, const type_decl_sptr);

public:
  friend type_decl_diff_sptr
  compute_diff(const type_decl_sptr, const type_decl_sptr);

  const type_decl_sptr
  first_type_decl() const;

  const type_decl_sptr
  second_type_decl() const;

  unsigned
  length() const;

  void
  report(ostream& out, const string& indent = "") const;
};// end type_decl_diff

type_decl_diff_sptr
compute_diff(const type_decl_sptr, const type_decl_sptr);

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
  typedef_diff(const typedef_decl_sptr first,
	       const typedef_decl_sptr second);


public:
  friend typedef_diff_sptr
  compute_diff(const typedef_decl_sptr, const typedef_decl_sptr);

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
};// end class typedef_diff

typedef_diff_sptr
compute_diff(const typedef_decl_sptr, const typedef_decl_sptr);

class translation_unit_diff;

/// Convenience typedef for a shared pointer on a
/// @ref translation_unit_diff type.
typedef shared_ptr<translation_unit_diff> translation_unit_diff_sptr;

class translation_unit_diff : public scope_diff
{
protected:
  translation_unit_diff(translation_unit_sptr first,
			translation_unit_sptr second);

public:
  friend translation_unit_diff_sptr
  compute_diff(const translation_unit_sptr first,
	       const translation_unit_sptr second);

  virtual unsigned
  length() const;

  virtual void
  report(ostream& out, const string& indent = "") const;
};//end clss translation_unit_diff

translation_unit_diff_sptr
compute_diff(const translation_unit_sptr first,
	     const translation_unit_sptr second);

}// end namespace comparison

}// end namespace abigail
