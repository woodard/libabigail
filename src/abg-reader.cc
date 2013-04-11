// -*- mode: C++ -*-
/// @file

#include <cstring>
#include <cstdlib>
#include <tr1/unordered_map>
#include <stack>
#include <assert.h>
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

using xml::xml_char_sptr;

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

  /// Return the current lexical scope.  For this function to return a
  /// sane result, the path to the current decl element (starting from the
  /// root element) must be up to date.  It is updated by a call to
  /// #update_read_context.
  scope_decl*
  get_cur_scope()
  {
    shared_ptr<decl_base> cur_decl = get_cur_decl();

    if (dynamic_cast<scope_decl*>(cur_decl.get()))
      // The current decl is a scope_decl, so it's our lexical scope.
      return dynamic_pointer_cast<scope_decl>(cur_decl).get();
    else if (cur_decl)
      // The current decl is not a scope_decl, so our lexical scope is
      // the scope of this decl.
      return cur_decl->get_scope();
    else
      // We have no scope set.
      return 0;
  }

  shared_ptr<decl_base>
  get_cur_decl() const
  {
    if (m_decls_stack.empty())
      return shared_ptr<decl_base>(static_cast<decl_base*>(0));

    return m_decls_stack.top();
  }

  translation_unit*
  get_translation_unit()
  {
    global_scope* global = 0;
    if (shared_ptr<decl_base> d = get_cur_decl ())
      global = get_global_scope(d);

    if (global)
      return global->get_translation_unit();

    return 0;
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
    assert(type);

    const_types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end())
      return false;

    m_types_map[id] = type;
    return true;
  }

  /// This function must be called on each decl that is created during
  /// the parsing.  It adds the decl to the current scope, and updates
  /// the state of the parsing context accordingly.
  ///
  /// \param decl the newly created decl.
  void
  finish_decl_creation(shared_ptr<decl_base>	decl)
  {
    add_decl_to_scope(decl, get_cur_scope());
    push_decl(decl);
  }

  /// This function must be called on each type decl that is created
  /// during the parsing.  It adds the decl to the current scope and
  /// updates the state of the parsing context accordingly.
  ///
  /// \param decl the newly created decl.
  ///
  bool
  finish_type_decl_creation(shared_ptr<type_base> t,
			    const string& id)
  {
    shared_ptr<decl_base> decl = dynamic_pointer_cast<decl_base>(t);
    if (!decl)
      return false;

    finish_decl_creation(decl);
    add_type_decl(id, t);
    return true;
  }

private:
  // The depth of the current node in the xml tree.
  int m_depth;
  unordered_map<string, shared_ptr<type_base> > m_types_map;
  xml::reader_sptr m_reader;
  stack<shared_ptr<decl_base> > m_decls_stack;
};//end class read_context

static void	update_read_context(read_context&);
static int	advance_cursor(read_context&);
static bool	read_input(read_context&, translation_unit&);
static bool	read_location(read_context&, location&);
static bool	read_location(read_context&, xmlNodePtr, location&);
static bool	read_visibility(read_context&, decl_base::visibility&);
static shared_ptr<function_decl::parameter>
build_function_parameter (read_context&, const xmlNodePtr);
static bool	read_binding(read_context&, decl_base::binding&);
static bool	handle_element(read_context&);
static bool	handle_type_decl(read_context&);
static bool	handle_namespace_decl(read_context&);
static bool	handle_qualified_type_decl(read_context&);
static bool	handle_pointer_type_def(read_context&);
static bool	handle_reference_type_def(read_context&);
static bool	handle_enum_type_decl(read_context&);
static bool	handle_typedef_decl(read_context&);
static bool	handle_var_decl(read_context&);
static bool	handle_function_decl(read_context&);

bool
read_file(const string&	file_path,
	  translation_unit&	tu)
{
  read_context read_ctxt(xml::new_reader_from_file(file_path));

  return read_input(read_ctxt, tu);
}

