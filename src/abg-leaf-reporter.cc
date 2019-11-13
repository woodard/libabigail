// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2019 Red Hat, Inc.
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
/// This is the implementation of the
/// abigail::comparison::default_reporter type.

#include "abg-comparison-priv.h"
#include "abg-reporter.h"
#include "abg-reporter-priv.h"

namespace abigail
{
namespace comparison
{

/// Test if a diff node is to be reported by the current instance of
/// @ref leaf_reporter.
///
/// A node is said to be reported by the current instance of @ref
/// leaf_reporter if the node carries local changes and if the node's
/// reporting hasn't been suppressed.
bool
leaf_reporter::diff_to_be_reported(const diff *d) const
{return d && d->to_be_reported() && d->has_local_changes();}

/// Report the changes carried by the diffs contained in an instance
/// of @ref string_diff_ptr_map.
///
/// @param mapp the set of diffs to report for.
///
/// @param out the output stream to report the diffs to.
///
/// @param indent the string to use for indentation.
static void
report_diffs(const reporter_base& r,
	     const string_diff_ptr_map& mapp,
	     ostream& out,
	     const string& indent)
{
  diff_ptrs_type sorted_diffs;
  sort_string_diff_ptr_map(mapp, sorted_diffs);

  bool started_to_emit = false;
  for (diff_ptrs_type::const_iterator i = sorted_diffs.begin();
       i != sorted_diffs.end();
       ++i)
    {
      if (const var_diff *d = is_var_diff(*i))
	if (is_data_member(d->first_var()))
	  continue;

      if (r.diff_to_be_reported((*i)->get_canonical_diff()))
	{
	  if (started_to_emit)
	    out << "\n\n";

	  string n = (*i)->first_subject()->get_pretty_representation();

	  out << indent << "'" << n ;

	  report_loc_info((*i)->first_subject(),
			  *(*i)->context(), out);

	  out << "' changed:\n";

	  (*i)->get_canonical_diff()->report(out, indent + "  ");
	  out << "\n";
	  started_to_emit = true;
	}
    }
}

/// Report the type changes carried by an instance of @ref diff_maps.
///
/// @param maps the set of diffs to report.
///
/// @param out the output stream to report the diffs to.
///
/// @param indent the string to use for indentation.
static void
report_type_changes_from_diff_maps(const leaf_reporter& reporter,
				   const diff_maps& maps,
				   ostream& out,
				   const string& indent)
{
  // basic types
  report_diffs(reporter, maps.get_type_decl_diff_map(), out, indent);

  // enums
  report_diffs(reporter, maps.get_enum_diff_map(), out, indent);

  // classes
  report_diffs(reporter, maps.get_class_diff_map(), out, indent);

  // unions
  report_diffs(reporter, maps.get_union_diff_map(), out, indent);

  // typedefs
  report_diffs(reporter, maps.get_typedef_diff_map(), out, indent);

  // arrays
  report_diffs(reporter, maps.get_array_diff_map(), out, indent);

  // It doesn't make sense to report function type changes, does it?
  // report_diffs(reporter, maps.get_function_type_diff_map(), out, indent);

  // distinct diffs
  report_diffs(reporter, maps.get_distinct_diff_map(), out, indent);

  // function parameter diffs
  report_diffs(reporter, maps.get_fn_parm_diff_map(), out, indent);
}

/// Report the changes carried by an instance of @ref diff_maps.
///
/// @param maps the set of diffs to report.
///
/// @param out the output stream to report the diffs to.
///
/// @param indent the string to use for indentation.
void
leaf_reporter::report_changes_from_diff_maps(const diff_maps& maps,
					     ostream& out,
					     const string& indent) const
{
  report_type_changes_from_diff_maps(*this, maps, out, indent);

  // function decls
  report_diffs(*this, maps.get_function_decl_diff_map(), out, indent);

  // var decl
  report_diffs(*this, maps.get_var_decl_diff_map(), out, indent);
}

/// Report the changes carried by a @ref typedef_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const typedef_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  report_local_typedef_changes(d, out, indent);

