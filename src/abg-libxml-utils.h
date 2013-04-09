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

#include <tr1/memory>
#include <libxml/xmlreader.h>

using std::tr1::shared_ptr;

namespace abigail
{
namespace xml
{

/// This functor is used to instantiate a shared_ptr for the
/// xmlTextReader.
struct textReaderDeleter
{
  void
  operator()(xmlTextReaderPtr reader)
  {
    xmlFreeTextReader(reader);
  }
};//end struct textReaderDeleter


typedef shared_ptr<xmlTextReader> reader_sptr;

/// This functor is used to instantiate a shared_ptr for xmlChar
struct charDeleter
{
  void
  operator()(xmlChar* str)
  {
    xmlFree(str);
  }
};//end struct xmlCharDeleter
typedef shared_ptr<xmlChar> xml_char_sptr;

reader_sptr new_reader_from_file(const std::string& path);
xml_char_sptr build_xml_char_sptr(xmlChar *);

template<class T> shared_ptr<T> build_sptr(T* p);

/// Specialization of build_sptr for xmlTextReader
template<>
shared_ptr<xmlTextReader>
build_sptr<xmlTextReader>(xmlTextReader *p);

/// Specialization of build_str for xmlChar.
template<>
shared_ptr<xmlChar>
build_sptr<xmlChar>(xmlChar *p);

int get_xml_node_depth(xmlNodePtr);

/// Get the name of the current element node the reader is pointing
/// to.  Note that this macro returns an instance of
/// shared_ptr<xmlChar> so that the caller doesn't have to worry about
/// managing memory itself.  Also note that the reader is a
/// shared_ptr<xmlTextReader>
#define XML_READER_GET_NODE_NAME(reader) \
  xml::build_sptr(xmlTextReaderName(reader.get()))

/// Get the type of the current node of the shared_ptr<xmlTextReader>
/// passed in argument.
#define XML_READER_GET_NODE_TYPE(reader) \
  static_cast<xmlReaderTypes> (xmlTextReaderNodeType(reader.get()))

/// Get the value of attribute 'name' on the current node of 'reader'
/// which is an instance of shared_ptr<xmlTextReader>.
#define XML_READER_GET_ATTRIBUTE(reader, name) \
  xml::build_sptr(xmlTextReaderGetAttribute(reader.get(), BAD_CAST(name)))

/// Get the value of attribute 'name' ont the instance of xmlNodePtr
/// denoted by 'node'.
#define XML_NODE_GET_ATTRIBUTE(node, name) \
  xml::build_sptr(xmlGetProp(node, BAD_CAST(name)))

#define CHAR_STR(xml_char_str) \
  reinterpret_cast<char*>(xml_char_str.get())

}//end namespace xml
}//end namespace abigail
