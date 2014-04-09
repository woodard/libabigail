// -*- mode: C++ -*-
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

#include "abg-hash.h"
#include "abg-ir.h"

namespace abigail
{

namespace hashing
{

// Mix 3 32 bits values reversibly.  Borrowed from hashtab.c in gcc tree.
#define abigail_hash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<< 8); \
  c -= a; c -= b; c ^= ((b&0xffffffff)>>13); \
  a -= b; a -= c; a ^= ((c&0xffffffff)>>12); \
  b -= c; b -= a; b = (b ^ (a<<16)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>> 5)) & 0xffffffff; \
  a -= b; a -= c; a = (a ^ (c>> 3)) & 0xffffffff; \
  b -= c; b -= a; b = (b ^ (a<<10)) & 0xffffffff; \
  c -= a; c -= b; c = (c ^ (b>>15)) & 0xffffffff; \
}

size_t
combine_hashes(size_t val1, size_t val2)
{
  /* the golden ratio; an arbitrary value.  */
  size_t a = 0x9e3779b9;
  abigail_hash_mix(a, val1, val2);
  return val2;
}

}//end namespace hashing

using std::list;
using std::vector;

// See forward declarations in abg-ir.h.

// Definitions.
struct type_base::hash
{
  size_t
  operator()(const type_base& t) const
  {
    std::tr1::hash<size_t> size_t_hash;
    std::tr1::hash<string> str_hash;

    size_t v = str_hash(typeid(t).name());
    v = hashing::combine_hashes(v, size_t_hash(t.get_size_in_bits()));
    v = hashing::combine_hashes(v, size_t_hash(t.get_alignment_in_bits()));

    return v;
  }
};

struct decl_base::hash
{
  size_t
  operator()(const decl_base& d) const
  {
    if (d.hash_ == 0)
      {
	std::tr1::hash<string> str_hash;

	size_t v = str_hash(typeid(d).name());
	if (!d.get_mangled_name().empty())
	  v = hashing::combine_hashes(v, str_hash(d.get_mangled_name()));
	else if (!d.get_name().empty())
	  v = hashing::combine_hashes(v, str_hash(d.get_qualified_name()));
	if (is_member_decl(d))
	  {
	    v = hashing::combine_hashes(v, get_member_access_specifier(d));
	    v = hashing::combine_hashes(v, get_member_is_static(d));
	  }
	d.hash_ = v;
      }
    return d.hash_;
  }
}; // end struct decl_base::hash

struct type_decl::hash
{
  size_t
  operator()(const type_decl& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	decl_base::hash decl_hash;
	type_base::hash type_hash;
	std::tr1::hash<string> str_hash;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, type_hash(t));
	t.hash_ = v;
      }
    return t.hash_;
  }
};

/// Hashing operator for the @ref scope_decl type.
///
/// @param d the scope_decl to hash.
///
/// @return the hash value.
size_t
scope_decl::hash::operator()(const scope_decl& d) const
{
  if (d.hash_ == 0 || d.hashing_started_)
    {
      std::hash<string> hash_string;
      size_t v = hash_string(typeid(d).name());
      for (scope_decl::declarations::const_iterator i =
	     d.get_member_decls().begin();
	   i != d.get_member_decls().end();
	   ++i)
	v = hashing::combine_hashes(v, (*i)->get_hash());
      d.hash_ = v;
    }
  return d.hash_;
}

/// Hashing operator for the @ref scope_decl type.
///
/// @param d the scope_decl to hash.
///
/// @return the hash value.
size_t
scope_decl::hash::operator()(const scope_decl* d) const
{return d? operator()(*d) : 0;}

struct scope_type_decl::hash
{
  size_t
  operator()(const scope_type_decl& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	decl_base::hash decl_hash;
	type_base::hash type_hash;
	std::tr1::hash<string> str_hash;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, type_hash(t));
	t.hash_ = v;
      }

    return t.hash_;
  }
};

struct qualified_type_def::hash
{
  size_t
  operator()(const qualified_type_def& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	type_base::hash type_hash;
	decl_base::hash decl_hash;
	std::tr1::hash<string> str_hash;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, type_hash(t));
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, t.get_cv_quals());
	t.hash_ = v;
      }
    return t.hash_;
  }
};