  maybe_report_interfaces_impacted_by_diff(&d, out, indent);
}

/// Report the changes carried by a @ref qualified_type_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const qualified_type_diff& d, ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  report_local_qualified_type_changes(d, out, indent);
}

/// Report the changes carried by a @ref pointer_diff node.
///
/// Note that this function does nothing because a @ref pointer_diff
/// node never carries local changes.
void
leaf_reporter::report(const pointer_diff &d,
		      ostream& out,
		      const string& indent) const
{
  // Changes that modify the representation of a pointed-to type is
  // considered local to the pointer type.
  if (!diff_to_be_reported(&d))
    return;

  out << indent
      << "pointer type changed from: '"
      << d.first_pointer()->get_pretty_representation()
      << "' to: '"
      << d.second_pointer()->get_pretty_representation()
      << "'\n";
}

/// Report the changes carried by a @ref reference_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const reference_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  report_local_reference_type_changes(d, out, indent);
}

/// Report the changes carried by a @ref fn_parm_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const fn_parm_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  ABG_ASSERT(diff_to_be_reported(d.type_diff().get()));

  function_decl::parameter_sptr f = d.first_parameter();

  out << indent
      << "parameter " << f->get_index();

  report_loc_info(f, *d.context(), out);

  out << " of type '"
      << f->get_type_pretty_representation()
      << "' changed:\n";
  d.type_diff()->report(out, indent);
}

/// Report the changes carried by a @ref function_type_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const function_type_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  report_local_function_type_changes(d, out, indent);

  if (diff_to_be_reported(d.priv_->return_type_diff_.get()))
    {
      out << indent << "return type changed:\n";
      d.priv_->return_type_diff_->report(out, indent + "  ");
    }

  // Hmmh, the above was quick.  Now report about function parameters;
  //
  // Report about the parameter types that have changed sub-types.
  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 d.priv_->sorted_subtype_changed_parms_.begin();
       i != d.priv_->sorted_subtype_changed_parms_.end();
       ++i)
    {
      diff_sptr dif = *i;
      if (diff_to_be_reported(dif.get()))
	dif->report(out, indent);
    }
}

/// Report the changes carried by a @ref scope_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const scope_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!d.to_be_reported())
    return;

  // Report changed types.
  unsigned num_changed_types = d.changed_types().size();
  if (num_changed_types)
    out << indent << "changed types:\n";

  for (diff_sptrs_type::const_iterator dif = d.changed_types().begin();
       dif != d.changed_types().end();
       ++dif)
    {
      if (!*dif || !diff_to_be_reported((*dif).get()))
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
	  << "' changed:\n";
      (*dif)->report(out, indent + "    ");
    }

  // Report changed decls
  unsigned num_changed_decls = d.changed_decls().size();
  if (num_changed_decls)
    out << indent << "changed declarations:\n";

  for (diff_sptrs_type::const_iterator dif= d.changed_decls().begin();
       dif != d.changed_decls().end ();
       ++dif)
    {
      if (!*dif || !diff_to_be_reported((*dif).get()))
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
          << "' was changed to '"
          << (*dif)->second_subject()->get_pretty_representation() << "'";
      report_loc_info((*dif)->second_subject(), *d.context(), out);
      out << ":\n";

      (*dif)->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_types_.begin();
       i != d.priv_->deleted_types_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_types_.size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_decls_.begin();
       i != d.priv_->deleted_decls_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_decls_.size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->inserted_types_.begin();
       i != d.priv_->inserted_types_.end();
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
	 d.priv_->inserted_decls_.begin();
       i != d.priv_->inserted_decls_.end();
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

/// Report the changes carried by a @ref array_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const array_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(d.first_array(),
						    d.second_array(),
						    "array type");

  report_name_size_and_alignment_changes(d.first_array(),
					 d.second_array(),
					 d.context(),
					 out, indent,
					 /*new line=*/false);

  diff_sptr dif = d.element_type_diff();
  if (diff_to_be_reported(dif.get()))
    {
      string fn = ir::get_pretty_representation(is_type(dif->first_subject()));
      // report array element type changes
      out << indent << "array element type '"
	  << fn << "' changed: \n";
      dif->report(out, indent + "  ");
    }

  report_loc_info(d.second_array(), *d.context(), out);

  maybe_report_interfaces_impacted_by_diff(&d, out, indent);
}

