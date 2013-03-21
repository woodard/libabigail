// -*- mode: C++ -*-

#include "abg-config.h"

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