struct pointer_type_def::hash
{
  size_t
  operator()(const pointer_type_def& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	std::tr1::hash<string> str_hash;
	type_base::hash type_base_hash;
	decl_base::hash decl_hash;
	type_base::shared_ptr_hash hash_type_ptr;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, type_base_hash(t));
	v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));
	t.hash_ = v;
      }

    return t.hash_;
  }
};

struct reference_type_def::hash
{
  size_t
  operator()(const reference_type_def& t)
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	std::tr1::hash<string> hash_str;
	type_base::hash hash_type_base;
	decl_base::hash hash_decl;
	type_base::shared_ptr_hash hash_type_ptr;

	size_t v = hash_str(typeid(t).name());
	v = hashing::combine_hashes(v, hash_str(t.is_lvalue()
						? "lvalue"
						: "rvalue"));
	v = hashing::combine_hashes(v, hash_type_base(t));
	v = hashing::combine_hashes(v, hash_decl(t));
	v = hashing::combine_hashes(v, hash_type_ptr(t.get_pointed_to_type()));
	t.hash_ = v;
      }

    return t.hash_;
  }
};

struct enum_type_decl::hash
{
  size_t
  operator()(const enum_type_decl& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	std::tr1::hash<string> str_hash;
	decl_base::hash decl_hash;
	type_base::shared_ptr_hash type_ptr_hash;
	std::tr1::hash<size_t> size_t_hash;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));
	for (enum_type_decl::enumerators::const_iterator i =
	       t.get_enumerators().begin();
	     i != t.get_enumerators().end();
	     ++i)
	  {
	    v = hashing::combine_hashes(v, str_hash(i->get_name()));
	    v = hashing::combine_hashes(v, size_t_hash(i->get_value()));
	  }
	t.hash_ = v;
      }
    return t.hash_;
  }
};

struct typedef_decl::hash
{
  size_t
  operator()(const typedef_decl& t) const
  {
    if (t.hash_ == 0 || t.hashing_started_)
      {
	std::tr1::hash<string> str_hash;
	type_base::hash hash_type;
	decl_base::hash decl_hash;
	type_base::shared_ptr_hash type_ptr_hash;

	size_t v = str_hash(typeid(t).name());
	v = hashing::combine_hashes(v, hash_type(t));
	v = hashing::combine_hashes(v, decl_hash(t));
	v = hashing::combine_hashes(v, type_ptr_hash(t.get_underlying_type()));
	t.hash_ = v;
      }

     return t.hash_;
   }
 };

/// Compute a hash for an instance @ref var_decl.
///
/// Note that this function caches the hashing value the
/// decl_base::hash_ data member of the input instance and re-uses it
/// when it is already calculated.
///
/// @param t the instance of @ref var_decl to compute the hash for.
///
/// @return the calculated hash value, or the one that was previously
/// calculated.
size_t
var_decl::hash::operator()(const var_decl& t) const
{
    if (t.hash_ == 0 || t.hashing_started_)
      {
	std::tr1::hash<string> hash_string;
	decl_base::hash hash_decl;
	type_base::shared_ptr_hash hash_type_ptr;
	std::tr1::hash<size_t> hash_size_t;

	size_t v = hash_string(typeid(t).name());
	v = hashing::combine_hashes(v, hash_decl(t));
	v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));

	if (is_data_member(t) && get_data_member_is_laid_out(t))
	  v = hashing::combine_hashes(v,
				      hash_size_t(get_data_member_offset(t)));

	t.hash_ = v;
      }
    return t.hash_;
}

/// Compute a hash for a pointer to @ref var_decl.
///
/// @param t the pointer to @ref var_decl to compute the hash for.
///
/// @return the calculated hash value
size_t
var_decl::hash::operator()(const var_decl* t) const
{return operator()(*t);}

