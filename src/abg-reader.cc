// -*- mode: C++ -*-

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

class read_context;

static void	update_read_context(read_context&);
static void	update_read_context(read_context&, xmlNodePtr);
static void	update_depth_info_of_read_context(read_context&, int);

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

  typedef unordered_map<string,
			shared_ptr<function_template_decl> >::const_iterator
  const_fn_tmpl_map_it;

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

  /// Return the type that is identified by a unique ID.  Note that
  /// for a type to be "identified" by #id, the function key_type_decl
  /// must have been previously called with that type and with #id.
  ///
  /// \param id the unique id to consider.
  ///
  /// \return the type identified by the unique id #id, or a null
  /// pointer if no type has ever been associated with id before.
  shared_ptr<type_base>
  get_type_decl(const string& id) const
  {
    const_types_map_it i = m_types_map.find(id);
    if (i == m_types_map.end())
      return shared_ptr<type_base>();
    return shared_ptr<type_base>(i->second);
  }

  /// Return the function template that is identified by a unique ID.
  /// Note that a function template to be indentified by #id, the
  /// function key_fn_tmpl_decl must have been previously called
  /// with that function template and with #id.
  ///
  /// \param id the ID to consider.
  ///
  /// \return the function template identified by #id, or a null
  /// pointer if no function template has ever been associated with
  /// #id before.
  shared_ptr<function_template_decl>
  get_fn_tmpl_decl(const string& id) const
  {
    const_fn_tmpl_map_it i = m_fn_tmpl_map.find(id);
    if (i == m_fn_tmpl_map.end())
      return shared_ptr<function_template_decl>();
    return i->second;
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

  /// Associate an ID with a type.
  ///
  /// \param type the type to associate witht he ID.
  ///
  /// \param the ID to associate to the type.
  ///
  /// \return true upon successful completion, false otherwise.  Note
  /// that this returns false if the was already associate to an ID
  /// before.
  bool
  key_type_decl(shared_ptr<type_base> type,
		const string&         id)
  {
    assert(type);

    const_types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end())
      return false;

    m_types_map[id] = type;
    return true;
  }

  /// Associate an ID to a function template.
  ///
  /// \param fn_tmpl_decl the function template to consider.
  ///
  /// \param id the ID to associate to the function template.
  ///
  /// \return true upon successful completion, false otherwise.  Note
  /// that the function returns false if an ID was previously
  /// associated to the function template.
  bool
  key_fn_tmpl_decl(shared_ptr<function_template_decl>	fn_tmpl_decl,
		   const string&			id)
  {
    assert(fn_tmpl_decl);

    const_fn_tmpl_map_it i = m_fn_tmpl_map.find(id);
    if (i != m_fn_tmpl_map.end())
      return false;

    m_fn_tmpl_map[id] = fn_tmpl_decl;
    return true;
  }

  /// This function must be called on each decl that is created during
  /// the parsing.  It adds the decl to the current scope, and updates
  /// the state of the parsing context accordingly.
  ///
  /// \param decl the newly created decl.
  void
  push_decl_to_current_scope(shared_ptr<decl_base>	decl)
  {
    assert(decl);

    add_decl_to_scope(decl, get_cur_scope());
    push_decl(decl);
  }

  /// This function must be called on each decl that is created during
  /// the parsing.  It adds the decl to the current scope, and updates
  /// the state of the parsing context accordingly.
  ///
  /// \param the xml node from which the decl has been created if any.
  ///
  /// \param update_depth_info should be set to true if the function
  /// should update the depth information maintained in the parsing
  /// context.  If the xml element node has been 'hit' by the
  /// advence_cursor then this should be set to false, because that
  /// function updates the depth information maintained in the parsing
  /// context already.
  ///
  /// \param decl the newly created decl.
 void
 push_decl_to_current_scope(shared_ptr<decl_base>	decl,
			    xmlNodePtr			node,
			    bool			update_depth_info)
  {
    assert(decl);

    if (update_depth_info)
      update_read_context(*this, node);

    push_decl_to_current_scope(decl);
  }

  /// This function must be called on each type decl that is created
  /// during the parsing.  It adds the type decl to the current scope
  /// and associates a unique ID to it.
  ///
  /// \param decl the newly created decl
  ///
  /// \param id the unique ID to be associated to #t
  ///
  /// \return true upon successful completion.
  ///
  bool
  push_and_key_type_decl(shared_ptr<type_base>	t,
			 const string&		id)
  {
    shared_ptr<decl_base> decl = dynamic_pointer_cast<decl_base>(t);
    if (!decl)
      return false;

    push_decl_to_current_scope(decl);
    key_type_decl(t, id);
    return true;
  }

  /// This function must be called on each type decl that is created
  /// during the parsing.  It adds the type decl to the current scope
  /// and associates a unique ID to it.
  ///
  /// \param t the type decl to consider.
  ///
  /// \param the ID to associate to it.
  ///
  /// \param node the xml elment node that #t was constructed from.
  ///
  /// \param update_read_context should be set to true if this
  /// function should update the depth information maintained in the
  /// parsing context.
  ///
  /// \return true upon successful completion, false otherwise.
  bool
  push_and_key_type_decl(shared_ptr<type_base>	t,
			 const string&		id,
			 xmlNodePtr		node,
			 bool			update_depth_info)
  {
    if (update_depth_info)
      update_read_context(*this, node);

    return push_and_key_type_decl(t, id);
  }

