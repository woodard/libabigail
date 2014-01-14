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
using std::tr1::static_pointer_cast;

/// Try to compute a diff on two instances of DiffType representation.
///
/// The function template performs the diff if and only if the decl
/// representations are of a DiffType.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
///@return the diff of the two types @ref first and @ref second if and
///only if they represent the parametrized type DiffType.  Otherwise,
///returns a NULL pointer value.
template<typename DiffType>
diff_sptr
try_to_diff(const decl_base_sptr first, const decl_base_sptr second)
{
  if (shared_ptr<DiffType> f =
      dynamic_pointer_cast<DiffType>(first))
    {
      shared_ptr<DiffType> s =
	dynamic_pointer_cast<DiffType>(second);
      return compute_diff(f, s);
    }
  return diff_sptr();
}

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
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const decl_base_sptr first, const decl_base_sptr second)
{
  diff_sptr d;

  ((d = try_to_diff<type_decl>(first, second))
   ||(d = try_to_diff<enum_type_decl>(first, second))
   ||(d = try_to_diff<class_decl>(first, second))
   ||(d = try_to_diff<pointer_type_def>(first, second))
   ||(d = try_to_diff<reference_type_def>(first, second))
   ||(d = try_to_diff<qualified_type_def>(first, second))
   ||(d = try_to_diff<typedef_decl>(first, second)));

  return d;
}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// @param first the first construct to consider for the diff
///
/// @param second the second construct to consider for the diff.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const type_base_sptr first, const type_base_sptr second)
{
  decl_base_sptr f = dynamic_pointer_cast<decl_base>(first);
  decl_base_sptr s = dynamic_pointer_cast<decl_base>(second);
  return compute_diff_for_types(f, s);
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
static diff_sptr
compute_diff_for_decls(const decl_base_sptr first, const decl_base_sptr second)
{

  diff_sptr d;

  ((d = try_to_diff<function_decl>(first, second))
   || (d = try_to_diff<var_decl>(first, second)));

  return d;
}

/// Compute the difference between two decls.  The decls can represent
/// either type declarations, or non-type declaration.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @return the resulting diff, or NULL if the diff could not be
/// computed.
diff_sptr
compute_diff(decl_base_sptr first, decl_base_sptr second)
{
  if (!first || !second)
    return diff_sptr();

  diff_sptr d;
  if (is_type(first) && is_type(second))
    d = compute_diff_for_decls(first, second);

  return d;
}

/// Compute the difference between two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @return the resulting diff, or NULL if the diff couldn't be comupted.
diff_sptr
compute_diff(type_base_sptr first, type_base_sptr second)
{
    if (!first || !second)
      return diff_sptr();

  decl_base_sptr f = get_type_declaration(first),
    s = get_type_declaration(second);
  diff_sptr d = compute_diff_for_types(f,s);

  return d;
}

/// Return the length of the diff between two instances of @ref decl_base
///
/// @param first the first instance of @ref decl_base to consider.
///
/// @param second the second instance of @ref decl_base to consider.
///
/// @return the length of the differences between @ref first and @ref second.
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
/// @return the length of the differences between @ref first and @ref second.
static unsigned
diff_length_of_type_bases(type_base_sptr first, type_base_sptr second)
{
  unsigned l = 0;

  if (first->get_size_in_bits() != second->get_alignment_in_bits())
    ++l;
  if (first->get_alignment_in_bits() != second->get_alignment_in_bits())
    ++l;

  return l;
}

/// Stream a string representation for a member function.
///
/// @param mem_fn the member function to stream
///
/// @param out the output stream to send the representation to
static void
represent(class_decl::member_function_sptr mem_fn,
	  ostream& out)
{
  if (!mem_fn)
    return;

  out << "'" << mem_fn->get_pretty_representation() << "'";
  if (mem_fn->get_vtable_offset())
    out << ", virtual at voffset "
	<< mem_fn->get_vtable_offset()
	<< "/"
	<< mem_fn->get_type()->get_class_type()->get_num_virtual_functions()
	<< "\n";
}

/// Stream a string representation for a data member.
///
/// @param data_mem the data member to stream
///
/// @param out the output stream to send the representation to
static void
represent(class_decl::data_member_sptr data_mem,
	  ostream& out)
{
  if (!data_mem || !data_mem->is_laid_out())
    return;

  out << "'" << data_mem->get_pretty_representation() << "'"
      << ", at offset "
      << data_mem->get_offset_in_bits()
      << " (in bits)\n";
}

/// Represent the changes that happened on two versions of a given
/// class data member.
///
/// @param o the older version of the data member.
///
/// @param n the newer version of the data member.
///
/// @param out the output stream to send the representation to.
static void
represent(class_decl::data_member_sptr o,
	  class_decl::data_member_sptr n,
	  ostream& out,
	  const string indent = "")
{
  bool emitted = false;
  string name = o->get_qualified_name();
  string name2 = n->get_qualified_name();
  assert(name == name2);

  if (o->is_laid_out() != n->is_laid_out())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";
      if (o->is_laid_out())
	out << "is no more laid out";
      else
	out << "now becomes laid out";
      emitted = true;
    }
  if (o->get_offset_in_bits() != n->get_offset_in_bits())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";
      out << "offset changed from " << o->get_offset_in_bits()
	  << " to " << n->get_offset_in_bits();
      emitted = true;
    }
  if (o->get_binding() != n->get_binding())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";
      out << "elf binding changed from " << o->get_binding()
	  << " to " << n->get_binding();
      emitted = true;
    }
  if (o->get_visibility() != n->get_visibility())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";
      out << "visibility changed from " << o->get_visibility()
	  << " to " << n->get_visibility();
    }
  if (o->get_access_specifier() != n->get_access_specifier())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";

      out << "access changed from " << o->get_access_specifier()
	  << n->get_access_specifier();
      emitted = true;
    }
  if (o->is_static() != n->is_static())
    {
      if (!emitted)
	out << indent << "'" << name << "' ";
      else
	out << ", ";

      if (o->is_static())
	out << "is no more static";
      else
	out << "now becomes static";
      emitted = true;
    }
  if (*o->get_type() != *n->get_type())
    {
      if (!emitted)
	out << indent << "'" << name << "' type changed:\n";
      else
	out << "\n" << indent << "type changed:\n";
      diff_sptr d = compute_diff_for_types(o->get_type(), n->get_type());
      d->report(out, indent + "  ");
      emitted = false;
    }
  if (emitted)
    out << "\n";
}

