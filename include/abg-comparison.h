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

// Injecting lower-level types from diff_utils into this namespace.
using diff_utils::insertion;
using diff_utils::deletion;
using diff_utils::edit_script;

/// This type encapsulates an edit script (a set of insertions and
/// deletions) for a given scope.  It's the base class to represents
/// changes that appertain to a certain kind of construct.
class diff
{
  shared_ptr<scope_decl> scope_;

public:
  typedef edit_script changes_type;

  diff(shared_ptr<scope_decl> scope)
    : scope_(scope)
  {}

  shared_ptr<scope_decl>
  scope() const
  {return scope_;}
};// end class diff

/// This type abstracts changes for a class_decl.
class class_decl_diff : public diff
{
  struct priv;
  shared_ptr<priv> priv_;

public:

  class_decl_diff(shared_ptr<scope_decl> scope);

  const changes_type&
  base_changes() const;

  changes_type&
  base_changes();

  const changes_type&
  member_types_changes() const;

  changes_type&
  member_types_changes();

  const changes_type&
  data_member_changes() const;

  changes_type&
  data_member_changes();

  const changes_type&
  member_fns_changes() const;

  changes_type&
  member_fns_changes();

  const changes_type&
  member_fn_tmpl_changes() const;

  changes_type&
  member_fn_tmpl_changes();

  const changes_type&
  member_class_tmpl_changes() const;

  changes_type&
  member_class_tmpl_changes();
};// end class_decl_edit_script

void
compute_diff(class_decl_sptr	 first,
	     class_decl_sptr	 second,
	     class_decl_diff	&changes);

}// end namespace comparison

}// end namespace abigail