private:
  // The depth of the current node in the xml tree.
  int m_depth;
  unordered_map<string, shared_ptr<type_base> > m_types_map;
  unordered_map<string, shared_ptr<function_template_decl> > m_fn_tmpl_map;
  xml::reader_sptr m_reader;
  stack<shared_ptr<decl_base> > m_decls_stack;
};//end class read_context

static int	advance_cursor(read_context&);
static bool	read_input(read_context&, translation_unit&);
static bool	read_location(read_context&, location&);
static bool	read_location(read_context&, xmlNodePtr, location&);
static bool	read_visibility(read_context&, decl_base::visibility&);
static bool	read_visibility(xmlNodePtr, decl_base::visibility&);
static bool	read_binding(read_context&, decl_base::binding&);
static bool	read_binding(xmlNodePtr, decl_base::binding&);
static bool	read_access(xmlNodePtr, class_decl::access_specifier&);
static bool	read_size_and_alignment(xmlNodePtr, size_t&, size_t&);
static bool	read_static(xmlNodePtr, bool&);
static bool	read_var_offset_in_bits(xmlNodePtr, size_t&);
static bool	read_cdtor_const(xmlNodePtr, bool&, bool&, bool&);

// <build a c++ class  from an instance of xmlNodePtr>
//
// Note that whenever a new function to build a type is added here,
// you should make sure to call it from the build_type function, which
// should be the last function of the list of declarated function below.
static shared_ptr<function_decl::parameter>
build_function_parameter (read_context&, const xmlNodePtr);
static shared_ptr<function_decl>
build_function_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<var_decl>
build_var_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<type_decl>
build_type_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<qualified_type_def>
build_qualified_type_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<pointer_type_def>
build_pointer_type_def(read_context&, const xmlNodePtr, bool);
static shared_ptr<reference_type_def>
build_reference_type_def(read_context&, const xmlNodePtr, bool);
static shared_ptr<enum_type_decl>
build_enum_type_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<typedef_decl>
build_typedef_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<class_decl>
build_class_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<function_template_decl>
build_function_template_decl(read_context&, const xmlNodePtr, bool);
static shared_ptr<template_type_parameter>
build_template_type_parameter(read_context&, const xmlNodePtr, unsigned, bool);
static shared_ptr<tmpl_parm_type_composition>
build_tmpl_parm_type_composition(read_context&,
				 const xmlNodePtr,
				 unsigned, bool);
static shared_ptr<template_non_type_parameter>
build_template_non_type_parameter(read_context&, const xmlNodePtr,
				  unsigned, bool);
static shared_ptr<template_template_parameter>
build_template_template_parameter(read_context&, const xmlNodePtr,
				  unsigned, bool);
static shared_ptr<template_parameter>
build_template_parameter(read_context&, const xmlNodePtr,
			 unsigned, bool);

