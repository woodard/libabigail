// -*- mode: C++ -*-

#include <ostream>
#include <sstream>
#include <tr1/memory>
#include "abg-writer.h"
#include "abg-config.h"

using std::tr1::shared_ptr;
using std::tr1::dynamic_pointer_cast;
using std::ostream;
using std::ostringstream;
using std::list;

namespace abigail
{
namespace writer
{

class id_manager
{

  unsigned long long
  get_new_id()
  {
    return ++m_cur_id;
  }

public:
  id_manager()
    : m_cur_id(0)
  {
  }

  /// Return a unique string representing a numerical id.
  string
  get_id()
  {
    ostringstream o;
    o << get_new_id();
    return o.str();
  }

  /// Return a unique string representing a numerical ID, prefixed by
  /// #prefix.
  ///
  /// \param prefix the prefix of the returned unique id.
  string
  get_id_with_prefix(const string& prefix)
  {
  ostringstream o;
    o << prefix << get_new_id();
    return o.str();
  }

private:
  unsigned long long m_cur_id;
};//end class id_manager

class write_context
{

  write_context();

public:

  write_context(ostream& os)
    : m_ostream(os)
  {
  }

  const config&
  get_config() const
  {
    return m_config;
  }

  ostream&
  get_ostream()
  {
    return m_ostream;
  }

  id_manager&
  get_id_manager()
  {
    return m_id_manager;
  }

private:
  id_manager m_id_manager;
  config m_config;
  ostream& m_ostream;
};//end write_context

static bool write_corpus(const abi_corpus&,
			 write_context&,
			 unsigned);
static bool write_decl(const shared_ptr<decl_base>,
		       const abi_corpus&,
		       write_context&,
		       unsigned);
static bool write_type_decl(const shared_ptr<type_decl>,
			    const abi_corpus&,
			    write_context&,
			    unsigned);
static bool write_namespace_decl(const shared_ptr<namespace_decl>,
				 const abi_corpus&,
				 write_context&,
				 unsigned);

void do_indent(ostream&, unsigned);

/// Emit #nb_whitespaces white spaces into the output stream #o.
void
do_indent(ostream& o, unsigned nb_whitespaces)
{
  for (unsigned i = 0; i < nb_whitespaces; ++i)
    o << ' ';
}

/// Serialize an abi corpus into an output stream.
///
/// \param corpus the corpus to serialize
///
/// \param out the output stream.
///
/// \return true upon successful completion, false otherwise.
bool
write_to_ostream(const abi_corpus& corpus,
		 ostream &out)
{
  write_context ctxt(out);

  return write_corpus(corpus, ctxt, /*indent=*/0);
}

/// Serialize a pointer to an of decl_base into an output stream.
///
/// \param decl, the pointer to decl_base to serialize
///
/// \param corpus the abi corpus the decl belongs to.
///
/// \param ctxt the context of the serialization.  It contains e.g, the
/// output stream to serialize to.
///
/// \param indent how many indentation spaces to use during the
/// serialization.
///
/// \return true upon successful completion, false otherwise.
static bool
write_decl(const shared_ptr<decl_base>	decl,
	   const abi_corpus&		corpus,
	   write_context&		ctxt,
	   unsigned			indent)
{
  if (write_type_decl(dynamic_pointer_cast<type_decl> (decl),
		      corpus, ctxt, indent))
    return true;
  if (write_namespace_decl(dynamic_pointer_cast<namespace_decl>(decl),
			    corpus, ctxt, indent))
    return true;

  return false;
}

/// Serialize an abi corpus into an output stream.
///
/// \param a corpus the abi corpus to serialize.
///
/// \param ctxt the context of the serialization.  It contains e.g,
/// the output stream to serialize to.
///
/// \param indent how many indentation spaces to use during the
/// serialization.
///
/// \return true upon successful completion, false otherwise.
static bool
write_corpus(const abi_corpus& corpus, write_context& ctxt, unsigned indent)
{
  ostream &o = ctxt.get_ostream();
  const config &c = ctxt.get_config();

  do_indent(o, indent);

  o << "<abi-instr version='"
    << static_cast<int> (c.get_format_major_version_number())
    << "." << static_cast<int>(c.get_format_minor_version_number())
    << "'";

  if (corpus.is_empty())
    {
      o << "/>";
      return true;
    }
  else
    o << ">";

  for (abi_corpus::decls_type::const_iterator i = corpus.get_decls().begin();
       i != corpus.get_decls().end();
       ++i)
    {
      o << "\n";
      write_decl(*i, corpus, ctxt,
		 indent + c.get_xml_element_indent());
    }

  o << "\n";
  do_indent(o, indent);
  o << "</abi-instr>\n";

  return true;
}

/// Serialize a pointer to an instance of basic type declaration, into
/// an output stream.
///
/// \param d the basic type declaration to serialize.
///
/// \param corpus the instance of abi corpus the declaration belongs
/// to.
///
/// \param ctxt the context of the serialization.  It contains e.g, the
/// output stream to serialize to.
///
/// \param indent how many indentation spaces to use during the
/// serialization.
///
/// \return true upon successful completion, false otherwise.
static bool
write_type_decl(const shared_ptr<type_decl>	d,
		const abi_corpus&		corpus,
		write_context&			ctxt,
		unsigned			indent)
{
  if (!d)
    return false;

  ostream &o = ctxt.get_ostream();

  do_indent(o, indent);

  o << "<type-decl name='" << d->get_name() << "'";

  size_t size_in_bits = d->get_size_in_bits();
  if (size_in_bits)
    o << " size-in-bits='" << size_in_bits << "'";
  size_t alignment_in_bits = d->get_alignment_in_bits();
  if (alignment_in_bits)
    o << " alignment-in-bits='" << alignment_in_bits << "'";

  location loc = d->get_location();
  if (loc)
    {
      string path;
      unsigned line = 0, column = 0;
      corpus.get_loc_mgr().expand_location(loc, path, line, column);
      o << " filepath='" << path << "'"
	<< " line='" << line << "'"
	<< " column='" << column << "'";
    }

  o << " xml:id='"
    << ctxt.get_id_manager().get_id_with_prefix("type-decl-")
    << "'";

  o<< "/>";

  return true;
}

/// Serialize a namespace declaration int an output stream.
///
/// \param decl the namespace declaration to serialize.
///
/// \param corpus the instance of abi corpus the declaration belongs
/// to.
///
/// \param ctxt the context of the serialization.  It contains e.g, the
/// output stream to serialize to.
///
/// \param indent how many indentation spaces to use during the
/// serialization.
///
/// \return true upon successful completion, false otherwise.
static bool
write_namespace_decl(const shared_ptr<namespace_decl> decl,
		     const abi_corpus& corpus,
		     write_context& ctxt,
		     unsigned indent)
{
  if (!decl)
    return false;

  ostream &o = ctxt.get_ostream();
  const config &c = ctxt.get_config();

  do_indent(o, indent);

  o << "<namespace-decl name='" << decl->get_name() << "'>";

  for (list<shared_ptr<decl_base> >::const_iterator i =
	 decl->get_member_decls ().begin();
       i != decl->get_member_decls ().end();
       ++i)
    {
      o << "\n";
      write_decl(*i, corpus, ctxt,
		 indent + c.get_xml_element_indent());
    }

  o << "</namespace-decl-name>";

  return true;
}

}//end namespace writer
}//end namespace abigail
