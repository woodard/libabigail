// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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
// License along with this program; see the files COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.

/// @file

#include "config.h"
#include "abg-config.h"
#include "abg-version.h"

namespace abigail
{
config::config()
: m_format_minor(0), m_format_major(1),
  m_xml_element_indent(2),
  m_tu_instr_suffix(".bi"),
  m_tu_instr_archive_suffix (".abi")
{ }

unsigned char
config::get_format_minor_version_number() const
{ return m_format_minor; }

void
config::set_format_minor_version_number(unsigned char v)
{ m_format_minor = v; }

unsigned char
config::get_format_major_version_number() const
{ return m_format_major; }

void
config::set_format_major_version_number(unsigned char v)
{ m_format_major= v; }

unsigned
config::get_xml_element_indent() const
{ return m_xml_element_indent; }

void
config::set_xml_element_indent(unsigned indent)
{ m_xml_element_indent = indent; }

const std::string&
config::get_tu_instr_suffix() const
{ return m_tu_instr_suffix; }

void
config::set_tu_instr_suffix(const std::string& s)
{ m_tu_instr_suffix = s; }

const std::string&
config::get_tu_instr_archive_suffix() const
{ return m_tu_instr_archive_suffix; }

void
config::set_tu_instr_archive_suffix(const std::string& s)
{ m_tu_instr_archive_suffix = s; }

extern "C"
{
void
abigail_get_library_version(int& major, int& minor, int& revision)
{
  major = ABIGAIL_VERSION_MAJOR;
  minor = ABIGAIL_VERSION_MINOR;
  revision = ABIGAIL_VERSION_REVISION;
}

}
}//end namespace abigail