// Please make this build_type function be the last one of the list.
// Note that it should call each type-building function above.  So
// please make sure to update it accordingly, whenever a new
// type-building function is added here.
static shared_ptr<type_base>
build_type(read_context&, const xmlNodePtr, bool);
// </build a c++ class  from an instance of xmlNodePtr>

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
static bool	handle_class_decl(read_context&);
static bool	handle_function_template_decl(read_context&);
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
///
/// \param ctxt the context to update.
static void
update_read_context(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();

  if (XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    return;

  // Update the depth of the current reader cursor in the reader
  // context.
  int depth = xmlTextReaderDepth(reader.get());
  update_depth_info_of_read_context(ctxt, depth);
}

/// Updates the instance of read_context, from an instance of xmlNode.
/// Basically update thee path of elements from the root to the
/// current element, that we maintain to know the current scope.  This
/// function needs to be called each time a build_xxx builds an C++
/// from an xmlNodePtr.
static void
update_read_context(read_context& ctxt, xmlNodePtr node)
{
  if (node->type != XML_ELEMENT_NODE)
    return;

  int depth = xml::get_xml_node_depth(node);

  if (depth >= 0)
    update_depth_info_of_read_context(ctxt, depth);
}

/// Helper function used by update_read_context.
///
/// Updates the depth information maintained in the read_context.
/// Updates the stack of IR node we maintain to know our current
/// context.
static void
update_depth_info_of_read_context(read_context& ctxt, int new_depth)
{
  int ctxt_depth = ctxt.get_depth();

  if (new_depth > ctxt_depth)
    // we went down the tree.  There is nothing to do until we
    // actually parse the new element.
    ;
  else if (new_depth <= ctxt_depth)
    {
      // we went up the tree or went to a sibbling
      for (int nb = ctxt_depth - new_depth + 1; nb; --nb)
	{
	  shared_ptr<decl_base> d = ctxt.pop_decl();

	  /// OK, this is a hack needed because the libxml reader
	  /// interface doesn't provide us with a reliable way to know
	  /// when we read the end of an XML element.
	  if (is_at_class_scope(d) && nb > 2)
	    // This means we logically poped out at least a member of
	    // a class (i.e, during the xml parsing, we went up so
	    // that we got out of an e.g, member-type, data-member or
	    // member-function xml element.  The issue is that there
	    // are two nested XML elment in that case (e.g,
	    // data-member -> var-decl) to represent just one concrete
	    // c++ type (e.g, the var_decl that is in the class_decl
	    // scope).  So libxml reports that we should pop two *XML*
	    // elements, but we should only pop one *C++* instance
	    // from our stack.
	    nb--;
	}
    }

  ctxt.set_depth(new_depth);
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
  if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		   BAD_CAST("class-decl")))
    return handle_class_decl(ctxt);
  if (xmlStrEqual(XML_READER_GET_NODE_NAME(reader).get(),
		  BAD_CAST("function-template-decl")))
    return handle_function_template_decl(ctxt);

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

/// Parse the visibility attribute.
///
/// \param node the xml node to read from.
///
/// \param vis the resulting visibility.
///
/// \return true upon successful completion, false otherwise.
static bool
read_visibility(xmlNodePtr node, decl_base::visibility& vis)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "visibility"))
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

/// Parse the "binding" attribute on the current element.
///
/// \param ctxt the context to use for the parsing.
///
/// \param node the xml node to build parse the bind from.
///
/// \param bind the resulting binding attribute.
///
/// \return true upon successful completion, false otherwise.
static bool
read_binding(xmlNodePtr node, decl_base::binding& bind)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "binding"))
    {
      string b = CHAR_STR(s);

      if (b == "global")
	bind = decl_base::BINDING_GLOBAL;
      else if (b == "local")
	bind = decl_base::BINDING_LOCAL;
      else if (b == "weak")
	bind = decl_base::BINDING_WEAK;
      else
	bind = decl_base::BINDING_GLOBAL;
      return true;
    }

  return false;
}

/// Read the 'access' attribute on the current xml node.
///
/// \param node the xml node to consider.
///
/// \param the access attribute.  Set iff the function returns true.
///
/// \return true upon sucessful completion, false otherwise.
static bool
read_access(xmlNodePtr node, class_decl::access_specifier& access)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "access"))
    {
      string a = CHAR_STR(s);

      if (a == "private")
	access = class_decl::private_access;
      else if (a == "protected")
	access = class_decl::protected_access;
      else if (a == "public")
	access = class_decl::public_access;
      else
	access = class_decl::private_access;

      return true;
    }
  return false;
}

/// Parse 'size-in-bits' and 'alignment-in-bits' attributes of a given
/// xmlNodePtr reprensting an xml element.
///
/// \param node the xml element node to consider.
///
/// \param size_in_bits the resulting value for the 'size-in-bits'
/// attribute.  This set only if this function returns true and the if
/// the attribute was present on the xml element node.
///
/// \param align_in_bits the resulting value for the
/// 'alignment-in-bits' attribute.  This set only if this function
/// returns true and the if the attribute was present on the xml
/// element node.
///
/// \return true if either one of the two attributes above were set,
/// false otherwise.
static bool
read_size_and_alignment(xmlNodePtr node,
			size_t& size_in_bits,
			size_t& align_in_bits)
{

  bool got_something = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    {
      size_in_bits = atoi(CHAR_STR(s));
      got_something = true;
    }

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    {
      align_in_bits = atoi(CHAR_STR(s));
      got_something = true;
    }
  return got_something;
}

/// Parse the 'static' attribute of a given xml element node.
///
/// \param node the xml element node to consider.
///
/// \param is_static the resulting the parsing.  Is set if the
/// function returns true.
///
/// \return true if the xml element node has the 'static' attribute
/// set, false otherwise.
static bool
read_static(xmlNodePtr node, bool& is_static)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "static"))
    {
      string b = CHAR_STR(s);
      is_static = (b == "yes") ? true : false;
      return true;
    }
  return false;
}

