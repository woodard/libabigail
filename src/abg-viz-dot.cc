
// Copyright (C) 2013 Free Software Foundation, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License
// and a copy of the GCC Runtime Library Exception along with this
// program; see the files COPYING3 and COPYING.RUNTIME respectively.
// If not, see <http://www.gnu.org/licenses/>.

// -*- mode: C++ -*-

#include "abg-viz-dot.h"
#include <stdexcept>
#include <fstream>

namespace abigail
{

using std::ostream;
using std::ostringstream;

// Constants.
const parent::style parent_sty = { color::white, color::black, "" };
const child::style child_sty = { color::white, color::gray75, "" };

// DOT element beginning boilerplate.
// Variable: units, x=0, y=0, width, height
void
dot::start_element() 
{ }

void
dot::finish_element() 
{ }

void
dot::add_title() 
{ }

void
dot::write() 
{ }

void
dot::add_parent(const parent&) 
{ }

void
dot::add_child(const child&) 
{ }

}//end namespace abigail