/// Compute a hash value for an instance of @ref function_decl.
///
/// Note that this function caches the resulting hash in the
/// decl_base::hash_ data member of the instance of @ref
/// function_decl, and just returns if it is already calculated.
///
/// @param t the function to calculate the hash for.
///
/// @return the hash value.
size_t
function_decl::hash::operator()(const function_decl& t) const
{
  if (t.hash_ == 0 || t.hashing_started_)
    {
      std::tr1::hash<int> hash_int;
      std::tr1::hash<size_t> hash_size_t;
      std::tr1::hash<bool> hash_bool;
      std::tr1::hash<string> hash_string;
      decl_base::hash hash_decl_base;
      type_base::shared_ptr_hash hash_type_ptr;

      size_t v = hash_string(typeid(t).name());
      v = hashing::combine_hashes(v, hash_decl_base(t));
      v = hashing::combine_hashes(v, hash_type_ptr(t.get_type()));
      v = hashing::combine_hashes(v, hash_bool(t.is_declared_inline()));
      v = hashing::combine_hashes(v, hash_int(t.get_binding()));
      if (is_member_function(t))
	{
	  bool is_ctor = get_member_function_is_ctor(t),
	    is_dtor = get_member_function_is_dtor(t),
	    is_static = get_member_is_static(t),
	    is_const = get_member_function_is_const(t);
	  size_t voffset = get_member_function_vtable_offset(t);

	  v = hashing::combine_hashes(v, hash_bool(is_ctor));
	  v = hashing::combine_hashes(v, hash_bool(is_dtor));
	  v = hashing::combine_hashes(v, hash_bool(is_static));
	  v = hashing::combine_hashes(v, hash_bool(is_const));
	  if (!is_static && !is_ctor)
	    v = hashing::combine_hashes(v, hash_size_t(voffset));
	}
      t.hash_ = v;
    }
  return t.hash_;
}
/// Compute a hash for a pointer to @ref function_decl.
///
/// @param t the pointer to @ref function_decl to compute the hash for.
///
/// @return the calculated hash value
size_t
function_decl::hash::operator()(const function_decl* t) const
{return operator()(*t);}

struct function_decl::parameter::hash
{
  size_t
  operator()(const function_decl::parameter& p) const
  {
    type_base::shared_ptr_hash hash_type_ptr;
    std::tr1::hash<bool> hash_bool;
    std::tr1::hash<unsigned> hash_unsigned;
    size_t v = hash_type_ptr(p.get_type());
    v = hashing::combine_hashes(v, hash_unsigned(p.get_index()));
    v = hashing::combine_hashes(v, hash_bool(p.get_variadic_marker()));
    return v;
  }
};

struct function_type::hash
{
  size_t
  operator()(const function_type& t) const
  {
    std::tr1::hash<string> hash_string;
    type_base::shared_ptr_hash hash_type_ptr;
    function_decl::parameter::hash hash_parameter;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
    for (vector<shared_ptr<function_decl::parameter> >::const_iterator i =
	   t.get_first_non_implicit_parm();
	 i != t.get_parameters().end();
	 ++i)
      v = hashing::combine_hashes(v, hash_parameter(**i));
    return v;
  }
};

struct method_type::hash
{
  size_t
  operator()(const method_type& t) const
  {
    std::tr1::hash<string> hash_string;
    type_base::shared_ptr_hash hash_type_ptr;
    function_decl::parameter::hash hash_parameter;

    size_t v = hash_string(typeid(t).name());
    string class_name= t.get_class_type()->get_qualified_name();
    v = hashing::combine_hashes(v, hash_string(class_name));
    v = hashing::combine_hashes(v, hash_type_ptr(t.get_return_type()));
    vector<shared_ptr<function_decl::parameter> >::const_iterator i =
      t.get_first_non_implicit_parm();

    for (; i != t.get_parameters().end(); ++i)
      v = hashing::combine_hashes(v, hash_parameter(**i));

    return v;
  }
};

size_t
class_decl::member_base::hash::operator()(const member_base& m) const
{
  std::tr1::hash<int> hash_int;
  return hash_int(m.get_access_specifier());
}

size_t
class_decl::base_spec::hash::operator()(const base_spec& t) const
{
  member_base::hash hash_member;
  type_base::shared_ptr_hash hash_type_ptr;
  std::hash<size_t> hash_size;
  std::hash<bool> hash_bool;
  std::hash<string> hash_string;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_member(t));
  v = hashing::combine_hashes(v, hash_size(t.get_offset_in_bits()));
  v = hashing::combine_hashes(v, hash_bool(t.get_is_virtual()));
  v = hashing::combine_hashes(v, hash_type_ptr(t.get_base_class()));
  return v;
}