/// Report the size and alignement chanages of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param the output stream to report the change to.
///
/// @param indent the string to use for indentation.
///
/// @param nl whether to start the first report line with a new line.
///
/// @return true iff something was reported.
static bool
report_name_size_and_alignment_changes(decl_base_sptr first,
				       decl_base_sptr second,
				       ostream& out,
				       const string& indent,
				       bool nl)
{
  type_base_sptr f = dynamic_pointer_cast<type_base>(first),
    s = dynamic_pointer_cast<type_base>(second);

  if (!s || !f)
    return false;

  bool n = false;
  string fn = first->get_pretty_representation(),
    sn = second->get_pretty_representation();
  if (fn != sn)
    {
      if (nl)
	out << "\n";
      out << indent << "name changed from '"
	  << fn << "' to '" << sn << "'";
      n = true;
    }

  unsigned fs = f->get_size_in_bits(), ss = s->get_size_in_bits(),
    fa = f->get_alignment_in_bits(), sa = s->get_alignment_in_bits();

  if (fs != ss)
    {
      if (n)
	out << "\n";
      out << indent << "size changed from " << fs << " to " << ss << " bits";
      n = true;
    }
  if (fa != sa)
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

/// Represent the kind of difference we want report_mem_header() to
/// report.
enum diff_kind
{
  del_kind,
  ins_kind,
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
		  int number,
		  diff_kind k,
		  const string& section_name,
		  const string& indent)
{
  string change;
  switch (k)
    {
    case del_kind:
      change = (number > 1) ? "deletions" : "deletion";
      break;
    case ins_kind:
      change = (number > 1) ? "insertions" : "insertion";
      break;
    case change_kind:
      change = (number > 1) ? "changes" : "change";
    }

  if (number == 0)
    out << indent << "no " << section_name << " " << change << "\n";
  else if (number == 1)
    out << indent << "1 " << section_name << " " << change << ":\n";
  else
    out << indent << number << " " << section_name
	<< " " << change << ":\n";
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
var_diff::var_diff(var_decl_sptr first,
		   var_decl_sptr second,
		   diff_sptr type_diff)
  : diff(first, second),
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
{
  unsigned l = 0;

  var_decl_sptr f = first_var(), s = second_var();
  if (f->get_binding() != s->get_binding())
    ++l;
  l += diff_length_of_decl_bases(f, s);

  diff_sptr d = compute_diff(f->get_type(), s->get_type());
  l += d->length();

  return l;
}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param the prefix to use for the indentation of this
/// serialization.
void
var_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  decl_base_sptr first = first_var(), second = second_var();
  string n = first->get_pretty_representation();

  if (report_name_size_and_alignment_changes(first, second, out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  if (diff_sptr d = type_diff())
    {
      if (d->length())
	{
	  out << indent << "type of variable changed:\n";
	  d->report(out, indent + " ");
	}
    }
}

/// Compute the diff between two instances of @ref var_decl.
///
/// @param first the first @ref var_decl to consider for the diff.
///
/// @param second the second @ref var_decl to consider for the diff.
///
/// @return the resulting diff between the two @ref var_decl.
var_diff_sptr
compute_diff(const var_decl_sptr first, const var_decl_sptr second)
{
  diff_sptr type_diff = compute_diff(first->get_type(), second->get_type());
  var_diff_sptr d(new var_diff(first, second, type_diff));
  return d;
}

// </var_diff stuff>

/// Constructor for a pointer_diff.
///
/// @param first the first pointer to consider for the diff.
///
/// @param second the secon pointer to consider for the diff.
pointer_diff::pointer_diff(pointer_type_def_sptr first,
			   pointer_type_def_sptr second)
  : diff(first, second),
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
{return underlying_type_diff()->length();}

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
/// @param the prefix to use for the indentation of this
/// serialization.
void
pointer_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      out << indent << "in pointed to type '"
	  << d->first_subject()->get_pretty_representation() << "':\n";
      d->report(out, indent + "  ");
    }
}

/// Compute the diff between between two pointers.
///
/// @param first the pointer to consider for the diff.
///
/// @param second the pointer to consider for the diff.
///
/// @return the resulting diff between the two pointers.
pointer_diff_sptr
compute_diff(pointer_type_def_sptr first,
	     pointer_type_def_sptr second)
{
  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type());
  pointer_diff_sptr result(new pointer_diff(first, second));
  result->underlying_type_diff(d);
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
reference_diff::reference_diff(const reference_type_def_sptr first,
			       const reference_type_def_sptr second)
  : diff(first, second),
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
{return priv_->underlying_type_diff_ = d;}

/// Getter of the length of the diff.
///
/// @return the length of the diff.
unsigned
reference_diff::length() const
{return underlying_type_diff()->length();}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param the string to use for indenting the report.
void
reference_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      out << indent << "in referenced type '"
	  << d->first_subject()->get_pretty_representation() << "':\n";
      d->report(out, indent + "  ");
    }
}