/// Updates the instance of read_context.  Basically update thee path
/// of elements from the root to the current element, that we maintain
/// to know the current scope.  This function needs to be called after
/// each call to xmlTextReaderRead.
static void
update_read_context(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();

  if (XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    return;

  // Update the depth of the current reader cursor in the reader
  // context.
  int depth = xmlTextReaderDepth(reader.get()),
    ctxt_depth = ctxt.get_depth();

  if (depth > ctxt_depth)
    // we went down the tree.  There is nothing to do until we
    // actually parse the new element.
    ;
  else if (depth <= ctxt_depth)
    {
      // we went up the tree or went to a sibbling
      for (int nb = ctxt_depth - depth + 1; nb; --nb)
	ctxt.pop_decl();
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
/// \param tu the translation unit resulting from the parsing.
///
/// \return true upon successful parsing, false otherwise.
static bool
read_input(read_context&	ctxt,
	   translation_unit&	tu)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  // The document must start with the abi-instr node.
  int status = advance_cursor (ctxt);
  if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				   BAD_CAST("abi-instr")))
    return false;

  // We are at global scope, as we've just seen the top-most
  // "abi-instr" element.
  ctxt.push_decl(tu.get_global_scope());

  for (status = advance_cursor(ctxt);
       (status == 1
	// There must be at least one decl pushed in the context
	// during the parsing.
	&& ctxt.get_cur_decl());
       status = advance_cursor(ctxt))
    {
      xmlReaderTypes node_type = XML_READER_GET_NODE_TYPE(reader);

      switch (node_type)
	{
	case XML_READER_TYPE_ELEMENT:
	  if (!handle_element(ctxt))
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
/// it to the current translation unit.
///
/// \param ctxt the current parsing context.
///
/// \return true upon successful completion, false otherwise.
static bool
handle_element(read_context&	ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("namespace-decl")))
    return handle_namespace_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("type-decl")))
    return handle_type_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("qualified-type-def")))
    return handle_qualified_type_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("pointer-type-def")))
    return handle_pointer_type_def(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("reference-type-def")))
    return handle_reference_type_def(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("enum-decl")))
    return handle_enum_type_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("typedef-decl")))
    return handle_typedef_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("var-decl")))
    return handle_var_decl(ctxt);
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("function-decl")))
    return handle_function_decl(ctxt);

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
read_location(read_context& ctxt, location& loc)
{
  translation_unit& tu = *ctxt.get_translation_unit();

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

  loc = tu.get_loc_mgr().create_new_location
    (reinterpret_cast<char*>(f.get()),
     atoi(reinterpret_cast<char*>(l.get())),
     atoi(reinterpret_cast<char*>(c.get())));
  return true;
}

/// Parses location attributes on an xmlNodePtr.
///
///\param ctxt the current parsing context
///
///\param loc the resulting location.
///
/// \return true upon sucessful parsing, false otherwise.
static bool
read_location(read_context&	ctxt,
	      xmlNodePtr	node,
	      location&	loc)
{
  string file_path;
  size_t line = 0, column = 0;

  if (xml_char_sptr f = xml::build_sptr(xmlGetProp(node, BAD_CAST("filepath"))))
    file_path = CHAR_STR(f);

  if (file_path.empty())
    return false;

  if (xml_char_sptr l = xml::build_sptr(xmlGetProp(node, BAD_CAST("line"))))
    line = atoi(CHAR_STR(l));

  if (xml_char_sptr c = xml::build_sptr(xmlGetProp(node, BAD_CAST("column"))))
    column = atoi(CHAR_STR(c));

  loc =
    ctxt.get_translation_unit()->get_loc_mgr().create_new_location(file_path,
								   line,
								   column);
  return true;
}

/// Parse the visibility attribute.
///
/// \param ctxt the read context to use for the parsing.
///
/// \param vis the resulting visibility.
///
/// \return true upon successful completion, false otherwise.
static bool
read_visibility(read_context&		ctxt,
		decl_base::visibility&	vis)
{
  xml::reader_sptr r = ctxt.get_reader();

  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "visibility"))
    {
      string v = CHAR_STR(s);

      if (v == "default")
	vis = decl_base::VISIBILITY_DEFAULT;
      else if (v == "hidden")
	vis = decl_base::VISIBILITY_HIDDEN;
      else if (v == "internal")
	vis = decl_base::VISIBILITY_INTERNAL;
      else if (v == "protected")
	vis = decl_base::VISIBILITY_PROTECTED;
      else
	vis = decl_base::VISIBILITY_DEFAULT;
      return true;
    }
  return false;
}

/// Parse the "binding" attribute on the current element.
///
/// \param ctxt the context to use for the parsing.
///
/// \param bind the resulting binding attribute.
///
/// \return true upon successful completion, false otherwise.
static bool
read_binding(read_context&		ctxt,
	     decl_base::binding&	bind)
{
  xml::reader_sptr r = ctxt.get_reader();

  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "binding"))
    {
      string b = CHAR_STR(s);

      if (b == "global")
	bind = decl_base::BINDING_GLOBAL;
      if (b == "local")
	bind = decl_base::BINDING_LOCAL;
      if (b == "weak")
	bind = decl_base::BINDING_WEAK;
      else
	bind = decl_base::BINDING_GLOBAL;
      return true;
    }

  return false;
}