size_t
class_decl::member_function_template::hash::operator()
  (const member_function_template& t) const
{
  std::tr1::hash<bool> hash_bool;
  function_tdecl::hash hash_function_tdecl;
  member_base::hash hash_member;
  std::tr1::hash<string> hash_string;

  size_t v = hash_member(t);
  string n = t.get_qualified_name();
  v = hashing::combine_hashes(v, hash_string(n));
  v = hashing::combine_hashes(v, hash_function_tdecl(t));
  v = hashing::combine_hashes(v, hash_bool(t.is_constructor()));
  v = hashing::combine_hashes(v, hash_bool(t.is_const()));
  return v;
}

size_t
class_decl::member_class_template::hash::operator()
  (const member_class_template& t) const
{
  member_base::hash hash_member;
  class_tdecl::hash hash_class_tdecl;
  std::tr1::hash<string> hash_string;

  size_t v = hash_member(t);
  string n = t.get_qualified_name();
  v = hashing::combine_hashes(v, hash_string(n));
  v = hashing::combine_hashes(v, hash_class_tdecl(t));
  return v;
}

/// Compute a hash for a @ref class_decl
///
/// @param t the class_decl for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_decl::hash::operator()(const class_decl& t) const
{
    if (t.hashing_started())
      return 0;

    if (t.hash_ == 0)
      {
	std::tr1::hash<string> hash_string;
	std::tr1::hash<bool> hash_bool;
#if 0
	type_base::dynamic_hash hash_type;
#endif
	scope_type_decl::hash hash_scope_type;
	class_decl::base_spec::hash hash_base;
	var_decl::hash hash_data_member;
	function_decl::hash hash_member_fn;
	class_decl::member_function_template::hash hash_member_fn_tmpl;
	class_decl::member_class_template::hash hash_member_class_tmpl;

	size_t v = hash_string(typeid(t).name());
	v = hashing::combine_hashes(v, hash_scope_type(t));
	v = hashing::combine_hashes(v, hash_bool(t.get_is_declaration_only()));

	t.hashing_started(true);

	// Hash bases.
	for (class_decl::base_specs::const_iterator b =
	       t.get_base_specifiers().begin();
	     b != t.get_base_specifiers().end();
	     ++b)
	  v = hashing::combine_hashes(v, hash_base(**b));

	// Hash member types.
#if 0
	for (class_decl::member_types::const_iterator ti =
	       t.get_member_types().begin();
	     ti != t.get_member_types().end();
	     ++ti)
	  v = hashing::combine_hashes(v, hash_type((*ti).get()));
#endif

	// Hash data members.
	for (class_decl::data_members::const_iterator d =
	       t.get_data_members().begin();
	     d != t.get_data_members().end();
	     ++d)
	  v = hashing::combine_hashes(v, hash_data_member(**d));

	// Hash member_function
	for (class_decl::member_functions::const_iterator f =
	       t.get_member_functions().begin();
	     f != t.get_member_functions().end();
	     ++f)
	  v = hashing::combine_hashes(v, hash_member_fn(**f));

	// Hash member function templates
	for (class_decl::member_function_templates::const_iterator f =
	       t.get_member_function_templates().begin();
	     f != t.get_member_function_templates().end();
	     ++f)
	  v = hashing::combine_hashes(v, hash_member_fn_tmpl(**f));

	// Hash member class templates
	for (class_decl::member_class_templates::const_iterator c =
	       t.get_member_class_templates().begin();
	     c != t.get_member_class_templates().end();
	     ++c)
	  v = hashing::combine_hashes(v, hash_member_class_tmpl(**c));

	t.hashing_started(false);

	t.hash_ = v;
      }
    return t.hash_;
}

/// Compute a hash for a @ref class_decl
///
/// @param t the class_decl for which to compute the hash value.
///
/// @return the computed hash value.
size_t
class_decl::hash::operator()(const class_decl* t) const
{return t ? operator()(*t) : 0;}

