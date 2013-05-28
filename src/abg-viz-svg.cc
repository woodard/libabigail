
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


// Utility function, like regex_replace.
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


void
svg::write()
{
  try
    {
      std::string filename(_M_title + ".svg");
      std::ofstream f(filename);
      if (!f.is_open() || !f.good())
	throw std::runtime_error("abigail::svg::write fail");
	  
      f << _M_sstream.str() << std::endl; 
    }
  catch(std::exception& e)
    {
      throw e;
    }
}

// SVG element beginning boilerplate.
// Variable: units, x=0, y=0, width, height
void
svg::start_element()
{
  const std::string start = R"_delimiter_(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1"
     id="svg2" xml:space="preserve"
     xmlns:dc="http://purl.org/dc/elements/1.1/"
     xmlns:cc="http://creativecommons.org/ns#"
     xmlns:svg="http://www.w3.org/2000/svg"
     xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink"
)_delimiter_";

  const std::string units("__units");
  const std::string width("__width");
  const std::string height("__height");

  std::string strip = R"_delimiter_(x="0__units" y="0__units" 
width="__width__units" height="__height__units"
viewBox="0 0 __width __height" enable-background="new 0 0 __width __height">
)_delimiter_";

  string_replace(strip, units, units_to_string(_M_canvas._M_units));
  string_replace(strip, width, std::to_string(_M_canvas._M_width));
  string_replace(strip, height, std::to_string(_M_canvas._M_height));

  _M_sstream << start;
  _M_sstream << strip << std::endl;
}

// SVG element end boilerplate.
void
svg::finish_element()
{
  _M_sstream << "</svg>" << std::endl;
}

void
svg::add_title()
{
  _M_sstream << "<title>" << _M_title << "</title>" << std::endl;
}

// Column labels
// Variable: x, y
void
svg::add_y_label()
{
  unsigned int xcur = 0;
  const unsigned int padding = 10;
  const std::string x("__x");
  const std::string y("__y");
  const std::string label("__label");
  const std::string style("__style");
  const std::string offset("OFFSET");
  const std::string size("SIZE");
  const std::string align("ALIGN");

  // Base text element.
  std::string text_strip =  R"_delimiter_(<text x="__x" y="__y" transform="rotate(270 __x __y)" __style>__label</text>
)_delimiter_";

  // These parts are the same for every text element ...
  string_replace(text_strip, y, std::to_string(_M_y_origin - padding));
  string_replace(text_strip, style, _M_typo.to_attribute(typography::start));

  // ... just the label and the x position in the center of the current column.
  xcur = _M_x_origin + (.5 * _M_x_space);
  std::string offset_strip = text_strip;
  string_replace(offset_strip, x, std::to_string(xcur));
  string_replace(offset_strip, label, offset);
  
  xcur += _M_x_space;
  std::string size_strip = text_strip;
  string_replace(size_strip, x, std::to_string(xcur));
  string_replace(size_strip, label, size);

  xcur += _M_x_space;
  std::string align_strip = text_strip;
  string_replace(align_strip, x, std::to_string(xcur));
  string_replace(align_strip, label, align);

  _M_sstream << "<g><!-- vertical labels -->" << std::endl;
  _M_sstream << offset_strip;
  _M_sstream << size_strip;
  _M_sstream << align_strip;
  _M_sstream << "</g>" << std::endl;
}

// Draws in 4 vertical hairlines.
// Variable: x, y, _M_y_size, _M_y_space
void
svg::add_y_lines()
{
  unsigned int xcur = 0;
  const unsigned int yend = _M_y_origin + _M_y_size * _M_y_space;
  const std::string x("__x");
  const std::string y1("__y1");
  const std::string y2("__y2");

  std::string strip = R"_delimiter_(<path stroke="black" stroke-width="1" d="M __x __y1 L __x __y2"/>
)_delimiter_";

  // These parts are the same for every text element ...
  string_replace(strip, y1, std::to_string(_M_y_origin - _M_y_space));
  string_replace(strip, y2, std::to_string(yend));

  xcur = _M_x_origin;
  std::string strip_1 = strip;
  string_replace(strip_1, x, std::to_string(xcur));

  xcur += _M_x_space;
  std::string strip_2 = strip;
  string_replace(strip_2, x, std::to_string(xcur));

  xcur += _M_x_space;
  std::string strip_3 = strip;
  string_replace(strip_3, x, std::to_string(xcur));

  xcur += _M_x_space;
  std::string strip_4 = strip;
  string_replace(strip_4, x, std::to_string(xcur));


  _M_sstream << "<g><!-- vertical lines -->" << std::endl;
  _M_sstream << strip_1;
  _M_sstream << strip_2;
  _M_sstream << strip_3;
  _M_sstream << strip_4;
  _M_sstream << "</g>" << std::endl;
}

