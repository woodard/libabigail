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
  // Inject some types.
  using std::tr1::shared_ptr;
  using std::tr1::hash;
  using std::string;

  // Pull in relational operators.
  using namespace std::rel_ops;

  // Forward class declarations.
  class base_spec;
  class class_decl;
  class class_template_decl;
  class data_member;
  class decl_base;
  class enumerator;
  class enum_type_decl;
  class function_decl;
  class function_template_decl;
  class function_type;
  class global_scope;
  class ir_node_visitor;
  class location;
  class location_manager;
  class member;
  class member_class_template;
  class member_function;
  class member_function_template;
  class member_type;
  class method_decl;
  class method_type;
  class namespace_decl;
  class parameter;
  class pointer_type_def;
  class qualified_type_def;
  class reference_type_def;
  class scope_decl;
  class scope_type_decl;
  class template_decl;
  class template_non_type_parameter;
  class template_parameter;
  class template_template_parameter;
  class template_type_parameter;
  class tmpl_parm_type_composition;
  class translation_unit;
  class type_base;
  class type_decl;
  class typedef_decl;
  class var_decl;

  // Forward struct declarations.
  struct base_spec_hash;
  struct class_decl_hash;
  struct class_template_decl_hash;
  struct class_tmpl_shared_ptr_hash;
  struct data_member_hash;
  struct decl_base_hash;
  struct dynamic_template_parameter_hash;
  struct dynamic_type_hash;
  struct enum_type_decl_hash;
  struct fn_tmpl_shared_ptr_hash;
  struct function_decl_hash;
  struct function_template_decl_hash;
  struct function_type_hash;
  struct member_class_template_hash;
  struct member_function_hash;
  struct member_function_template_hash;
  struct member_hash;
  struct member_type_hash;
  struct parameter_hash;
  struct pointer_type_def_hash;
  struct qualified_type_def_hash;
  struct reference_type_def_hash;
  struct scope_type_decl_hash;
  struct template_decl_hash;
  struct template_non_type_parameter_hash;
  struct template_parameter_hash;
  struct template_parameter_shared_ptr_hash;
  struct template_template_parameter_hash;
  struct template_type_parameter_hash;
  struct type_base_hash;
  struct type_decl_hash;
  struct typedef_decl_hash;
  struct type_shared_ptr_hash;
  struct var_decl_hash;

  struct type_shared_ptr_equal;

  struct traversable;

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
  is_template_parm_composition_type(const shared_ptr<decl_base>);

  bool
  is_template_decl(const shared_ptr<decl_base>);

  bool
  is_function_template_pattern(const shared_ptr<decl_base>);
} // end namespace abigail
#endif // __ABG_IRFWD_H__
