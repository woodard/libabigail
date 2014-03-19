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

/// @file

#ifndef __ABG_IRFWD_H__
#define __ABG_IRFWD_H__

#include <cstddef>
#include <tr1/memory>
#include <list>
#include <vector>
#include <string>
#include <tr1/functional>
#include <typeinfo>
#include <utility> // for std::rel_ops, at least.
#include <ostream>
#include "abg-hash.h"

/// Toplevel namespace for libabigail.
namespace abigail
{
/**
   @mainpage libabigail


   The GNU Application Binary Interface Generic Analysis and
   Instrumentation Library.

   This is an interface to the GNU Compiler Collection for the
   collection and analysis of compiler-generated binaries.

   Checkout out the project homepage <a
   href="http://sourceware.org/libabigail"> here</a>.

   The current libabigail source code can be checked out with:
   git clone git://sourceware.org/git/libabigail.git.

   The mailing list to send messages and patches to is
   libabigail@sourceware.org.
*/

// Inject some types.
using std::tr1::shared_ptr;
using std::string;
using std::vector;

// Pull in relational operators.
using namespace std::rel_ops;

// Forward declarations for corpus.

class corpus;

// Forward declarations for ir.
class location;
class location_manager;
class translation_unit;
class class_decl;
class class_tdecl;
class decl_base;
class enum_type_decl;
class function_decl;
class function_tdecl;
class function_type;
class global_scope;
class node_visitor;
class location;
class location_manager;
class method_type;
class namespace_decl;
class parameter;
class pointer_type_def;
class qualified_type_def;
class reference_type_def;
class scope_decl;
class scope_type_decl;
class template_decl;
class template_parameter;
class non_type_tparameter;
class type_tparameter;
class template_tparameter;

class type_composition;
class type_base;
class type_decl;
class typedef_decl;
class var_decl;

struct type_shared_ptr_equal;
struct traversable_base;

shared_ptr<decl_base>
add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

shared_ptr<decl_base>
add_decl_to_scope (shared_ptr<decl_base>, shared_ptr<scope_decl>);

const global_scope*
get_global_scope(const shared_ptr<decl_base>);

translation_unit*
get_translation_unit(const shared_ptr<decl_base>);

translation_unit*
get_translation_unit(decl_base*);

bool
is_global_scope(const scope_decl*);

bool
is_global_scope(const shared_ptr<scope_decl>);

bool
is_at_global_scope(const shared_ptr<decl_base>);

bool
is_at_class_scope(const shared_ptr<decl_base>);

bool
is_at_class_scope(const decl_base*);

bool
is_at_class_scope(const decl_base&);

bool
is_at_template_scope(const shared_ptr<decl_base>);

bool
is_template_parameter(const shared_ptr<decl_base>);

bool
is_type(const decl_base&);

shared_ptr<type_base>
is_type(const shared_ptr<decl_base>);

shared_ptr<class_decl>
look_through_decl_only_class(shared_ptr<class_decl>);

bool
is_var_decl(const shared_ptr<decl_base>);

bool
is_template_parm_composition_type(const shared_ptr<decl_base>);

bool
is_template_decl(const shared_ptr<decl_base>);

bool
is_function_template_pattern(const shared_ptr<decl_base>);

shared_ptr<decl_base>
add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

shared_ptr<decl_base>
add_decl_to_scope(shared_ptr<decl_base>, shared_ptr<scope_decl>);

shared_ptr<decl_base>
insert_decl_into_scope(shared_ptr<decl_base>,
		       vector<shared_ptr<decl_base> >::iterator,
		       scope_decl*);

shared_ptr<decl_base>
insert_decl_into_scope(shared_ptr<decl_base>,
		       vector<shared_ptr<decl_base> >::iterator,
		       shared_ptr<scope_decl>);

bool
has_scope(const decl_base&);

bool
has_scope(const shared_ptr<decl_base>);

bool
is_member_decl(const shared_ptr<decl_base>);

bool
is_member_decl(const decl_base*);

bool
is_member_decl(const decl_base&);

bool
is_member_type(const shared_ptr<type_base>);

bool
is_member_type(const shared_ptr<decl_base>);

void
remove_decl_from_scope(shared_ptr<decl_base>);

bool
get_member_is_static(const decl_base&);

bool
get_member_is_static(const decl_base*);

bool
get_member_is_static(const shared_ptr<decl_base>);

void
set_member_is_static(decl_base&,
		     bool);

void
set_member_is_static(shared_ptr<decl_base>,
		     bool);

bool
is_data_member(const var_decl&);

bool
is_data_member(const var_decl*);

bool
is_data_member(const shared_ptr<var_decl>);

void
set_data_member_offset(shared_ptr<var_decl>, size_t);

size_t
get_data_member_offset(const var_decl&);

size_t
get_data_member_offset(const shared_ptr<var_decl>);

void
set_data_member_is_laid_out(shared_ptr<var_decl>, bool);

bool
get_data_member_is_laid_out(const var_decl&);

bool
get_data_member_is_laid_out(const shared_ptr<var_decl>);

const global_scope*
get_global_scope(const decl_base* decl);

translation_unit*
get_translation_unit(decl_base* decl);

translation_unit*
get_translation_unit(const shared_ptr<decl_base>);

string
get_type_name(const shared_ptr<type_base>);

const decl_base*
get_type_declaration(const type_base*);

decl_base*
get_type_declaration(type_base*);

shared_ptr<decl_base>
get_type_declaration(const shared_ptr<type_base>);

const scope_decl*
get_top_most_scope_under(const decl_base*,
			 const scope_decl*);

const scope_decl*
get_top_most_scope_under(const shared_ptr<decl_base>,
			 const scope_decl*);

const scope_decl*
get_top_most_scope_under(const shared_ptr<decl_base>,
			 const shared_ptr<scope_decl>);

void
fqn_to_components(const std::string&,
		  std::list<string>&);

const shared_ptr<decl_base>
lookup_type_in_translation_unit(const string&,
				const translation_unit&);

const shared_ptr<decl_base>
lookup_type_in_translation_unit(const std::list<string>&,
				const translation_unit&);

const shared_ptr<decl_base>
lookup_type_in_scope(const string&,
		     const shared_ptr<scope_decl>);

const shared_ptr<decl_base>
lookup_type_in_scope(const std::list<string>&,
		     const shared_ptr<scope_decl>);

const shared_ptr<decl_base>
lookup_var_decl_in_scope(const string&,
			 const shared_ptr<scope_decl>);

const shared_ptr<decl_base>
lookup_var_decl_in_scope(const std::list<string>&,
			 const shared_ptr<scope_decl>);

string
demangle_cplus_mangled_name(const string&);

void
dump(const shared_ptr<decl_base>, std::ostream&);

void
dump(const shared_ptr<decl_base>);

void
dump(const shared_ptr<type_base>, std::ostream&);

void
dump(const shared_ptr<type_base>);

void
dump(const shared_ptr<var_decl>, std::ostream&);

void
dump(const shared_ptr<var_decl>);

void
dump(const translation_unit&, std::ostream&);

void
dump(const translation_unit&);

void
dump(const shared_ptr<translation_unit>, std::ostream&);

void
dump(const shared_ptr<translation_unit>);

} // end namespace abigail
#endif // __ABG_IRFWD_H__