/// Report the changes carried by a @ref class_or_union_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const class_or_union_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  class_or_union_sptr first = d.first_class_or_union(),
    second = d.second_class_or_union();

  const diff_context_sptr& ctxt = d.context();

  // Report class decl-only -> definition change.
  if (ctxt->get_allowed_category() & CLASS_DECL_ONLY_DEF_CHANGE_CATEGORY)
    if (filtering::has_class_decl_only_def_change(first, second))
      {
	string was =
	  first->get_is_declaration_only()
	  ? " was a declaration-only type"
	  : " was a defined type";

	string is_now =
	  second->get_is_declaration_only()
	  ? " and is now a declaration-only type"
	  : " and is now a defined type";

	out << indent << "type " << first->get_pretty_representation()
	    << was << is_now;
	return;
      }

  // member functions
  if (d.member_fns_changes())
    {
      // report deletions
      int numdels = d.get_priv()->deleted_member_functions_.size();
      size_t num_filtered =
	d.get_priv()->count_filtered_deleted_mem_fns(ctxt);
      if (numdels)
	report_mem_header(out, numdels, num_filtered, del_kind,
			  "member function", indent);
      bool emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     d.get_priv()->deleted_member_functions_.begin();
	   i != d.get_priv()->deleted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (emitted
	      && i != d.get_priv()->deleted_member_functions_.begin())
	    out << "\n";
	  method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report insertions;
      int numins = d.get_priv()->inserted_member_functions_.size();
      num_filtered = d.get_priv()->count_filtered_inserted_mem_fns(ctxt);
      if (numins)
	report_mem_header(out, numins, num_filtered, ins_kind,
			  "member function", indent);
      emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     d.get_priv()->inserted_member_functions_.begin();
	   i != d.get_priv()->inserted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (emitted
	      && i != d.get_priv()->inserted_member_functions_.begin())
	    out << "\n";
	  method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report member function with sub-types changes
      int numchanges = d.get_priv()->sorted_changed_member_functions_.size();
      if (numchanges)
	report_mem_header(out, change_kind, "member function", indent);
      emitted = false;
      for (function_decl_diff_sptrs_type::const_iterator i =
	     d.get_priv()->sorted_changed_member_functions_.begin();
	   i != d.get_priv()->sorted_changed_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !(get_member_function_is_virtual
		   ((*i)->first_function_decl()))
	      && !(get_member_function_is_virtual
		   ((*i)->second_function_decl())))
	    continue;

	  diff_sptr diff = *i;
	  if (!diff_to_be_reported(diff.get()))
	    continue;

	  string repr =
	    (*i)->first_function_decl()->get_pretty_representation();
	  if (emitted
	      && i != d.get_priv()->sorted_changed_member_functions_.begin())
	    out << "\n";
	  out << indent << "  '" << repr << "' has some changes:\n";
	  diff->report(out, indent + "    ");
	  emitted = true;
	}
      if (numchanges)
	out << "\n";
    }

  // data members
  if (d.data_members_changes())
    {
      // report deletions
      int numdels = d.class_or_union_diff::get_priv()->
	get_deleted_non_static_data_members_number();
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->deleted_data_members_,
	     sorted_dms);
	  bool emitted = false;
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      ABG_ASSERT(data_mem);
	      if (get_member_is_static(data_mem))
		continue;
	      if (emitted)
		out << "\n";
	      out << indent << "  ";
	      represent_data_member(data_mem, ctxt, out);
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      //report insertions
      int numins =
	d.class_or_union_diff::get_priv()->inserted_data_members_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->inserted_data_members_,
	     sorted_dms);
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      ABG_ASSERT(data_mem);
	      out << indent << "  ";
	      represent_data_member(data_mem, ctxt, out);
	    }
	}

      // report changes
      size_t numchanges =
	d.class_or_union_diff::get_priv()->sorted_changed_dm_.size();
      size_t num_filtered =
	d.class_or_union_diff::get_priv()->
	count_filtered_changed_dm(/*local_only =*/ true);
      ABG_ASSERT(numchanges >= num_filtered);
      size_t net_numchanges = numchanges - num_filtered;

      bool emitted_data_members_changes = false;
      if (net_numchanges)
	{
	  report_mem_header(out, subtype_change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->
		 sorted_changed_dm_.begin();
	       it != d.class_or_union_diff::get_priv()->
		 sorted_changed_dm_.end();
	       ++it)
	    {
	      if (diff_to_be_reported((*it).get()))
		{
		  represent(*it, ctxt, out, indent + " ",
			    /*local_only=*/true);
		  out << "\n";
		  emitted_data_members_changes = true;
		}
	    }
	}

      numchanges =
	d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.size();
      num_filtered =
	d.class_or_union_diff::get_priv()->
	count_filtered_subtype_changed_dm(/*local_only =*/ true);
      ABG_ASSERT(numchanges >= num_filtered);
      net_numchanges = numchanges - num_filtered;

      if (net_numchanges)
	{
	  if (!emitted_data_members_changes)
	    report_mem_header(out, change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.begin();
	       it != d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.end();
	       ++it)
	    {
	      if (diff_to_be_reported((*it).get()))
		{
		  represent(*it, ctxt, out, indent + " ",
			    /*local_only=*/true);
		  out << "\n";
		  emitted_data_members_changes = true;
		}
	    }
	}
    }
}