/// Parse the 'layout-offset-in-bits' attribute of a given xml element node.
///
/// \param offset_in_bits set to true if the element node contains the
/// attribute.
///
/// \return true iff the xml element node contain$s the attribute.
static bool
read_var_offset_in_bits(xmlNodePtr	node,
			size_t&	offset_in_bits)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "layout-offset-in-bits"))
    {
      offset_in_bits = atoi(CHAR_STR(s));
      return true;
    }
  return false;
}

/// Parse the 'constructor', 'destructor' and 'const' attribute of a
/// given xml node.
///
/// \param is_constructor the resulting value of the parsing of the
/// 'constructor' attribute.  Is set if the xml node contains the
/// attribute and if the function returns true.
///
/// \param is_destructor the resulting value of the parsing of the
/// 'destructor' attribute.  Is set if the xml node contains the
/// attribute and if the function returns true.
///
/// \param is_const the resulting value of the parsing of the 'const'
/// attribute.  Is set if the xml node contains the attribute and if
/// the function returns true.
///
/// \return true if at least of the attributes above is set, false
/// otherwise.
///
/// Note that callers of this function should initialize
/// #is_constructor, is_destructor and is_const prior to passing them
/// to this function.
static bool
read_cdtor_const(xmlNodePtr	node,
		 bool&		is_constructor,
		 bool&		is_destructor,
		 bool&		is_const)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "constructor"))
    {
      string b = CHAR_STR(s);
      if (b == "yes")
	is_constructor = true;
      else
	is_constructor = false;

      return true;
    }

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "destructor"))
    {
      string b = CHAR_STR(s);
      if (b == "yes")
	is_destructor = true;
      else
	is_destructor = false;

      return true;
    }

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is_const"))
    {
      string b = CHAR_STR(s);
      if (b == "yes")
	is_const = true;
      else
	is_const = false;

      return true;
    }

  return false;
}

/// Build a function parameter from a 'parameter' xml element node.
///
/// \param ctxt the contexte of the xml parsing.
///
/// \param node the xml 'parameter' element node to de-serialize from.
static shared_ptr<function_decl::parameter>
build_function_parameter(read_context& ctxt, const xmlNodePtr node)
{
  shared_ptr<function_decl::parameter> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("parameter")))
    return nil;

  string type_id;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("type-id"))))
    type_id = CHAR_STR(a);

  shared_ptr<type_base> type = ctxt.get_type_decl(type_id);
  if (!type)
    return nil;

  string name;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("name"))))
    name = CHAR_STR(a);

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<function_decl::parameter> p
    (new function_decl::parameter(type, name, loc));

  return p;
}

/// Build a function_decl from a 'function-decl' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the function_decl from.
///
/// \return a pointer to a newly created function_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<function_decl>
build_function_decl(read_context&	ctxt,
		    const xmlNodePtr	node,
		    bool update_depth_info)
{
  shared_ptr<function_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("function-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  string mangled_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    mangled_name = CHAR_STR(s);

  string inline_prop;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "declared-inline"))
    inline_prop = CHAR_STR(s);
  bool declared_inline = inline_prop == "yes" ? true : false;

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  location loc;
  read_location(ctxt, node, loc);

  std::list<shared_ptr<function_decl::parameter> > parms;
  shared_ptr<type_base> return_type;

  shared_ptr<function_decl> fn_decl(new function_decl(name, parms, return_type,
						      declared_inline, loc,
						      mangled_name, vis));

  ctxt.push_decl_to_current_scope(fn_decl, node, update_depth_info);

  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("parameter")))
	{
	  if (shared_ptr<function_decl::parameter> p =
	      build_function_parameter(ctxt, n))
	    fn_decl->add_parameter(p);
	}
      else if (xmlStrEqual(n->name, BAD_CAST("return")))
	{
	  string type_id;
	  if (xml_char_sptr s =
	      xml::build_sptr(xmlGetProp(n, BAD_CAST("type-id"))))
	    type_id = CHAR_STR(s);
	  if (!type_id.empty())
	    fn_decl->set_return_type(ctxt.get_type_decl(type_id));
	}
    }

  return fn_decl;
}

