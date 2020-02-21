// Copyright (C) 2013-2019 Red Hat, Inc.
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
    parent_node p("base");
    obj.start(); 
    obj.add_parent(p);
    obj.finish();
  }


  // sa-A
  {
    dot obj("sa-A");
    parent_node p("base");
    child_node c1("A");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.finish();
  }

  // sa-B
  {
    dot obj("sa-B");
    parent_node p("base");
    child_node c1("B");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.finish();
  }

  // sa-D1
  {
    dot obj("sa-D1");
    parent_node p("base");
    child_node c1("A");
    child_node c2("D1");
    obj.start(); 
    obj.add_parent(p);
    obj.add_child_to_node(c1, p);
    obj.add_child_to_node(c2, c1);
    obj.finish();
  }

  return 0;
}
