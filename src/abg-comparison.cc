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

#include "abg-comparison.h"

namespace abigail
{

namespace comparison
{
struct class_decl_diff::priv
{
  edit_script base_changes_;
  edit_script member_types_changes_;
  edit_script data_members_changes_;
  edit_script member_fns_changes_;
  edit_script member_fn_tmpls_changes_;
  edit_script member_class_tmpls_changes_;
};//end struct class_decl_diff::priv

/// Constructor of class_decl_diff
///
/// @param scope the scope of the class_diff.
class_decl_diff::class_decl_diff(shared_ptr<scope_decl> scope)
  : diff(scope),
    priv_(new priv)
{}

const edit_script&
class_decl_diff::base_changes() const
{return priv_->base_changes_;}

edit_script&
class_decl_diff::base_changes()
{return priv_->base_changes_;}

const edit_script&
class_decl_diff::member_types_changes() const
{return priv_->member_types_changes_;}

edit_script&
class_decl_diff::member_types_changes()
{return priv_->member_types_changes_;}

const edit_script&
class_decl_diff::data_members_changes() const
{return priv_->data_members_changes_;}

edit_script&
class_decl_diff::data_members_changes()
{return priv_->data_members_changes_;}

const edit_script&
class_decl_diff::member_fns_changes() const
{return priv_->member_fns_changes_;}

edit_script&
class_decl_diff::member_fns_changes()
{return priv_->member_fns_changes_;}

const edit_script&
class_decl_diff::member_fn_tmpls_changes() const
{return priv_->member_fn_tmpls_changes_;}

edit_script&
class_decl_diff::member_fn_tmpls_changes()
{return priv_->member_fn_tmpls_changes_;}

const edit_script&
class_decl_diff::member_class_tmpls_changes() const
{return priv_->member_class_tmpls_changes_;}

edit_script&
class_decl_diff::member_class_tmpls_changes()
{return priv_->member_class_tmpls_changes_;}

void
compute_diff(class_decl_sptr	 /*first*/,
	     class_decl_sptr	 /*second*/,
	     class_decl_diff	&/*changes*/)
{
}
}// end namespace comparison
} // end namespace abigail
