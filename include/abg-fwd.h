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
     
     aka
     GNU Application Binary Interface 
     Generic Analysis and Instrumentation Library

     This is an interface to the GNU Compiler Collection for the
     collection and analysis of compiler-generated binaries.

     The project homepage is at https://sourceware.org/libabigail
     
     The current libabigail source code can be checked out with:
     git clone git://git.sourceware.org/git/libabigail

     The mailing list to send messages and patches to is
     libabigail@sourceware.org.
  */

  // Inject some types.
  using std::tr1::shared_ptr;
  using std::string;

  // Pull in relational operators.
  using namespace std::rel_ops;

  // Forward declarations for corpus.
  class location;
  class location_manager;
  class translation_unit;
  class corpus;

  // Forward declarations for ir.
  class class_decl;
  class class_tdecl;
  class decl_base;
  class enum_type_decl;
  //class enumerator;
  class function_decl;
  class function_tdecl;
  class function_type;
  class global_scope;
  class ir_node_visitor;
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

  /*
    Nested types in class_decl:

    class member;
    class member_type;
    class base_spec;
    class data_member;
    class method_decl;
    class member_function;
    class member_function_template;
    class member_class_template;
  */

  void
  add_decl_to_scope(shared_ptr<decl_base>, scope_decl*);

  void
  add_decl_to_scope (shared_ptr<decl_base>, shared_ptr<scope_decl>);

  global_scope*
  get_global_scope(const shared_ptr<decl_base>);

  translation_unit*
  get_translation_unit(const shared_ptr<decl_base>);
  
  /// Tests whether if a given scope is the global scope.
  ///
  /// @param scpe the scope to consider.
  ///
  /// @return true iff the current scope is the global one.
  bool
  is_global_scope(const scope_decl* scpe);

  /// Tests whether if a given scope is the global scope.
  ///
  /// @param scpe the scope to consider.
  ///
  /// @return true iff the current scope is the global one.
  bool
  is_global_scope(const shared_ptr<scope_decl> scpe);

  /// Tests whether a given declaration is at global scope.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff dcl is at global scope.
  bool
  is_at_global_scope(const shared_ptr<decl_base> dcl);

  /// Tests whether a given decl is at class scope.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff dcl is at class scope.
  bool
  is_at_class_scope(const shared_ptr<decl_base> dcl);

  /// Tests whether a given decl is at template scope.
  ///
  /// Note that only template parameters , types that are compositions,
  /// and template patterns (function or class) can be at template scope.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff the dcl is at template scope.
  bool
  is_at_template_scope(const shared_ptr<decl_base> dcl);

  /// Tests whether a decl is a template parameter.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff decl is a template parameter.
  bool
  is_template_parameter(const shared_ptr<decl_base> dcl);

  /// Tests whether a declaration is a type.
  ///
  /// @param decl the decl to consider.
  ///
  /// @return true iff decl is a type.
  bool
  is_type(const shared_ptr<decl_base> decl);

  /// Tests whether a decl is a template parameter composition type.
  ///
  /// @param dcl the declaration to consider.
  ///
  /// @return true iff dcl is a template parameter composition type.
  bool
  is_template_parm_composition_type(const shared_ptr<decl_base> dcl);

  /// Tests whether a decl is a template.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff dcl is a function template, class template, or
  /// template template parameter.
  bool
  is_template_decl(const shared_ptr<decl_base> dcl);

  /// Test whether a decl is the pattern of a function template.
  ///
  /// @param dcl the decl to consider.
  ///
  /// @return true iff dcl is the pattern of a function template.
  bool
  is_function_template_pattern(const shared_ptr<decl_base> dcl);


  /// Appends a declaration to a given scope, if the declaration
  /// doesn't already belong to one.
  ///
  /// @param dcl the declaration to add to the scope
  ///
  /// @param scpe the scope to append the declaration to
  void
  add_decl_to_scope(shared_ptr<decl_base> dcl, scope_decl* scpe);

  /// Appends a declaration to a given scope, if the declaration doesn't already
  /// belong to a scope.
  ///
  /// @param dcl the declaration to add append to the scope
  ///
  /// @param scpe the scope to append the dcl to
  void
  add_decl_to_scope(shared_ptr<decl_base> dcl, shared_ptr<scope_decl> scpe);

  /// Return the global scope as seen by a given declaration.
  ///
  /// @param dcl the declaration to consider.
  ///
  /// @return the global scope of the decl, or a null pointer if the
  /// decl is not yet added to a translation_unit.
  global_scope*
  get_global_scope(const shared_ptr<decl_base> dcl);

  /// Return the translation unit a declaration belongs to.
  ///
  /// @param dcl the declaration to consider.
  ///
  /// @return the resulting translation unit, or null if the decl is not
  /// yet added to a translation unit.
  translation_unit*
  get_translation_unit(const shared_ptr<decl_base> dcl);


} // end namespace abigail
#endif // __ABG_IRFWD_H__
