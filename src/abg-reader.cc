// -*- mode: C++ -*-

#include <cstring>
#include <cstdlib>
#include <tr1/unordered_map>
#include <stack>
#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>
#include "abg-reader.h"
#include "abg-libxml-utils.h"

using std::string;
using std::stack;
using std::tr1::unordered_map;
using std::tr1::dynamic_pointer_cast;

namespace abigail
{
namespace reader
{

/// This abstracts the context in which the current ABI
/// instrumentation dump is being de-serialized.  It carries useful
/// information needed during the de-serialization, but that does not
/// make sense to be stored in the final resulting in-memory
/// representation of ABI Corpus.
class read_context
{
  read_context();

public:

  typedef unordered_map<string,
			shared_ptr<type_base> >::const_iterator
  const_types_map_it;

  read_context(xml::reader_sptr reader)
    : m_depth(0),
      m_reader(reader)
  {
  }

  int
  get_depth() const
  {
    return m_depth;
  }

  void
  set_depth(int d)
  {
    m_depth = d;
  }

  xml::reader_sptr
  get_reader() const
  {
    return m_reader;
  }

  shared_ptr<type_base>
  get_type_decl(const string& id) const
  {
    const_types_map_it i = m_types_map.find(id);
    if (i == m_types_map.end())
      return shared_ptr<type_base>();
    return shared_ptr<type_base>(i->second);
  }

  shared_ptr<scope_decl>
  get_cur_scope() const
  {
    return m_cur_scope;
  }

  void
  set_cur_scope(shared_ptr<scope_decl> s)
  {
    m_cur_scope = s;
  }

  shared_ptr<decl_base>
  get_cur_decl() const
  {
    if (m_decls_stack.empty())
      return shared_ptr<decl_base>(static_cast<decl_base*>(0));

    return m_decls_stack.top();
  }

  void
  push_decl(shared_ptr<decl_base> d)
  {
    m_decls_stack.push(d);
  }

  shared_ptr<decl_base>
  pop_decl()
  {
    if (m_decls_stack.empty())
      return shared_ptr<decl_base>(static_cast<decl_base*>(0));

    shared_ptr<decl_base> t = get_cur_decl();
    m_decls_stack.pop();
    return t;
  }

  bool
  add_type_decl(const string&         id,
		shared_ptr<type_base> type)
  {
    const_types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end())
      return false;

    m_types_map[id] = type;
    return true;
  }

private:
  // The depth of the current node in the xml tree.
  int m_depth;
  unordered_map<string, shared_ptr<type_base> > m_types_map;
  xml::reader_sptr m_reader;
  stack<shared_ptr<decl_base> > m_decls_stack;
  shared_ptr<scope_decl> m_cur_scope;
};//end class read_context

static void update_read_context(read_context&);
static int advance_cursor(read_context&);
static bool read_input(read_context&, abi_corpus&);
static bool read_location(read_context&, abi_corpus& , location&);
static bool handle_element(read_context&, abi_corpus&);
static bool handle_type_decl(read_context&, abi_corpus&);
static bool handle_namespace_decl(read_context&, abi_corpus&);

bool
read_file(const string&	file_path,
	  abi_corpus&	corpus)
{
  read_context read_ctxt(xml::new_reader_from_file(file_path));

  return read_input(read_ctxt, corpus);
}

// Updates the instance of read_context.  This function needs to be
// called after each call to xmlTextReaderReader.
static void
update_read_context(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();

  // Update the depth of the current reader cursor in the reader
  // context.
  int depth = xmlTextReaderDepth(reader.get()),
    ctxt_depth = ctxt.get_depth();

  if (depth > ctxt_depth)
    {
      // we went down the tree.
      shared_ptr<decl_base> cur_decl = ctxt.get_cur_decl();
      if (dynamic_cast<scope_decl*>(cur_decl.get()))
	ctxt.set_cur_scope(dynamic_pointer_cast<scope_decl>(cur_decl));
    }
  else if (depth < ctxt_depth)
    {
      // we went up the tree.
      for (int nb = ctxt_depth - depth; nb; --nb)
	ctxt.pop_decl();
      shared_ptr<decl_base> cur_decl = ctxt.get_cur_decl();
      if (dynamic_cast<scope_decl*> (cur_decl.get()))
	ctxt.set_cur_scope(dynamic_pointer_cast<scope_decl>(cur_decl));
      else if (cur_decl)
	ctxt.set_cur_scope(cur_decl->get_scope());
    }

  ctxt.set_depth(depth);
}

/// Moves the xmlTextReader cursor to the next xml node in the input
/// document.  Return 1 of the parsing was successful, 0 if no input
/// xml token is left, or -1 in case of error.
///
/// \param ctxt the read context
///
static int
advance_cursor(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();
  int status = xmlTextReaderRead(reader.get());
  if (status == 1)
    update_read_context(ctxt);

  return status;
}

/// Parse the input xml document associated to the current context.
///
/// \param ctxt the current input context
///
/// \param corpus the result of the parsing.
///
/// \return true upon successufl parsing, false otherwise.
static bool
read_input(read_context& ctxt,
	   abi_corpus&   corpus)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  // The document must start with the abi-instr node.
  int status = advance_cursor (ctxt);
  if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				   BAD_CAST("abi-instr")))
    return false;

  for (status = advance_cursor(ctxt);
       status == 1;
       status = advance_cursor(ctxt))
    {
      xmlReaderTypes node_type = XML_READER_GET_NODE_TYPE(reader);

      switch (node_type)
	{
	case XML_READER_TYPE_ELEMENT:
	  if (!handle_element(ctxt, corpus))
	    return false;
	  break;
	default:
	  break;
	}
    }

  if(status == 0)
    return true;
  return false;
}

