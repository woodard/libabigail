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

// Inject types from outside in here.
using std::vector;
using std::tr1::dynamic_pointer_cast;

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
class_decl_diff::class_decl_diff(shared_ptr<class_decl> first_scope,
				 shared_ptr<class_decl> second_scope)
  : diff(first_scope, second_scope),
    priv_(new priv)
{}

unsigned
class_decl_diff::length() const
{
  return (base_changes().length()
	  + member_types_changes()
	  + data_members_changes()
	  + member_fns_changes()
	  + member_fn_tmpls_changes()
	  + member_class_tmpls_changes());
}

shared_ptr<class_decl>
class_decl_diff::first_class_decl() const
{return dynamic_pointer_cast<class_decl>(first_scope());}


shared_ptr<class_decl>
class_decl_diff::second_class_decl() const
{return dynamic_pointer_cast<class_decl>(second_scope());}

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

/// Compute the set of changes between two instances of class_decl.
///
/// @param first the first class_decl to consider.
///
/// @param second the second class_decl to consider.
///
/// @param changes the resulting changes.
void
compute_diff(class_decl_sptr	 first,
	     class_decl_sptr	 second,
	     class_decl_diff	&changes)
{
  assert(first && second
	 && changes.first_scope()
	 && changes.second_scope());

  // Compare base specs
  compute_diff(first->get_base_specifiers().begin(),
	       first->get_base_specifiers().begin(),
	       first->get_base_specifiers().end(),
	       second->get_base_specifiers().begin(),
	       second->get_base_specifiers().begin(),
	       second->get_base_specifiers().end(),
	       changes.base_changes());

  // Compare member types
  compute_diff(first->get_member_types().begin(),
	       first->get_member_types().begin(),
	       first->get_member_types().end(),
	       second->get_member_types().begin(),
	       second->get_member_types().begin(),
	       second->get_member_types().end(),
	       changes.member_types_changes());

  // Compare data member
  compute_diff(first->get_data_members().begin(),
	       first->get_data_members().begin(),
	       first->get_data_members().end(),
	       second->get_data_members().begin(),
	       second->get_data_members().begin(),
	       second->get_data_members().end(),
	       changes.data_members_changes());

  // Compare member functions
  compute_diff(first->get_member_functions().begin(),
	       first->get_member_functions().begin(),
	       first->get_member_functions().end(),
	       second->get_member_functions().begin(),
	       second->get_member_functions().begin(),
	       second->get_member_functions().end(),
	       changes.member_fns_changes());

  // Compare member function templates
  compute_diff(first->get_member_function_templates().begin(),
	       first->get_member_function_templates().begin(),
	       first->get_member_function_templates().end(),
	       second->get_member_function_templates().begin(),
	       second->get_member_function_templates().begin(),
	       second->get_member_function_templates().end(),
	       changes.member_fn_tmpls_changes());

  // Compare member class templates
  compute_diff(first->get_member_class_templates().begin(),
	       first->get_member_class_templates().begin(),
	       first->get_member_class_templates().end(),
	       second->get_member_class_templates().begin(),
	       second->get_member_class_templates().begin(),
	       second->get_member_class_templates().end(),
	       changes.member_class_tmpls_changes());
}

/// Produce a basic report about the changes on class_decl.
///
/// @param changes the changes to report against.
///
/// @param the output stream to report the changes to.
void
report_changes(class_decl_diff &changes,
	       ostream& out)
{
  string name;
  changes.first_scope()->get_qualified_name(name);

  int changes_length = changes.length();
  if (changes_length == 0)
    {
      out << "the two versions of type " << name << "are identical\n";
      return;
    }

  // Now report the changes about the differents parts of the type.

  // First the bases.
  if (!changes.base_changes().is_empty())
    {
      edit_script& e = changes.base_changes();

      // Report deletions.
      int num_deletions = e.num_deletions();
      if (num_deletions == 0)
	out << "No base class deletion\n";
      else if (num_deletions == 1)
	out << "1 base class deletion:\n\t";
      else
	out << num_deletions << " base class deletions:\n\t";

      class_decl_sptr first_class = changes.first_class_decl();
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  shared_ptr<class_decl> base_class =
	    first_class->get_base_specifiers()[i->index()]->get_base_class();

	  if (i != e.deletions().begin())
	    out << ", ";
	  out << base_class->get_qualified_name();
	}

      //Report insertions.
      int num_insertions = e.num_insertions();
      if (num_insertions == 0)
	out << "no base class insertion\n";
      else if (num_insertions == 1)
	out << "1 base class insertion:\n\t";
      else
	out << num_insertions << " base class insertions:\n\t";

      class_decl_sptr second_class = changes.second_class_decl();
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  shared_ptr<class_decl> base_class;
	  for (vector<int>::const_iterator j = i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      base_class =
		second_class->get_base_specifiers()[*j] ->get_base_class();

	      if (i != e.insertions().begin()
		  || j != i->inserted_indexes().begin())
		out << ", ";
	      out << base_class->get_qualified_name();
	    }
	}
    }
}

}// end namespace comparison
} // end namespace abigail