/// Build pointer to var_decl from a 'var-decl' xml Node
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the var_decl from.
///
/// \return a pointer to a newly built var_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<var_decl>
build_var_decl(read_context& ctxt, const xmlNodePtr node,
	       bool update_depth_info)
{
  shared_ptr<var_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("var-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(type_id);
  if (!underlying_type)
    return nil;

  string mangled_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    mangled_name = CHAR_STR(s);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  location locus;
  read_location(ctxt, node, locus);

  shared_ptr<var_decl> decl(new var_decl(name, underlying_type,
					 locus, mangled_name,
					 vis, bind));

  ctxt.push_decl_to_current_scope(decl, node, update_depth_info);

  return decl;
}

/// Build a type_decl from a "type-decl" XML Node.
///
/// \param cxt the context of the parsing.
///
/// \param node the XML node to build the type_decl from.
///
/// \return a pointer to type_decl upon successful completion, a null
/// pointer otherwise.
static shared_ptr<type_decl>
build_type_decl(read_context&		ctxt,
		const xmlNodePtr	node,
		bool update_depth_info)
{
  shared_ptr<type_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("type-decl")))
    return shared_ptr<type_decl>((type_decl*)0);

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  size_t size_in_bits= 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));

  size_t alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  if (ctxt.get_type_decl(id))
    // Hugh?  How come a type which ID is supposed to be unique exist
    // already?  Bail out!
    return shared_ptr<type_decl>((type_decl*)0);

  shared_ptr<type_decl> decl(new type_decl(name, size_in_bits,
					   alignment_in_bits,
					   loc));
  if (ctxt.push_and_key_type_decl(decl, id, node, update_depth_info))
    return decl;

  return nil;
}

/// Build a qualified_type_def from a 'qualified-type-def' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the qualified_type_def from.
///
/// \return a pointer to a newly built qualified_type_def upon
/// successful completion, a null pointer otherwise.
static shared_ptr<qualified_type_def>
build_qualified_type_decl(read_context& ctxt,
			  const xmlNodePtr node,
			  bool update_depth_info)
{
  if (!xmlStrEqual(node->name, BAD_CAST("qualified-type-def")))
    return shared_ptr<qualified_type_def>((qualified_type_def*)0);

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(type_id);
  if (!underlying_type)
    // The type-id must point to a pre-existing type.
    return shared_ptr<qualified_type_def>((qualified_type_def*)0);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE (node, "id"))
    id = CHAR_STR(s);

  if (id.empty() || ctxt.get_type_decl(id))
    // We should have an id and it should be a new one.
    return shared_ptr<qualified_type_def>((qualified_type_def*)0);

  string const_str;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "const"))
    const_str = CHAR_STR(s);
  bool const_cv = const_str == "yes" ? true : false;

  string volatile_str;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "volatile"))
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
  read_location(ctxt, node, loc);

  shared_ptr<qualified_type_def> decl(new qualified_type_def(underlying_type,
							     cv, loc));
  if (ctxt.push_and_key_type_decl(decl, id, node, update_depth_info))
    return decl;

  return shared_ptr<qualified_type_def>((qualified_type_def*)0);
}

/// Build a pointer_type_def from a 'pointer-type-def' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the pointer_type_def from.
///
/// \return a pointer to a newly built pointer_type_def upon
/// successful completion, a null pointer otherwise.
static shared_ptr<pointer_type_def>
build_pointer_type_def(read_context&	ctxt,
		       const xmlNodePtr node,
		       bool update_depth_info)
{

  shared_ptr<pointer_type_def> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("pointer-type-def")))
    return nil;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type = ctxt.get_type_decl(type_id);
  if (!pointed_to_type)
    shared_ptr<pointer_type_def>((pointer_type_def*)0);

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<pointer_type_def> t(new pointer_type_def(pointed_to_type,
						      size_in_bits,
						      alignment_in_bits,
						      loc));
  if (ctxt.push_and_key_type_decl(t, id, node, update_depth_info))
    return t;

  return nil;
}

/// Build a reference_type_def from a pointer to 'reference-type-def'
/// xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the reference_type_def from.
///
/// \return a pointer to a newly built reference_type_def upon
/// successful completio, a null pointer otherwise.
static shared_ptr<reference_type_def>
build_reference_type_def(read_context&		ctxt,
			 const xmlNodePtr	node,
			 bool update_depth_info)
{
  shared_ptr<reference_type_def> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("reference-type-def")))
    return nil;

  string kind;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "kind"))
    kind = CHAR_STR(s); // this should be either "lvalue" or "rvalue".
  bool is_lvalue = kind == "lvalue" ? true : false;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type = ctxt.get_type_decl(type_id);
  if (!pointed_to_type)
    return nil;

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<reference_type_def> t(new reference_type_def(pointed_to_type,
							  is_lvalue,
							  size_in_bits,
							  alignment_in_bits,
							  loc));
  if (ctxt.push_and_key_type_decl(t, id, node, update_depth_info))
    return t;

  return nil;
}

