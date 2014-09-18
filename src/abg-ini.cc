// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2014 Red Hat, Inc.
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
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions for the ini file reader used in
/// the libabigail library.

#include <cassert>
#include <utility>
#include <memory>
#include <fstream>
#include "abg-ini.h"

namespace abigail
{
namespace ini
{

using std::istream;
using std::pair;

class config::section::priv
{
  string name_;
  property_vector properties_;

  // Forbid this;
  priv();

public:
  priv(const string& name)
    : name_(name)
  {}

  friend class config::section;
};//end struct config::section::priv

// <config::section stuff>

/// Constructor for config::section.
///
/// @param name the name of the ini section.
config::section::section(const string& name)
  : priv_(new priv(name))
{}

/// Constructor for the config::section.
///
/// @param name the name of the ini section.
///
/// @param properties the properties of the section.
config::section::section(const string& name,
			 const property_vector& properties)
  : priv_(new priv(name))
{set_properties(properties);}

/// Get the name of the section.
///
/// @return the name of the section.
const string&
config::section::get_name() const
{return priv_->name_;}

/// Get the properties of the section.
///
/// @return a vector of the properties of the section.
const config::property_vector&
config::section::get_properties() const
{return priv_->properties_;}

/// Set the properties of the section.
///
/// @param properties the new properties to set.
void
config::section::set_properties(const property_vector& properties)
{priv_->properties_ = properties;}

/// Add one property to this section.
///
/// @param prop the property to add to the section.
void
config::section::add_property(const property_sptr prop)
{priv_->properties_.push_back(prop);}

/// Destructor of config::section.
config::section::~section()
{}
// /<config::section stuff>

// <read_context stuff>

/// The context of the ini file parsing.
///
/// This is a private type that is used only in the internals of the
/// ini file parsing.
class read_context
{
  /// The input stream we are parsing from.
  istream& in_;
  /// The current line being parsed.
  unsigned cur_line_;
  /// The current column on the current line.
  unsigned cur_column_;

  // Forbid this;
  read_context();

public:

  /// @param in the input stream to parse from.
  read_context(istream& in)
    : in_(in),
      cur_line_(0),
      cur_column_(0)
  {}

  /// Test if a given character is a delimiter.
  ///
  ///
  ///@param b the value of the character to test for.
  ///
  /// @return true iff @p b is a delimiter.
  bool
  char_is_delimiter(int b)
  {
    return (b == '['
	    || b == ']'
	    || b == '='
	    || char_is_white_space(b)
	    || char_is_comment_start(b));
  }

  /// Test if a given character is meant to be part of a section name.
  ///
  /// @param b the character to test against.
  ///
  /// @return true iff @p b is a character that is meant to be part of
  /// a section name.
  bool
  char_is_section_name_char(int b)
  {
    if (b == '[' || b == ']' || b == '\n' || char_is_comment_start(b))
      return false;
    return true;
  }

  /// Test if a given character is meant to be part of a property name.
  ///
  /// @param b the character to test against.
  ///
  /// @return true iff @p b is a character that is meant to be part of
  /// a section name.
  bool
  char_is_property_name_char(int b)
  {
    if (char_is_delimiter(b))
      return false;
    return true;
  }

  /// Test if a given character is meant to be the start of a comment.
  ///
  /// @param b the character to test against.
  ///
  /// @return true iff @p b is the start of a comment.
  bool
  char_is_comment_start(int b)
  {return b == ';' || b == '#';}

  /// Test if a character is meant a white space.
  ///
  /// @param b the character to test against.
  ///
  /// @return true iff @p b is a white space.
  bool
  char_is_white_space(int b)
  {return b == ' ' || b == '\t' || b == '\n';}

