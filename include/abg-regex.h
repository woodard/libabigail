// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.
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
///
/// Wrappers around regex types and functions.

#ifndef __ABG_REGEX_H__
#define __ABG_REGEX_H__

#include <regex.h>

#include "abg-cxx-compat.h"
#include "abg-sptr-utils.h"

namespace abigail
{

/// Namespace for regex types and functions.
namespace regex
{

/// A convenience typedef for a shared pointer of regex_t.
typedef abg_compat::shared_ptr<regex_t> regex_t_sptr;

/// A delete functor for a shared_ptr of regex_t.
struct regex_t_deleter
{
  /// The operator called to de-allocate the pointer to regex_t
  /// embedded in a shared_ptr<regex_t>
  ///
  /// @param r the pointer to regex_t to de-allocate.
  void
  operator()(::regex_t* r)
  {
    regfree(r);
    delete r;
  }
};//end struct regex_deleter

}// end namespace regex

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>(regex_t *p);

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex::regex_t_sptr
sptr_utils::build_sptr<regex_t>();

}// end namespace abigail

#endif //__ABG_REGEX_H__