/// Compute the diff between two references.
///
/// @param first the first reference to consider for the diff.
///
/// @param second the second reference to consider for the diff.
reference_diff_sptr
compute_diff(reference_type_def_sptr first,
	     reference_type_def_sptr second)
{
  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type());
  reference_diff_sptr result(new reference_diff(first, second));
  result->underlying_type_diff(d);
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
qualified_type_diff::qualified_type_diff(qualified_type_def_sptr first,
					 qualified_type_def_sptr second)
  : diff(first, second),
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
/// @param the string to use to indent the lines of the report.
void
qualified_type_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  string fname = first_qualified_type()->get_pretty_representation(),
    sname = second_qualified_type()->get_pretty_representation();

  if (fname != sname)
    {
      out << indent << "'" << fname << "' changed to '" << sname << "'\n";
      return;
    }

  type_base_sptr flt = get_leaf_type(first_qualified_type()),
    slt = get_leaf_type(second_qualified_type());
  string fltname = get_type_declaration(flt)->get_pretty_representation(),
    sltname = get_type_declaration(slt)->get_pretty_representation();

  diff_sptr d = compute_diff_for_types(flt, slt);
  out << indent << "in unqualified underlying type '" << fltname << "':\n";
  d->report(out, indent + "  ");
}

/// Compute the diff between two qualified types.
///
/// @param first the first qualified type to consider for the diff.
///
/// @param second the second qualified type to consider for the diff.
qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr first,
	     const qualified_type_def_sptr second)
{
  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type());
  qualified_type_diff_sptr result(new qualified_type_diff(first, second));
  result->underlying_type_diff(d);
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
	    priv_->inserted_enumerators_[name] = n;
	  }
      }

    for (string_enumerator_map::const_iterator i =
	   priv_->deleted_enumerators_.begin();
	 i != priv_->deleted_enumerators_.end();
	 ++i)
      {
	string_enumerator_map::const_iterator r =
	  priv_->inserted_enumerators_.find(i->first);
	if (r != priv_->inserted_enumerators_.end())
	  priv_->changed_enumerators_[i->first] =
	    std::make_pair(i->second, r->second);
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
enum_diff::enum_diff(const enum_type_decl_sptr first,
		     const enum_type_decl_sptr second,
		     const diff_sptr underlying_type_diff)
  : diff(first, second),
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
  return (underlying_type_diff()->length()
	  + priv_->enumerators_changes_.length());
}

