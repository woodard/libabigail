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
/// This file contains the declarations for the ini file reader used in
/// the libabigail library.

#ifndef __ABG_INI_H__
#define __ABG_INI_H__

#include <tr1/memory>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

namespace abigail
{
/// Namespace for handling ini-style files
namespace ini
{
// Inject some standard types in this namespace.
using std::tr1::shared_ptr;
using std::string;
using std::vector;
using std:: pair;

class config;

/// A convenience typedef for a shared pointer to @ref config
typedef shared_ptr<config> config_sptr;

/// The abstraction of the structured content of an .ini file.  This
/// roughly follows what is explained at
/// http://en.wikipedia.org/wiki/INI_file.
class config
{
  class priv;
  typedef shared_ptr<priv> priv_sptr;

public:
  class section;
  /// A convenience typedef for a shared pointer to a config::section.
  typedef shared_ptr<section> section_sptr;

  /// A convenience typedef for a vector of config::section_sptr.
  typedef vector<section_sptr> section_vector;

  /// A convenience typedef for a pair of strings representing a
  /// property that lies inside a section.  The first element of the
  /// pair is the property name, and the second is the property value.
  typedef std::pair<string, string> property;

  /// A convenience typedef for a shared pointer to a @ref property.
  typedef shared_ptr<property> property_sptr;

  /// A convenience typedef for a vector of @ref property_sptr
  typedef vector<property_sptr> property_vector;

private:
  priv_sptr priv_;

public:

  config();

  config(const string& path,
	 section_vector& sections);

  virtual ~config();

  const string&
  get_path() const;

  void
  set_path(const string& path);

  const section_vector&
  get_sections() const;

  void
  set_sections(const section_vector& sections);
}; // end class config

/// The abstraction of one section of the .ini config.
class config::section
{
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

  priv_sptr priv_;

  // Forbid this
  section();

public:
  section(const string& name);

  section(const string& name, const property_vector& properties);

  const string&
  get_name() const;

  const property_vector&
  get_properties() const;

  void
  set_properties(const property_vector& properties);

  void
  add_property(const property_sptr prop);
  virtual ~section();
}; //end class config::property

bool
read_sections(std::istream& input,
	      config::section_vector& sections);

bool
read_sections(const string& path,
	      config::section_vector& sections);

bool
read_config(std::istream& input,
	    config& conf);

config_sptr
read_config(std::istream& input);

bool
read_config(const string& path,
	    config& conf);

config_sptr
read_config(const string& path);

bool
write_sections(const config::section_vector& sections,
	       std::ostream& output);

bool
write_sections(const config::section_vector& sections,
	       const string& path);

bool
write_config(const config& conf,
	     std::ostream& output);

bool
write_config(const config& conf,
	     const string& path);

}// end namespace ini
}// end namespace abigail
#endif // __ABG_INI_H__
