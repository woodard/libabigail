// -*- mode: C++ -*-
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
// License along with this program; see the file COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.

/// @file

#include <string>
#include "abg-libxml-utils.h"

namespace abigail
{
namespace xml
{
/// Instantiate an xmlTextReader that parses the content of an on-disk
/// file, wrap it into a smart pointer and return it.
///
/// @param path the path to the file to be parsed by the returned
/// instance of xmlTextReader.
reader_sptr
new_reader_from_file(const std::string& path)
{
  reader_sptr p =
    build_sptr(xmlNewTextReaderFilename (path.c_str()));

  return p;
}

/// Build and return a shared_ptr for a pointer to xmlTextReader
template<>
shared_ptr<xmlTextReader>
build_sptr<xmlTextReader>(xmlTextReader *p)
{
  return shared_ptr<xmlTextReader>(p, textReaderDeleter());
}

/// Build and return a shared_ptr for a pointer to xmlChar
template<>
shared_ptr<xmlChar>
build_sptr<xmlChar>(xmlChar *p)
{
  return shared_ptr<xmlChar>(p, charDeleter());
}

/// Return the depth of an xml element node.
///
/// Note that the node must be attached to an XML document.
///
/// @param n the xml to consider.
///
/// @return a positive or zero number for an XML node properly
/// attached to an xml document, -1 otherwise.  Note that the function
/// returns -1 if passed an xml document as well.
int
get_xml_node_depth(xmlNodePtr n)
{
  if (n->type == XML_DOCUMENT_NODE || n->parent == NULL)
    return -1;

  if (n->parent->type == XML_DOCUMENT_NODE)
    return 0;

  return 1 + get_xml_node_depth(n->parent);
}

}//end namespace xml
}//end namespace abigail