  /// Read the next character from the input stream.
  ///
  /// This method updates the current line/column number after looking
  /// at the actual char that got read.  It also handle escaped
  /// characters.
  ///
  /// @param c output parameter.  This is set by this function to the
  /// character that was read.  It's set iff the function returned
  /// true.
  ///
  /// @return true if the reading went well and if the input stream is
  /// in a non-erratic state.
  bool
  read_next_char(char& c)
  {
    char b = in_.get();
    if (!in_.good())
      return false;

    bool escaping = false;
    if (b == '\\')
      escaping = true;

    // Handle escape
    if (escaping)
      {
	b = in_.get();
	if (!in_.good())
	  return false;

	switch (b)
	  {
	  case '0':
	  case 'a':
	  case 'b':
	  case 'r':
	    // let's replace this by by a space
	    c = ' ';
	    break;
	  case 't':
	    c = '\t';
	    break;
	  case '\n':
	    // continuation line.  So we should drop both the backslash
	    // character and this end-of-line character on the floor
	    // just like if they never existed.
	    ++cur_column_;
	    b = in_.get();
	    if (!in_.good())
	      return false;
	    break;
	  case '\\':
	  case ';':
	  case '#':
	  default:
	    c = b;
	    break;
	  }
	++cur_column_;
      }
    else
      c = b;

    if (cur_line_ == 0)
      cur_line_ = 1;

    if (b == '\n')
      {
	++cur_line_;
	cur_column_ = 0;
      }
    else
      ++cur_column_;

    return true;
  }

  /// Skip (that is, read characters and drop them on the floor) all
  /// the characters up to the next line.
  ///
  /// Note that the new line character (\n' on unices) is skipped as
  /// well.
  ///
  /// @return true iff the skipping proceeded successfully and that
  /// the input stream is left in a non-erratic state.
  bool
  skip_line()
  {
    char c = 0;
    for (bool is_ok = read_next_char(c);
	 is_ok;
	 is_ok = read_next_char(c))
      if (c == '\n')
	break;

    return (c == '\n' || in_.eof());
  }

  /// If the current character is a white space, skip it and all the
  /// contiguous ones that follow.
  ///
  /// @return true iff the input stream is left in a non-erratic state.
  bool
  skip_white_spaces()
  {
    for (char c = in_.peek(); in_.good(); c = in_.peek())
      if (char_is_white_space(c))
	assert(read_next_char(c));
      else
	break;
    return in_.good() || in_.eof();
  }

  /// If the current character is the beginning of a comment, skip
  /// (read and drop on the floor) the entire remaining line,
  /// including the current character.
  ///
  /// @return true if the input stream is left in a non-erratic state.
  bool
  skip_comments()
  {
    for (char c = in_.peek(); in_.good(); c = in_.peek())
      if (char_is_comment_start(c))
	skip_line();
      else
	break;
    return in_.good() || in_.eof();
  }

  /// If the current character is either the beginning of a comment or
  /// a white space, skip the entire commented line or the subsequent
  /// contiguous white spaces.
  ///
  /// @return true iff the stream is left in a non-erratic state.
  bool
  skip_white_spaces_or_comments()
  {
    int b = 0;
    while (in_.good())
      {
	b = in_.peek();
	if (char_is_white_space(b))
	  skip_white_spaces();
	else if (char_is_comment_start(b))
	  skip_comments();
	else
	  break;
      }
    return in_.good() || in_.eof();
  }

  /// Read a property name.
  ///
  /// @param name out parameter.  Is set to the parsed property name,
  /// if any.  Note that this is set only if the function returned
  /// true.
  ///
  /// @return true iff the input stream is left in a non-erratic
  /// state.
  bool
  read_property_name(string& name)
  {
    char c = in_.peek();
    if (!in_.good() || !char_is_property_name_char(c))
      return false;

    assert(read_next_char(c));
    name += c;

    for (c = in_.peek(); in_.good(); c = in_.peek())
      {
	if (!char_is_property_name_char(c))
	  break;
	assert(read_next_char(c));
	name += c;
      }

    return true;
  }

  /// Read a property value.
  ///
  /// @param value out parameter.  Is set to the property value that
  /// has been parsed.  This is set only if the function returned true.
  ///
  /// @return true if the stream is left in non-erratic state.
  bool
  read_property_value(string& value)
  {
    int b = in_.peek();
    if (!in_.good())
      return false;

    if (char_is_delimiter(b))
      // Empty property value.  This is accepted.
      return true;

    char c = 0;
    assert(read_next_char(c));
    value += c;

    for (b = in_.peek(); in_.good();b = in_.peek())
      {
	if (char_is_delimiter(b))
	  break;
	assert(read_next_char(c));
	value += c;
      }
    return true;
  }

