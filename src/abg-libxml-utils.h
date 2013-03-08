// -*- mode: C++ -*-

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

/// Get the value attribute name of the current node of reader which is an
/// instance of shared_ptr<xmlTextReader>.
#define XML_READER_GET_ATTRIBUTE(reader, name) \
  xml::build_sptr(xmlTextReaderGetAttribute(reader.get(), BAD_CAST(name)))

#define CHAR_STR(xml_char_str) \
  reinterpret_cast<char*>(xml_char_str.get())

}//end namespace xml
}//end namespace abigail