/// Report the differences between the two enums.
///
/// @param out the output stream to send the report to.
///
/// @param the string to use for indentation.
void
enum_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  string name = first_enum()->get_pretty_representation();

  enum_type_decl_sptr first = first_enum(), second = second_enum();

  if (report_name_size_and_alignment_changes(first, second, out, indent,
					     /*start_with_num_line=*/false))
    out << "\n";

  // name
  if (first->get_name() != second->get_name())
    out << indent << "enum name changed from '"
	<< first->get_qualified_name() << "' to '"
	<< second->get_qualified_name() << "'\n";

  //underlying type
  underlying_type_diff()->report(out, indent);

  //report deletions/insertions/change of enumerators
  unsigned numdels = deleted_enumerators().size();
  unsigned numins = inserted_enumerators().size();
  unsigned numchanges = changed_enumerators().size();
  assert(numchanges <= numdels
	 && numchanges <= numins);
  numdels -= numchanges;
  numins -= numchanges;

  if (numdels)
    {
      report_mem_header(out, numdels, del_kind, "enumerator", indent);
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
      report_mem_header(out, numins, ins_kind, "enumerator", indent);
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
      report_mem_header(out, numchanges, change_kind, "enumerator", indent);
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

/// Compute the set of changes between two instances of @ref
/// enum_type_decl.
///
/// @param first a pointer to the first enum_type_decl to consider.
///
/// @param second a pointer to the second enum_type_decl to consider.
///
/// @return the resulting diff of the two enums @ref first and @ref
/// second.
enum_diff_sptr
compute_diff(const enum_type_decl_sptr first,
	     const enum_type_decl_sptr second)
{
  diff_sptr ud = compute_diff_for_types(first->get_underlying_type(),
					second->get_underlying_type());
  enum_diff_sptr d(new enum_diff(first, second, ud));

  compute_diff(first->get_enumerators().begin(),
	       first->get_enumerators().end(),
	       second->get_enumerators().begin(),
	       second->get_enumerators().end(),
	       d->priv_->enumerators_changes_);

  d->ensure_lookup_tables_populated();

  return d;
}
// </enum_diff stuff>

//<class_diff stuff>

struct class_diff::priv
{
  edit_script base_changes_;
  edit_script member_types_changes_;
  edit_script data_members_changes_;
  edit_script member_fns_changes_;
  edit_script member_fn_tmpls_changes_;
  edit_script member_class_tmpls_changes_;

  string_decl_base_sptr_map deleted_bases_;
  string_decl_base_sptr_map inserted_bases_;
  string_changed_type_or_decl_map changed_bases_;
  string_decl_base_sptr_map deleted_member_types_;
  string_decl_base_sptr_map inserted_member_types_;
  string_changed_type_or_decl_map changed_member_types_;
  string_decl_base_sptr_map deleted_data_members_;
  string_decl_base_sptr_map inserted_data_members_;
  string_changed_type_or_decl_map changed_data_members_;
  string_decl_base_sptr_map deleted_member_class_tmpls_;
  string_decl_base_sptr_map inserted_member_class_tmpls_;
  string_changed_type_or_decl_map changed_member_class_tmpls_;

  decl_base_sptr
  base_has_changed(decl_base_sptr) const;

  decl_base_sptr
  member_type_has_changed(decl_base_sptr) const;

  decl_base_sptr
  data_member_has_changed(decl_base_sptr) const;

  decl_base_sptr
  member_class_tmpl_has_changed(decl_base_sptr) const;
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
  priv_->changed_data_members_.clear();
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
	  && priv_->changed_data_members_.empty()
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
	decl_base_sptr b =
	  first_class_decl()->get_base_specifiers()[i]->get_base_class();
	string qname = b->get_qualified_name();
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
	    decl_base_sptr b =
	      second_class_decl()->get_base_specifiers()[i]->get_base_class();
	    string qname = b->get_qualified_name();
	    assert(priv_->inserted_bases_.find(qname)
		   == priv_->inserted_bases_.end());
	    priv_->inserted_bases_[qname] = b;
	  }
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_bases_.begin();
	 i != priv_->deleted_bases_.end();
	 ++i)
      {
	string_decl_base_sptr_map::const_iterator r =
	  priv_->inserted_bases_.find(i->first);

	if (r != priv_->inserted_bases_.end()
	    && i->second != r->second)
	  priv_->changed_bases_[i->first]
	    = std::make_pair(i->second, r->second);
      }
  }

  {
    edit_script& e = priv_->member_types_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d = first_class_decl()->get_member_types()[i];
	string qname = d->get_qualified_name();
	assert(priv_->deleted_member_types_.find(qname)
	       == priv_->deleted_member_types_.end());
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
	    decl_base_sptr d = second_class_decl()->get_member_types()[i];
	    string qname = d->get_qualified_name();
	    assert(priv_->inserted_member_types_.find(qname)
		   == priv_->inserted_member_types_.end());
	    priv_->inserted_member_types_[qname] = d;
	  }
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_member_types_.begin();
	 i != priv_->deleted_member_types_.end();
	 ++i)
      {
	string_decl_base_sptr_map::const_iterator r =
	  priv_->inserted_member_types_.find(i->first);

	if (r != priv_->inserted_member_types_.end()
	    && i->second != r->second)
	  priv_->changed_member_types_[i->first]
	    = std::make_pair(i->second, r->second);
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
	    priv_->inserted_data_members_[qname] = d;
	  }
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_data_members_.begin();
	 i != priv_->deleted_data_members_.end();
	 ++i)
      {
	string_decl_base_sptr_map::const_iterator r =
	  priv_->inserted_data_members_.find(i->first);

	if (r != priv_->inserted_data_members_.end()
	    && i->second != r->second)
	  priv_->changed_data_members_[i->first]
	    = std::make_pair(i->second, r->second);
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
	    priv_->inserted_member_class_tmpls_[qname] = d;
	  }
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_member_class_tmpls_.begin();
	 i != priv_->deleted_member_class_tmpls_.end();
	 ++i)
      {
	string_decl_base_sptr_map::const_iterator r =
	  priv_->inserted_member_class_tmpls_.find(i->first);

	if (r != priv_->inserted_member_class_tmpls_.end()
	    && i->second != r->second)
	  priv_->changed_member_class_tmpls_[i->first]
	    = std::make_pair(i->second, r->second);
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
decl_base_sptr
class_diff::priv::base_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_changed_type_or_decl_map::const_iterator it =
    changed_bases_.find(qname);

  return (it == changed_bases_.end()) ? decl_base_sptr() : it->second.second;

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
class_diff::priv::data_member_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_changed_type_or_decl_map::const_iterator it =
    changed_data_members_.find(qname);

  return ((it == changed_data_members_.end())
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

/// Constructor of class_diff
///
/// @param scope the scope of the class_diff.
class_diff::class_diff(shared_ptr<class_decl> first_scope,
				 shared_ptr<class_decl> second_scope)
  : diff(first_scope, second_scope),
    priv_(new priv)
{}

/// Getter for the length of the diff.
///
/// @return the length of the diff.
unsigned
class_diff::length() const
{
  return (base_changes().length()
	  + member_types_changes()
	  + data_members_changes()
	  + member_fns_changes()
	  + member_fn_tmpls_changes()
	  + member_class_tmpls_changes());
}

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

/// @return the edit script of the member functions of the two
/// classes.
const edit_script&
class_diff::member_fns_changes() const
{return priv_->member_fns_changes_;}

/// @return the edit script of the member functions of the two
/// classes.
edit_script&
class_diff::member_fns_changes()
{return priv_->member_fns_changes_;}

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
  int changes_length = length();

  if (changes_length== 0)
    return;

  string name = first_subject()->get_pretty_representation();

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = first_class_decl(),
    second = second_class_decl();

  if (report_name_size_and_alignment_changes(first, second, out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  // bases classes
  if (const edit_script& e = base_changes())
    {
      // Report deletions.
      int numdels = e.num_deletions();
      int numchanges = priv_->changed_bases_.size();
      assert(numchanges <= numdels);
      numdels -= numchanges;

      if (numdels)
	{
	  report_mem_header(out, numdels, del_kind,
			    "base class", indent);

	  for (vector<deletion>::const_iterator i = e.deletions().begin();
	       i != e.deletions().end();
	       ++i)
	    {
	      if (i != e.deletions().begin())
		out << "\n";

	      class_decl_sptr base_class =
		first->get_base_specifiers()[i->index()]->
		get_base_class();

	      if ( priv_->base_has_changed(base_class))
		continue;
	      out << indent << "  " << base_class->get_qualified_name();
	    }
	  out << "\n\n";
	}
      // Report changes.
      if (numchanges)
	{
	  report_mem_header(out, numchanges, change_kind,
			    "base class", indent);
	  for (string_changed_type_or_decl_map::const_iterator it =
		 priv_->changed_bases_.begin();
	       it != priv_->changed_bases_.end();
	       ++it)
	    {
	      class_decl_sptr o =
		dynamic_pointer_cast<class_decl>(it->second.first);
	      class_decl_sptr n =
		dynamic_pointer_cast<class_decl>(it->second.second);
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed:\n";
	      diff_sptr dif = compute_diff(o, n);
	      dif->report(out, indent + "  ");
	    }
	  out << "\n";
	}

      //Report insertions.
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;
      if (numins)
	{
	  report_mem_header(out, numins, ins_kind,
			    "base class", indent);

	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      shared_ptr<class_decl> b;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  if (emitted)
		    out << "\n";

		  b= second->get_base_specifiers()[*j] ->
		    get_base_class();
		  if (!priv_->base_has_changed(b))
		    {
		      out << indent << b->get_qualified_name();
		      emitted = true;
		    }
		}
	    }
	  out << "\n";
	}
    }

  // member types
  if (const edit_script& e = member_types_changes())
    {
      int numchanges = priv_->changed_member_types_.size();
      int numdels = e.num_deletions();
      assert(numchanges <= numdels);
      numdels -= numchanges;

      // report deletions
      if (numdels)
	{
	  report_mem_header(out, numdels, del_kind,
			    "member type", indent);

	  for (vector<deletion>::const_iterator i = e.deletions().begin();
	       i != e.deletions().end();
	       ++i)
	    {
	      if (i != e.deletions().begin())
		out << "\n";
	      decl_base_sptr mem_type = first->get_member_types()[i->index()];

	      if (decl_base_sptr n = priv_->member_type_has_changed(mem_type))
		continue;
	      out << indent << "  '"
		  << mem_type->get_pretty_representation()
		  << "'";
	    }
	  out << "\n\n";
	}
      // report changes
      if (numchanges)
	{
	  report_mem_header(out, numchanges, change_kind,
			    "member type", indent);

	  for (string_changed_type_or_decl_map::const_iterator it =
		 priv_->changed_member_types_.begin();
	       it != priv_->changed_member_types_.end();
	       ++it)
	    {
	      decl_base_sptr o =
		get_type_declaration(as_non_member_type(it->second.first));
	      decl_base_sptr n =
		get_type_declaration(as_non_member_type(it->second.second));
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed:\n";
	      diff_sptr dif = compute_diff_for_types(o, n);
	      dif->report(out, indent + "  ");
	    }
	  out << "\n";
	}

      // report insertions
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;

      if (numins)
	{
	  report_mem_header(out, numins, ins_kind,
			    "member type", indent);

	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      class_decl::member_type_sptr mem_type;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  if (emitted)
		    out << "\n";
		  mem_type = second->get_member_types()[*j];
		  if (!priv_->member_type_has_changed(mem_type))
		    {
		      out << indent << "  '"
			  << mem_type->get_pretty_representation()
			  << "'";
		      emitted = true;
		    }
		}
	    }
	  out << "\n\n";
	}
    }

  // data members
  if (const edit_script& e = data_members_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      int numchanges = priv_->changed_data_members_.size();
      assert(numchanges <= numdels);
      numdels -= numchanges;

      if (numdels)
	{
	  report_mem_header(out, numdels, del_kind,
			    "data member", indent);
	  bool emitted = false;
	  for (vector<deletion>::const_iterator i = e.deletions().begin();
	       i != e.deletions().end();
	       ++i)
	    {
	      class_decl::data_member_sptr data_mem =
		first->get_data_members()[i->index()];

	      if (priv_->data_member_has_changed(data_mem))
		continue;

	      if (emitted)
		out << "\n";

	      out << indent << "  ";
	      represent(data_mem, out);
	      emitted = true;
	    }
	  out << "\n";
	}

      // report change
      if (numchanges)
	{
	  report_mem_header(out, numchanges, change_kind,
			    "data member", indent);

	  for (string_changed_type_or_decl_map::const_iterator it =
		 priv_->changed_data_members_.begin();
	       it != priv_->changed_data_members_.end();
	       ++it)
	    {
	      class_decl::data_member_sptr o =
		dynamic_pointer_cast<class_decl::data_member>(it->second.first);
	      class_decl::data_member_sptr n =
		dynamic_pointer_cast<class_decl::data_member>(it->second.second);
	      represent(o, n, out, indent + " ");
	    }
	  out << "\n";
	}

      //report insertions
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;
      if (numins)
	{
	  report_mem_header(out, numins, ins_kind,
			    "data member", indent);
	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      class_decl::data_member_sptr data_mem;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  data_mem = second->get_data_members()[*j];
		  if (priv_->data_member_has_changed(data_mem))
		    continue;
		  if (emitted)
		    out << "\n";
		  out << indent << "  ";
		  represent(data_mem, out);
		  emitted = true;
		}
	    }
	  if (emitted)
	    out << "\n";
	}
    }

  // member_fns
  if (const edit_script& e = member_fns_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, del_kind,
			  "member function", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  class_decl::member_function_sptr mem_fun =
	    first->get_member_functions()[i->index()];
	  out << indent << "  ";
	  represent(mem_fun, out);
	}
      if (numdels)
	out << "\n";

      // report insertions;
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, ins_kind,
			  "member function", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  class_decl::member_function_sptr mem_fun;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_fun = second->get_member_functions()[*j];
	      out << indent << "  ";
	      represent(mem_fun, out);
	      emitted = true;
	    }
	}
      if (emitted)
	out << "\n";
    }

  // member function templates
  if (const edit_script& e = member_fn_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, del_kind,
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
	report_mem_header(out, numins, ins_kind,
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
	report_mem_header(out, numdels, del_kind,
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
	report_mem_header(out, numins, ins_kind,
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
}

/// Compute the set of changes between two instances of class_decl.
///
/// @param first the first class_decl to consider.
///
/// @param second the second class_decl to consider.
///
/// @return changes the resulting changes.
class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second)
{
  class_diff_sptr changes(new class_diff(first, second));

  // Compare base specs
  compute_diff(first->get_base_specifiers().begin(),
	       first->get_base_specifiers().end(),
	       second->get_base_specifiers().begin(),
	       second->get_base_specifiers().end(),
	       changes->base_changes());

  // Compare member types
  compute_diff(first->get_member_types().begin(),
	       first->get_member_types().end(),
	       second->get_member_types().begin(),
	       second->get_member_types().end(),
	       changes->member_types_changes());

  // Compare data member
  compute_diff(first->get_data_members().begin(),
	       first->get_data_members().end(),
	       second->get_data_members().begin(),
	       second->get_data_members().end(),
	       changes->data_members_changes());

  // Compare member functions
  compute_diff(first->get_member_functions().begin(),
	       first->get_member_functions().end(),
	       second->get_member_functions().begin(),
	       second->get_member_functions().end(),
	       changes->member_fns_changes());

  // Compare member function templates
  compute_diff(first->get_member_function_templates().begin(),
	       first->get_member_function_templates().end(),
	       second->get_member_function_templates().begin(),
	       second->get_member_function_templates().end(),
	       changes->member_fn_tmpls_changes());

  // Compare member class templates
  compute_diff(first->get_member_class_templates().begin(),
	       first->get_member_class_templates().end(),
	       second->get_member_class_templates().begin(),
	       second->get_member_class_templates().end(),
	       changes->member_class_tmpls_changes());

  changes->ensure_lookup_tables_populated();

  return changes;
}

//</class_diff stuff>

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
      if (r != priv_->inserted_types_.end()
	  && i->second != r->second)
	priv_->changed_types_[i->first] = std::make_pair(i->second, r->second);
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_decls_.find(i->first);
      if (r != priv_->inserted_decls_.end()
	  && i->second != r->second)
	priv_->changed_decls_[i->first] = std::make_pair(i->second, r->second);
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
scope_diff::scope_diff(scope_decl_sptr first_scope,
		       scope_decl_sptr second_scope)
  : diff(first_scope, second_scope),
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
/// @param changes the changes to report about.
///
/// @param out the out stream to report the changes to.
void
scope_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
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
					      i->second.second);
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
					      i->second.second);
      if (diff)
	diff->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i = removed_types().begin();
       i != removed_types().end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (removed_types().size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i = removed_decls().begin();
       i != removed_decls().end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (removed_decls().size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i = added_types().begin();
       i != added_types().end();
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
  for (string_decl_base_sptr_map::const_iterator i = added_decls().begin();
       i != added_decls().end();
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
scope_diff_sptr
compute_diff(const scope_decl_sptr first,
	     const scope_decl_sptr second,
	     scope_diff_sptr d)
{
  assert(d->first_scope() == first && d->second_scope() == second);

  compute_diff(first->get_member_decls().begin(),
	       first->get_member_decls().end(),
	       second->get_member_decls().begin(),
	       second->get_member_decls().end(),
	       d->member_changes());

  d->ensure_lookup_tables_populated();

  return d;
}

/// Compute the diff between two scopes.
///
/// @param first the first scope to consider in computing the diff.
///
/// @param second the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @return return the resulting diff
scope_diff_sptr
compute_diff(const scope_decl_sptr first_scope,
	     const scope_decl_sptr second_scope)
{
    scope_diff_sptr d(new scope_diff(first_scope, second_scope));
    return compute_diff(first_scope, second_scope, d);
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


  diff_sptr return_type_diff_;
  edit_script parm_changes_;
  vector<char> first_fn_flags_;
  vector<char> second_fn_flags_;
  edit_script fn_flags_changes_;

  // useful lookup tables.
  string_parm_map deleted_parms_;
  string_parm_map inserted_parms_;

  string_changed_parm_map changed_parms_;
  string_parm_map removed_parms_;
  string_parm_map added_parms_;

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
  string parm_type_name;
  function_decl::parameter_sptr parm;
  for (vector<deletion>::const_iterator i =
	 priv_->parm_changes_.deletions().begin();
       i != priv_->parm_changes_.deletions().end();
       ++i)
    {
      parm = deleted_parameter_at(i->index());
      parm_type_name = get_type_name(parm->get_type());
      // If for a reason the type name is empty we want to know and
      // fix that.
      assert(!parm_type_name.empty());
      priv_->deleted_parms_[parm_type_name] = parm;
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
	  parm = inserted_parameter_at(*j);
	  parm_type_name = get_type_name(parm->get_type());
	  // If for a reason the type name is empty we want to know and
	  // fix that.
	  assert(!parm_type_name.empty());
	  priv_->inserted_parms_[parm_type_name] = parm;
	}
    }

  for (string_parm_map::const_iterator i = priv_->deleted_parms_.begin();
       i != priv_->deleted_parms_.end();
       ++i)
    {
      string_parm_map::const_iterator j;
      if ((j = priv_->inserted_parms_.find(i->first))
	  == priv_->inserted_parms_.end())
	priv_->removed_parms_[i->first] = i->second;
      else
	priv_->changed_parms_[i->first] = changed_parm(i->second,
						       j->second);
    }

  for (string_parm_map::const_iterator i = priv_->inserted_parms_.begin();
       i != priv_->inserted_parms_.end();
       ++i)
    {
      string_parm_map::const_iterator j;
      if ((j = priv_->deleted_parms_.find(i->first)) ==
	  priv_->deleted_parms_.end())
	priv_->added_parms_[i->first] = i->second;
    }
}

/// Constructor for function_decl_diff
///
/// @param first the first function considered by the diff.
///
/// @param second the second function considered by the diff.
function_decl_diff::function_decl_diff(const function_decl_sptr first,
				       const function_decl_sptr second)
  : diff(first, second),
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

/// @return a map of the parameters whose type got changed.  The key
/// of the map is the name of the type.
const string_changed_parm_map&
function_decl_diff::changed_parms() const
{return priv_->changed_parms_;}

/// @return a map of parameters that got removed.
const string_parm_map&
function_decl_diff::removed_parms() const
{return priv_->removed_parms_;}

/// @return a map of parameters that got added.
const string_parm_map&
function_decl_diff::added_parms() const
{return priv_->added_parms_;}

/// @return the length of the changes of the function.
unsigned
function_decl_diff::length() const
{return changed_parms().size() + removed_parms().size() + added_parms().size();}

/// Serialize a report of the changes encapsulated in the current
/// instance of function_decl_diff over to an output stream.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
function_decl_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  // Report about return type differences.
  if (priv_->return_type_diff_)
    priv_->return_type_diff_->report(out, indent);

  // Hmmh, the above was quick.  Now report about function
  // parameters; this shouldn't as straightforward.
  //
  // Report about the parameter types that have changed.
  for (string_changed_parm_map::const_iterator i =
	 priv_->changed_parms_.begin();
       i != priv_->changed_parms_.end();
       ++i)
    {
      out << indent
	  << "parameter " << i->second.first->get_index()
	  << " of type '" << get_type_name(i->second.first->get_type())
	  << "' changed:\n";
      diff_sptr d = compute_diff_for_types(i->second.first->get_type(),
					   i->second.second->get_type());
      if (d)
	d->report(out, indent + "  ");
    }

  // Report about the parameters that got removed.
  bool emitted = false;
  for (string_parm_map::const_iterator i = priv_->removed_parms_.begin();
       i != priv_->removed_parms_.end();
       ++i)
    {
      out << indent << "parameter " << i->second->get_index()
	  << " of type '" << get_type_name(i->second->get_type())
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
	  << " of type '" << get_type_name(i->second->get_type())
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// Compute the diff between two function_decl.
///
/// @param first the first function_decl to consider for the diff
///
/// @param second the second function_decl to consider for the diff
///
/// @return the computed diff
function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second)
{
  if (first && second)
    {
      function_decl_diff_sptr result(new function_decl_diff(first, second));

      result->priv_->return_type_diff_ =
	compute_diff_for_types(first->get_return_type(),
			       second->get_return_type());

      diff_utils::compute_diff(first->get_parameters().begin(),
			       first->get_parameters().end(),
			       second->get_parameters().begin(),
			       second->get_parameters().end(),
			       result->priv_->parm_changes_);

      diff_utils::compute_diff(result->priv_->first_fn_flags_.begin(),
			       result->priv_->first_fn_flags_.end(),
			       result->priv_->second_fn_flags_.begin(),
			       result->priv_->second_fn_flags_.end(),
			       result->priv_->fn_flags_changes_);

      result->ensure_lookup_tables_populated();

      return result;
    }
  // TODO: implement this for either first or second being NULL.
  return function_decl_diff_sptr();
}

// </function_decl_diff stuff>

// <type_decl_diff stuff>

/// Constructor for type_decl_diff.
type_decl_diff::type_decl_diff(const type_decl_sptr first,
			       const type_decl_sptr second)
  : diff(first, second)
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
/// @param the string to use for indentatino indent.
void
type_decl_diff::report(ostream& out, const string& indent) const
{
  if (length() == 0)
    return;

  type_decl_sptr f = first_type_decl(), s = second_type_decl();

  string name = f->get_pretty_representation();

  bool n = report_name_size_and_alignment_changes(f, s, out, indent,
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
/// @return a pointer to the resulting type_decl_diff.
type_decl_diff_sptr
compute_diff(const type_decl_sptr first, const type_decl_sptr second)
{
  type_decl_diff_sptr result(new type_decl_diff(first, second));

  // We don't need to actually compute a diff here as a type_decl
  // doesn't have complicated sub-components.  type_decl_diff::report
  // just walks the members of the type_decls and display information
  // about the ones that have changed.  On a similar note,
  // type_decl_diff::length returns 0 if the two type_decls are equal,
  // and 1 otherwise.

  return result;
}

// </type_decl_diff stuff>

// <typedef_diff stuff>

struct typedef_diff::priv
{
  diff_sptr underlying_type_diff_;
};//end struct typedef_diff::priv

/// Constructor for typedef_diff.
typedef_diff::typedef_diff(const typedef_decl_sptr first,
			   const typedef_decl_sptr second)
  : diff(first, second),
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
  if (!underlying_type_diff())
    return 0;
  return underlying_type_diff()->length();
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
  if (length() == 0)
    return;

  bool emit_nl = false;
  typedef_decl_sptr f = first_typedef_decl(), s = second_typedef_decl();
  if (f->get_name() != s->get_name())
    {
      out << indent << "typedef name changed from "
	  << f->get_name() << " to " << s->get_name() << "\n";
      emit_nl = true;
    }

  if (diff_sptr d = underlying_type_diff())
    {
      out << indent << "underlying type changed:\n";
      d->report(out, indent + "  ");
      emit_nl = false;
    }

  if (emit_nl)
    out << "\n";
}

/// Compute a diff between two typedef_decl.
///
/// @param first a pointer to the first typedef_decl to consider.
///
/// @param second a pointer to the second typedef_decl to consider.
///
/// @return a pointer to the the resulting typedef_diff.
typedef_diff_sptr
compute_diff(const typedef_decl_sptr first, const typedef_decl_sptr second)
{
  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type());
  typedef_diff_sptr result(new typedef_diff(first, second));
  result->underlying_type_diff(d);
  return result;
}

// </typedef_diff stuff>

// <translation_unit_diff stuff>

/// Constructor for translation_unit_diff.
///
/// @param first the first translation unit to consider for this diff.
///
/// @param second the second translation unit to consider for this diff.
translation_unit_diff::translation_unit_diff(translation_unit_sptr first,
					     translation_unit_sptr second)
  : scope_diff(first->get_global_scope(), second->get_global_scope())
{
}

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

/// Compute the diff between two translation_units.
///
/// @param first the first translation_unit to consider.
///
/// @param second the second translation_unit to consider.
translation_unit_diff_sptr
compute_diff(const translation_unit_sptr first,
	     const translation_unit_sptr second)
{
  // TODO: handle first or second having empty contents.
  translation_unit_diff_sptr tu_diff(new translation_unit_diff(first, second));
  scope_diff_sptr sc_diff = dynamic_pointer_cast<scope_diff>(tu_diff);

  compute_diff(static_pointer_cast<scope_decl>(first->get_global_scope()),
	       static_pointer_cast<scope_decl>(second->get_global_scope()),
	       sc_diff);

  return tu_diff;
}

// </translation_unit_diff stuff>

// <corpus stuff>
struct corpus_diff::priv
{
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
	  n = deleted_fn->get_name();
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
	      n = added_fn->get_name();
	    assert(!n.empty());
	    assert(added_fns_.find(n) == added_fns_.end());
	    added_fns_[n] = added_fn;
	  }
      }

    for (string_function_ptr_map::const_iterator it = deleted_fns_.begin();
	 it != deleted_fns_.end();
	 ++it)
      {
	string_function_ptr_map::const_iterator it2 =
	  added_fns_.find(it->first);
	if (it2 != added_fns_.end())
	  changed_fns_[it->first] = std::make_pair(it->second,
						   it2->second);
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
	  n = deleted_var->get_name();
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
	    added_vars_[n] = added_var;
	  }
      }

    for (string_var_ptr_map::const_iterator it = deleted_vars_.begin();
	 it != deleted_vars_.end();
	 ++it)
      {
	string_var_ptr_map::const_iterator it2 =
	  added_vars_.find(it->first);
	if (it2 != added_vars_.end())
	  changed_vars_[it->first] = std::make_pair(it->second,
						    it2->second);
      }
  }
}

