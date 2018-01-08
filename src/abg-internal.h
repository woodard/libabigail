// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2018 Red Hat, Inc.
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
//
// Author: Dodji Seketeli

#ifndef __ABG_INTERNAL_H__
#define __ABG_INTERNAL_H__
#include "config.h"

#ifdef HAS_GCC_VISIBILITY_ATTRIBUTE

/// This macro makes a declaration be hidden at the binary level.
///
/// On ELF systems, this means that the symbol for the declaration
/// (function or variable) is going to be local to the file.  External
/// ELF files won't be able to link against the symbol.
#define ABG_HIDDEN  __attribute__((visibility("hidden")))

/// This macro makes a declaration be exported at the binary level.
///
/// On ELF systems, this means that the symbol for the declaration
///(function or variable) is going to be global.  External ELF files
///will be able to link against the symbol.
#define ABG_EXPORTED __attribute__((visibility("default")))
#define ABG_BEGIN_EXPORT_DECLARATIONS _Pragma("GCC visibility push(default)")
#define ABG_END_EXPORT_DECLARATIONS _Pragma("GCC visibility pop")
#else
#define ABG_HIDDEN
#define ABG_EXPORTED
#define ABG_BEGIN_EXPORT_DECLARATIONS
#define ABG_END_EXPORT_DECLARATIONS
#endif
#endif // __ABG_INTERNAL_H__
