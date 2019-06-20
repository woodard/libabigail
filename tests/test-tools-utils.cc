// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.
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
/// This is a collection of unit tests for functions defined in
/// abg-tgools-utils.cc.

#include <iostream>
#include "abg-tools-utils.h"

using namespace abigail::tools_utils;
using std::cerr;

int
main(int, char**)
{

  /// These are unit tests for abigail::tools_utils::decl_names_equal.
  /// Just run the resulting runtesttoolsutils program under the
  /// debugger to debug this if need be.

  ABG_ASSERT(decl_names_equal("foo", "foo") == true);

  ABG_ASSERT(decl_names_equal("foo", "bar") == false);

  ABG_ASSERT(decl_names_equal("__anonymous_struct__1::foo",
			      "__anonymous_struct__2::foo") == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_union__1::foo::__anonymous_union__2::bar",
	      "__anonymous_union__10::foo::__anonymous_union__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_enum__1::foo::__anonymous_enum__2::bar",
	      "__anonymous_enum__10::foo::__anonymous_enum__11::bar")
	     == true);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::bar::__anonymous_struct__2::baz",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::baz",
	      "__anonymous_struct__10::foo::__anonymous_struct__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_union__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("__anonymous_struct__1::foo::__anonymous_struct__2::bar",
	      "__anonymous_struct__10::foo::__anonymous_enum__11::bar")
	     == false);

  ABG_ASSERT(decl_names_equal
	     ("OT::Extension<OT::ExtensionSubst>::__anonymous_union__",
	      "OT::Extension<OT::ExtensionSubst>::__anonymous_union__")
	     == true);

  ABG_ASSERT(decl_names_equal("S0::m2", "S0::m12") == false);

  return 0;
}
