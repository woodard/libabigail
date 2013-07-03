
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
const style parent_sty = { color::white, color::black, "" };
const style child_sty = { color::white, color::gray75, "" };

void
dot::write() 
{
  try
    {
      std::string filename(_M_title + ".gv");
      std::ofstream f(filename);
      if (!f.is_open() || !f.good())
	throw std::runtime_error("abigail::dot::write fail");
	  
      f << _M_sstream.str() << std::endl; 
    }
  catch(std::exception& e)
    {
      throw e;
    }
}

// DOT element beginning boilerplate.
void
dot::start_element() 
{ 
  const std::string start = R"_delimiter_(digraph "__title" {)_delimiter_";
  _M_sstream << "digraph " << std::endl;
  add_title();
  _M_sstream << "{" << std::endl;
}

void
dot::finish_element() 
{
  _M_sstream << "}" << std::endl;
}

void
dot::add_title() 
{

  _M_sstream << '"' << _M_title << '"' << std::endl;
}

void
dot::add_parent(const parent& __p) 
{ 
   _M_sstream << "" << std::endl;
}

void
dot::add_child(const child& __c) 
{ 
   _M_sstream << "" << std::endl;
}

}//end namespace abigail
