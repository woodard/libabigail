// -*- mode: C++ -*-

#include <ostream>
#include <sstream>
#include <tr1/memory>
#include <tr1/unordered_map>
#include "abg-writer.h"
#include "abg-config.h"

using std::tr1::shared_ptr;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;
using std::ostream;
using std::ostringstream;
using std::list;
using std::tr1::unordered_map;

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

typedef unordered_map<shared_ptr<type_base>,
		      string,
		      type_shared_ptr_hash,
		      type_shared_ptr_equal> type_shared_ptr_map;
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

  /// Associate a unique id to a given type.  For that, put the type
  /// in a hash table, hashing the type.  So if the type has no id
  /// associated to it, create a new one and return it.  Otherwise,
  /// return the existing id for that type.
  string
  get_id_for_type(shared_ptr<type_base> t)
  {
    type_shared_ptr_map::const_iterator it = m_type_id_map.find(t);
    string id;
    if ( it == m_type_id_map.end())
      {
	string id = get_id_manager().get_id_with_prefix("type-id-");
	m_type_id_map[t] = id;
	return id;
      }
    else
      return m_type_id_map[t];
  }

private:
  id_manager m_id_manager;
  config m_config;
  ostream& m_ostream;
  type_shared_ptr_map m_type_id_map;
};//end write_context

static bool write_corpus(const abi_corpus&,
			 write_context&,
			 unsigned);
static void write_decl_location(const shared_ptr<decl_base>&,
				const abi_corpus&, ostream&);
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
static bool write_qualified_type_def(const shared_ptr<qualified_type_def>,
				     const abi_corpus&,
				     write_context&,
				     unsigned);
static bool write_pointer_type_def(const shared_ptr<pointer_type_def>,
				   const abi_corpus&,
				   write_context&,
				   unsigned);
static bool write_reference_type_def(const shared_ptr<reference_type_def>,
				     const abi_corpus&,
				     write_context&,
				     unsigned);
static bool write_enum_type_decl(const shared_ptr<enum_type_decl>,
				 const abi_corpus&,
				 write_context&,
				 unsigned);
static void	do_indent(ostream&, unsigned);

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

/// Write the location of a decl to the output stream.
///
/// If the location is empty, nothing is written.
///
/// \param decl the decl to consider.
///
/// \param corpus the corpus the decl belongs to.  The location
/// manager to use belongs to that that corpus.
///
/// \param o the output stream to write to.
static void
write_decl_location(const shared_ptr<decl_base>&	decl,
		    const abi_corpus&			corpus,
		    ostream&				o)
{
  if (!decl)
    return;

  location loc = decl->get_location();
  if (!loc)
    return;

  string filepath;
  unsigned line = 0, column = 0;
  corpus.get_loc_mgr().expand_location(loc, filepath, line, column);

  o << " filepath='" << filepath << "'"
    << " line='"     << line     << "'"
    << " column='"   << column   << "'";
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
		      corpus, ctxt, indent)
      || write_namespace_decl(dynamic_pointer_cast<namespace_decl>(decl),
			      corpus, ctxt, indent)
      || write_qualified_type_def (dynamic_pointer_cast<qualified_type_def>
				   (decl),
				   corpus, ctxt, indent)
      || write_pointer_type_def(dynamic_pointer_cast<pointer_type_def>(decl),
				corpus, ctxt, indent)
      || write_reference_type_def(dynamic_pointer_cast
				  <reference_type_def>(decl),
				  corpus, ctxt, indent)
      || write_enum_type_decl(dynamic_pointer_cast<enum_type_decl>(decl),
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

  write_decl_location(d, corpus, o);

  o << " id='" << ctxt.get_id_for_type(d) << "'" <<  "/>";

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
write_namespace_decl(const shared_ptr<namespace_decl>	decl,
		     const abi_corpus&			corpus,
		     write_context&			ctxt,
		     unsigned				indent)
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

  o << "\n";
  do_indent(o, indent);
  o << "</namespace-decl>";

  return true;
}

/// Serialize a qualified type declaration to an output stream.
///
/// \param decl the qualfied type declaration to write.
///
/// \param corpus the abi corpus it belongs to.
///
/// \param ctxt the write context.
///
/// \param indent the number of space to indent to during the
/// serialization.
///
/// \return true upon successful completion, false otherwise.
static bool
write_qualified_type_def(const shared_ptr<qualified_type_def>	decl,
			 const abi_corpus&			corpus,
			 write_context&			ctxt,
			 unsigned				indent)
{
  if (!decl)
    return false;

  ostream &o = ctxt.get_ostream();

  do_indent(o, indent);

  o << "<qualified-type-def type-id='"
    << ctxt.get_id_for_type(decl->get_underlying_type())
    << "'";

  if (decl->get_cv_quals() & qualified_type_def::CV_CONST)
    o << " const='yes'";
  if (decl->get_cv_quals() & qualified_type_def::CV_VOLATILE)
    o << " volatile='yes'";

  write_decl_location(static_pointer_cast<decl_base>(decl), corpus, o);

  o<< " id='"
    << ctxt.get_id_for_type(decl)
    << "'";

  o << "/>";

  return true;
}