  /// Read the name of a section.
  ///
  /// @param name out parameter.  Is set to the name of the section
  /// that was parsed.  Note that this is set only if the function
  /// returned true.
  ///
  /// @return true if the input stream was left in a non-erratic
  /// state.
  bool
  read_section_name(string& name)
  {
    int b = in_.peek();
    if (!in_.good() || !char_is_section_name_char(b))
      return false;

    char c = 0;
    assert(read_next_char(c) || char_is_section_name_char(b));
    name += c;

    for (b = in_.peek(); in_.good(); b = in_.peek())
      {
	if (!char_is_section_name_char(b))
	  break;
	assert(read_next_char(c));
	name += c;
      }

    return true;
  }

  /// Read a property (<name> = <value>).
  ///
  /// @return the resulting pointer to property iff one could be
  /// parsed.
  config::property_sptr
  read_property()
  {
    config::property_sptr nil;

    string name;
    if (!read_property_name(name))
      return nil;

    skip_white_spaces();
    if (!in_.good())
      return nil;

    char c = 0;
    if (!read_next_char(c) || c != '=')
      return nil;

    skip_white_spaces();
    if (!in_.good())
      return nil;

    string value;
    if (!read_property_value(value))
      return nil;

    config::property_sptr result(new std::pair<string, string>(name, value));
    return result;
  }

  /// Read an ini section.
  ///
  /// @return a pointer to a section iff it could be successfully
  /// parsed.
  config::section_sptr
  read_section()
  {
    config::section_sptr nil;

    int b = in_.peek();
    if (!in_.good())
      return nil;

    char c = 0;
    if (b == '[')
      assert(read_next_char(c) && c == '[');

    string name;
    if (!read_section_name(name))
      return nil;

    if (!skip_white_spaces())
      return nil;

    if (! read_next_char(c) || c != ']')
      return nil;

    if (!skip_white_spaces_or_comments())
      return nil;

    config::property_vector properties;
    while (config::property_sptr prop = read_property())
      {
	properties.push_back(prop);
	skip_white_spaces_or_comments();
      }

    if (!properties.empty())
      {
	config::section_sptr section(new config::section(name, properties));
	return section;
      }

    return nil;
  }
};//end struct read_context

// </read_context stuff>

// <config stuff>

class config::priv
{
  string path_;
  sections_type sections_;

public:
  friend class config;

  priv()
  {}

