// -*- Mode: C++ -*-
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

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-traverse.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

namespace ir
{

/// Private data type of the @ref traversable_base type.
struct traversable_base::priv
{
  bool visiting_;

  priv(bool visiting = false)
    : visiting_(visiting)
  {}
}; // end struct traversable_base::priv

/// Default constructor of the @ref traversable_base type.
traversable_base::traversable_base()
  :priv_(new priv)
{}

/// Destructor of the @ref traversable_base type.
traversable_base::~traversable_base()
{}

/// This should returns false before and after the node has been
/// visiting.  During the visiting of the node (and of its children)
/// this should return true.
///
/// @return true if the current node is being visited.
bool
traversable_base::visiting() const
{return priv_->visiting_;}

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
traversable_base::visiting(bool f)
{priv_->visiting_ = f;}

}// end namaspace ir
}// end namespace abigail