/// Report the changes carried by a @ref class_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const class_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  string name = d.first_subject()->get_pretty_representation();

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = d.first_class_decl(),
    second = d.second_class_decl();

  if (report_name_size_and_alignment_changes(first, second, d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  const diff_context_sptr& ctxt = d.context();
  maybe_report_diff_for_member(first, second, ctxt, out, indent);

  d.class_or_union_diff::report(out, indent);

  maybe_report_interfaces_impacted_by_diff(&d, out, indent);

  d.reported_once(true);
}

/// Report the changes carried by a @ref union_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const union_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  // Now report the changes about the differents parts of the type.
  union_decl_sptr first = d.first_union_decl(), second = d.second_union_decl();

  if (report_name_size_and_alignment_changes(first, second, d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second,d. context(), out, indent);

  d.class_or_union_diff::report(out, indent);

  maybe_report_interfaces_impacted_by_diff(&d, out, indent);
}

/// Report the changes carried by a @ref distinct_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const distinct_diff& d,
		      ostream& out,
		      const string& indent) const
{
    if (!diff_to_be_reported(&d))
    return;

  type_or_decl_base_sptr f = d.first(), s = d.second();

  string f_repr = f ? f->get_pretty_representation() : "'void'";
  string s_repr = s ? s->get_pretty_representation() : "'void'";

  diff_sptr diff = d.compatible_child_diff();

  string compatible = diff ? " to compatible type '": " to '";

  out << indent << "entity changed from '" << f_repr << "'"
      << compatible << s_repr << "'";
  report_loc_info(s, *d.context(), out);
  out << "\n";

  if (report_size_and_alignment_changes(f, s, d.context(), out, indent,
					/*start_with_new_line=*/false))
    out << "\n";

  maybe_report_interfaces_impacted_by_diff(&d, out, indent);
}