/// Constructor for @ref corpus_diff.
///
/// @param first the first corpus of the diff.
///
/// @param second the second corpus of the diff.
corpus_diff::corpus_diff(corpus_sptr first,
			 corpus_sptr second)
  : priv_(new priv)
{
  priv_->first_ = first;
  priv_->second_ = second;
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

/// @return the length of the changes as recorded by the diff.
unsigned
corpus_diff::length() const
{
  return (priv_->deleted_fns_.size()
	  + priv_->added_fns_.size()
	  - priv_->changed_fns_.size());
}

/// A deleter for shared pointers that ... doesn't delete the object
/// managed by the shared pointer.
struct noop_deleter
{
  template<typename T>
  void
  operator()(T*)
  {}
};

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param the prefix to use for the indentation of this
/// serialization.
void
corpus_diff::report(ostream& out, const string& indent) const
{
  unsigned removed = 0, added = 0;

  /// Report added/removed/changed functions.
  for (string_function_ptr_map::const_iterator i =
	 priv_->deleted_fns_.begin();
       i != priv_->deleted_fns_.end();
       ++i)
    {
      if (priv_->added_fns_.find(i->first) == priv_->added_fns_.end())
	{
	  out << indent
	      << "  '"
	      << i->second->get_pretty_representation()
	      << "' was removed\n";
	  ++removed;
	}
    }
  if (removed)
    out << "\n";

  for (string_function_ptr_map::const_iterator i =
	 priv_->added_fns_.begin();
       i != priv_->added_fns_.end();
       ++i)
    {
      if (priv_->deleted_fns_.find(i->first) == priv_->deleted_fns_.end())
	{
	  out << indent
	      << "  '"
	      << i->second->get_pretty_representation()
	      << "' was added\n";
	  ++added;
	}
    }
  if (added)
    out << "\n";

  for (string_changed_function_ptr_map::const_iterator i =
	 priv_->changed_fns_.begin();
       i != priv_->changed_fns_.end();
       ++i)
    {
      out << indent << "  '"
	  << i->second.first->get_pretty_representation()
	  << "' was changed to '"
	  << i->second.second->get_pretty_representation()
	  << "':\n";
      {
	function_decl_sptr f(i->second.first, noop_deleter());
	function_decl_sptr s(i->second.second, noop_deleter());

	diff_sptr diff = compute_diff_for_decls(f, s);
	if (diff)
	  diff->report(out, indent + "    ");
      }
    }
  if (priv_->changed_fns_.size())
    out << "\n";


  /// Report added/removed/changed variables.
  for (string_var_ptr_map::const_iterator i =
	 priv_->deleted_vars_.begin();
       i != priv_->deleted_vars_.end();
       ++i)
    {
      if (priv_->added_vars_.find(i->first) == priv_->added_vars_.end())
	{
	  out << indent
	      << "  '"
	      << i->second->get_pretty_representation()
	      << "' was removed\n";
	  ++removed;
	}
    }
  if (removed)
    out << "\n";

  for (string_var_ptr_map::const_iterator i =
	 priv_->added_vars_.begin();
       i != priv_->added_vars_.end();
       ++i)
    {
      if (priv_->deleted_vars_.find(i->first) == priv_->deleted_vars_.end())
	{
	  out << indent
	      << "  '"
	      << i->second->get_pretty_representation()
	      << "' was added\n";
	  ++added;
	}
    }
  if (added)
    out << "\n";

  for (string_changed_var_ptr_map::const_iterator i =
	 priv_->changed_vars_.begin();
       i != priv_->changed_vars_.end();
       ++i)
    {
      out << indent << "  '"
	  << i->second.first->get_pretty_representation()
	  << "' was changed to '"
	  << i->second.second->get_pretty_representation()
	  << "':\n";
      {
	var_decl_sptr f(i->second.first, noop_deleter());
	var_decl_sptr s(i->second.second, noop_deleter());

	diff_sptr diff = compute_diff_for_decls(f, s);
	if (diff)
	  diff->report(out, indent + "    ");
      }
    }
  if (priv_->changed_vars_.size())
    out << "\n";
}

/// Compute the diff between two instances fo the @ref corpus
///
/// @param f the first @ref corpus to consider for the diff.
///
/// @param s the second @ref corpus to consider for the diff.
///
/// @return the resulting diff between the two @ref corpus.
corpus_diff_sptr
compute_diff(const corpus_sptr f, const corpus_sptr s)
{
  typedef corpus::functions::const_iterator fns_it_type;
  typedef corpus::variables::const_iterator vars_it_type;
  typedef diff_utils::deep_ptr_eq_functor eq_type;

  corpus_diff_sptr r(new corpus_diff(f, s));

  diff_utils::compute_diff<fns_it_type, eq_type>(f->get_functions().begin(),
						 f->get_functions().end(),
						 s->get_functions().begin(),
						 s->get_functions().end(),
						 r->priv_->fns_edit_script_);

  diff_utils::compute_diff<vars_it_type, eq_type>(f->get_variables().begin(),
						  f->get_variables().end(),
						  s->get_variables().begin(),
						  s->get_variables().end(),
						  r->priv_->vars_edit_script_);

  r->priv_->ensure_lookup_tables_populated();

  return r;
}
// </corpus stuff>

}// end namespace comparison
} // end namespace abigail
