// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2019-2020 Google, Inc.

/// @file

#ifndef __ABG_CXX_COMPAT_H
#define __ABG_CXX_COMPAT_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace abg_compat {

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

}

#endif  // __ABG_CXX_COMPAT_H