/// Report the changes carried by a @ref function_decl_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const function_decl_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  maybe_report_diff_for_member(d.first_function_decl(),
			       d.second_function_decl(),
			       d.context(), out, indent);

  function_decl_sptr ff = d.first_function_decl();
  function_decl_sptr sf = d.second_function_decl();

  diff_context_sptr ctxt = d.context();
  corpus_sptr fc = ctxt->get_corpus_diff()->first_corpus();
  corpus_sptr sc = ctxt->get_corpus_diff()->second_corpus();

  string qn1 = ff->get_qualified_name(), qn2 = sf->get_qualified_name(),
    linkage_names1, linkage_names2;
  elf_symbol_sptr s1 = ff->get_symbol(), s2 = sf->get_symbol();

  if (s1)
    linkage_names1 = s1->get_id_string();
  if (s2)
    linkage_names2 = s2->get_id_string();

  // If the symbols for ff and sf have aliases, get all the names of
  // the aliases;
  if (fc && s1)
    linkage_names1 =
      s1->get_aliases_id_string(fc->get_fun_symbol_map());
  if (sc && s2)
    linkage_names2 =
      s2->get_aliases_id_string(sc->get_fun_symbol_map());

  /// If the set of linkage names of the function have changed, report
  /// it.
  if (linkage_names1 != linkage_names2)
    {
      if (linkage_names1.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " didn't have any linkage name, and it now has: '"
	      << linkage_names2 << "'\n";
	}
      else if (linkage_names2.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " did have linkage names '" << linkage_names1
	      << "'\n"
	      << indent << "but it doesn't have any linkage name anymore\n";
	}
      else
	out << indent << "linkage names of "
	    << ff->get_pretty_representation()
	    << "\n" << indent << "changed from '"
	    << linkage_names1 << "' to '" << linkage_names2 << "'\n";
    }

  if (qn1 != qn2
      && diff_to_be_reported(d.type_diff().get()))
    {
      // So the function has sub-type changes that are to be
      // reported.  Let's see if the function name changed too; if it
      // did, then we'd report that change right before reporting the
      // sub-type changes.
      string frep1 = d.first_function_decl()->get_pretty_representation(),
	frep2 = d.second_function_decl()->get_pretty_representation();
      out << indent << "'" << frep1 << " {" << linkage_names1<< "}"
	  << "' now becomes '"
	  << frep2 << " {" << linkage_names2 << "}" << "'\n";
    }

  maybe_report_diff_for_symbol(ff->get_symbol(),
			       sf->get_symbol(),
			       ctxt, out, indent);

  // Now report about inline-ness changes
  if (ff->is_declared_inline() != sf->is_declared_inline())
    {
      out << indent;
      if (ff->is_declared_inline())
	out << sf->get_pretty_representation()
	    << " is not declared inline anymore\n";
      else
	out << sf->get_pretty_representation()
	    << " is now declared inline\n";
    }

  // Report about vtable offset changes.
  if (is_member_function(ff) && is_member_function(sf))
    {
      bool ff_is_virtual = get_member_function_is_virtual(ff),
	sf_is_virtual = get_member_function_is_virtual(sf);
      if (ff_is_virtual != sf_is_virtual)
	{
	  out << indent;
	  if (ff_is_virtual)
	    out << ff->get_pretty_representation()
		<< " is no more declared virtual\n";
	  else
	    out << ff->get_pretty_representation()
		<< " is now declared virtual\n";
	}

      size_t ff_vtable_offset = get_member_function_vtable_offset(ff),
	sf_vtable_offset = get_member_function_vtable_offset(sf);
      if (ff_is_virtual && sf_is_virtual
	  && (ff_vtable_offset != sf_vtable_offset))
	{
	  out << indent
	      << "the vtable offset of "  << ff->get_pretty_representation()
	      << " changed from " << ff_vtable_offset
	      << " to " << sf_vtable_offset << "\n";
	}

      // the classes of the two member functions.
      class_decl_sptr fc =
	is_class_type(is_method_type(ff->get_type())->get_class_type());
      class_decl_sptr sc =
	is_class_type(is_method_type(sf->get_type())->get_class_type());

      // Detect if the virtual member function changes above
      // introduced a vtable change or not.
      bool vtable_added = false, vtable_removed = false;
      if (!fc->get_is_declaration_only() && !sc->get_is_declaration_only())
	{
	  vtable_added = !fc->has_vtable() && sc->has_vtable();
	  vtable_removed = fc->has_vtable() && !sc->has_vtable();
	}
      bool vtable_changed = ((ff_is_virtual != sf_is_virtual)
			     || (ff_vtable_offset != sf_vtable_offset));
      bool incompatible_change = (ff_vtable_offset != sf_vtable_offset);

      if (vtable_added)
	out << indent
	    << "  note that a vtable was added to "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_removed)
	out << indent
	    << "  note that the vtable was removed from "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_changed)
	{
	  out << indent;
	  if (incompatible_change)
	    out << "  note that this is an ABI incompatible "
	      "change to the vtable of ";
	  else
	    out << "  note that this induces a change to the vtable of ";
	  out << fc->get_pretty_representation()
	      << "\n";
	}

    }

  // Report about function type differences.
  if (diff_to_be_reported(d.type_diff().get()))
    d.type_diff()->report(out, indent);
}

