// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2020 Red Hat, Inc.
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
/// Some specialization for shared pointer utility templates.
///

#include "abg-sptr-utils.h"

namespace abigail
{

namespace sptr_utils
{

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex_t_sptr
build_sptr<regex_t>(regex_t *p)
{return regex_t_sptr(p, regex_t_deleter());}

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex_t_sptr
build_sptr<regex_t>()
{return build_sptr(new regex_t);}

}
}
