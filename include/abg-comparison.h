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

#include "abg-ir.h"
#include "abg-diff-utils.h"

namespace abigail
{

namespace comparison
{

// Inject types we need into this namespace.
using std::ostream;
using std::vector;
using std::tr1::unordered_map;
using diff_utils::insertion;
using diff_utils::deletion;
using diff_utils::edit_script;

typedef unordered_map<string, decl_base_sptr> string_decl_base_sptr_map;

/// This type encapsulates an edit script (a set of insertions and
/// deletions) for a given scope.  It's the base class to represents
/// changes that appertain to a certain kind of construct.
class diff
{
  scope_decl_sptr first_scope_;
  scope_decl_sptr second_scope_;

public:

  diff(shared_ptr<scope_decl> first_scope,
       shared_ptr<scope_decl> second_scope)
    : first_scope_(first_scope),
      second_scope_(second_scope)
  {}

  shared_ptr<scope_decl>
  first_scope() const
  {return first_scope_;}

  shared_ptr<scope_decl>
  second_scope() const
  {return second_scope_;}

};// end class diff

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

public:

  typedef std::pair<decl_base_sptr, decl_base_sptr> changed_type_or_decl;

  friend void
  compute_diff(scope_decl_sptr first,
	       scope_decl_sptr second,
	       scope_diff& d);

  scope_diff(scope_decl_sptr first_scope,
	     scope_decl_sptr second_scope);

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

  const unordered_map<string, changed_type_or_decl>&
  changed_types() const;

  const unordered_map<string, changed_type_or_decl>&
  changed_decls() const;
};// end class scope_diff

void
compute_diff(scope_decl_sptr first_scope,
	     scope_decl_sptr second_scope,
	     scope_diff& d);

void
report_changes(const scope_diff& changes,
	       ostream& out);

/// This type abstracts changes for a class_decl.
class class_decl_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

public:

  class_decl_diff(class_decl_sptr first_scope,
		  class_decl_sptr second_scope);

  unsigned
  length() const;

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

};// end class_decl_edit_script

void
compute_diff(class_decl_sptr	 first,
	     class_decl_sptr	 second,
	     class_decl_diff	&changes);

void
report_changes(class_decl_diff &changes,
	       ostream& out);
}// end namespace comparison

}// end namespace abigail
