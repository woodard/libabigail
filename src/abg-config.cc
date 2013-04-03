// -*- Mode: C++ -*-
/// @file

#include "abg-config.h"
#include "abg-version.h"

namespace abigail
{
config::config()
  : m_format_minor(0),
    m_format_major(1),// The version number of the serialization
		     // format.
    m_xml_element_indent(2),
    m_tu_instr_suffix(".bi"),
    m_tu_instr_archive_suffix (".abi")
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

const std::string&
config::get_tu_instr_suffix() const
{
  return m_tu_instr_suffix;
}

void
config::set_tu_instr_suffix(const std::string& s)
{
  m_tu_instr_suffix = s;
}

const std::string&
config::get_tu_instr_archive_suffix() const
{
  return m_tu_instr_archive_suffix;
}

void
config::set_tu_instr_archive_suffix(const std::string& s)
{
  m_tu_instr_archive_suffix = s;
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
