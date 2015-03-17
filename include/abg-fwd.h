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

   This is the API documentation of the Application Binary
   Interface Generic Analysis and Instrumentation Library, aka,
   <em>libabigail</em>.

   Check out <a href="http://sourceware.org/libabigail"> the project
   homepage</a>!

   The current libabigail source code can be browsed at
   http://sourceware.org/git/gitweb.cgi?p=libabigail.git

   It can be checked out with:
       <em>git clone git://sourceware.org/git/libabigail.git</em>

   The mailing list to send messages and patches to is
   libabigail@sourceware.org.

   You can hang out with libabigail developers and users on irc at
   irc://irc.oftc.net\#libabigail.
*/

// Inject some types.
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::string;
using std::vector;

// Pull in relational operators.
using namespace std::rel_ops;

namespace ir
{

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
class array_type_def;
class subrange_type;

struct type_shared_ptr_equal;
struct traversable_base;

shared_ptr<decl_base>
add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

shared_ptr<decl_base>
add_decl_to_scope (shared_ptr<decl_base>, shared_ptr<scope_decl>);

const global_scope*
get_global_scope(const decl_base&);

const global_scope*
get_global_scope(const decl_base*);

const global_scope*
get_global_scope(const shared_ptr<decl_base>);

translation_unit*
get_translation_unit(const decl_base&);

translation_unit*
get_translation_unit(const decl_base*);

translation_unit*
get_translation_unit(const shared_ptr<decl_base>);

bool
is_global_scope(const scope_decl&);

bool
is_global_scope(const scope_decl*);

bool
is_global_scope(const shared_ptr<scope_decl>);

bool
is_at_global_scope(const decl_base&);

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

shared_ptr<function_decl>
is_function_decl(shared_ptr<decl_base>);

bool
is_type(const decl_base&);

shared_ptr<type_base>
is_type(const shared_ptr<decl_base>);

type_base*
is_type(decl_base*);

shared_ptr<type_decl>
is_type_decl(const shared_ptr<type_base>);

shared_ptr<typedef_decl>
is_typedef(const shared_ptr<type_base>);

shared_ptr<typedef_decl>
is_typedef(const shared_ptr<decl_base>);

shared_ptr<enum_type_decl>
is_enum_type(const shared_ptr<type_base>&);

shared_ptr<enum_type_decl>
is_enum_type(const shared_ptr<decl_base>&);

class_decl*
is_class_type(decl_base*);

class_decl*
is_class_type(type_base*);

shared_ptr<class_decl>
is_class_type(const shared_ptr<type_base>);

shared_ptr<class_decl>
is_class_type(const shared_ptr<decl_base>);

shared_ptr<class_decl>
is_compatible_with_class_type(const shared_ptr<type_base>);

pointer_type_def*
is_pointer_type(type_base*);

shared_ptr<pointer_type_def>
is_pointer_type(const shared_ptr<type_base>);

reference_type_def*
is_reference_type(type_base*);

shared_ptr<reference_type_def>
is_reference_type(const shared_ptr<type_base>);

qualified_type_def*
is_qualified_type(type_base*);

shared_ptr<qualified_type_def>
is_qualified_type(const shared_ptr<type_base>);

shared_ptr<function_type>
is_function_type(const shared_ptr<type_base>);

shared_ptr<method_type>
is_method_type(const shared_ptr<type_base>);

shared_ptr<class_decl>
look_through_decl_only_class(shared_ptr<class_decl>);

shared_ptr<var_decl>
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

void
remove_decl_from_scope(shared_ptr<decl_base>);

bool
get_member_is_static(const decl_base&);

bool
get_member_is_static(const decl_base*);

bool
get_member_is_static(const shared_ptr<decl_base>&);

void
set_member_is_static(decl_base&, bool);

void
set_member_is_static(const shared_ptr<decl_base>&, bool);

bool
is_data_member(const var_decl&);

bool
is_data_member(const var_decl*);

bool
is_data_member(const shared_ptr<var_decl>);

shared_ptr<var_decl>
is_data_member(const shared_ptr<decl_base>&);

shared_ptr<array_type_def>
is_array_type(const shared_ptr<type_base> decl);

void
set_data_member_offset(shared_ptr<var_decl>, size_t);

size_t
get_data_member_offset(const var_decl&);

size_t
get_data_member_offset(const shared_ptr<var_decl>);

size_t
get_data_member_offset(const shared_ptr<decl_base>);

void
set_data_member_is_laid_out(shared_ptr<var_decl>, bool);

bool
get_data_member_is_laid_out(const var_decl&);

bool
get_data_member_is_laid_out(const shared_ptr<var_decl>);

bool
is_member_function(const function_decl&);

bool
is_member_function(const function_decl*);

bool
is_member_function(const shared_ptr<function_decl>&);

bool
get_member_function_is_ctor(const function_decl&);

bool
get_member_function_is_ctor(const shared_ptr<function_decl>&);

void
set_member_function_is_ctor(const function_decl&, bool);

void
set_member_function_is_ctor(const shared_ptr<function_decl>&, bool);

bool
get_member_function_is_dtor(const function_decl&);

bool
get_member_function_is_dtor(const shared_ptr<function_decl>&);

void
set_member_function_is_dtor(function_decl&, bool);

void
set_member_function_is_dtor(const shared_ptr<function_decl>&, bool);

bool
get_member_function_is_const(const function_decl&);

bool
get_member_function_is_const(const shared_ptr<function_decl>&);

void
set_member_function_is_const(function_decl&, bool);

void
set_member_function_is_const(const shared_ptr<function_decl>&, bool);

size_t
get_member_function_vtable_offset(const function_decl&);

size_t
get_member_function_vtable_offset(const shared_ptr<function_decl>&);

void
set_member_function_vtable_offset(const function_decl& f,
				  size_t s);

void
set_member_function_vtable_offset(const shared_ptr<function_decl> &f,
				  size_t s);

bool
get_member_function_is_virtual(const function_decl&);

bool
get_member_function_is_virtual(const shared_ptr<function_decl>&);

bool
get_member_function_is_virtual(const function_decl*);

void
set_member_function_is_virtual(function_decl&, bool);

void
set_member_function_is_virtual(const shared_ptr<function_decl>&, bool);

shared_ptr<type_base>
strip_typedef(const shared_ptr<type_base>);

string
get_type_name(const shared_ptr<type_base>);

string
get_pretty_representation(const decl_base*);

string
get_pretty_representation(const type_base*);

string
get_pretty_representation(const shared_ptr<decl_base>&);

string
get_pretty_representation(const shared_ptr<type_base>&);

const decl_base*
get_type_declaration(const type_base*);

decl_base*
get_type_declaration(type_base*);

shared_ptr<decl_base>
get_type_declaration(const shared_ptr<type_base>);

bool
types_are_compatible(const shared_ptr<type_base>,
		     const shared_ptr<type_base>);

bool
types_are_compatible(const shared_ptr<decl_base>,
		     const shared_ptr<decl_base>);

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
lookup_type_in_corpus(const string&, const corpus&);

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

shared_ptr<type_base>
type_or_void(const shared_ptr<type_base>);

shared_ptr<type_base>
canonicalize(shared_ptr<type_base>);

bool
type_has_non_canonicalized_subtype(shared_ptr<type_base> t);

void
keep_type_alive(shared_ptr<type_base> t);
} // end namespace ir

using namespace abigail::ir;

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

void
dump_decl_location(const decl_base&);

void
dump_decl_location(const decl_base*);

void
dump_decl_location(const shared_ptr<decl_base>&);

} // end namespace abigail
#endif // __ABG_IRFWD_H__