/// Parses 'type-decl' xml element.
///
/// \param ctxt the parsing context.
///
/// \return true upon successful parsing, false otherwise.
static bool
handle_type_decl(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(reader, "name"))
    name = CHAR_STR(s);
  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(reader, "id"))
    id = CHAR_STR(s);

  size_t size_in_bits= 0;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(reader, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));

  size_t alignment_in_bits = 0;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(reader, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  location loc;
  read_location(ctxt, loc);

  if (ctxt.get_type_decl(id))
    // Hugh?  How come a type which ID is supposed to be unique exist
    // already?  Bail out!
    return false;

  shared_ptr<type_base> decl(new type_decl(name, size_in_bits,
					   alignment_in_bits,
					   loc));
  return ctxt.finish_type_decl_creation(decl, id);
}

/// Parses 'namespace-decl' xml element.
///
/// \param ctxt the parsing context.
///
/// \return true upon successful parsing, false otherwise.
static bool
handle_namespace_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  /// If we are not at global scope, then the current scope must
  /// itself be a namespace.
  if (!is_global_scope(ctxt.get_cur_scope())
      && !dynamic_cast<namespace_decl*>(ctxt.get_cur_scope()))
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "name"))
      name = CHAR_STR(s);

  location loc;
  read_location(ctxt, loc);

  shared_ptr<decl_base> decl(new namespace_decl(name, loc));
  ctxt.finish_decl_creation(decl);
  return true;
}

/// Parse a qualified-type-def xml element.
///
/// \param ctxt the parsing context.
///
/// \return true upon successful parsing, false otherwise.
static bool
handle_qualified_type_decl(read_context& ctxt)
{
 xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string type_id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(type_id);
  if (!underlying_type)
    // The type-id must point to a pre-existing type.
    return false;

  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE (r, "id"))
    id = CHAR_STR(s);

  if (id.empty() || ctxt.get_type_decl(id))
    // We should have an id and it should be a new one.
    return false;

  string const_str;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "const"))
    const_str = CHAR_STR(s);
  bool const_cv = const_str == "yes" ? true : false;

  string volatile_str;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "volatile"))
    volatile_str = CHAR_STR(s);
  bool volatile_cv = volatile_str == "yes" ? true : false;

  qualified_type_def::CV cv = qualified_type_def::CV_NONE;
  if (const_cv)
    cv =
      static_cast<qualified_type_def::CV>(cv | qualified_type_def::CV_CONST);
  if (volatile_cv)
    cv =
      static_cast<qualified_type_def::CV>(cv | qualified_type_def::CV_VOLATILE);

  location loc;
  read_location(ctxt, loc);

  shared_ptr<type_base> decl(new qualified_type_def(underlying_type,
						    cv, loc));
  return ctxt.finish_type_decl_creation(decl, id);
}

/// Parse a pointer-type-decl element.
///
/// \param ctxt the context of the parsing.
///
/// \return true upon successful completion, false otherwise.
static bool
handle_pointer_type_def(read_context& ctxt)
{
   xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string type_id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type = ctxt.get_type_decl(type_id);
  if (!pointed_to_type)
    return false;

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return false;

  location loc;
  read_location(ctxt, loc);

  shared_ptr<type_base> t(new pointer_type_def(pointed_to_type,
					       size_in_bits,
					       alignment_in_bits,
					       loc));
  return ctxt.finish_type_decl_creation(t, id);
}

/// Parse a reference-type-def element.
///
/// \param ctxt the context of the parsing.
///
/// reference_type_def is added to.
static bool
handle_reference_type_def(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string kind;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "kind"))
    kind = CHAR_STR(s); // this should be either "lvalue" or "rvalue".
  bool is_lvalue = kind == "lvalue" ? true : false;

  string type_id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type = ctxt.get_type_decl(type_id);
  if (!pointed_to_type)
    return false;

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return false;

  location loc;
  read_location(ctxt, loc);

  shared_ptr<type_base> t(new reference_type_def(pointed_to_type,
						 is_lvalue,
						 size_in_bits,
						 alignment_in_bits,
						 loc));
  return ctxt.finish_type_decl_creation(t, id);
}

