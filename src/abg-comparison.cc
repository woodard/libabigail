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
#include "abg-comparison.h"

namespace abigail
{

namespace comparison
{

// Inject types from outside in here.
using std::vector;
using std::tr1::dynamic_pointer_cast;

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
  unordered_map<string, changed_type_or_decl> changed_types_;
  unordered_map<string, changed_type_or_decl> changed_decls_;

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
	  assert(priv_->deleted_types_.find(qname)
		 == priv_->deleted_types_.end());
	  priv_->deleted_types_[qname] = decl;
	}
      else
	{
	  assert(priv_->deleted_decls_.find(qname)
		 == priv_->deleted_types_.end());
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
	      assert(priv_->inserted_types_.find(qname)
		     == priv_->inserted_types_.end());
	      priv_->inserted_types_[qname] = decl;
	    }
	  else
	    {
	      assert(priv_->inserted_decls_.find(qname)
		     == priv_->inserted_decls_.end());
	      priv_->inserted_decls_[qname] = decl;
	    }
	}
    }

  // Populate changed_types_/changed_decls_
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_types_.find(i->first);
      if (r != priv_->inserted_types_.end())
	priv_->changed_types_[i->first] = std::make_pair(i->second, r->second);
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_decls_.find(i->first);
      if (r != priv_->inserted_decls_.end())
	priv_->changed_types_[i->first] = std::make_pair(i->second, r->second);
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

scope_diff::scope_diff(scope_decl_sptr first_scope,
		       scope_decl_sptr second_scope)
  : diff(first_scope, second_scope),
    priv_(new priv)
{}

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
{return first_scope()->get_member_decls()[i];}

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
{return second_scope()->get_member_decls()[i];}

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
const unordered_map<string, scope_diff::changed_type_or_decl>&
scope_diff::changed_types() const
{return priv_->changed_types_;}

/// @return a map containing the decls which content has changed from
/// the first scope to the other.
const unordered_map<string, scope_diff::changed_type_or_decl>&
scope_diff::changed_decls() const
{return priv_->changed_decls_;}

/// Compute the diff between two scopes.
///
/// @param first the first scope to consider in computing the diff.
///
/// @param second the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param d an out parameter that is populated with the result of the
/// computed diff.
void
compute_diff(scope_decl_sptr first,
	     scope_decl_sptr second,
	     scope_diff& d)
{
  assert(first && second);

  compute_diff(first->get_member_decls().begin(),
	       first->get_member_decls().end(),
	       second->get_member_decls().begin(),
	       second->get_member_decls().end(),
	       d.member_changes());

  d.ensure_lookup_tables_populated();
}

//</scope_diff stuff>


//<class_decl_diff stuff>
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
	       first->get_base_specifiers().end(),
	       second->get_base_specifiers().begin(),
	       second->get_base_specifiers().end(),
	       changes.base_changes());

  // Compare member types
  compute_diff(first->get_member_types().begin(),
	       first->get_member_types().end(),
	       second->get_member_types().begin(),
	       second->get_member_types().end(),
	       changes.member_types_changes());

  // Compare data member
  compute_diff(first->get_data_members().begin(),
	       first->get_data_members().end(),
	       second->get_data_members().begin(),
	       second->get_data_members().end(),
	       changes.data_members_changes());

  // Compare member functions
  compute_diff(first->get_member_functions().begin(),
	       first->get_member_functions().end(),
	       second->get_member_functions().begin(),
	       second->get_member_functions().end(),
	       changes.member_fns_changes());

  // Compare member function templates
  compute_diff(first->get_member_function_templates().begin(),
	       first->get_member_function_templates().end(),
	       second->get_member_function_templates().begin(),
	       second->get_member_function_templates().end(),
	       changes.member_fn_tmpls_changes());

  // Compare member class templates
  compute_diff(first->get_member_class_templates().begin(),
	       first->get_member_class_templates().end(),
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
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
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
//</class_decl_diff stuff>

}// end namespace comparison
} // end namespace abigail