struct template_parameter::hash
{
  size_t
  operator()(const template_parameter& t) const
  {
    std::tr1::hash<unsigned> hash_unsigned;
    std::tr1::hash<std::string> hash_string;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_unsigned(t.get_index()));

    return v;
  }
};

struct template_parameter::dynamic_hash
{
  size_t
  operator()(const template_parameter* t) const;
};

struct template_parameter::shared_ptr_hash
{
  size_t
  operator()(const shared_ptr<template_parameter> t) const
  { return template_parameter::dynamic_hash()(t.get()); }
};

struct template_decl::hash
{
  size_t
  operator()(const template_decl& t) const
  {
    std::tr1::hash<string> hash_string;
    template_parameter::shared_ptr_hash hash_template_parameter;

    size_t v = hash_string(typeid(t).name());

    for (list<shared_ptr<template_parameter> >::const_iterator p =
	   t.get_template_parameters().begin();
	 p != t.get_template_parameters().end();
	 ++p)
      {
	v = hashing::combine_hashes(v, hash_template_parameter(*p));
      }
    return v;
  }
};

struct type_tparameter::hash
{
  size_t
  operator()(const type_tparameter& t) const
  {
    std::tr1::hash<string> hash_string;
    template_parameter::hash hash_template_parameter;
    type_decl::hash hash_type;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_template_parameter(t));
    v = hashing::combine_hashes(v, hash_type(t));

    return v;
  }
};

/// Compute a hash value for a @ref non_type_tparameter
///
/// @param t the non_type_tparameter for which to compute the value.
///
/// @return the computed hash value.
size_t
non_type_tparameter::hash::operator()(const non_type_tparameter& t) const
{
  template_parameter::hash hash_template_parameter;
  std::tr1::hash<string> hash_string;
  type_base::shared_ptr_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_template_parameter(t));
  v = hashing::combine_hashes(v, hash_string(t.get_name()));
  v = hashing::combine_hashes(v, hash_type(t.get_type()));

  return v;
}

/// Compute a hash value for a @ref non_type_tparameter
///
/// @param t the non_type_tparameter to compute the hash value for.
///
/// @return the computed hash value.
size_t
non_type_tparameter::hash::operator()(const non_type_tparameter* t) const
{return t ? operator()(*t) : 0;}

struct template_tparameter::hash
{
  size_t
  operator()(const template_tparameter& t) const
  {
    std::tr1::hash<string> hash_string;
    type_tparameter::hash hash_template_type_parm;
    template_decl::hash hash_template_decl;

    size_t v = hash_string(typeid(t).name());
    v = hashing::combine_hashes(v, hash_template_type_parm(t));
    v = hashing::combine_hashes(v, hash_template_decl(t));

    return v;
  }
};

size_t
template_parameter::dynamic_hash::
operator()(const template_parameter* t) const
{
  if (const template_tparameter* p =
      dynamic_cast<const template_tparameter*>(t))
    return template_tparameter::hash()(*p);
  else if (const type_tparameter* p =
	   dynamic_cast<const type_tparameter*>(t))
    return type_tparameter::hash()(*p);
  if (const non_type_tparameter* p =
      dynamic_cast<const non_type_tparameter*>(t))
    return non_type_tparameter::hash()(*p);

  // Poor man's fallback.
  return template_parameter::hash()(*t);
}

/// Compute a hash value for a @ref type_composition type.
///
/// @param t the type_composition to compute the hash value for.
///
/// @return the computed hash value.
size_t
type_composition::hash::operator()(const type_composition& t) const
{
  std::hash<string> hash_string;
  type_base::dynamic_hash hash_type;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_type(t.get_composed_type().get()));
  return v;
}

/// Compute a hash value for a @ref type_composition type.
///
/// @param t the type_composition to compute the hash value for.
///
/// @return the computed hash value.
size_t
type_composition::hash::operator()(const type_composition* t) const
{return t ? operator()(*t): 0;}

size_t
function_tdecl::hash::
operator()(const function_tdecl& t) const
{
  std::tr1::hash<string> hash_string;
  decl_base::hash hash_decl_base;
  template_decl::hash hash_template_decl;
  function_decl::hash hash_function_decl;

  size_t v = hash_string(typeid(t).name());

  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  if (t.get_pattern())
    v = hashing::combine_hashes(v, hash_function_decl(*t.get_pattern()));

  return v;
}