  priv(const string& path,
       sections_type& sections)
    : path_(path),
      sections_(sections)
  {}

};

/// @param path the path to the config file.
///
/// @param sections the sections of the config file.
config::config(const string& path,
	       sections_type& sections)
  : priv_(new priv(path, sections))
{}

config::config()
  : priv_(new priv)
{}

config::~config()
{}

/// @return the path to the config file.
const string&
config::get_path() const
{return priv_->path_;}

/// Set the path to the config file.
///
/// @param the new path to the config file.
void
config::set_path(const string& path)
{priv_->path_ = path;}

/// @return the sections of the config file.
const config::sections_type&
config::get_sections() const
{return priv_->sections_;}

/// Set new sections to the ini config
///
/// @param sections the new sections to set.
void
config::set_sections(const sections_type& sections)
{priv_->sections_ = sections;}

// </config stuff>

// <config reader stuff>

/// Parse the sections of an *.ini file.
///
/// @param input the input stream to parse the ini file from.
///
/// @param section out parameter.  This is set to the vector of
/// sections that have been parsed from the input stream.
///
/// @return true upon successful completion and if if the stream is
/// left in a non-erratic state.
bool
read_sections(std::istream& input,
	      config::sections_type& sections)
{
  read_context ctxt(input);

  while (input.good())
    {
      ctxt.skip_white_spaces_or_comments();
      if (config::section_sptr section = ctxt.read_section())
	sections.push_back(section);
      else
	break;
    }

  return input.good() || input.eof();
}

/// Parse the sections of an *.ini file.
///
/// @param path the path of the ini file to parse.
///
/// @param section out parameter.  This is set to the vector of
/// sections that have been parsed from the input stream.
///
/// @return true upon successful completion and if if the stream is
/// left in a non-erratic state.
bool
read_sections(const string& path,
	      config::sections_type& sections)
{
  std::ifstream in(path, std::ifstream::binary);
  if (!in.good())
    return false;

  bool is_ok = read_sections(in, sections);
  in.close();

  return is_ok;
}

/// Parse an ini config file from an input stream.
///
/// @param input the input stream to parse the ini config file from.
///
/// @return true upon successful parsing.
bool
read_config(istream& input,
	    config& conf)
{
  config::sections_type sections;
  if (!read_sections(input, sections))
    return false;
  conf.set_sections(sections);
  return true;
}

/// Parse an ini config file from a file on disk.
///
/// @param path the path to the ini file to parse.
///
/// @param conf the resulting config file to populate as a result of
/// the parsing.  This is populated iff the function returns true.
///
/// @return true upon succcessful completion.
bool
read_config(const string& path,
	    config& conf)
{
  config::sections_type sections;
  if (!read_sections(path, sections))
    return false;
  conf.set_path(path);
  conf.set_sections(sections);
  return true;
}

/// Parse an ini config file from an input stream.
///
/// @return a shared pointer to the resulting config, or nil if it
/// couldn't be parsed.
config_sptr
read_config(std::istream& input)
{
  config_sptr c(new config);
  if (!read_config(input, *c))
    return config_sptr();
  return c;
}


/// Parse an ini config file from an on-disk file.
///
/// @return a shared pointer to the resulting config, or nil if it
/// couldn't be parsed.
config_sptr
read_config(const string& path)
{
  config_sptr c(new config);
  if (!read_config(path, *c))
    return config_sptr();
  return c;
}
// <config reader stuff>

// <config writer stuff>

/// Serialize an ini property to an output stream.
///
/// @param prop the property to serialize to the output stream.
///
/// @param out the output stream to serialize to.
///
/// @return true if the ouput stream is left in a non-erratic state.
static bool
write_property(const config::property& prop,
	       std::ostream& out)
{
  out << prop.first << " = " << prop.second;
  return out.good();
}

/// Serialize an ini section to an output stream.
///
/// @param section the ini section to serialize.
///
/// @param out the output stream to serialize the section to.
static bool
write_section(const config::section& section,
	      std::ostream& out)
{
  out << "[" << section.get_name() << "]\n";
  for (config::property_vector::const_iterator i =
	 section.get_properties().begin();
       i != section.get_properties().end();
       ++i)
    {
      out << "  ";
      write_property(**i, out);
      out << "\n";
    }
  return out.good();
}

/// Serialize a vector of sections that make up an ini config file to
/// an output stream.
///
/// Note that an ini config is just a collection of sections.
///
/// @param sections the vector of sections to serialize.
///
/// @param out the output stream.
///
/// @return true if the output stream is left in a non-erratic state.
bool
write_sections(const config::sections_type& sections,
	       std::ostream& out)
{
  for (config::sections_type::const_iterator i = sections.begin();
       i != sections.end();
       ++i)
    {
      write_section(**i, out);
      out << "\n";
    }
  return out.good();
}

/// Serialize a vector of sections that make up an ini config to a
/// file.
///
/// @param sections the vector of sections to serialize.
///
/// @param out the output stream.
///
/// @return true if the output stream is left in a non-erratic state.
bool
write_sections(const config::sections_type& sections,
	       const string& path)
{
  std::ofstream f(path, std::ofstream::binary);

  if (!f.good())
    return false;

  bool is_ok = write_sections(sections, f);

  f.close();

  return is_ok;
}

/// Serialize an instance of @ref config to an output stream.
///
/// @param conf the instance of @ref config to serialize.
///
/// @param output the output stream to serialize @p conf to.
///
/// @return true upon successful completion.
bool
write_config(const config& conf,
	     std::ostream& output)
{
  if (!write_sections(conf.get_sections(), output))
    return false;
  return true;
}

/// Serialize an instance of @ref conf to an on-disk file.
///
/// @param conf the instance of @ref config to serialize.
///
/// @param path the path to the on-disk file to serialize to.
///
/// @return true upon successful completion.
bool
write_config(const config& conf,
	     const string& path)
{
  if (!write_sections(conf.get_sections(), path))
    return false;
  return true;
}
// </confg writer stuff>

}// end namespace ini
}// end namespace abigail