/// Build an enum_type_decl from an 'enum-type-decl' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the enum_type_decl from.
///
/// \return a pointer to a newly built enum_type_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<enum_type_decl>
build_enum_type_decl(read_context&	ctxt,
		     const xmlNodePtr	node,
		     bool update_depth_info)
{
  shared_ptr<enum_type_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("enum-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  if (id.empty() || ctxt.get_type_decl(id))
    return nil;

  string base_type_id;
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

  shared_ptr<type_base> underlying_type = ctxt.get_type_decl(base_type_id);
  if (!underlying_type)
    return nil;

  location loc;
  read_location(ctxt, node, loc);
  shared_ptr<enum_type_decl> t(new enum_type_decl(name, loc,
						  underlying_type,
						  enumerators));
  if (ctxt.push_and_key_type_decl(t, id, node, update_depth_info))
    return t;

  return nil;
}

/// Build a typedef_decl from a 'typedef-decl' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the typedef_decl from.
///
/// \return a pointer to a newly built typedef_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<typedef_decl>
build_typedef_decl(read_context&	ctxt,
		   const xmlNodePtr	node,
		   bool update_depth_info)
{
  shared_ptr<typedef_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("typedef-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type(ctxt.get_type_decl(type_id));
  if (!underlying_type)
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_type_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<typedef_decl> t(new typedef_decl(name, underlying_type, loc));

  if (ctxt.push_and_key_type_decl(t, id, node, update_depth_info))
    return t;

  return nil;
}

/// Build a class_decl from a 'class-decl' xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the class_decl from.
///
/// \return a pointer to class_decl upon successful completion, a null
/// pointer otherwise.
static shared_ptr<class_decl>
build_class_decl(read_context&		ctxt,
		 const xmlNodePtr	node,
		 bool update_depth_info)
{
  shared_ptr<class_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("class-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  size_t size_in_bits = 0, alignment_in_bits = 0;
  read_size_and_alignment(node, size_in_bits, alignment_in_bits);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  if (id.empty() || ctxt.get_type_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  std::list<shared_ptr<class_decl::member_type> > member_types;
  std::list<shared_ptr<class_decl::data_member> > data_members;
  std::list<shared_ptr<class_decl::member_function> > member_functions;
  std::list<shared_ptr<class_decl::base_spec> > bases;

  shared_ptr<class_decl> decl(new class_decl(name, size_in_bits,
					     alignment_in_bits,
					     loc, vis, bases,
					     member_types,
					     data_members,
					     member_functions));

  ctxt.push_decl_to_current_scope(decl, node, update_depth_info);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("base-class")))
	{
	  class_decl::access_specifier access = class_decl::private_access;
	  read_access(n, access);

	  string type_id;
	  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "type-id"))
	    type_id = CHAR_STR(s);
	  shared_ptr<class_decl> b =
	    dynamic_pointer_cast<class_decl>(ctxt.get_type_decl(type_id));
	  if (!b)
	    return nil;
	  shared_ptr<class_decl::base_spec> base
	    (new class_decl::base_spec(b, access));
	  decl->add_base_specifier(base);
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-type")))
	{
	  class_decl::access_specifier access = class_decl::private_access;
	  read_access(n, access);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<type_base> t =
		  build_type(ctxt, p, /*update_depth_info=*/true))
		{
		  shared_ptr<class_decl::member_type> m
		    (new class_decl::member_type(t, access));

		  decl->add_member_type(m);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("data-member")))
	{
	  class_decl::access_specifier access = class_decl::private_access;
	  read_access(n, access);

	  bool is_laid_out = false;
	  size_t offset_in_bits = 0;
	  if (read_var_offset_in_bits(n, offset_in_bits))
	    is_laid_out = true;

	  bool is_static = false;
	  read_static(n, is_static);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<var_decl> v =
		  build_var_decl(ctxt, p, /*update_depth_info=*/true))
		{
		  shared_ptr<class_decl::data_member> m
		    (new class_decl::data_member(v, access,
						 is_laid_out,
						 is_static,
						 offset_in_bits));

		  decl->add_data_member(m);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-function")))
	{
	  class_decl::access_specifier access = class_decl::private_access;
	  read_access(n, access);

	  size_t vtable_offset = 0;
	  if (xml_char_sptr s =
	      XML_NODE_GET_ATTRIBUTE(n, "vtable-offset-in-bits"))
	    vtable_offset = atoi(CHAR_STR(s));

	  bool is_static = false;
	  read_static(n, is_static);

	  bool is_ctor = false, is_dtor = false, is_const = false;
	  read_cdtor_const(n, is_ctor, is_dtor, is_const);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<function_decl> f =
		  build_function_decl(ctxt, p, /*update_depth_info=*/true))
		{
		  shared_ptr<class_decl::member_function> m
		    (new class_decl::member_function(f, access,
						     vtable_offset,
						     is_static,
						     is_ctor, is_dtor,
						     is_const));

		  decl->add_member_function(m);
		}
	    }
	}
    }

  if (decl)
    ctxt.key_type_decl(decl, id);

  return decl;
}

