// -*- mode: C++ -*-
//
// Copyright (C) 2013-2018 Red Hat, Inc.
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

namespace ir
{

/// The base class for the visitor type hierarchy used for traversing
/// a hierarchy of nodes.
///
/// Client code willing to get notified for a certain kind of node
/// during traversal might want to define a visitor class that inherit
/// \a node_visitor, define and overload a node_visitor::visit method
/// for it, like what is done for the ir_node_visitor::visit set of
/// functions, for traversing internal representation nodes.
struct node_visitor_base
{};

/// The interface for types which are feeling social and want to be
/// visited during the traversal of a hierarchy of nodes.
class traversable_base
{
  bool visiting_;

protected:

  /// This should returns false before and after the node has been
  /// visiting.  During the visiting of the node (and of its children)
  /// this should return true.
  ///
  /// @return true if the current node is being visited.
  bool
  visiting()
  {return visiting_;}

  /// The traversing code should be responsible of calling this, not
  /// the user code.
  ///
  /// This is the setter of the "visiting" flag of the node being
  /// visited.  If set to yes, it means the node is being visited.
  /// False means either the node has not yet been visited, or it
  /// has already been visited.
  ///
  /// @param f the new value of the "visiting" flag.
  void
  visiting(bool f)
  {visiting_ = f;}

public:
  traversable_base()
    : visiting_()
  {}
  /// This virtual method is overloaded and implemented by any single
  /// type which instance is going to be visited during the traversal
  /// of translation unit nodes.
  ///
  /// The method visits a given node and, for scopes, visits their
  /// member nodes.  Visiting a node means calling the
  /// ir_node_visitor::visit method with the node passed as an
  /// argument.
  ///
  /// @param v the visitor used during the traverse.
  ///
  /// @return true if traversed until the end of the type tree, false
  /// otherwise.
  virtual bool traverse(node_visitor_base& v);
};

}// end namespace ir.
}//end namespace abigail
#endif //__ABG_TRAVERSE_H__
