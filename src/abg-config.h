// -*- Mode: C++ -*-
/// @file

#ifndef __ABG_CONFIG_H__
#define __ABG_CONFIG_H__

namespace abigail
{

/// This type abstracts the configuration information of the library.
class config
{
public:
  config();

  unsigned char
  get_format_minor_version_number() const;

  void
  set_format_minor_version_number(unsigned char);

  unsigned char
  get_format_major_version_number() const;


  void
  set_format_major_version_number(unsigned char);

  unsigned
  get_xml_element_indent() const;

  void
  set_xml_element_indent(unsigned);

private:
  unsigned char m_format_minor;
  unsigned char m_format_major;
  unsigned m_xml_element_indent;
};//end class config

}//end namespace abigail

#endif //__ABG_CONFIG_H__
