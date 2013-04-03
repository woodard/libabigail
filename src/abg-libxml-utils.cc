// -*- mode: C++ -*-
/// @file

#include <string>
#include "abg-libxml-utils.h"

namespace abigail
{
namespace xml
{
/// Instanciate an xmlTextReader, wrap it into a smart pointer and
/// return it.
///
/// \param path the path to the file to be parsed by the returned
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

}//end namespace xml
}//end namespace abigail