/// This function is called by #read_input.  It handles the current
/// xml element node of the reading context.  The result of the
/// "handling" is to build the representation of the xml node and tied
/// it to the corpus.
///
/// \param ctxt the current parsing context.
///
/// \param corpus the resulting ABI Corpus.
///
/// \return true upon successful completion, false otherwise.
static bool
handle_element(read_context&	ctxt,
	       abi_corpus&	corpus)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("namespace-decl")))
    return handle_namespace_decl(ctxt, corpus);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("type-decl")))
    return handle_type_decl(ctxt, corpus);

  return false;
}

/// Parses location attributes on the current xml element node.
///
///\param ctxt the current parsing context
///
///\param loc the resulting location.
///
/// \return true upon sucessful parsing, false otherwise.
static bool
read_location(read_context& ctxt, abi_corpus& corpus, location& loc)
{
  xml::reader_sptr r = ctxt.get_reader();
  xml::xml_char_sptr f = XML_READER_GET_ATTRIBUTE(r, "filepath");
  if (!f)
    {
      loc = location();
      return true;
    }

  xml::xml_char_sptr l = XML_READER_GET_ATTRIBUTE(r, "line");
  xml::xml_char_sptr c = XML_READER_GET_ATTRIBUTE(r, "column");
  if (!l || !c)
    return false;

  loc = corpus.get_loc_mgr().create_new_location
    (reinterpret_cast<char*>(f.get()),
     atoi(reinterpret_cast<char*>(l.get())),
     atoi(reinterpret_cast<char*>(c.get())));
  return true;
}

/// Parses 'type-decl' xml element.
///
/// \param ctxt the parsing context.
///
/// \param corpus the ABI corpus the resulting in-memory
/// representation is added to.
///
/// \return true upon successful parsing, false otherwise.
static bool
handle_type_decl(read_context& ctxt,
		 abi_corpus&   corpus)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  string name = CHAR_STR(XML_READER_GET_ATTRIBUTE(reader, "name"));
  string id = CHAR_STR(XML_READER_GET_ATTRIBUTE(reader, "id"));
  size_t size_in_bits=
    atoi(CHAR_STR(XML_READER_GET_ATTRIBUTE(reader, "size-in-bits")));
  size_t alignment_in_bits =
    atoi(CHAR_STR (XML_READER_GET_ATTRIBUTE(reader, "alignment-in-bits")));
  location loc;
  read_location(ctxt, corpus, loc);

  if (ctxt.get_type_decl(id))
    // Hugh?  How come a type which ID is supposed to be unique exist
    // already?  Bail out!
    return false;

  shared_ptr<decl_base> decl(new type_decl(name, size_in_bits,
					   alignment_in_bits,
					   ctxt.get_cur_scope(),
					   loc));

  // If This decl is at global scope, then it needs to be added to the
  // corpus.  If it's not at global scope, then it's scope is (maybe
  // indirectely) in the corpus already.
  if (!decl->get_scope())
    corpus.add(decl);

  ctxt.push_decl(decl);

  return true;
}

/// Parses 'namespace-decl' xml element.
///
/// \param ctxt the parsing context.
///
/// \param corpus the ABI corpus the resulting in-memory
/// representation is added to.
///
/// \return true upon successful parsing, false otherwise.
static bool
handle_namespace_decl(read_context& ctxt, abi_corpus& corpus)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;
  if (ctxt.get_cur_scope()
      && !dynamic_cast<namespace_decl*>(ctxt.get_cur_scope().get()))
    return false;

  string name = CHAR_STR(XML_READER_GET_ATTRIBUTE(r, "name"));
  location loc;
  read_location(ctxt, corpus, loc);

  shared_ptr<namespace_decl> scope(dynamic_pointer_cast<namespace_decl>
				   (ctxt.get_cur_scope()));
  shared_ptr<decl_base> decl(new namespace_decl(name, scope, loc));

  // If This decl is at global scope, then it needs to be added to the
  // corpus.  If it's not at global scope, then it's scope is (maybe
  // indirectely) in the corpus already.
  if (!decl->get_scope())
    corpus.add(decl);

  ctxt.push_decl(decl);

  return true;
}

}//end namespace reader
}//end namespace abigail