/// Build an intance of #function_template_decl, from an
/// 'function-template-decl' xml element node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to parse from.
///
/// \param update_depth_info this must be set to false, if we reached
/// this xml node by calling the xmlTextReaderRead function.  In that
/// case, build_function_decl doesn't have to update the depth
/// information that is maintained in the context of the parsing.
/// Otherwise if this node if just a child grand child of a node that
/// we reached using xmlTextReaderRead, of if it wasn't reached via
/// xmlTextReaderRead at all,then the argument to this parameter
/// should be true.  In that case this function will update the depth
/// information that is maintained by in the context of the parsing.
///
/// \return the newly built function_template_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<function_template_decl>
build_function_template_decl(read_context& ctxt,
			     const xmlNodePtr node,
			     bool update_depth_info)
{
  shared_ptr<function_template_decl> nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("function-template-decl")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_fn_tmpl_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  shared_ptr<function_template_decl> fn_tmpl_decl
    (new function_template_decl(loc, vis, bind));

  ctxt.push_decl_to_current_scope(fn_tmpl_decl, node, update_depth_info);

  unsigned parm_index = 0;
  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (shared_ptr<template_parameter> parm =
	  build_template_parameter(ctxt, n, parm_index,
				   /*update_depth_info=*/true))
	{
	  fn_tmpl_decl->add_template_parameter(parm);
	  ++parm_index;
	}
      else if (shared_ptr<function_decl> f =
	       build_function_decl(ctxt, n, /*update_depth_info=*/true))
	fn_tmpl_decl->set_pattern(f);
    }

  ctxt.key_fn_tmpl_decl(fn_tmpl_decl, id);

  return fn_tmpl_decl;
}

/// Build a template_type_parameter from a 'template-type-parameter'
/// xml element node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to parse from.
///
/// \param index the index (occurrence index, starting from 0) of the
/// template parameter.
///
/// \return a pointer to a newly created instance of
/// template_type_parameter, a null pointer otherwise.
static shared_ptr<template_type_parameter>
build_template_type_parameter(read_context& ctxt,
			      const xmlNodePtr node,
			      unsigned index,
			      bool update_depth_info)
{
  shared_ptr<template_type_parameter> nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("template-type-parameter")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (!id.empty() && ctxt.get_type_decl(id))
    return nil;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  if (!type_id.empty()
      && !(result = dynamic_pointer_cast<template_type_parameter>
	   (ctxt.get_type_decl(type_id))))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  location loc;
  read_location(ctxt, node,loc);

  result.reset(new template_type_parameter(index, name, loc));

  if (id.empty())
    ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(result),
				    node, update_depth_info);
  else
    ctxt.push_and_key_type_decl(result, id, node, update_depth_info);

  return result;
}


/// Build a tmpl_parm_type_composition from a
/// "template-parameter-type-composition" xml element node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to parse from.
///
/// \param index the index of the previous normal template parameter.
///
/// \param update_depth_info wheter to udpate the depth information
/// maintained in the context of the parsing.
///
/// \return a pointer to a new instance of tmpl_parm_type_composition
/// upon successful completion, a null pointer otherwise.
static shared_ptr<tmpl_parm_type_composition>
build_tmpl_parm_type_composition(read_context& ctxt,
				 const xmlNodePtr node,
				 unsigned index,
				 bool update_depth_info)
{
  shared_ptr<tmpl_parm_type_composition> nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("template-parameter-type-composition")))
    return nil;

  shared_ptr<type_base> composed_type;
  result.reset(new tmpl_parm_type_composition(index, composed_type));
  ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(result),
				  node, update_depth_info);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if ((composed_type =
	   build_pointer_type_def(ctxt, n,
				  /*update_depth_info=*/true))
	  ||(composed_type =
	     build_reference_type_def(ctxt, n,
				      /*update_depth_info=*/true))
	  || (composed_type =
	      build_qualified_type_decl(ctxt, n,
					/**update_depth_info=*/true)))
	{
	  result->set_composed_type(composed_type);
	  break;
	}
    }

  return result;
}

/// Build an instance of template_non_type_parameter from a
/// 'template-non-type-parameter' xml element node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to parse from.
///
/// \param index the index of the parameter.
///
/// \return a pointer to a newly created instance of
/// template_non_type_parameter upon successful completion, a null
/// pointer code otherwise.
static shared_ptr<template_non_type_parameter>
build_template_non_type_parameter(read_context&	ctxt,
				  const xmlNodePtr	node,
				  unsigned		index,
				  bool update_depth_info)
{
  shared_ptr<template_non_type_parameter> r;

  if (!xmlStrEqual(node->name, BAD_CAST("template-non-type-parameter")))
    return r;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> type;
  if (type_id.empty()
      || !(type = ctxt.get_type_decl(type_id)))
    return r;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  location loc;
  read_location(ctxt, node,loc);

  r.reset(new template_non_type_parameter(index, name, type, loc));
  ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(r),
				  node, update_depth_info);

  return r;
}