size_t
function_tdecl::shared_ptr_hash::
operator()(const shared_ptr<function_tdecl> f) const
{
  function_tdecl::hash hash_fn_tmpl_decl;
  if (f)
    return hash_fn_tmpl_decl(*f);
  return 0;
}

size_t
class_tdecl::hash::
operator()(const class_tdecl& t) const
{
  std::tr1::hash<string> hash_string;
  decl_base::hash hash_decl_base;
  template_decl::hash hash_template_decl;
  class_decl::hash hash_class_decl;

  size_t v = hash_string(typeid(t).name());
  v = hashing::combine_hashes(v, hash_decl_base(t));
  v = hashing::combine_hashes(v, hash_template_decl(t));
  if (t.get_pattern())
    v = hashing::combine_hashes(v, hash_class_decl(*t.get_pattern()));

  return v;
}

size_t
class_tdecl::shared_ptr_hash::
operator()(const shared_ptr<class_tdecl> t) const
{
  class_tdecl::hash hash_class_tmpl_decl;

  if (t)
    return hash_class_tmpl_decl(*t);
  return 0;
}

/// A hashing function for type declarations.
///
/// This function gets the dynamic type of the actual type
/// declaration and calls the right hashing function for that type.
///
/// Note that each time a new type declaration kind is added to the
/// system, this function needs to be updated.  For a given
/// inheritance hierarchy, make sure to handle the most derived type
/// first.
///
/// @param t a pointer to the type declaration to be hashed
///
/// @return the resulting hash
size_t
type_base::dynamic_hash::operator()(const type_base* t) const
{
  if (t == 0)
    return 0;
  if (const class_decl::member_function_template* d =
      dynamic_cast<const class_decl::member_function_template*>(t))
    return class_decl::member_function_template::hash()(*d);
  if (const class_decl::member_class_template* d =
      dynamic_cast<const class_decl::member_class_template*>(t))
    return class_decl::member_class_template::hash()(*d);
  if (const template_tparameter* d =
      dynamic_cast<const template_tparameter*>(t))
    return template_tparameter::hash()(*d);
  if (const type_tparameter* d =
      dynamic_cast<const type_tparameter*>(t))
    return type_tparameter::hash()(*d);
  if (const type_decl* d = dynamic_cast<const type_decl*> (t))
    return type_decl::hash()(*d);
  if (const qualified_type_def* d = dynamic_cast<const qualified_type_def*>(t))
    return qualified_type_def::hash()(*d);
  if (const pointer_type_def* d = dynamic_cast<const pointer_type_def*>(t))
    return pointer_type_def::hash()(*d);
  if (const reference_type_def* d = dynamic_cast<const reference_type_def*>(t))
    return reference_type_def::hash()(*d);
  if (const enum_type_decl* d = dynamic_cast<const enum_type_decl*>(t))
    return enum_type_decl::hash()(*d);
  if (const typedef_decl* d = dynamic_cast<const typedef_decl*>(t))
    return typedef_decl::hash()(*d);
  if (const class_decl* d = dynamic_cast<const class_decl*>(t))
    return class_decl::hash()(*d);
  if (const scope_type_decl* d = dynamic_cast<const scope_type_decl*>(t))
    return scope_type_decl::hash()(*d);
  if (const method_type* d = dynamic_cast<const method_type*>(t))
    return method_type::hash()(*d);
  if (const function_type* d = dynamic_cast<const function_type*>(t))
    return function_type::hash()(*d);

  // Poor man's fallback case.
  return type_base::hash()(*t);
}

size_t
type_base::shared_ptr_hash::operator()(const shared_ptr<type_base> t) const
{return type_base::dynamic_hash()(t.get());}

size_t
type_base::cached_hash::operator()(const type_base* t) const
{
  const decl_base* d = get_type_declaration(t);
  return d->get_hash();
}

size_t
type_base::cached_hash::operator() (const type_base_sptr t) const
{return type_base::cached_hash()(t.get());}
}//end namespace abigail
