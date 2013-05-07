// -*- Mode: C++ -*-
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


/// @file

#include "abg-config.h"
#include "abg-version.h"

namespace abigail
{
config::config()
  : m_format_minor(0),
    m_format_major(1),// The version number of the serialization
		     // format.
    m_xml_element_indent(2)
{
}

unsigned char
config::get_format_minor_version_number() const
{
  return m_format_minor;
}

void
config::set_format_minor_version_number(unsigned char v)
{
  m_format_minor = v;
}

unsigned char
config::get_format_major_version_number() const
{
  return m_format_major;
}

void
config::set_format_major_version_number(unsigned char v)
{
  m_format_major= v;
}

unsigned
config::get_xml_element_indent() const
{
  return m_xml_element_indent;
}

void
config::set_xml_element_indent(unsigned indent)
{
  m_xml_element_indent = indent;
}

}//end namespace abigail

extern "C"
{

  /// Return the relevant version numbers of the library.
  ///
  /// \param major the majar version number of the library.
  ///
  /// \param minor the minor version number of the library.
  ///
  /// \param revision the revision version number of the library.
void
abigail_get_library_version(int& major, int& minor, int& revision)
{
  major = ABIGAIL_VERSION_MAJOR;
  minor = ABIGAIL_VERSION_MINOR;
  revision = ABIGAIL_VERSION_REVISION;
}

}
