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
   git clone git://git.sourceware.org/git/libabigail.git.

   The mailing list to send messages and patches to is
   libabigail@sourceware.org.
*/

// Inject some types.
using std::tr1::shared_ptr;
using std::string;

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

void
add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

void
add_decl_to_scope (shared_ptr<decl_base>, shared_ptr<scope_decl>);

global_scope*
get_global_scope(const shared_ptr<decl_base>);

translation_unit*
get_translation_unit(const shared_ptr<decl_base>);

bool
is_global_scope(const scope_decl*);

bool
is_global_scope(const shared_ptr<scope_decl>);

bool
is_at_global_scope(const shared_ptr<decl_base>);

bool
is_at_class_scope(const shared_ptr<decl_base>);

bool
is_at_template_scope(const shared_ptr<decl_base>);

bool
is_template_parameter(const shared_ptr<decl_base>);

bool
is_type(const shared_ptr<decl_base>);

bool
is_var_decl(const shared_ptr<decl_base>);

bool
is_template_parm_composition_type(const shared_ptr<decl_base>);

bool
is_template_decl(const shared_ptr<decl_base>);

bool
is_function_template_pattern(const shared_ptr<decl_base>);

void
add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

void
add_decl_to_scope(shared_ptr<decl_base>, shared_ptr<scope_decl>);

global_scope*
get_global_scope(const shared_ptr<decl_base>);

translation_unit*
get_translation_unit(const shared_ptr<decl_base>);

string
get_type_name(const shared_ptr<type_base>);

void
dump(const shared_ptr<decl_base>);

void
dump(const shared_ptr<type_base>);

void
dump(const shared_ptr<var_decl>);

void
dump(const translation_unit&);

} // end namespace abigail
#endif // __ABG_IRFWD_H__
