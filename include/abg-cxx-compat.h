// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2019-2020 Google, Inc.

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
