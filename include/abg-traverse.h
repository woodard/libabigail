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

#ifndef __ABG_TRAVERSE_H__
#define __ABG_TRAVERSE_H__

#include "abg-fwd.h"

namespace abigail
{

/// The interface for types which are feeling social and want to
/// be visited during the traversal of translation unit nodes.
struct traversable_base
{
  /// This virtual pure method is implemented by any single type which
  /// instance is going to be visited during the traversal of
  /// translation unit nodes.
  ///
  /// The method visits a given node and, for scopes, visits their
  /// member nodes.  Visiting a node means calling the
  /// ir_node_visitor::visit method with the node passed as an
  /// argument.
  ///
  /// @param v the visitor used during the traverse.
  virtual void traverse(ir_node_visitor& v) = 0;
};
}//end namespace abigail
#endif //__ABG_TRAVERSE_H__