/// Parse an enum-decl element.
///
/// \param ctxt the context of the parsing.
static bool
handle_enum_type_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "name"))
    name = CHAR_STR(s);

  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "id"))
    id = CHAR_STR(s);

  if (id.empty() || ctxt.get_type_decl(id))
    return false;

  string base_type_id;
  xmlNodePtr node = xmlTextReaderExpand(r.get());
  std::list<enum_type_decl::enumerator> enumerators;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("underlying-type")))
	{
	  xml_char_sptr a = xml::build_sptr(xmlGetProp(n, BAD_CAST("type-id")));
	  if (a)
	    base_type_id = CHAR_STR(a);
	  continue;
	}

      if (xmlStrEqual(n->name, BAD_CAST("enumerator")))
	{
	  string name;
	  size_t value = 0;

	  xml_char_sptr a = xml::build_sptr(xmlGetProp(n, BAD_CAST("name")));
	  if (a)
	    name = CHAR_STR(a);

	  a = xml::build_sptr(xmlGetProp(n, BAD_CAST("value")));
	  if (a)
	    value = atoi(CHAR_STR(a));

	  enumerators.push_back(enum_type_decl::enumerator(name, value));
	}
    }

  // now advance the xml reader cursor to the xml node after this
  // expanded 'enum-decl' node.
  xmlTextReaderNext(r.get());

  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(base_type_id);
  if (!underlying_type)
    return false;

  location loc;
  read_location(ctxt, loc);
  shared_ptr<type_base> t(new enum_type_decl(name, loc,
					     underlying_type,
					     enumerators));
  return ctxt.finish_type_decl_creation(t, id);
}

/// Parse a typedef-decl element.
///
/// \param ctxt the context of the parsing.
static bool
handle_typedef_decl(read_context& ctxt)
{
    xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "name"))
    name = CHAR_STR(s);

  string type_id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type(ctxt.get_type_decl(type_id));
  if (!underlying_type)
    return false;

  string id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return false;

  location loc;
  read_location(ctxt, loc);

  shared_ptr<type_base> t(new typedef_decl(name, underlying_type, loc));

  return ctxt.finish_type_decl_creation(t, id);
}

/// Parse a var-decl element.
///
/// \param ctxt the context of the parsing.
static bool
handle_var_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "name"))
    name = CHAR_STR(s);

  string type_id;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(type_id);
  if (!underlying_type)
    return false;

  string mangled_name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "mangled-name"))
    mangled_name = CHAR_STR(s);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(ctxt, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(ctxt, bind);

  location locus;
  read_location(ctxt, locus);

  shared_ptr<decl_base> decl(new var_decl(name, underlying_type,
					  locus, mangled_name,
					  vis, bind));
  ctxt.finish_decl_creation(decl);

  return true;
}

/// Build function parameter from an 'parameter' xml element node.
///
/// \param ctxt the contexte of the xml parsing.
///
/// \param node the xml 'parameter' element node to de-serialize from.
static shared_ptr<function_decl::parameter>
build_function_parameter (read_context& ctxt, const xmlNodePtr node)
{
  string type_id;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("type-id"))))
    type_id = CHAR_STR(a);

  shared_ptr<type_base> type = ctxt.get_type_decl(type_id);
  if (!type)
    return shared_ptr<function_decl::parameter>((function_decl::parameter*)0);

  string name;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("name"))))
    name = CHAR_STR(a);

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<function_decl::parameter> p
    (new function_decl::parameter(type, name, loc));

  return p;
}

/// Parse a function-decl element.
///
/// \param ctxt the context of the parsing
///
/// \return true upon successful completion of the parsing, false
/// otherwise.
static bool
handle_function_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  string name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "name"))
    name = CHAR_STR(s);

  string mangled_name;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "mangled-name"))
    mangled_name = CHAR_STR(s);

  string inline_prop;
  if (xml_char_sptr s = XML_READER_GET_ATTRIBUTE(r, "declared-inline"))
    inline_prop = CHAR_STR(s);
  bool declared_inline = inline_prop == "yes" ? true : false;

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(ctxt, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(ctxt, bind);

  location loc;
  read_location(ctxt, loc);

  xmlNodePtr node = xmlTextReaderExpand(r.get());
  std::list<shared_ptr<function_decl::parameter> > parms;
  shared_ptr<type_base> return_type;
  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("parameter")))
	{
	  if (shared_ptr<function_decl::parameter> p =
	      build_function_parameter(ctxt, n))
	    parms.push_back(p);
	}
      else if (xmlStrEqual(n->name, BAD_CAST("return")))
	{
	  string type_id;
	  if (xml_char_sptr s =
	      xml::build_sptr(xmlGetProp(n, BAD_CAST("type-id"))))
	    type_id = CHAR_STR(s);
	  if (!type_id.empty())
	    return_type = ctxt.get_type_decl(type_id);
	}
    }
  // now advance the xml reader cursor to the xml node after this
  // expanded 'enum-decl' node.
  xmlTextReaderNext(r.get());

  shared_ptr<decl_base> decl(new function_decl(name, parms, return_type,
					       declared_inline, loc,
					       mangled_name, vis));
  ctxt.finish_decl_creation(decl);

  return true;
}

}//end namespace reader
}//end namespace abigail
