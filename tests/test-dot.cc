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

int main()
{
  using namespace abigail;

  // sa-base
  {
    dot obj("sa-base");
    parent r = { "base", parent_sty };
    obj.start(); 
    obj.add_parent(r);
    obj.finish();
  }

  // sa-A
  {
    dot obj("sa-A");
    child r1 = { "A", child_sty };
    parent r2 = { "base", parent_sty };
    obj.start(); 
    obj.add_parent(r2);
    obj.add_child(r1);
    obj.finish();
  }

  // sa-B
  {
    dot obj("sa-B");
    child r1 = { "B", child_sty };
    parent r2 = { "base", parent_sty };
    obj.start(); 
    obj.add_parent(r2);
    obj.add_child(r1);
    obj.finish();
  }

  return 0;
}