/// Serialize a pointer to an instance of pointer_type_def.
///
/// \param decl the pointer_type_def to serialize.
///
/// \param corpus the ABI corpus it belongs to.
///
/// \param ctxt the context of the serialization.
///
/// \param indent the number of indentation white spaces to use.
///
/// \return true upon succesful completion, false otherwise.
static bool
write_pointer_type_def(const shared_ptr<pointer_type_def>	decl,
		       const abi_corpus&			corpus,
		       write_context&				ctxt,
		       unsigned				indent)
{
  if (!decl)
    return false;

  ostream &o = ctxt.get_ostream();

  do_indent(o, indent);

  o << "<pointer-type-def type-id='"
    << ctxt.get_id_for_type(decl->get_pointed_to_type())
    << "'";

  if (size_t s = decl->get_size_in_bits())
    o << " size-in-bits='" << s << "'";
  if (size_t s = decl->get_alignment_in_bits())
    o << " alignment-in-bits='" << s << "'";

  o << " id='" << ctxt.get_id_for_type(decl) << "'";

  write_decl_location(static_pointer_cast<decl_base>(decl), corpus, o);
  o << "/>";

  return true;
}

/// Serialize a pointer to an instance of reference_type_def.
///
/// \param decl the reference_type_def to serialize.
///
/// \param corpus the ABI corpus it belongs to.
///
/// \param ctxt the context of the serialization.
///
/// \param indent the number of indentation white spaces to use.
///
/// \return true upon succesful completion, false otherwise.
static bool
write_reference_type_def(const shared_ptr<reference_type_def>	decl,
			 const abi_corpus&			corpus,
			 write_context&			ctxt,
			 unsigned				indent)
{
  if (!decl)
    return false;

  ostream &o = ctxt.get_ostream();

  do_indent(o, indent);

  o << "<reference-type-def kind='";
  if (decl->is_lvalue())
    o << "lvalue";
  else
    o << "rvalue";
  o << "'";

  o << " type-id='" << ctxt.get_id_for_type(decl->get_pointed_to_type()) << "'";
  if (size_t s = decl->get_size_in_bits())
    o << " size-in-bits='" << s << "'";
  if (size_t s = decl->get_alignment_in_bits())
    o << " alignment-in-bits='" << s << "'";

  o << " id='" << ctxt.get_id_for_type(decl) << "'";

  write_decl_location(static_pointer_cast<decl_base>(decl), corpus, o);

  o << "/>";
  return true;
}

/// Serialize a pointer to an instance of enum_type_decl.
///
/// \param decl the enum_type_decl to serialize.
///
/// \param corpus the ABI corpus it belongs to.
///
/// \param ctxt the context of the serialization.
///
/// \param indent the number of indentation white spaces to use.
///
/// \return true upon succesful completion, false otherwise.
static bool
write_enum_type_decl(const shared_ptr<enum_type_decl>	decl,
		     const abi_corpus&			corpus,
		     write_context&			ctxt,
		     unsigned				indent)
{
    if (!decl)
    return false;

  ostream &o = ctxt.get_ostream();

  do_indent(o, indent);
  o << "<enum-decl name='" << decl->get_name() << "'";

  write_decl_location(static_pointer_cast<decl_base>(decl), corpus, o);

  o << " id='" << ctxt.get_id_for_type(decl) << "'>\n";

  do_indent(o, indent + ctxt.get_config().get_xml_element_indent());
  o << "<base type-id='"
    << ctxt.get_id_for_type(decl->get_underlying_type())
    << "'/>\n";

  for (list<enum_type_decl::enumerator>::const_iterator i =
	 decl->get_enumerators().begin();
       i != decl->get_enumerators().end();
       ++i)
    {
      do_indent(o, indent + ctxt.get_config().get_xml_element_indent());
      o << "<enumerator name='"
	<< i->get_name()
	<< "' value='"
	<< i->get_value()
	<< "'/>\n";
    }

  do_indent(o, indent);
  o << "</enum-decl>";

  return true;
}

}//end namespace writer
}//end namespace abigail
