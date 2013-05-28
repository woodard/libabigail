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
/// @file

#ifndef __ABG_VIZ_SVG_H__
#define __ABG_VIZ_SVG_H__

#include <sstream> //stringstream
#include <string>
#include <tuple>
#include <array>


namespace abigail
{

/// Measurement abstraction type, conversion function.
enum class units
{
  // NB: 1 pixel = .264583 mm
  millimeter, 		// mm
  pixel			// px
};

std::string
units_to_string(units);

typedef unsigned short	units_type;


/// Color, conversion function.
enum class color
{
  white,	
  gray25,		// gainsboro
  gray75,		// slategray
  black
};

std::string
color_to_string(color);


/*
  Page/Canvas/Drawing area description.
  Size, origin location in 2D (x,y), heigh, width

  ANSI Letter mm == (units::millimeter, 215.9, 279.4);
  ANSI Letter pixels == (units::pixel, 765, 990);
  ISO A4 mm == (units::millimeter, 210, 297);
  ISO A4 pixels == (units::pixel, 744.09, 1052.36);
 */
struct canvas
{
  units		       	_M_units;
  units_type	       	_M_width;
  units_type	       	_M_height;
};
 
/// Useful canvas constants.
extern const canvas  	ansi_letter_canvas;
extern const canvas	iso_a4_canvas;


/*
  Character rendering, type, fonts, styles.  

  Expect to keep changing the output, so use this abstraction to set
  styling defaults, so that one can just assign types instead of doing
  a bunch of search-and-replace operations when changing type
  characteristics.
 */
struct typography
{
  enum anchor { start, middle };

  std::string		_M_face;	// System font name
  unsigned short	_M_size;	// Display size
  color			_M_color;
  std::string		_M_style;	// Any other attributes

  std::string
  to_attribute(anchor) const;

  std::string
  anchor_to_string(anchor) const;
};

/// Useful typography constants.
extern const typography arial_typo;
extern const typography source_code_pro_typo;
extern const typography roboto_light_typo;


/*
  Row displaying one element of member data.

  Wide open background spaces, what do they look like, what do the
  things in the foreground look like? Rectangle, for one.

  Some characteristics:
  - horizontal label  (text anchor = start ie left).
  - background box
  - text data (text anchor = middle ie centered).
 */
struct row
{
  // XXX need type-based mechanism for styles
  struct style
  {
    color	       	_M_text_color;
    color	       	_M_fill_color;
    std::string		_M_style;	// Any other attributes
  };

  std::string		_M_id;
  const style&		_M_style;

  units_type		_M_offset;
  units_type		_M_size;
  units_type		_M_align;
};

/// Useful row constants. Maybe just do enum->value map.
 extern const row::style primary_row_sty;
 extern const row::style base_row_sty;
 extern const row::style member_row_sty;
 extern const row::style implementation_row_sty;


/*
  SVG "array" style notation for size/layout/alignment.

  This is a compact SVG representation of class layout.

  It is composed of a minimum of three data points for each member or
  base of a class:

  - offset index
  - size
  - align

  Including annotations for holes, padding, and
  implementation-specified and otherwise invisible objects.

  It's constructed by creating vertical columns for each of the data
  points specified above, and filling in horizontal rows for each of
  the class components.
 */
struct svg
{

private:

  const std::string    	_M_title;
  const canvas&	       	_M_canvas;
  const typography&    	_M_typo;	

  const units_type	_M_x_size = 3;	// Number of columns
  units_type   		_M_x_space;	// Column spacing.
  units_type   		_M_x_origin;	// X origin

  units_type   		_M_y_size;	// Number of rows
  units_type   		_M_y_space;	// Row spacing.
  units_type   		_M_y_origin;	// Y origin

  std::ostringstream   	_M_sstream;
  
  // static const units_type _M_stroke_width = 1;
  // static const units_type _M_text_padding = 10;

public:

  svg(const std::string __title, 
      const canvas& __cv = ansi_letter_canvas,
      const typography& __typo = arial_typo) 
  : _M_title(__title), _M_canvas(__cv), _M_typo(__typo), _M_y_size(0)
  { 
    // Offsets require: typo, canvas units, size.
    _M_x_space = 40;
    _M_y_space = 40;
    _M_x_origin = _M_x_space * 1;
    _M_y_origin = _M_y_space * 2;
  }
  
  // Empty when the output buffer is.
  bool
  empty() { return _M_sstream.str().empty(); }

  void 
  start_element();
  
  void 
  finish_element();
  
  void 
  add_title();

  void 
  add_y_row(const row&);

  void 
  add_y_lines();

  void 
  add_y_label();

  void
  write();

  void 
  start()
  {
    this->start_element();
    this->add_title();
  }

  void 
  finish()
  {
    this->add_y_label();
    this->add_y_lines();
    this->finish_element();
    this->write();
  }
};

// XXX connect external xml file to input. 
// parse input, pick apart elements, attributes.

}// end namespace abigail

#endif //__ABG_VIZ_SVG_H__