/// Report the changes carried by a @ref var_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const var_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!diff_to_be_reported(&d))
    return;

  decl_base_sptr first = d.first_var(), second = d.second_var();
  string n = first->get_pretty_representation();

  if (report_name_size_and_alignment_changes(first, second,
					     d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_symbol(d.first_var()->get_symbol(),
			       d.second_var()->get_symbol(),
			       d.context(), out, indent);

  maybe_report_diff_for_member(first, second, d.context(), out, indent);

  if (diff_sptr dif = d.type_diff())
    {
      if (diff_to_be_reported(dif.get()))
	{
	  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "type");
	  out << indent << "type of variable changed:\n";
	  dif->report(out, indent + " ");
	}
    }
}

/// Report the changes carried by a @ref translation_unit_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const translation_unit_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!d.to_be_reported())
    return;

  static_cast<const scope_diff&>(d).report(out, indent);
}

/// Report the changes carried by a @ref corpus_diff node.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
void
leaf_reporter::report(const corpus_diff& d,
		      ostream& out,
		      const string& indent) const
{
  if (!d.has_changes())
    return;

  const diff_context_sptr& ctxt = d.context();

  size_t removed = 0, added = 0;
  const corpus_diff::diff_stats &s =
    const_cast<corpus_diff&>(d).
    apply_filters_and_suppressions_before_reporting();

  d.priv_->emit_diff_stats(s, out, indent);
  if (ctxt->show_stats_only())
    return;

  out << "\n";

  if (ctxt->show_soname_change()
      && !d.priv_->sonames_equal_)
    out << indent << "SONAME changed from '"
	<< d.first_corpus()->get_soname() << "' to '"
	<< d.second_corpus()->get_soname() << "'\n\n";

  if (ctxt->show_architecture_change()
      && !d.priv_->architectures_equal_)
    out << indent << "architecture changed from '"
	<< d.first_corpus()->get_architecture_name() << "' to '"
	<< d.second_corpus()->get_architecture_name() << "'\n\n";

  if (ctxt->show_deleted_fns())
    {
      if (s.net_num_func_removed() == 1)
	out << indent << "1 Removed function:\n\n";
      else if (s.net_num_func_removed() > 1)
	out << indent << s.net_num_func_removed() << " Removed functions:\n\n";

      vector<function_decl*>sorted_deleted_fns;
      sort_string_function_ptr_map(d.priv_->deleted_fns_, sorted_deleted_fns);
      for (vector<function_decl*>::const_iterator i =
	     sorted_deleted_fns.begin();
	   i != sorted_deleted_fns.end();
	   ++i)
	{
	  if (d.priv_->deleted_function_is_suppressed(*i))
	    continue;

	  out << indent
	      << "  ";
	  out << "[D] ";
	  out << "'" << (*i)->get_pretty_representation() << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.first_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this removes an entry from the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  ++removed;
	}
      if (removed)
	out << "\n";
    }

  if (ctxt->show_changed_fns())
    {
      // Show changed functions.
      size_t num_changed = s.net_num_leaf_func_changes();
      if (num_changed == 1)
	out << indent << "1 function with some sub-type change:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " functions with some sub-type change:\n\n";

      bool changed = false;
      vector<function_decl_diff_sptr> sorted_changed_fns;
      sort_string_function_decl_diff_sptr_map(d.priv_->changed_fns_map_,
					      sorted_changed_fns);
      for (vector<function_decl_diff_sptr>::const_iterator i =
	     sorted_changed_fns.begin();
	   i != sorted_changed_fns.end();
	   ++i)
	{
	  diff_sptr diff = *i;
	  if (!diff)
	    continue;

	  if (diff_to_be_reported(diff.get()))
	    {
	      function_decl_sptr fn = (*i)->first_function_decl();
	      out << indent << "  [C]'"
		  << fn->get_pretty_representation() << "'";
	      report_loc_info((*i)->second_function_decl(), *ctxt, out);
	      out << " has some sub-type changes:\n";
	      if ((fn->get_symbol()->has_aliases()
		   && !(is_member_function(fn)
			&& get_member_function_is_ctor(fn))
		   && !(is_member_function(fn)
			&& get_member_function_is_dtor(fn)))
		  || (is_c_language(get_translation_unit(fn)->get_language())
		      && fn->get_name() != fn->get_linkage_name()))
		{
		  int number_of_aliases =
		    fn->get_symbol()->get_number_of_aliases();
		  if (number_of_aliases == 0)
		    {
		      out << indent << "    "
			  << "Please note that the exported symbol of "
			"this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n";
		    }
		  else
		    {
		      out << indent << "    "
			  << "Please note that the symbol of this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n     and it aliases symbol";
		      if (number_of_aliases > 1)
			out << "s";
		      out << ": "
			  << fn->get_symbol()->get_aliases_id_string(false)
			  << "\n";
		    }
		}
	      diff->report(out, indent + "    ");
	      out << "\n";
	      changed |= true;
	    }
	}
      if (changed)
	{
	  out << "\n";
	  changed = false;
	}
    }

  if (ctxt->show_added_fns())
    {
      if (s.net_num_func_added() == 1)
	out << indent << "1 Added function:\n\n";
      else if (s.net_num_func_added() > 1)
	out << indent << s.net_num_func_added()
	    << " Added functions:\n\n";
      vector<function_decl*> sorted_added_fns;
      sort_string_function_ptr_map(d.priv_->added_fns_, sorted_added_fns);
      for (vector<function_decl*>::const_iterator i = sorted_added_fns.begin();
	   i != sorted_added_fns.end();
	   ++i)
	{
	  if (d.priv_->added_function_is_suppressed(*i))
	    continue;

	  out
	    << indent
	    << "  ";
	  out << "[A] ";
	  out << "'"
	      << (*i)->get_pretty_representation()
	      << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases
		(out, "", *(*i)->get_symbol(),
		 d.second_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this adds a new entry to the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  ++added;
	}
      if (added)
	{
	  out << "\n";
	  added = false;
	}
    }

  // Now show the changed types.
  const diff_maps& leaf_diffs = d.get_leaf_diffs();
  report_type_changes_from_diff_maps(*this, leaf_diffs, out, indent);

  // Report added/removed/changed types not reacheable from public
  // interfaces.
  maybe_report_unreachable_type_changes(d, s, indent, out);
}
} // end namespace comparison
} // end namespace abigail