/// Build an intance of template_template_parameter from a
/// 'template-template-parameter' xml element node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to parse from.
///
/// \param index the index of the template parameter.
///
/// \return a pointer to a new instance of template_template_parameter
/// upon successful completion, a null pointer otherwise.
static shared_ptr<template_template_parameter>
build_template_template_parameter(read_context& ctxt,
				  const xmlNodePtr node,
				  unsigned index,
				  bool update_depth_info)
{
  shared_ptr<template_template_parameter> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("template-template-parameter")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  // Bail out if a type with the same ID already exists.
  if (!id.empty() && ctxt.get_type_decl(id))
    return nil;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  // Bail out if no type with this ID exists.
  if (!type_id.empty()
      && !(dynamic_pointer_cast<template_template_parameter>
	   (ctxt.get_type_decl(type_id))))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<template_template_parameter> result
    (new template_template_parameter(index, name, loc));

  ctxt.push_decl_to_current_scope(result, node, update_depth_info);

  // Go parse template parameters that are children nodes
  int parm_index = 0;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (node->type != XML_ELEMENT_NODE)
	continue;

      if (shared_ptr<template_parameter> p =
	  build_template_parameter(ctxt, n, parm_index,
				   /*update_depth_info=*/true))
	{
	  result->add_template_parameter(p);
	  ++parm_index;
	}
    }

  if (result)
    ctxt.key_type_decl(result, id);

  return result;
}

/// Build a template parameter type from several possible xml elment
/// nodes representing a serialized form a template parameter.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml element node to parse from.
///
/// \param the index of the template parameter we are parsing.
///
/// \return a pointer to a newly created instance of
/// template_parameter upon successful completion, a null pointer
/// otherwise.
static shared_ptr<template_parameter>
build_template_parameter(read_context&		ctxt,
			 const xmlNodePtr	node,
			 unsigned		index,
			 bool update_depth_info)
{
  shared_ptr<template_parameter> r;
  ((r = build_template_type_parameter(ctxt, node, index,
				      update_depth_info))
   || (r = build_template_non_type_parameter(ctxt, node, index,
					     update_depth_info))
   || (r = build_template_template_parameter(ctxt, node, index,
					     update_depth_info))
   || (r = build_tmpl_parm_type_composition(ctxt, node, index,
					    update_depth_info)));

  return r;
}

/// Build a type from an xml node.
///
/// \param ctxt the context of the parsing.
///
/// \param node the xml node to build the type_base from.
///
/// \return a pointer to the newly built type_base upon successful
/// completion, a null pointer otherwise.
static shared_ptr<type_base>
build_type(read_context& ctxt, const xmlNodePtr node, bool update_depth_info)
{
  shared_ptr<type_base> t;

  ((t = build_type_decl(ctxt, node, update_depth_info))
   || (t = build_qualified_type_decl(ctxt, node, update_depth_info))
   || (t = build_pointer_type_def(ctxt, node, update_depth_info))
   || (t = build_reference_type_def(ctxt, node,update_depth_info))
   || (t = build_enum_type_decl(ctxt, node,update_depth_info))
   || (t = build_typedef_decl(ctxt, node, update_depth_info))
   || (t = build_class_decl(ctxt, node, update_depth_info)));

  return t;
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
  return ctxt.push_and_key_type_decl(decl, id);
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
  ctxt.push_decl_to_current_scope(decl);
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
  return ctxt.push_and_key_type_decl(decl, id);
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
  return ctxt.push_and_key_type_decl(t, id);
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
  return ctxt.push_and_key_type_decl(t, id);
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
  return ctxt.push_and_key_type_decl(t, id);
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

  return ctxt.push_and_key_type_decl(t, id);
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
  ctxt.push_decl_to_current_scope(decl);

  return true;
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
  ctxt.push_decl_to_current_scope(decl);

  return true;
}

/// Parse a 'class-decl' xml element.
///
/// \param ctxt the context of the parsing.
///
/// \return true upon successful completion of the parsing, false
/// otherwise.
static bool
handle_class_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  xmlNodePtr node = xmlTextReaderExpand(r.get());
  if (!node)
    return false;

  shared_ptr<class_decl> decl = build_class_decl(ctxt, node,
						 /*update_depth_info=*/false);

  xmlTextReaderNext(r.get());

  return decl;
}

/// Parse a 'function-template-decl' xml element.
///
/// \param the parsing context.
///
/// \return true upon successful completion of the parsing, false
/// otherwise.
static bool
handle_function_template_decl(read_context& ctxt)
{
  xml::reader_sptr r = ctxt.get_reader();
  if (!r)
    return false;

  xmlNodePtr node = xmlTextReaderExpand(r.get());
  if (!node)
    return false;

  bool is_ok = build_function_template_decl(ctxt, node,
					    /*update_depth_info=*/false);

  xmlTextReaderNext(r.get());

  return is_ok;
}

}//end namespace reader
}//end namespace abigail