// Add in a row of data.
// Columns assumed to be: offset, size, align, data member name/label
// Variable: x, y, row type, 
void
svg::add_y_row(const row& __r)
{
  // Background rectangles are horizontally-oriented on column and row
  // boundaries, and span the second to third column.
  unsigned int xcur = 0;
  std::string chroma;
  const unsigned int ycur = _M_y_origin + (_M_y_size * _M_y_space) + (.5 * _M_y_space);
  const std::string x("__x");
  const std::string y("__y");
  const std::string name("__name");
  const std::string style("__style");
  const std::string color("__color");
  const std::string width("__width");
  const std::string height("__height");
  const std::string val("__val");

  std::string rect_strip = R"_delimiter_(<rect x="__x" y="__y" fill="__color" stroke="__color" stroke-width="1" width="__width" height="__height"/>
)_delimiter_";

  xcur = _M_x_origin + _M_x_space;
  chroma = color_to_string(__r._M_style._M_fill_color);
  string_replace(rect_strip, x, std::to_string(xcur));
  string_replace(rect_strip, y, std::to_string(ycur - (.5 * _M_y_space)));
  string_replace(rect_strip, width, std::to_string(_M_x_space * 2));
  string_replace(rect_strip, height, std::to_string(_M_y_space));
  string_replace(rect_strip, color, chroma);


  // Text template for each bit of data.
  std::string text_strip = R"_delimiter_(<text x="__x" y="__y" fill="__color" __style>__val</text>
)_delimiter_";

  // Column 1 offset
  // Optional offset, if not a primary type row.
  std::string offset_strip(text_strip);
  xcur = _M_x_origin + (.5 * _M_x_space);
  chroma = color_to_string(abigail::color::black);
  string_replace(offset_strip, x, std::to_string(xcur));
  string_replace(offset_strip, y, std::to_string(ycur));
  string_replace(offset_strip, val, std::to_string(__r._M_offset));
  string_replace(offset_strip, style, _M_typo.to_attribute(typography::middle));
  string_replace(offset_strip, color, chroma);


  // Column 2 size
  std::string size_strip(text_strip);
  xcur += _M_x_space;
  chroma = color_to_string(__r._M_style._M_text_color);
  string_replace(size_strip, x, std::to_string(xcur));
  string_replace(size_strip, y, std::to_string(ycur));
  string_replace(size_strip, val, std::to_string(__r._M_size));
  string_replace(size_strip, style, _M_typo.to_attribute(typography::middle));
  string_replace(size_strip, color, chroma);
		 

  // Column 3 align
  std::string align_strip(text_strip);
  xcur += _M_x_space;
  string_replace(align_strip, x, std::to_string(xcur));
  string_replace(align_strip, y, std::to_string(ycur));
  string_replace(align_strip, val, std::to_string(__r._M_align));
  string_replace(align_strip, style, _M_typo.to_attribute(typography::middle));
  string_replace(align_strip, color, chroma);


   // Column 4 data member id
  const unsigned int padding = 10;
  std::string name_strip(text_strip);
  xcur = _M_x_origin + (_M_x_size * _M_x_space) + padding;
  chroma = color_to_string(abigail::color::black);
  string_replace(name_strip, x, std::to_string(xcur));
  string_replace(name_strip, y, std::to_string(ycur));
  string_replace(name_strip, val, __r._M_id);
  string_replace(name_strip, style, _M_typo.to_attribute(typography::start));
  string_replace(name_strip, color, chroma);


  // Write out stripped strings.
  _M_sstream << "<g><!-- row " << _M_y_size << " -->" << std::endl;
  _M_sstream << rect_strip;
  _M_sstream << offset_strip;
  _M_sstream << size_strip;
  _M_sstream << align_strip;
  _M_sstream << name_strip;
  _M_sstream << "</g>" << std::endl;

  ++_M_y_size;
 }


}//end namespace abigail
