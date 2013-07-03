
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

#include "abg-viz-svg.h"
#include <stdexcept>
#include <fstream>

namespace abigail
{

using std::ostream;
using std::ostringstream;

// Constants.

// Using pixels, units vs. representation
// const canvas ansi_letter_mm = { units::millimeter, 215.9, 279.4 };
// const canvas iso_a4_mm = { units::millimeter, 210, 297 };
// const canvas ansi_letter_px = { units::pixel, 765, 990 };
// const canvas iso_a4_px = { units::pixel, 765, 990 };
const canvas ansi_letter_canvas = { units::pixel, 765, 990 };
const canvas iso_a4_canvas = { units::pixel, 765, 990 };

const typography arial_typo = \
  { "'ArialMT'", 12, color::black, R"(text-anchor="middle")"};

const typography source_code_pro_typo = \
  { "Source Code Pro Light", 12, color::black, R"(text-anchor="middle")"};

const typography roboto_typo = \
  { "Roboto Light", 12, color::black, R"(text-anchor="middle")"};

const row::style primary_row_sty = { color::white, color::black, "" };
const row::style base_row_sty = { color::white, color::gray75, "" };
const row::style member_row_sty = { color::black, color::gray25, "" };
const row::style implementation_row_sty = { color::black, color::white, "" };


// XXX Do not export.
void
string_replace(std::string& target, const std::string& match,
	       const std::string& replace)
{
  size_t pos = 0;
  while((pos = target.find(match, pos)) != std::string::npos)
    {
      target.replace(pos, match.length(), replace);
      pos += replace.length();
    }
}

std::string
units_to_string(units __val)
{
  std::string ret;
  switch (__val)
    {
    case units::millimeter:
      ret = "mm";
      break;
    case units::pixel:
      ret = "px";
      break;
    default:
      throw std::logic_error("abigail::units_to_string units not recognized");
      break;
    }
  return ret;
}

std::string
color_to_string(color __val)
{
  std::string ret;
  switch (__val)
    {
    case color::white:
      ret = "white";
      break;
    case color::gray25:
      ret = "gainsboro";
      break;
    case color::gray75:
      ret = "slategray";
      break;
    case color::black:
      ret = "black";
      break;
    default:
      throw std::logic_error("abigail::color_to_string color not recognized");
      break;
    }
  return ret;
}

std::string
typography::anchor_to_string(anchor __val) const
{
  std::string ret;
  switch (__val)
    {
    case start:
      ret = "start";
      break;
    case middle:
      ret = "middle";
      break;
    default:
      throw std::logic_error("abigail::anchor_to_string anchor not recognized");
      break;
    }
  return ret;
}


std::string
typography::to_attribute(anchor __a) const
{
  const std::string name("__name");
  const std::string size("__size");
  const std::string anchor("__anchor");
  std::string strip = R"(font-family="__name" font-size="__size" text-anchor="__anchor")";
  string_replace(strip, name, _M_face);
  string_replace(strip, size, std::to_string(_M_size));
  string_replace(strip, anchor, anchor_to_string(__a));

  // NB: Add in extra _M_style if necessary.
  return strip;
}

}//end namespace abigail
