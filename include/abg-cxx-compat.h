// -*- Mode: C++ -*-
//
// Copyright (C) 2019-2020 Google, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 2, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this program; see the file COPYING-LGPLV2.  If
// not, see <http://www.gnu.org/licenses/>.

/// @file

#ifndef __ABG_CXX_COMPAT_H
#define __ABG_CXX_COMPAT_H

#if __cplusplus >= 201103L

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#else

#include <tr1/functional>
#include <tr1/memory>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#endif

namespace abg_compat {

#if __cplusplus >= 201103L

// <functional>
using std::hash;

// <memory>
using std::shared_ptr;
using std::weak_ptr;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;

// <unordered_map>
using std::unordered_map;

// <unordered_set>
using std::unordered_set;

#else

// <functional>
using std::tr1::hash;

// <memory>
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;

// <unordered_map>
using std::tr1::unordered_map;

// <unordered_set>
using std::tr1::unordered_set;

#endif

}

#endif  // __ABG_CXX_COMPAT_H
