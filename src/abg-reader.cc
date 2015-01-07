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
///
/// This file contains the definitions of the entry points to
/// de-serialize an instance of @ref abigail::translation_unit from an
/// ABI Instrumentation file in libabigail native XML format.

#include "config.h"
#include <cstring>
#include <cstdlib>
#include <tr1/unordered_map>
#include <deque>
#include <assert.h>
#include <sstream>
#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>
#include "abg-libxml-utils.h"
#include "abg-corpus.h"

#ifdef WITH_ZIP_ARCHIVE
#include "abg-libzip-utils.h"
#endif

namespace abigail
{

using xml::xml_char_sptr;

/// The namespace for the native XML file format reader.
namespace xml_reader
{
using std::string;
using std::deque;
using std::tr1::shared_ptr;
using std::tr1::unordered_map;
using std::tr1::dynamic_pointer_cast;
using std::vector;
using std::istream;
#ifdef WITH_ZIP_ARCHIVE
using zip_utils::zip_sptr;
using zip_utils::zip_file_sptr;
using zip_utils::open_archive;
using zip_utils::open_file_in_archive;
#endif //WITH_ZIP_ARCHIVE

class read_context;

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
			shared_ptr<function_tdecl> >::const_iterator
  const_fn_tmpl_map_it;

  typedef unordered_map<string,
			shared_ptr<class_tdecl> >::const_iterator
  const_class_tmpl_map_it;

  typedef unordered_map<string, xmlNodePtr> string_xml_node_map;

  typedef unordered_map<xmlNodePtr, decl_base_sptr> xml_node_decl_base_sptr_map;

private:
  unordered_map<string, shared_ptr<type_base> > m_types_map;
  unordered_map<string, shared_ptr<function_tdecl> > m_fn_tmpl_map;
  unordered_map<string, shared_ptr<class_tdecl> > m_class_tmpl_map;
  string_xml_node_map		m_id_xml_node_map;
  xml_node_decl_base_sptr_map	m_xml_node_decl_map;
  xml::reader_sptr		m_reader;
  deque<shared_ptr<decl_base> > m_decls_stack;
  corpus_sptr			m_corpus;

public:
  read_context(xml::reader_sptr reader) : m_reader(reader)
  {}

  xml::reader_sptr
  get_reader() const
  {return m_reader;}

  const string_xml_node_map&
  get_id_xml_node_map() const
  {return m_id_xml_node_map;}

  string_xml_node_map&
  get_id_xml_node_map()
  {return m_id_xml_node_map;}

  void
  clear_id_xml_node_map()
  {get_id_xml_node_map().clear();}

  const xml_node_decl_base_sptr_map&
  get_xml_node_decl_map() const
  {return m_xml_node_decl_map;}

  xml_node_decl_base_sptr_map&
  get_xml_node_decl_map()
  {return m_xml_node_decl_map;}

  void
  map_xml_node_to_decl(xmlNodePtr node,
		       decl_base_sptr decl)
  {
    if (node)
      get_xml_node_decl_map()[node]= decl;
  }

  decl_base_sptr
  get_decl_for_xml_node(xmlNodePtr node) const
  {
    xml_node_decl_base_sptr_map::const_iterator i =
      get_xml_node_decl_map().find(node);

    if (i != get_xml_node_decl_map().end())
      return i->second;

    return decl_base_sptr();
  }

  void
  clear_xml_node_decl_map()
  {get_xml_node_decl_map().clear();}

  void
  map_id_and_node (const string& id,
		   xmlNodePtr node)
  {
    if (!node)
      return;

    string_xml_node_map::const_iterator i = get_id_xml_node_map().find(id);
    if (i != get_id_xml_node_map().end())
      {
	// So, there has already been an xml node that has been mapped
	// to this ID.  That means, there ware an another xml node
	// with the same ID.  There are just a few cases where we
	// should allow this. Let's check that we are in one of these cases.
	xml_char_sptr p0 = XML_NODE_GET_ATTRIBUTE(node, "is-declaration-only");
	bool node_is_declaration_only =
	  (p0 && xmlStrEqual(p0.get(), BAD_CAST("yes")));
	bool node_is_class =
	  (xmlStrEqual(node->name, BAD_CAST("class-decl")));
	bool node_is_type_decl =
	  (xmlStrEqual(node->name, BAD_CAST("type-decl")));
	bool node_is_pointer_or_reference =
	  (xmlStrEqual(node->name, BAD_CAST("pointer-type-def"))
	   || xmlStrEqual(node->name, BAD_CAST("reference-type-def")));
	bool node_is_array =
	  xmlStrEqual(node->name, BAD_CAST("array-type-def"));
	bool node_is_typedef =
	  xmlStrEqual(node->name, BAD_CAST("typedef-decl"));
	bool node_is_qualified_type =
	  (xmlStrEqual(node->name, BAD_CAST("qualified-type-def")));
	xml_char_sptr name_val = XML_NODE_GET_ATTRIBUTE(node, "name");
	bool node_is_unnamed_enum_ut =
	  xmlStrEqual(name_val.get(), BAD_CAST("unnamed-enum-underlying-type"));

	xml_char_sptr p1 = XML_NODE_GET_ATTRIBUTE(i->second,
						  "is-declaration-only");
	bool is_ok = (node_is_declaration_only
		      || (p1 && xmlStrEqual(p1.get(), BAD_CAST("yes")))
		      || node_is_class
		      || node_is_type_decl
		      || node_is_pointer_or_reference
		      || node_is_array
		      || node_is_typedef
		      || node_is_qualified_type
		      || node_is_unnamed_enum_ut);
	assert(is_ok);
	if (is_ok)
	  get_id_xml_node_map()[id] = node;
      }
    else
      get_id_xml_node_map()[id] = node;
  }

  xmlNodePtr
  get_xml_node_from_id(const string& id) const
  {
    string_xml_node_map::const_iterator i = get_id_xml_node_map().find(id);
    if (i != get_id_xml_node_map().end())
     return i->second;
    return 0;
  }

  scope_decl_sptr
  get_scope_for_node(xmlNodePtr node);

  // This is defined later, after build_type() is declared, because it
  // uses it.
  type_base_sptr
  build_or_get_type_decl(const string& id,
			 bool add_decl_to_scope);

  /// Return the type that is identified by a unique ID.  Note that
  /// for a type to be "identified" by id, the function key_type_decl
  /// must have been previously called with that type and with id.
  ///
  /// @param id the unique id to consider.
  ///
  /// @return the type identified by the unique id id, or a null
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
  ///
  /// Note that for a function template to be identified by id, the
  /// function key_fn_tmpl_decl must have been previously called with
  /// that function template and with id.
  ///
  /// @param id the ID to consider.
  ///
  /// @return the function template identified by id, or a null
  /// pointer if no function template has ever been associated with
  /// id before.
  shared_ptr<function_tdecl>
  get_fn_tmpl_decl(const string& id) const
  {
    const_fn_tmpl_map_it i = m_fn_tmpl_map.find(id);
    if (i == m_fn_tmpl_map.end())
      return shared_ptr<function_tdecl>();
    return i->second;
  }

  /// Return the class template that is identified by a unique ID.
  ///
  /// Note that for a class template to be identified by id, the
  /// function key_class_tmpl_decl must have been previously called
  /// with that class template and with id.
  ///
  /// @param id the ID to consider.
  ///
  /// @return the class template identified by id, or a null pointer
  /// if no class template has ever been associated with id before.
  shared_ptr<class_tdecl>
  get_class_tmpl_decl(const string& id) const
  {
    const_class_tmpl_map_it i = m_class_tmpl_map.find(id);
    if (i == m_class_tmpl_map.end())
      return shared_ptr<class_tdecl>();
    return i->second;
  }

  /// Return the current lexical scope.
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

  decl_base_sptr
  get_cur_decl() const
  {
    if (m_decls_stack.empty())
      return shared_ptr<decl_base>(static_cast<decl_base*>(0));

    return m_decls_stack.back();
  }

  translation_unit*
  get_translation_unit()
  {
    const global_scope* global = 0;
    if (shared_ptr<decl_base> d = m_decls_stack.front())
      global = get_global_scope(d);

    if (global)
      return global->get_translation_unit();

    return 0;
  }

  void
  push_decl(decl_base_sptr d)
  {
    m_decls_stack.push_back(d);
  }

  decl_base_sptr
  pop_decl()
  {
    if (m_decls_stack.empty())
      return decl_base_sptr();

    shared_ptr<decl_base> t = get_cur_decl();
    m_decls_stack.pop_back();
    return t;
  }

  /// Pop all decls until a give scope is popped.
  ///
  /// @param scope the scope to pop.
  ///
  /// @return true if the scope was popped, false otherwise.  Note
  /// that if the scope wasn't found, it might mean that many other
  /// decls were popped.
  bool
  pop_scope(scope_decl_sptr scope)
  {
    decl_base_sptr d;
    do
      {
	d = pop_decl();
	scope_decl_sptr s = dynamic_pointer_cast<scope_decl>(d);
	if (s == scope)
	  break;
      }
    while (d);

    if (!d)
      return false;

    return dynamic_pointer_cast<scope_decl>(d) == scope;
  }

  /// like @ref pop_scope, but if the scope couldn't be popped, the
  /// function aborts the execution of the process.
  ///
  /// @param scope the scope to pop.
  void
  pop_scope_or_abort(scope_decl_sptr scope)
  {assert(pop_scope(scope));}

  void
  clear_decls_stack()
  {m_decls_stack.clear();}

  void
  clear_type_map()
  {m_types_map.clear();}

  /// Associate an ID with a type.
  ///
  /// @param type the type to associate witht he ID.
  ///
  /// @param id the ID to associate to the type.
  ///
  /// @return true upon successful completion, false otherwise.  Note
  /// that this returns false if the was already associate to an ID
  /// before.
  bool
  key_type_decl(shared_ptr<type_base> type, const string& id,
		bool force = false)
  {
    assert(type);

    const_types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end() && !force)
      return false;

    m_types_map[id] = type;
    return true;
  }

  /// Associate an ID with a type.
  ///
  /// If ID is an id for an existing type, this function replaces the
  /// exising type with the new DEFINITION type passe in argument.
  ///
  /// @param definition the type to associate witht he ID.
  ///
  /// @param id the ID to associate to the type.
  ///
  /// @return true upon successful completion, false otherwise.  Note
  /// that this returns false if the was already associate to an ID
  /// before.
  bool
  key_replacement_of_type_decl(shared_ptr<type_base> definition,
			       const string& id)
  {
    const_types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end())
      m_types_map.erase(i);
    key_type_decl(definition, id);

    return true;
  }

  /// Associate an ID to a function template.
  ///
  /// @param fn_tmpl_decl the function template to consider.
  ///
  /// @param id the ID to associate to the function template.
  ///
  /// @return true upon successful completion, false otherwise.  Note
  /// that the function returns false if an ID was previously
  /// associated to the function template.
  bool
  key_fn_tmpl_decl(shared_ptr<function_tdecl> fn_tmpl_decl,
		   const string& id)
  {
    assert(fn_tmpl_decl);

    const_fn_tmpl_map_it i = m_fn_tmpl_map.find(id);
    if (i != m_fn_tmpl_map.end())
      return false;

    m_fn_tmpl_map[id] = fn_tmpl_decl;
    return true;
  }

    /// Associate an ID to a class template.
  ///
  /// @param class_tmpl_decl the class template to consider.
  ///
  /// @param id the ID to associate to the class template.
  ///
  /// @return true upon successful completion, false otherwise.  Note
  /// that the function returns false if an ID was previously
  /// associated to the class template.
  bool
  key_class_tmpl_decl(shared_ptr<class_tdecl> class_tmpl_decl,
		      const string& id)
  {
    assert(class_tmpl_decl);

    const_class_tmpl_map_it i = m_class_tmpl_map.find(id);
    if (i != m_class_tmpl_map.end())
      return false;

    m_class_tmpl_map[id] = class_tmpl_decl;
    return true;
  }

  /// This function must be called on each declaration that is created during
  /// the parsing.  It adds the declaration to the current scope, and updates
  /// the state of the parsing context accordingly.
  ///
  /// @param decl the newly created declaration.
  void
  push_decl_to_current_scope(shared_ptr<decl_base> decl,
			     bool add_to_current_scope)
  {
    assert(decl);

    if (add_to_current_scope)
      add_decl_to_scope(decl, get_cur_scope());
    push_decl(decl);
  }

  /// This function must be called on each type decl that is created
  /// during the parsing.  It adds the type decl to the current scope
  /// and associates a unique ID to it.
  ///
  /// @param t type_decl
  ///
  /// @param id the unique ID to be associated to t
  ///
  /// @return true upon successful completion.
  ///
  bool
  push_and_key_type_decl(shared_ptr<type_base> t, const string& id,
			 bool add_to_current_scope)
  {
    shared_ptr<decl_base> decl = dynamic_pointer_cast<decl_base>(t);
    assert(decl);

    push_decl_to_current_scope(decl, add_to_current_scope);
    key_type_decl(t, id);
    return true;
  }

  const corpus_sptr
  get_corpus() const
  {return m_corpus;}

  corpus_sptr
  get_corpus()
  {return m_corpus;}

  void
  set_corpus(corpus_sptr c)
  {m_corpus = c;}

};// end class read_context

static int	advance_cursor(read_context&);
static bool	read_translation_unit_from_input(read_context&,
						 translation_unit&);
static bool	read_symbol_db_from_input(read_context&, bool,
					  string_elf_symbols_map_sptr&);
static bool	read_location(read_context&, xmlNodePtr, location&);
static bool	read_visibility(xmlNodePtr, decl_base::visibility&);
static bool	read_binding(xmlNodePtr, decl_base::binding&);
static bool	read_access(xmlNodePtr, access_specifier&);
static bool	read_size_and_alignment(xmlNodePtr, size_t&, size_t&);
static bool	read_static(xmlNodePtr, bool&);
static bool	read_offset_in_bits(xmlNodePtr, size_t&);
static bool	read_cdtor_const(xmlNodePtr, bool&, bool&, bool&);
static bool	read_is_declaration_only(xmlNodePtr, bool&);
static bool	read_is_virtual(xmlNodePtr, bool&);
static bool	read_is_struct(xmlNodePtr, bool&);
static bool	read_elf_symbol_type(xmlNodePtr, elf_symbol::type&);
static bool	read_elf_symbol_binding(xmlNodePtr, elf_symbol::binding&);

static namespace_decl_sptr
build_namespace_decl(read_context&, const xmlNodePtr, bool);

// <build a c++ class from an instance of xmlNodePtr>
//
// Note that whenever a new function to build a type is added here,
// you should make sure to call it from the build_type function, which
// should be the last function of the list of declarated function
// below.

static elf_symbol_sptr
build_elf_symbol(read_context&, const xmlNodePtr);

static elf_symbol_sptr
build_elf_symbol_from_reference(read_context&, const xmlNodePtr,
				bool);

static string_elf_symbols_map_sptr
build_elf_symbol_db(read_context&, const xmlNodePtr, bool);

static shared_ptr<function_decl::parameter>
build_function_parameter (read_context&, const xmlNodePtr);

static shared_ptr<function_decl>
build_function_decl(read_context&, const xmlNodePtr,
		    shared_ptr<class_decl>, bool);

static shared_ptr<var_decl>
build_var_decl(read_context&, const xmlNodePtr, bool);

static shared_ptr<type_decl>
build_type_decl(read_context&, const xmlNodePtr, bool);

static qualified_type_def_sptr
build_qualified_type_decl(read_context&, const xmlNodePtr, bool);

static shared_ptr<pointer_type_def>
build_pointer_type_def(read_context&, const xmlNodePtr, bool);

static shared_ptr<reference_type_def>
build_reference_type_def(read_context&, const xmlNodePtr, bool);

static array_type_def::subrange_sptr
build_subrange_type(read_context&, const xmlNodePtr);

static array_type_def_sptr
build_array_type_def(read_context&, const xmlNodePtr, bool);

static enum_type_decl_sptr
build_enum_type_decl(read_context&, const xmlNodePtr, bool);

static shared_ptr<typedef_decl>
build_typedef_decl(read_context&, const xmlNodePtr, bool);

static class_decl_sptr
build_class_decl(read_context&, const xmlNodePtr, bool);

static shared_ptr<function_tdecl>
build_function_tdecl(read_context&, const xmlNodePtr, bool);

static shared_ptr<class_tdecl>
build_class_tdecl(read_context&, const xmlNodePtr, bool);

static type_tparameter_sptr
build_type_tparameter(read_context&, const xmlNodePtr,
		      unsigned, template_decl_sptr);

static type_composition_sptr
build_type_composition(read_context&, const xmlNodePtr,
		       unsigned, template_decl_sptr);

static non_type_tparameter_sptr
build_non_type_tparameter(read_context&, const xmlNodePtr,
			  unsigned, template_decl_sptr);

static template_tparameter_sptr
build_template_tparameter(read_context&, const xmlNodePtr,
			  unsigned, template_decl_sptr);

static template_parameter_sptr
build_template_parameter(read_context&, const xmlNodePtr,
			 unsigned, template_decl_sptr);

// Please make this build_type function be the last one of the list.
// Note that it should call each type-building function above.  So
// please make sure to update it accordingly, whenever a new
// type-building function is added here.
static shared_ptr<type_base>
build_type(read_context&, const xmlNodePtr, bool);
// </build a c++ class  from an instance of xmlNodePtr>

static decl_base_sptr	handle_element_node(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_type_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_namespace_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_qualified_type_decl(read_context&,
						   xmlNodePtr, bool);
static decl_base_sptr	handle_pointer_type_def(read_context&,
						xmlNodePtr, bool);
static decl_base_sptr	handle_reference_type_def(read_context&,
						  xmlNodePtr, bool);
static decl_base_sptr	handle_array_type_def(read_context&,
					      xmlNodePtr, bool);
static decl_base_sptr	handle_enum_type_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_typedef_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_var_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_function_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_class_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_function_tdecl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_class_tdecl(read_context&, xmlNodePtr, bool);

/// Get the IR node representing the scope for a given XML node.
///
/// This function might trigger the building of a full sub-tree of IR.
///
/// @param node the XML for which to return the scope decl.  If its
/// parent XML node has no corresponding IR node, that IR node is constructed.
///
/// @return the IR node representing the scope of the IR node for the
/// XML node given in argument.
scope_decl_sptr
read_context::get_scope_for_node(xmlNodePtr node)
{
  scope_decl_sptr nil, scope;
  if (!node)
    return nil;

  xmlNodePtr parent = node->parent;
  if (parent
      && (xmlStrEqual(parent->name, BAD_CAST("data-member"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-type"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-function"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-template"))))
    parent = parent->parent;

  xml_node_decl_base_sptr_map::const_iterator i =
    get_xml_node_decl_map().find(parent);
  if (i == get_xml_node_decl_map().end())
    {
      scope_decl_sptr parent_scope = get_scope_for_node(parent);
      push_decl(parent_scope);
      scope = dynamic_pointer_cast<scope_decl>
	(handle_element_node(*this, parent, /*add_decl_to_scope=*/true));
      assert(scope);
      pop_scope_or_abort(parent_scope);
    }
  else
    scope = dynamic_pointer_cast<scope_decl>(i->second);

  return scope;
}

/// Get the type declaration IR node that matches a given XML type node ID.
///
/// If no IR node has been built for this ID, this function builds the
/// type declaration IR node and returns it.  Subsequent invocation of
/// this function with this ID will just return that ID previously returned.
///
/// @param id the XML node ID to consider.
///
/// @return the type declaration for the ID given in parameter.
type_base_sptr
read_context::build_or_get_type_decl(const string& id,
				     bool add_decl_to_scope)
{
  type_base_sptr t = get_type_decl(id);

  if (!t)
    {
      xmlNodePtr n = get_xml_node_from_id(id);
      assert(n);

      scope_decl_sptr scope;
      if (add_decl_to_scope)
	{
	  scope = get_scope_for_node(n);
	  /// In some cases, if for instance the scope of 'n' is a
	  /// namespace, get_scope_for_node() can trigger the building
	  /// of what is underneath of the namespace, if that has not
	  /// already been done.  So after that, the IR node for 'n'
	  /// might have been built; let's try to see if we are in
	  /// that case.  Otherwise, we'll just build the IR node for
	  /// 'n' ourselves.
	  if ((t = get_type_decl(id)))
	    return t;
	  assert(scope);
	  push_decl(scope);
	}

      t = build_type(*this, n, add_decl_to_scope);
      assert(t);
      map_xml_node_to_decl(n, get_type_declaration(t));

      if (add_decl_to_scope)
	pop_scope_or_abort(scope);
    }
  return t;
}

/// Moves the xmlTextReader cursor to the next xml node in the input
/// document.  Return 1 of the parsing was successful, 0 if no input
/// xml token is left, or -1 in case of error.
///
/// @param ctxt the read context
///
static int
advance_cursor(read_context& ctxt)
{
  xml::reader_sptr reader = ctxt.get_reader();
  return xmlTextReaderRead(reader.get());
}

/// Walk an entire XML sub-tree to build a map where the key is the
/// the value of the 'id' attribute (for type definitions) and the key
/// is the xml node containing the 'id' attribute.
static void
walk_xml_node_to_map_type_ids(read_context& ctxt,
			      xmlNodePtr node)
{
  for (xmlNodePtr n = node; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "id"))
	{
	  string id = CHAR_STR(s);
	  ctxt.map_id_and_node(id, n);
	}

      walk_xml_node_to_map_type_ids(ctxt, n->children);
    }
}

/// Parse the input XML document containing a translation_unit,
/// represented by an 'abi-instr' element node, associated to the current
/// context.
///
/// @param ctxt the current input context
///
/// @param tu the translation unit resulting from the parsing.
///
/// @return true upon successful parsing, false otherwise.
static bool
read_translation_unit_from_input(read_context&	ctxt,
				 translation_unit&	tu)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  // The document must start with the abi-instr node.
  int status = 1;
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    status = advance_cursor (ctxt);

  if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				   BAD_CAST("abi-instr")))
    return false;

  xmlNodePtr node = xmlTextReaderExpand(reader.get());
  if (!node)
    return false;

  walk_xml_node_to_map_type_ids(ctxt, node);

  xml::xml_char_sptr addrsize_str =
    XML_NODE_GET_ATTRIBUTE(node, "address-size");
  if (addrsize_str)
    {
      char address_size = atoi(reinterpret_cast<char*>(addrsize_str.get()));
      tu.set_address_size(address_size);
    }

  xml::xml_char_sptr path_str = XML_NODE_GET_ATTRIBUTE(node, "path");
  if (path_str)
    tu.set_path(reinterpret_cast<char*>(path_str.get()));

  // We are at global scope, as we've just seen the top-most
  // "abi-instr" element.
  ctxt.push_decl(tu.get_global_scope());
  ctxt.map_xml_node_to_decl(node, tu.get_global_scope());

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;
      assert(handle_element_node(ctxt, n,
				 /*add_decl_to_scope=*/true));
    }

   xmlTextReaderNext(reader.get());

   ctxt.clear_id_xml_node_map();
   ctxt.clear_type_map();
   ctxt.clear_id_xml_node_map();
   ctxt.clear_xml_node_decl_map();
   ctxt.clear_decls_stack();

   return true;
}

/// Parse the input XML document containing a function symbols
/// or a variable symbol database.
///
/// A function symbols database is an XML element named
/// "elf-function-symbols" and a variable symbols database is an XML
/// element named "elf-variable-symbols."  They contains "elf-symbol"
/// XML elements.
///
/// @param ctxt the read_context to use for the parsing.
///
/// @param function_symbols is true if this function should look for a
/// function symbols database, false if it should look for a variable
/// symbols database.
///
/// @param symdb the resulting symbol database object.  This is set
/// iff the function return true.
///
/// @return true upon successful parsing, false otherwise.
static bool
read_symbol_db_from_input(read_context&		ctxt,
			  bool				function_symbols,
			  string_elf_symbols_map_sptr&	symdb)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  // The symbol db must start with the 'elf-function-symbols" or
  // 'elf-variable-symbols' element node.
  int status = 1;
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    status = advance_cursor (ctxt);

  if (status != 1)
    return false;

  if (function_symbols
      && !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		       BAD_CAST("elf-function-symbols")))
    return false;

  if (!function_symbols
      && !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		       BAD_CAST("elf-variable-symbols")))
    return false;

  xmlNodePtr node = xmlTextReaderExpand(reader.get());
  if (!node)
    return false;

  symdb = build_elf_symbol_db(ctxt, node, function_symbols);

  xmlTextReaderNext(reader.get());

  return symdb;
}

/// From an "elf-needed" XML_ELEMENT node, build a vector of strings
/// representing the vector of the dependencies needed by a given
/// corpus.
///
/// @param node the XML_ELEMENT node of name "elf-needed".
///
/// @param needed the output vector of string to populate with the
/// vector of dependency names found on the xml node @p node.
///
/// @return true upon successful completion, false otherwise.
static bool
build_needed(xmlNode* node, vector<string>& needed)
{
  if (!node)
    return false;

  if (!node || !xmlStrEqual(node->name,BAD_CAST("elf-needed")))
    return false;

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE
	  || !xmlStrEqual(n->name, BAD_CAST("dependency")))
	continue;

      string name;
      if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "name"))
	xml::xml_char_sptr_to_string(s, name);

      if (!name.empty())
	needed.push_back(name);
    }

  return true;
}

/// Move to the next xml element node and expext it to be named
/// "elf-needed".  Then read the sub-tree to made of that node and
/// extracts a vector of needed dependencies name from it.
///
/// @param ctxt the read context used to the xml reading.
///
/// @param needed the resulting vector of dependency names.
///
/// @return true upon successful completion, false otherwise.
static bool
read_elf_needed_from_input(read_context&	ctxt,
			   vector<string>&	needed)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  int status = 1;
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    status = advance_cursor (ctxt);

  if (status != 1)
    return false;

  if (!xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
		    BAD_CAST("elf-needed")))
    return false;

  xmlNodePtr node = xmlTextReaderExpand(reader.get());
  if (!node)
    return false;

  bool result = build_needed(node, needed);

  xmlTextReaderNext(reader.get());

  return result;
}

/// Parse the input XML document containing an ABI corpus, represented
/// by an 'abi-corpus' element node, associated to the current
/// context.
///
/// @param ctxt the current input context.
///
/// @return the corpus resulting from the parsing
static corpus_sptr
read_corpus_from_input(read_context& ctxt)
{
  corpus_sptr nil;

  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return nil;

  // The document must start with the abi-corpus node.
  int status = 1;
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    status = advance_cursor (ctxt);

  if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				   BAD_CAST("abi-corpus")))
    return nil;

  if (!ctxt.get_corpus())
    {
      corpus_sptr c(new corpus(""));
      ctxt.set_corpus(c);
    }

  corpus& corp = *ctxt.get_corpus();

  xml::xml_char_sptr path_str = XML_READER_GET_ATTRIBUTE(reader, "path");
  if (path_str)
    corp.set_path(reinterpret_cast<char*>(path_str.get()));

  xml::xml_char_sptr architecture_str =
    XML_READER_GET_ATTRIBUTE(reader, "architecture");
  if (architecture_str)
    corp.set_architecture_name(reinterpret_cast<char*>(architecture_str.get()));

  xml::xml_char_sptr soname_str = XML_READER_GET_ATTRIBUTE(reader, "soname");
  if (soname_str)
    corp.set_soname(reinterpret_cast<char*>(soname_str.get()));

  // Advance the cursor until the next element.
  do
    status = advance_cursor (ctxt);
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT);

  // Read the needed element
  vector<string> needed;
  read_elf_needed_from_input(ctxt, needed);
  if (!needed.empty())
    corp.set_needed(needed);

  string_elf_symbols_map_sptr fn_sym_db, var_sym_db;
  bool is_ok = false;

  // Read the symbol databases.
  do
    {
      is_ok = (read_symbol_db_from_input(ctxt, true, fn_sym_db)
	       || read_symbol_db_from_input(ctxt, false, var_sym_db));
      if (is_ok)
	{
	  assert(fn_sym_db || var_sym_db);
	  if (fn_sym_db)
	    {
	      corp.set_fun_symbol_map(fn_sym_db);
	      fn_sym_db.reset();
	    }
	  else if (var_sym_db)
	    {
	      corp.set_var_symbol_map(var_sym_db);
	      var_sym_db.reset();
	    }
	}
    }
  while (is_ok);

  // Read the translation units.
  do
    {
      translation_unit_sptr tu(new translation_unit(""));
      is_ok = read_translation_unit_from_input(ctxt, *tu);
      if (is_ok)
	corp.add(tu);
    }
  while (is_ok);

  corp.set_origin(corpus::NATIVE_XML_ORIGIN);

  return ctxt.get_corpus();;
}

/// Parse an ABI instrumentation file (in XML format) at a given path.
///
/// @param input_file a path to the file containing the xml document
/// to parse.
///
/// @param tu the translation unit resulting from the parsing.
///
/// @return true upon successful parsing, false otherwise.
bool
read_translation_unit_from_file(const string&		input_file,
				translation_unit&	tu)
{
  read_context read_ctxt(xml::new_reader_from_file(input_file));
  return read_translation_unit_from_input(read_ctxt, tu);
}

/// Parse an ABI instrumentation file (in XML format) at a given path.
/// The path used is the one associated to the instance of @ref
/// translation_unit.
///
/// @param tu the translation unit to populate with the de-serialized
/// from of what is read at translation_unit::get_path().
///
/// @return true upon successful parsing, false otherwise.
bool
read_translation_unit_from_file(translation_unit&	tu)
{return read_translation_unit_from_file(tu.get_path(), tu);}

/// Parse an ABI instrumentation file (in XML format) from an
/// in-memory buffer.
///
/// @param buffer the in-memory buffer containing the xml document to
/// parse.
///
/// @param tu the translation unit resulting from the parsing.
///
/// @return true upon successful parsing, false otherwise.
bool
read_translation_unit_from_buffer(const string&	buffer,
				  translation_unit&	tu)
{
  read_context read_ctxt(xml::new_reader_from_buffer(buffer));
  return read_translation_unit_from_input(read_ctxt, tu);
}

/// This function is called by @ref read_translation_unit_from_input.
/// It handles the current xml element node of the reading context.
/// The result of the "handling" is to build the representation of the
/// xml node and tied it to the current translation unit.
///
/// @param ctxt the current parsing context.
///
/// @return true upon successful completion, false otherwise.
static decl_base_sptr
handle_element_node(read_context& ctxt, xmlNodePtr node,
		    bool add_to_current_scope)
{
  decl_base_sptr decl;
  if (!node)
    return decl;

  ((decl = handle_namespace_decl(ctxt, node, add_to_current_scope))
   ||(decl = handle_type_decl(ctxt, node, add_to_current_scope))
   ||(decl = handle_qualified_type_decl(ctxt, node,
					add_to_current_scope))
   ||(decl = handle_pointer_type_def(ctxt, node,
				     add_to_current_scope))
   || (decl = handle_reference_type_def(ctxt, node, add_to_current_scope))
   || (decl = handle_array_type_def(ctxt, node, add_to_current_scope))
   || (decl = handle_enum_type_decl(ctxt, node,
				    add_to_current_scope))
   || (decl = handle_typedef_decl(ctxt, node,
				  add_to_current_scope))
   || (decl = handle_var_decl(ctxt, node,
			      add_to_current_scope))
   || (decl = handle_function_decl(ctxt, node,
				   add_to_current_scope))
   || (decl = handle_class_decl(ctxt, node,
				add_to_current_scope))
   || (decl = handle_function_tdecl(ctxt, node,
				    add_to_current_scope))
   || (decl = handle_class_tdecl(ctxt, node,
				 add_to_current_scope)));
    return decl;
}

/// Parses location attributes on an xmlNodePtr.
///
///@param ctxt the current parsing context
///
///@param loc the resulting location.
///
/// @return true upon sucessful parsing, false otherwise.
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
/// @param node the xml node to read from.
///
/// @param vis the resulting visibility.
///
/// @return true upon successful completion, false otherwise.
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
/// @param node the xml node to build parse the bind from.
///
/// @param bind the resulting binding attribute.
///
/// @return true upon successful completion, false otherwise.
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
/// @param node the xml node to consider.
///
/// @param access the access attribute.  Set iff the function returns true.
///
/// @return true upon sucessful completion, false otherwise.
static bool
read_access(xmlNodePtr node, access_specifier& access)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "access"))
    {
      string a = CHAR_STR(s);

      if (a == "private")
	access = private_access;
      else if (a == "protected")
	access = protected_access;
      else if (a == "public")
	access = public_access;
      else
	access = private_access;

      return true;
    }
  return false;
}

/// Parse 'size-in-bits' and 'alignment-in-bits' attributes of a given
/// xmlNodePtr reprensting an xml element.
///
/// @param node the xml element node to consider.
///
/// @param size_in_bits the resulting value for the 'size-in-bits'
/// attribute.  This set only if this function returns true and the if
/// the attribute was present on the xml element node.
///
/// @param align_in_bits the resulting value for the
/// 'alignment-in-bits' attribute.  This set only if this function
/// returns true and the if the attribute was present on the xml
/// element node.
///
/// @return true if either one of the two attributes above were set,
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
/// @param node the xml element node to consider.
///
/// @param is_static the resulting the parsing.  Is set if the
/// function returns true.
///
/// @return true if the xml element node has the 'static' attribute
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
/// @param offset_in_bits set to true if the element node contains the
/// attribute.
///
/// @return true iff the xml element node contain$s the attribute.
static bool
read_offset_in_bits(xmlNodePtr	node,
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
/// @param is_constructor the resulting value of the parsing of the
/// 'constructor' attribute.  Is set if the xml node contains the
/// attribute and if the function returns true.
///
/// @param is_destructor the resulting value of the parsing of the
/// 'destructor' attribute.  Is set if the xml node contains the
/// attribute and if the function returns true.
///
/// @param is_const the resulting value of the parsing of the 'const'
/// attribute.  Is set if the xml node contains the attribute and if
/// the function returns true.
///
/// @return true if at least of the attributes above is set, false
/// otherwise.
///
/// Note that callers of this function should initialize
/// is_constructor, is_destructor and is_const prior to passing them
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

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "const"))
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

/// Read the "is-declaration-only" attribute of the current xml node.
///
/// @param node the xml node to consider.
///
/// @param is_decl_only is set to true iff the "is-declaration-only" attribute
/// is present and set to "yes"
///
/// @return true iff the is_decl_only attribute was set.
static bool
read_is_declaration_only(xmlNodePtr node, bool& is_decl_only)
{
    if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-declaration-only"))
      {
	string str = CHAR_STR(s);
	if (str == "yes")
	  is_decl_only = true;
	else
	  is_decl_only = false;
	return true;
      }
    return false;
}

/// Read the "is-virtual" attribute of the current xml node.
///
/// @param node the xml node to read the attribute from
///
/// @param is_virtual is set to true iff the "is-virtual" attribute is
/// present and set to "yes".
///
/// @return true iff the is-virtual attribute is present.
static bool
read_is_virtual(xmlNodePtr node, bool& is_virtual)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-virtual"))
    {
      string str = CHAR_STR(s);
      if (str == "yes")
	is_virtual = true;
      else
	is_virtual = false;
      return true;
    }
  return false;
}

/// Read the 'is-struct' attribute.
///
/// @param node the xml node to read the attribute from.
///
/// @param is_virtual is set to true iff the "is-struct" is present
/// and set to "yes".
///
/// @return true iff the "is-struct" attribute is present.
static bool
read_is_struct(xmlNodePtr node, bool& is_struct)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-struct"))
    {
      string str = CHAR_STR(s);
      if (str == "yes")
	  is_struct = true;
      else
	is_struct = false;
      return true;
    }
  return false;
}

/// Read the 'type' attribute of the 'elf-symbol' element.
///
/// @param node the XML node to read the attribute from.
///
/// @param t the resulting elf_symbol::type.
///
/// @return true iff the function completed successfully.
static bool
read_elf_symbol_type(xmlNodePtr node, elf_symbol::type& t)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type"))
    {
      string str;
      xml::xml_char_sptr_to_string(s, str);
      if (!string_to_elf_symbol_type(str, t))
	return false;
      return true;
    }
  return false;
}

/// Read the 'binding' attribute of the of the 'elf-symbol' element.
///
/// @param node the XML node to read the attribute from.
///
/// @param b the XML the resulting elf_symbol::binding.
///
/// @return true iff the function completed successfully.
static bool
read_elf_symbol_binding(xmlNodePtr node, elf_symbol::binding& b)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "binding"))
    {
      string str;
      xml::xml_char_sptr_to_string(s, str);
      if (!string_to_elf_symbol_binding(str, b))
	return false;
      return true;
    }
  return false;
}

/// Build a @ref namespace_decl from an XML element node which name is
/// "namespace-decl".  Note that this function recursively reads the
/// content of the namespace and builds the proper IR nodes
/// accordingly.
///
/// @param ctxt the read context to use.
///
/// @param node the XML node to consider.  It must constain the
/// content of the namespace, that is, children XML nodes representing
/// what is inside the namespace, unless the namespace is empty.
///
/// @param add_to_current_scope if set to yes, the resulting
/// namespace_decl is added to the IR being currently built.
///
/// @return a pointer to the the resulting @ref namespace_decl.
static namespace_decl_sptr
build_namespace_decl(read_context&	ctxt,
		     const xmlNodePtr	node,
		     bool		add_to_current_scope)
{
  namespace_decl_sptr nil;
  if (!node || !xmlStrEqual(node->name, BAD_CAST("namespace-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      namespace_decl_sptr result = dynamic_pointer_cast<namespace_decl>(d);
      assert(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<namespace_decl> decl(new namespace_decl(name, loc));
  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);
  ctxt.map_xml_node_to_decl(node, decl);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;
      assert(handle_element_node(ctxt, n, /*add_to_current_scope=*/true));
    }

  ctxt.pop_scope_or_abort(decl);

  return decl;
}

/// Build an instance of @ref elf_symbol from an XML element node
/// which name is 'elf-symbol'.
///
/// @param node the XML node to read.
///
/// @return the @ref elf_symbol built, or nil if it couldn't be built.
static elf_symbol_sptr
build_elf_symbol(read_context&, const xmlNodePtr node)
{
  elf_symbol_sptr nil;

  if (!node
      || node->type != XML_ELEMENT_NODE
      || !xmlStrEqual(node->name, BAD_CAST("elf-symbol")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    xml::xml_char_sptr_to_string(s, name);

  bool is_defined = true;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-defined"))
    {
      string value;
      xml::xml_char_sptr_to_string(s, value);
      if (value == "true" || value == "yes")
	is_defined = true;
      else
	is_defined = false;
    }

  string version_string;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "version"))
    xml::xml_char_sptr_to_string(s, version_string);

  bool is_default_version = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-default-version"))
    {
      string value;
      xml::xml_char_sptr_to_string(s, value);
      if (value == "true" || value == "yes")
	is_default_version = true;
    }

  elf_symbol::type type = elf_symbol::NOTYPE_TYPE;
  read_elf_symbol_type(node, type);

  elf_symbol::binding binding;
  read_elf_symbol_binding(node, binding);

  elf_symbol::version version(version_string, is_default_version);

  elf_symbol_sptr e(new elf_symbol(/*index=*/0, name,
				   type, binding,
				   is_defined, version));
  return e;
}

/// Build and instance of elf_symbol from an XML attribute named
/// 'elf-symbol-id' which value is the ID of a symbol that should
/// present in the symbol db of the corpus associated to the current
/// context.
///
/// @param ctxt the current context to consider.
///
/// @param node the xml element node to consider.
///
/// @param function_symbol is true if we should look for a function
/// symbol, is false if we should look for a variable symbol.
///
/// @return a shared pointer the resutling elf_symbol.
static elf_symbol_sptr
build_elf_symbol_from_reference(read_context& ctxt, const xmlNodePtr node,
				bool function_symbol)
{
  elf_symbol_sptr nil;

  if (!node)
    return nil;

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "elf-symbol-id"))
    {
      string sym_id;
      xml::xml_char_sptr_to_string(s, sym_id);
      if (sym_id.empty())
	return nil;

      string name, ver;
      elf_symbol::get_name_and_version_from_id(sym_id, name, ver);
      if (name.empty())
	return nil;

      string_elf_symbols_map_sptr sym_db =
	(function_symbol)
	? ctxt.get_corpus()->get_fun_symbol_map_sptr()
	: ctxt.get_corpus()->get_var_symbol_map_sptr();

      string_elf_symbols_map_type::const_iterator i = sym_db->find(name);
      if (i != sym_db->end())
	{
	  for (elf_symbols::const_iterator s = i->second.begin();
	       s != i->second.end();
	       ++s)
	    if ((*s)->get_id_string() == sym_id)
	      return *s;
	}
    }

  return nil;
}

/// Build an instance of string_elf_symbols_map_type from an XML
/// element representing either a function symbols data base, or a
/// variable symbols database.
///
/// @param ctxt the context to take in account.
///
/// @param node the XML node to consider.
///
/// @param function_syms true if we should look for a function symbols
/// data base, false if we should look for a variable symbols data
/// base.
static string_elf_symbols_map_sptr
build_elf_symbol_db(read_context& ctxt,
		    const xmlNodePtr node,
		    bool function_syms)
{
  string_elf_symbols_map_sptr map, nil;
  string_elf_symbol_sptr_map_type id_sym_map;

  if (!node)
    return nil;

  if (function_syms
      && !xmlStrEqual(node->name, BAD_CAST("elf-function-symbols")))
    return nil;

  if (!function_syms
      && !xmlStrEqual(node->name, BAD_CAST("elf-variable-symbols")))
    return nil;

  typedef std::tr1::unordered_map<xmlNodePtr, elf_symbol_sptr>
    xml_node_ptr_elf_symbol_sptr_map_type;
  xml_node_ptr_elf_symbol_sptr_map_type xml_node_ptr_elf_symbol_map;

  elf_symbol_sptr sym;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if ((sym = build_elf_symbol(ctxt, n)))
	{
	  id_sym_map[sym->get_id_string()] = sym;
	  xml_node_ptr_elf_symbol_map[n] = sym;
	}
    }

  if (id_sym_map.empty())
    return nil;

  map.reset(new string_elf_symbols_map_type);
  string_elf_symbols_map_type::iterator it;
  for (string_elf_symbol_sptr_map_type::const_iterator i = id_sym_map.begin();
       i != id_sym_map.end();
       ++i)
    {
      it = map->find(i->second->get_name());
      if (it == map->end())
	{
	  (*map)[i->second->get_name()] = elf_symbols();
	  it = map->find(i->second->get_name());
	}
      it->second.push_back(i->second);
    }

  // Now build the alias relations
  for (xml_node_ptr_elf_symbol_sptr_map_type::const_iterator x =
	 xml_node_ptr_elf_symbol_map.begin();
       x != xml_node_ptr_elf_symbol_map.end();
       ++x)
    {
      if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(x->first, "alias"))
	{
      string alias_id = CHAR_STR(s);

      // Symbol aliases can be multiple separated by comma(,), split them
      std::vector<std::string> elems;
      std::stringstream aliases(alias_id);
      std::string item;
      while (std::getline(aliases, item, ','))
        elems.push_back(item);
      for (std::vector<string>::iterator alias = elems.begin();
           alias != elems.end(); alias++)
        {
          string_elf_symbol_sptr_map_type::const_iterator i =
          id_sym_map.find(*alias);
          assert(i != id_sym_map.end());
          assert(i->second->is_main_symbol());

          x->second->get_main_symbol()->add_alias(i->second.get());
        }
	}
    }

  return map;
}

/// Build a function parameter from a 'parameter' xml element node.
///
/// @param ctxt the contexte of the xml parsing.
///
/// @param node the xml 'parameter' element node to de-serialize from.
static shared_ptr<function_decl::parameter>
build_function_parameter(read_context& ctxt, const xmlNodePtr node)
{
  shared_ptr<function_decl::parameter> nil;

  if (!node || !xmlStrEqual(node->name, BAD_CAST("parameter")))
    return nil;

  bool is_variadic = false;
  string is_variadic_str;
  if (xml_char_sptr s =
      xml::build_sptr(xmlGetProp(node, BAD_CAST("is-variadic"))))
    {
      is_variadic_str = CHAR_STR(s) ? CHAR_STR(s) : "";
      is_variadic = (is_variadic_str == "yes") ? true : false;
    }

  bool is_artificial = false;
  string is_artificial_str;
  if (xml_char_sptr s =
      xml::build_sptr(xmlGetProp(node, BAD_CAST("is-artificial"))))
    {
      is_artificial_str = CHAR_STR(s) ? CHAR_STR(s) : "";
      is_artificial = (is_artificial_str == "yes") ? true : false;
    }

  string type_id;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("type-id"))))
    type_id = CHAR_STR(a);

  shared_ptr<type_base> type;
  if (!is_variadic)
    {
      assert(!type_id.empty());
      type = ctxt.build_or_get_type_decl(type_id, true);
    }
  assert(type || is_variadic);

  string name;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("name"))))
    name = CHAR_STR(a);

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<function_decl::parameter> p
    (new function_decl::parameter(type, name, loc, is_variadic, is_artificial));

  return p;
}

/// Build a function_decl from a 'function-decl' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the function_decl from.
///
/// @param as_method_decl if this is set to a class_decl pointer, it
/// means that the 'function-decl' xml node should be parsed as a
/// method_decl.  The class_decl pointer is the class decl to which
/// the resulting method_decl is a member function of.  The resulting
/// shared_ptr<function_decl> that is returned is then really a
/// shared_ptr<class_decl::method_decl>.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly created function_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<function_decl>
build_function_decl(read_context&	ctxt,
		    const xmlNodePtr	node,
		    shared_ptr<class_decl> as_method_decl,
		    bool add_to_current_scope)
{
  shared_ptr<function_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("function-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string mangled_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    mangled_name = xml::unescape_xml_string(CHAR_STR(s));

  string inline_prop;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "declared-inline"))
    inline_prop = CHAR_STR(s);
  bool declared_inline = inline_prop == "yes" ? true : false;

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  size_t size = 0, align = 0;
  read_size_and_alignment(node, size, align);

  location loc;
  read_location(ctxt, node, loc);

  std::vector<shared_ptr<function_decl::parameter> > parms;
  shared_ptr<function_type> fn_type(as_method_decl
				    ? new method_type(as_method_decl,
						      size, align)
				    : new function_type(size, align));

  fn_type = ctxt.get_translation_unit()->get_canonical_function_type(fn_type);

  shared_ptr<function_decl> fn_decl(as_method_decl
				    ? new class_decl::method_decl
				    (name, fn_type,
				     declared_inline, loc,
				     mangled_name, vis, bind)
				    : new function_decl(name, fn_type,
							declared_inline, loc,
							mangled_name, vis,
							bind));

  ctxt.push_decl_to_current_scope(fn_decl, add_to_current_scope);

  elf_symbol_sptr sym = build_elf_symbol_from_reference(ctxt, node,
							/*function_sym=*/true);
  if (sym)
    fn_decl->set_symbol(sym);

  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      else if (xmlStrEqual(n->name, BAD_CAST("parameter")))
	{
	  if (shared_ptr<function_decl::parameter> p =
	      build_function_parameter(ctxt, n))
	    fn_type->append_parameter(p);
	}
      else if (xmlStrEqual(n->name, BAD_CAST("return")))
	{
	  string type_id;
	  if (xml_char_sptr s =
	      xml::build_sptr(xmlGetProp(n, BAD_CAST("type-id"))))
	    type_id = CHAR_STR(s);
	  if (!type_id.empty())
	    fn_type->set_return_type(ctxt.build_or_get_type_decl(type_id,
								 true));
	}
    }

  if (fn_decl->get_symbol() && fn_decl->get_symbol()->is_public())
    fn_decl->set_is_in_public_symbol_table(true);

  return fn_decl;
}

/// Build pointer to var_decl from a 'var-decl' xml Node
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the var_decl from.
///
/// @return a pointer to a newly built var_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<var_decl>
build_var_decl(read_context&	ctxt,
	       const xmlNodePtr node,
	       bool		add_to_current_scope)
{
  shared_ptr<var_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("var-decl")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type = ctxt.build_or_get_type_decl(type_id,
								      true);
  assert(underlying_type);

  string mangled_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    mangled_name = xml::unescape_xml_string(CHAR_STR(s));

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  location locus;
  read_location(ctxt, node, locus);

  shared_ptr<var_decl> decl(new var_decl(name, underlying_type,
					 locus, mangled_name,
					 vis, bind));

  elf_symbol_sptr sym = build_elf_symbol_from_reference(ctxt, node,
							/*function_sym=*/false);
  if (sym)
    decl->set_symbol(sym);

  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);

  if (decl->get_symbol() && decl->get_symbol()->is_public())
    decl->set_is_in_public_symbol_table(true);

  return decl;
}

/// Build a type_decl from a "type-decl" XML Node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the XML node to build the type_decl from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to type_decl upon successful completion, a null
/// pointer otherwise.
static shared_ptr<type_decl>
build_type_decl(read_context&		ctxt,
		const xmlNodePtr	node,
		bool			add_to_current_scope)
{
  shared_ptr<type_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("type-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      type_decl_sptr result = dynamic_pointer_cast<type_decl>(d);
      assert(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  assert(!id.empty());

  size_t size_in_bits= 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));

  size_t alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      // I've seen instances of DSOs where a type_decl would appear
      // several times.  Hugh.
      type_decl_sptr ty = dynamic_pointer_cast<type_decl>(d);
      assert(ty);
      assert(name == ty->get_name());
      assert(ty->get_size_in_bits() == size_in_bits);
      assert(ty->get_alignment_in_bits() == alignment_in_bits);
      return ty;
    }

  shared_ptr<type_decl> decl(new type_decl(name, size_in_bits,
					   alignment_in_bits,
					   loc));
  if (ctxt.push_and_key_type_decl(decl, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, decl);
      return decl;
    }

  return nil;
}

/// Build a qualified_type_def from a 'qualified-type-def' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the qualified_type_def from.
///
/// @param add_to_current_scope if set to yes, the resulting of this
/// function is added to its current scope.
///
/// @return a pointer to a newly built qualified_type_def upon
/// successful completion, a null pointer otherwise.
static qualified_type_def_sptr
build_qualified_type_decl(read_context&	ctxt,
			  const xmlNodePtr	node,
			  bool			add_to_current_scope)
{
  qualified_type_def_sptr nil;
  if (!xmlStrEqual(node->name, BAD_CAST("qualified-type-def")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      qualified_type_def_sptr result =
	dynamic_pointer_cast<qualified_type_def>(d);
      assert(result);
      return result;
    }

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> underlying_type =
    ctxt.build_or_get_type_decl(type_id, true);
  assert(underlying_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      qualified_type_def_sptr result =
	dynamic_pointer_cast<qualified_type_def>(d);
      assert(result);
      return result;
    }

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE (node, "id"))
    id = CHAR_STR(s);

  string const_str;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "const"))
    const_str = CHAR_STR(s);
  bool const_cv = const_str == "yes" ? true : false;

  string volatile_str;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "volatile"))
    volatile_str = CHAR_STR(s);
  bool volatile_cv = volatile_str == "yes" ? true : false;

  string restrict_str;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "restrict"))
    restrict_str = CHAR_STR(s);
  bool restrict_cv = restrict_str == "yes" ? true : false;

  qualified_type_def::CV cv = qualified_type_def::CV_NONE;
  if (const_cv)
    cv = cv | qualified_type_def::CV_CONST;
  if (volatile_cv)
    cv = cv | qualified_type_def::CV_VOLATILE;
  if (restrict_cv)
    cv = cv | qualified_type_def::CV_RESTRICT;

  location loc;
  read_location(ctxt, node, loc);

  assert(!id.empty());

  qualified_type_def_sptr decl;

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      qualified_type_def_sptr ty = dynamic_pointer_cast<qualified_type_def>(d);
      assert(ty);
      assert(*ty->get_underlying_type() == *underlying_type);
      assert(ty->get_cv_quals() == cv);
      return ty;
    }

  decl.reset(new qualified_type_def(underlying_type, cv, loc));
  if (ctxt.push_and_key_type_decl(decl, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, decl);
      return decl;
    }

  return shared_ptr<qualified_type_def>((qualified_type_def*)0);
}

/// Build a pointer_type_def from a 'pointer-type-def' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the pointer_type_def from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly built pointer_type_def upon
/// successful completion, a null pointer otherwise.
static shared_ptr<pointer_type_def>
build_pointer_type_def(read_context&	ctxt,
		       const xmlNodePtr node,
		       bool		add_to_current_scope)
{

  shared_ptr<pointer_type_def> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("pointer-type-def")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      pointer_type_def_sptr result =
	dynamic_pointer_cast<pointer_type_def>(d);
      assert(result);
      return result;
    }

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type =
    ctxt.build_or_get_type_decl(type_id, true);
  assert(pointed_to_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      pointer_type_def_sptr result =
	dynamic_pointer_cast<pointer_type_def>(d);
      assert(result);
      return result;
    }

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  assert(!id.empty());
  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      pointer_type_def_sptr ty = dynamic_pointer_cast<pointer_type_def>(d);
      assert(ty);
      assert(*pointed_to_type == *ty->get_pointed_to_type());
      return ty;
    }

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<pointer_type_def> t(new pointer_type_def(pointed_to_type,
						      size_in_bits,
						      alignment_in_bits,
						      loc));
  if (ctxt.push_and_key_type_decl(t, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, t);
      return t;
    }

  return nil;
}

/// Build a reference_type_def from a pointer to 'reference-type-def'
/// xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the reference_type_def from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly built reference_type_def upon
/// successful completio, a null pointer otherwise.
static shared_ptr<reference_type_def>
build_reference_type_def(read_context&		ctxt,
			 const xmlNodePtr	node,
			 bool			add_to_current_scope)
{
  shared_ptr<reference_type_def> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("reference-type-def")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      reference_type_def_sptr result =
	dynamic_pointer_cast<reference_type_def>(d);
      assert(result);
      return result;
    }

  string kind;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "kind"))
    kind = CHAR_STR(s); // this should be either "lvalue" or "rvalue".
  bool is_lvalue = kind == "lvalue" ? true : false;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type = ctxt.build_or_get_type_decl(type_id,
								      true);
  assert(pointed_to_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      reference_type_def_sptr result =
	dynamic_pointer_cast<reference_type_def>(d);
      assert(result);
      return result;
    }

  size_t size_in_bits = 0, alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  assert(!id.empty());

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      reference_type_def_sptr ty = dynamic_pointer_cast<reference_type_def>(d);
      assert(ty);
      assert(*pointed_to_type == *ty->get_pointed_to_type());
      return ty;
    }

  location loc;
  read_location(ctxt, node, loc);

  shared_ptr<reference_type_def> t(new reference_type_def(pointed_to_type,
							  is_lvalue,
							  size_in_bits,
							  alignment_in_bits,
							  loc));
  if (ctxt.push_and_key_type_decl(t, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, t);
      return t;
    }

  return nil;
}

/// Build a array_type_def from a 'array-type-def' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the array_type_def from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly built array_type_def upon
/// successful completion, a null pointer otherwise.
static array_type_def::subrange_sptr
build_subrange_type(read_context&	ctxt,
		    const xmlNodePtr	node)
{
  array_type_def::subrange_sptr nil;

  if (!node || !xmlStrEqual(node->name, BAD_CAST("subrange")))
    return nil;

  size_t length = 0;
  string length_str;
  if (xml_char_sptr s =
      xml::build_sptr(xmlGetProp(node, BAD_CAST("length"))))
    length = atoi(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  // Note that DWARF would actually have a lower_bound of -1 for an
  // array of length 0
  array_type_def::subrange_sptr p
    (new array_type_def::subrange_type(0,
				       length - 1,
				       loc));

  return p;
}
/// Build a array_type_def from a 'array-type-def' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the array_type_def from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly built array_type_def upon
/// successful completion, a null pointer otherwise.
static array_type_def_sptr
build_array_type_def(read_context&	ctxt,
		     const		xmlNodePtr node,
		     bool		add_to_current_scope)
{

  array_type_def_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("array-type-def")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      array_type_def_sptr result =
	dynamic_pointer_cast<array_type_def>(d);
      assert(result);
      return result;
    }

  int dimensions = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "dimensions"))
    dimensions = atoi(CHAR_STR(s));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  // The type of array elements.
  shared_ptr<type_base> type =
    ctxt.build_or_get_type_decl(type_id, true);
  assert(type);

  // maybe building the type of array elements triggered building this
  // one in the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      array_type_def_sptr result =
	dynamic_pointer_cast<array_type_def>(d);
      assert(result);
      return result;
    }

  size_t size_in_bits = 0, alignment_in_bits = 0;
  char *endptr;

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    {
      size_in_bits = strtoull(CHAR_STR(s), &endptr, 0);
      if (*endptr != '\0')
        {
          if (!strcmp(CHAR_STR(s), "infinite"))
            size_in_bits = (size_t) -1;
          else
            return nil;
        }
    }

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    {
      alignment_in_bits = strtoull(CHAR_STR(s), &endptr, 0);
      if (*endptr != '\0')
        return nil;
    }

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  assert(!id.empty());

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      array_type_def_sptr ty = dynamic_pointer_cast<array_type_def>(d);
      assert(ty);
      assert(*type == *ty->get_element_type());
      assert(type->get_alignment_in_bits() == alignment_in_bits);
      return ty;
    }

  location loc;
  read_location(ctxt, node, loc);
  array_type_def::subranges_type subranges;

  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      else if (xmlStrEqual(n->name, BAD_CAST("subrange")))
	{
	  if (array_type_def::subrange_sptr s =
	      build_subrange_type(ctxt, n))
	    subranges.push_back(s);
	}
    }

  array_type_def_sptr ar_type(new array_type_def(type,
						 subranges,
						 loc));

  if (dimensions != ar_type->get_dimension_count()
      || (alignment_in_bits
	  != ar_type->get_element_type()->get_alignment_in_bits()))
    return nil;

  if (size_in_bits != ar_type->get_size_in_bits())
    {
      assert(size_in_bits == (size_t) -1
	     || ar_type->get_element_type()->get_size_in_bits() == (size_t)-1
	     || ar_type->get_element_type()->get_size_in_bits() == 0);
    }

  if (ctxt.push_and_key_type_decl(ar_type, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, ar_type);
      return ar_type;
    }

  return nil;
}

/// Build an enum_type_decl from an 'enum-type-decl' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the enum_type_decl from.
///
/// param add_to_current_scope if set to yes, the resulting of this
/// function is added to its current scope.
///
/// @return a pointer to a newly built enum_type_decl upon successful
/// completion, a null pointer otherwise.
static enum_type_decl_sptr
build_enum_type_decl(read_context&	ctxt,
		     const xmlNodePtr	node,
		     bool		add_to_current_scope)
{
  shared_ptr<enum_type_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("enum-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      enum_type_decl_sptr result =
	dynamic_pointer_cast<enum_type_decl>(d);
      assert(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  assert(!id.empty() && !ctxt.get_type_decl(id));

  string base_type_id;
  enum_type_decl::enumerators enums;
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
	    name = xml::unescape_xml_string(CHAR_STR(a));

	  a = xml::build_sptr(xmlGetProp(n, BAD_CAST("value")));
	  if (a)
	    value = atoi(CHAR_STR(a));

	  enums.push_back(enum_type_decl::enumerator(name, value));
	}
    }

  shared_ptr<type_base> underlying_type =
    ctxt.build_or_get_type_decl(base_type_id, true);
  assert(underlying_type);

  shared_ptr<enum_type_decl> t(new enum_type_decl(name, loc,
						  underlying_type,
						  enums));
  if (ctxt.push_and_key_type_decl(t, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, t);
      return t;
    }

  return nil;
}

/// Build a typedef_decl from a 'typedef-decl' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the typedef_decl from.
///
/// @return a pointer to a newly built typedef_decl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<typedef_decl>
build_typedef_decl(read_context&	ctxt,
		   const xmlNodePtr	node,
		   bool		add_to_current_scope)
{
  shared_ptr<typedef_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("typedef-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      typedef_decl_sptr result = dynamic_pointer_cast<typedef_decl>(d);
      assert(result);
      return result;
    }

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  assert(!id.empty());

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type(ctxt.build_or_get_type_decl(type_id,
								    true));
  assert(underlying_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      typedef_decl_sptr result = dynamic_pointer_cast<typedef_decl>(d);
      assert(result);
      return result;
    }

  location loc;
  read_location(ctxt, node, loc);

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      typedef_decl_sptr ty = dynamic_pointer_cast<typedef_decl>(d);
      assert(ty);
      assert(name == ty->get_name());
      assert(underlying_type == ty->get_underlying_type());
      // it's possible to have the same typedef several times.
    }
  shared_ptr<typedef_decl> t(new typedef_decl(name, underlying_type, loc));

  if (ctxt.push_and_key_type_decl(t, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, t);
      return t;
    }

  return nil;
}

/// Build a class_decl from a 'class-decl' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the class_decl from.
///
/// @param add_to_current_scope if yes, the resulting class node
/// hasn't triggered voluntarily the adding of the resulting
/// class_decl_sptr to the current scope.
///
/// @return a pointer to class_decl upon successful completion, a null
/// pointer otherwise.
static class_decl_sptr
build_class_decl(read_context&		ctxt,
		 const xmlNodePtr	node,
		 bool			add_to_current_scope)
{
  shared_ptr<class_decl> nil;

  if (!xmlStrEqual(node->name, BAD_CAST("class-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      class_decl_sptr result = dynamic_pointer_cast<class_decl>(d);
      assert(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  size_t size_in_bits = 0, alignment_in_bits = 0;
  read_size_and_alignment(node, size_in_bits, alignment_in_bits);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  location loc;
  read_location(ctxt, node, loc);

  class_decl::member_types mbrs;
  class_decl::data_members data_mbrs;
  class_decl::member_functions mbr_functions;
  class_decl::base_specs  bases;

  shared_ptr<class_decl> decl;

  bool is_decl_only = false;
  read_is_declaration_only(node, is_decl_only);

  bool is_struct = false;
  read_is_struct(node, is_struct);

  // Keep in mind that there can be instances of DSOs that define a
  // class twice!! Hugh.  So the test + assert() below doesn't work in
  // these.  Oh well.
#if 0
  // If the id is not empty, then we should be seeing this type for
  // the first time, unless it's a declaration-only type class.
  if (!id.empty())
    {
      type_base_sptr t = ctxt.get_type_decl(id);
      if (t)
	{
	  class_decl_sptr c = as_non_member_class_decl(get_type_declaration(t));
	  assert((c && c->is_declaration_only())
		 || is_decl_only);
	}
    }
#endif

  if (!is_decl_only)
    decl.reset(new class_decl(name, size_in_bits, alignment_in_bits,
			      is_struct, loc, vis, bases, mbrs, data_mbrs,
			      mbr_functions));

  string def_id;
  bool is_def_of_decl = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "def-of-decl-id"))
    def_id = CHAR_STR(s);

  if (!def_id.empty())
    {
      shared_ptr<class_decl> d =
	dynamic_pointer_cast<class_decl>(ctxt.get_type_decl(def_id));
      if (d && d->get_is_declaration_only())
	{
	  is_def_of_decl = true;
	  decl->set_earlier_declaration(d);
	  d->set_definition_of_declaration(decl);
	}
    }

  assert(!is_decl_only || !is_def_of_decl);

  if (is_decl_only)
    decl.reset(new class_decl(name, is_struct));

  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);

  ctxt.map_xml_node_to_decl(node, decl);

  for (xmlNodePtr n = node->children; !is_decl_only && n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("base-class")))
	{
	  access_specifier access = private_access;
	  read_access(n, access);

	  string type_id;
	  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "type-id"))
	    type_id = CHAR_STR(s);
	  shared_ptr<class_decl> b =
	    dynamic_pointer_cast<class_decl>
	    (ctxt.build_or_get_type_decl(type_id, true));
	  assert(b);

	  size_t offset_in_bits = 0;
	  bool offset_present = read_offset_in_bits (n, offset_in_bits);

	  bool is_virtual = false;
	  read_is_virtual (n, is_virtual);

	  shared_ptr<class_decl::base_spec> base (new class_decl::base_spec
						  (b, access,
						   offset_present
						   ? (long) offset_in_bits
						   : -1,
						   is_virtual));
	  decl->add_base_specifier(base);
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-type")))
	{
	  access_specifier access = private_access;
	  read_access(n, access);

	  ctxt.map_xml_node_to_decl(n, decl);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<type_base> t =
		  build_type(ctxt, p, /*add_to_current_scope=*/false))
		{
		  if (!get_type_declaration(t)->get_scope())
		    {
		      type_base_sptr m =
			decl->add_member_type(t, access);

		      xml_char_sptr i= XML_NODE_GET_ATTRIBUTE(p, "id");
		      string id = CHAR_STR(i);
		      assert(!id.empty());
		      ctxt.key_type_decl(m, id, /*force=*/true);
		      ctxt.map_xml_node_to_decl(p, get_type_declaration(m));
		    }
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("data-member")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access = private_access;
	  read_access(n, access);

	  bool is_laid_out = false;
	  size_t offset_in_bits = 0;
	  if (read_offset_in_bits(n, offset_in_bits))
	    is_laid_out = true;

	  bool is_static = false;
	  read_static(n, is_static);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<var_decl> v =
		  build_var_decl(ctxt, p, /*add_to_current_scope=*/false))
		decl->add_data_member(v, access, is_laid_out,
				      is_static, offset_in_bits);
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-function")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access = private_access;
	  read_access(n, access);

	  bool is_virtual = false;
	  size_t vtable_offset = 0;
	  if (xml_char_sptr s =
	      XML_NODE_GET_ATTRIBUTE(n, "vtable-offset"))
	    {
	      is_virtual = true;
	      vtable_offset = atoi(CHAR_STR(s));
	    }

	  bool is_static = false;
	  read_static(n, is_static);

	  bool is_ctor = false, is_dtor = false, is_const = false;
	  read_cdtor_const(n, is_ctor, is_dtor, is_const);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (function_decl_sptr f =
		  build_function_decl(ctxt, p, decl,
				      /*add_to_current_scope=*/false))
		{
		  class_decl::method_decl_sptr m =
		    dynamic_pointer_cast<class_decl::method_decl>(f);
		  assert(m);
		  decl->add_member_function(m, access,
					    is_virtual,
					    vtable_offset,
					    is_static,
					    is_ctor, is_dtor,
					    is_const);
		  break;
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-template")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access = private_access;
	  read_access(n, access);

	  bool is_static = false;
	  read_static(n, is_static);

	  bool is_ctor = false, is_dtor = false, is_const = false;
	  read_cdtor_const(n, is_ctor, is_dtor, is_const);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (shared_ptr<function_tdecl> f =
		  build_function_tdecl(ctxt, p,
				       /*add_to_current_scope=*/false))
		{
		  shared_ptr<class_decl::member_function_template> m
		    (new class_decl::member_function_template(f, access,
							      is_static,
							      is_ctor,
							      is_const));
		  assert(!f->get_scope());
		  decl->add_member_function_template(m);
		}
	      else if (shared_ptr<class_tdecl> c =
		       build_class_tdecl(ctxt, p,
					 /*add_to_current_scope=*/false))
		{
		  shared_ptr<class_decl::member_class_template> m
		    (new class_decl::member_class_template(c, access,
							   is_static));
		  assert(!c->get_scope());
		  decl->add_member_class_template(m);
		}
	    }
	}
    }

  ctxt.pop_scope_or_abort(decl);

  if (decl)
    ctxt.key_type_decl(decl, id);

  return decl;
}

/// Build an intance of function_tdecl, from an
/// 'function-template-decl' xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return the newly built function_tdecl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<function_tdecl>
build_function_tdecl(read_context& ctxt,
		     const xmlNodePtr node,
		     bool add_to_current_scope)
{
  shared_ptr<function_tdecl> nil, result;

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

  function_tdecl_sptr fn_tmpl_decl(new function_tdecl(loc, vis, bind));

  ctxt.push_decl_to_current_scope(fn_tmpl_decl, add_to_current_scope);

  unsigned parm_index = 0;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (template_parameter_sptr parm =
	  build_template_parameter(ctxt, n, parm_index, fn_tmpl_decl))
	{
	  fn_tmpl_decl->add_template_parameter(parm);
	  ++parm_index;
	}
      else if (shared_ptr<function_decl> f =
	       build_function_decl(ctxt, n, shared_ptr<class_decl>(),
				   /*add_to_current_scope=*/true))
	fn_tmpl_decl->set_pattern(f);
    }

  ctxt.key_fn_tmpl_decl(fn_tmpl_decl, id);

  return fn_tmpl_decl;
}

/// Build an intance of class_tdecl, from a
/// 'class-template-decl' xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param add_to_current_scope if set to yes, the resulting of this
/// function is added to its current scope.
///
/// @return the newly built function_tdecl upon successful
/// completion, a null pointer otherwise.
static shared_ptr<class_tdecl>
build_class_tdecl(read_context&	ctxt,
		  const xmlNodePtr	node,
		  bool			add_to_current_scope)
{
  shared_ptr<class_tdecl> nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("class-template-decl")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (id.empty() || ctxt.get_class_tmpl_decl(id))
    return nil;

  location loc;
  read_location(ctxt, node, loc);

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  class_tdecl_sptr class_tmpl (new class_tdecl(loc, vis));

  ctxt.push_decl_to_current_scope(class_tmpl, add_to_current_scope);

  unsigned parm_index = 0;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (template_parameter_sptr parm=
	  build_template_parameter(ctxt, n, parm_index, class_tmpl))
	{
	  class_tmpl->add_template_parameter(parm);
	  ++parm_index;
	}
      else if (shared_ptr<class_decl> c =
	       build_class_decl(ctxt, n, add_to_current_scope))
	class_tmpl->set_pattern(c);
    }

  ctxt.key_class_tmpl_decl(class_tmpl, id);

  return class_tmpl;
}

/// Build a type_tparameter from a 'template-type-parameter'
/// xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param index the index (occurrence index, starting from 0) of the
/// template parameter.
///
/// @param tdecl the enclosing template declaration that holds the
/// template type parameter.
///
/// @return a pointer to a newly created instance of
/// type_tparameter, a null pointer otherwise.
static type_tparameter_sptr
build_type_tparameter(read_context&		ctxt,
		      const xmlNodePtr		node,
		      unsigned			index,
		      template_decl_sptr	tdecl)
{
  type_tparameter_sptr nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("template-type-parameter")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  if (!id.empty())
    assert(!ctxt.get_type_decl(id));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  if (!type_id.empty()
      && !(result = dynamic_pointer_cast<type_tparameter>
	   (ctxt.build_or_get_type_decl(type_id, true))))
    abort();

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node,loc);

  result.reset(new type_tparameter(index, tdecl, name, loc));

  if (id.empty())
    ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(result),
				    /*add_to_current_scope=*/true);
  else
    ctxt.push_and_key_type_decl(result, id, /*add_to_current_scope=*/true);

  return result;
}


/// Build a tmpl_parm_type_composition from a
/// "template-parameter-type-composition" xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param index the index of the previous normal template parameter.
///
/// @param tdecl the enclosing template declaration that holds this
/// template parameter type composition.
///
/// @return a pointer to a new instance of tmpl_parm_type_composition
/// upon successful completion, a null pointer otherwise.
static type_composition_sptr
build_type_composition(read_context&		ctxt,
		       const xmlNodePtr	node,
		       unsigned		index,
		       template_decl_sptr	tdecl)
{
  type_composition_sptr nil, result;

  if (!xmlStrEqual(node->name, BAD_CAST("template-parameter-type-composition")))
    return nil;

  type_base_sptr composed_type;
  result.reset(new type_composition(index, tdecl, composed_type));
  ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(result),
				  /*add_to_current_scope=*/true);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if ((composed_type =
	   build_pointer_type_def(ctxt, n,
				  /*add_to_current_scope=*/true))
	  ||(composed_type =
	     build_reference_type_def(ctxt, n,
				      /*add_to_current_scope=*/true))
	  ||(composed_type =
	     build_array_type_def(ctxt, n,
				  /*add_to_current_scope=*/true))
	  || (composed_type =
	      build_qualified_type_decl(ctxt, n,
					/*add_to_current_scope=*/true)))
	{
	  result->set_composed_type(composed_type);
	  break;
	}
    }

  return result;
}

/// Build an instance of non_type_tparameter from a
/// 'template-non-type-parameter' xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param index the index of the parameter.
///
/// @param tdecl the enclosing template declaration that holds this
/// non type template parameter.
///
/// @return a pointer to a newly created instance of
/// non_type_tparameter upon successful completion, a null
/// pointer code otherwise.
static non_type_tparameter_sptr
build_non_type_tparameter(read_context&	ctxt,
			  const xmlNodePtr	node,
			  unsigned		index,
			  template_decl_sptr	tdecl)
{
  non_type_tparameter_sptr r;

  if (!xmlStrEqual(node->name, BAD_CAST("template-non-type-parameter")))
    return r;

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  type_base_sptr type;
  if (type_id.empty()
      || !(type = ctxt.build_or_get_type_decl(type_id, true)))
    abort();

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node,loc);

  r.reset(new non_type_tparameter(index, tdecl, name, type, loc));
  ctxt.push_decl_to_current_scope(dynamic_pointer_cast<decl_base>(r),
				  /*add_to_current_scope=*/true);

  return r;
}

/// Build an intance of template_tparameter from a
/// 'template-template-parameter' xml element node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to parse from.
///
/// @param index the index of the template parameter.
///
/// @param tdecl the enclosing template declaration that holds this
/// template template parameter.
///
/// @return a pointer to a new instance of template_tparameter
/// upon successful completion, a null pointer otherwise.
static template_tparameter_sptr
build_template_tparameter(read_context&	ctxt,
			  const xmlNodePtr	node,
			  unsigned		index,
			  template_decl_sptr	tdecl)
{
  template_tparameter_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("template-template-parameter")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  // Bail out if a type with the same ID already exists.
  assert(!id.empty() && !ctxt.get_type_decl(id));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  // Bail out if no type with this ID exists.
  if (!type_id.empty()
      && !(dynamic_pointer_cast<template_tparameter>
	   (ctxt.build_or_get_type_decl(type_id, true))))
    abort();

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  template_tparameter_sptr result(new template_tparameter(index, tdecl,
							  name, loc));

  ctxt.push_decl_to_current_scope(result, /*add_to_current_scope=*/true);

  // Go parse template parameters that are children nodes
  int parm_index = 0;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (shared_ptr<template_parameter> p =
	  build_template_parameter(ctxt, n, parm_index, result))
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
/// @param ctxt the context of the parsing.
///
/// @param node the xml element node to parse from.
///
/// @param index the index of the template parameter we are parsing.
///
/// @param tdecl the enclosing template declaration that holds this
/// template parameter.
///
/// @return a pointer to a newly created instance of
/// template_parameter upon successful completion, a null pointer
/// otherwise.
static template_parameter_sptr
build_template_parameter(read_context&		ctxt,
			 const xmlNodePtr	node,
			 unsigned		index,
			 template_decl_sptr	tdecl)
{
  shared_ptr<template_parameter> r;
  ((r = build_type_tparameter(ctxt, node, index, tdecl))
   || (r = build_non_type_tparameter(ctxt, node, index, tdecl))
   || (r = build_template_tparameter(ctxt, node, index, tdecl))
   || (r = build_type_composition(ctxt, node, index, tdecl)));

  return r;
}

/// Build a type from an xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the type_base from.
///
/// @return a pointer to the newly built type_base upon successful
/// completion, a null pointer otherwise.
static shared_ptr<type_base>
build_type(read_context&	ctxt,
	   const xmlNodePtr	node,
	   bool		add_to_current_scope)
{
  shared_ptr<type_base> t;

  ((t = build_type_decl(ctxt, node, add_to_current_scope))
   || (t = build_qualified_type_decl(ctxt, node, add_to_current_scope))
   || (t = build_pointer_type_def(ctxt, node, add_to_current_scope))
   || (t = build_reference_type_def(ctxt, node , add_to_current_scope))
   || (t = build_array_type_def(ctxt, node, add_to_current_scope))
   || (t = build_enum_type_decl(ctxt, node, add_to_current_scope))
   || (t = build_typedef_decl(ctxt, node, add_to_current_scope))
   || (t = build_class_decl(ctxt, node, add_to_current_scope)));

  return t;
}

/// Parses 'type-decl' xml element.
///
/// @param ctxt the parsing context.
///
/// @return true upon successful parsing, false otherwise.
static decl_base_sptr
handle_type_decl(read_context&	ctxt,
		 xmlNodePtr	node,
		 bool		add_to_current_scope)
{
  type_decl_sptr decl = build_type_decl(ctxt, node, add_to_current_scope);
  return decl;
}

/// Parses 'namespace-decl' xml element.
///
/// @param ctxt the parsing context.
///
/// @return true upon successful parsing, false otherwise.
static decl_base_sptr
handle_namespace_decl(read_context&	ctxt,
		      xmlNodePtr	node,
		      bool		add_to_current_scope)
{
  namespace_decl_sptr d = build_namespace_decl(ctxt, node,
					       add_to_current_scope);
  return d;
}

/// Parse a qualified-type-def xml element.
///
/// @param ctxt the parsing context.
///
/// @return true upon successful parsing, false otherwise.
static decl_base_sptr
handle_qualified_type_decl(read_context&	ctxt,
			   xmlNodePtr		node,
			   bool		add_to_current_scope)
{
  qualified_type_def_sptr decl =
    build_qualified_type_decl(ctxt, node,
			      add_to_current_scope);
  return decl;
}

/// Parse a pointer-type-decl element.
///
/// @param ctxt the context of the parsing.
///
/// @return true upon successful completion, false otherwise.
static decl_base_sptr
handle_pointer_type_def(read_context&	ctxt,
			xmlNodePtr	node,
			bool		add_to_current_scope)
{
  pointer_type_def_sptr decl = build_pointer_type_def(ctxt, node,
						      add_to_current_scope);
  return decl;
}

/// Parse a reference-type-def element.
///
/// @param ctxt the context of the parsing.
///
/// reference_type_def is added to.
static decl_base_sptr
handle_reference_type_def(read_context& ctxt,
			  xmlNodePtr	node,
			  bool		add_to_current_scope)
{
  reference_type_def_sptr decl = build_reference_type_def(ctxt, node,
							  add_to_current_scope);
  return decl;
}

/// Parse a array-type-def element.
///
/// @param ctxt the context of the parsing.
///
/// array_type_def is added to.
static decl_base_sptr
handle_array_type_def(read_context&	ctxt,
		      xmlNodePtr	node,
		      bool		add_to_current_scope)
{
  array_type_def_sptr decl = build_array_type_def(ctxt, node,
						  add_to_current_scope);
  return decl;
}

/// Parse an enum-decl element.
///
/// @param ctxt the context of the parsing.
static decl_base_sptr
handle_enum_type_decl(read_context&	ctxt,
		      xmlNodePtr	node,
		      bool		add_to_current_scope)
{
  enum_type_decl_sptr decl = build_enum_type_decl(ctxt, node,
						  add_to_current_scope);
  return decl;
}

/// Parse a typedef-decl element.
///
/// @param ctxt the context of the parsing.
static decl_base_sptr
handle_typedef_decl(read_context&	ctxt,
		    xmlNodePtr		node,
		    bool		add_to_current_scope)
{
  typedef_decl_sptr decl = build_typedef_decl(ctxt, node,
					      add_to_current_scope);
  return decl;
}

/// Parse a var-decl element.
///
/// @param ctxt the context of the parsing.
///
/// @param node the node to read & parse from.
///
/// @param add_to_current_scope if set to yes, the resulting of this
/// function is added to its current scope.
static decl_base_sptr
handle_var_decl(read_context&	ctxt,
		xmlNodePtr	node,
		bool		add_to_current_scope)
{
  decl_base_sptr decl = build_var_decl(ctxt, node,
				       add_to_current_scope);
  return decl;
}

/// Parse a function-decl element.
///
/// @param ctxt the context of the parsing
///
/// @return true upon successful completion of the parsing, false
/// otherwise.
static decl_base_sptr
handle_function_decl(read_context&	ctxt,
		     xmlNodePtr	node,
		     bool		add_to_current_scope)
{
  function_decl_sptr fn = build_function_decl(ctxt, node,
					      class_decl_sptr(),
					      add_to_current_scope);
  return fn;
}

/// Parse a 'class-decl' xml element.
///
/// @param ctxt the context of the parsing.
///
/// @return true upon successful completion of the parsing, false
/// otherwise.
static decl_base_sptr
handle_class_decl(read_context& ctxt,
		  xmlNodePtr	node,
		  bool		add_to_current_scope)
{
  class_decl_sptr decl = build_class_decl(ctxt, node,
					  add_to_current_scope);
  return decl;
}

/// Parse a 'function-template-decl' xml element.
///
/// @param ctxt the parsing context.
///
/// @return true upon successful completion of the parsing, false
/// otherwise.
static decl_base_sptr
handle_function_tdecl(read_context&	ctxt,
		      xmlNodePtr	node,
		      bool		add_to_current_scope)
{
  function_tdecl_sptr d = build_function_tdecl(ctxt, node,
					       add_to_current_scope);
  return d;
}

/// Parse a 'class-template-decl' xml element.
///
/// @param ctxt the context of the parsing.
///
/// @return true upon successful completion, false otherwise.
static decl_base_sptr
handle_class_tdecl(read_context&	ctxt,
		   xmlNodePtr		node,
		   bool		add_to_current_scope)
{
  class_tdecl_sptr decl = build_class_tdecl(ctxt, node,
					    add_to_current_scope);
  return decl;
}

/// De-serialize a translation unit from an ABI Instrumentation xml
/// file coming from an input stream.
///
/// @param in a pointer to the input stream.
///
/// @param tu the translation unit resulting from the parsing.  This
/// is populated iff the function returns true.
///
/// @return true upon successful parsing, false otherwise.
bool
read_translation_unit_from_istream(istream* in,
				   translation_unit& tu)
{
  read_context read_ctxt(xml::new_reader_from_istream(in));
  return read_translation_unit_from_input(read_ctxt, tu);
}

/// De-serialize a translation unit from an ABI Instrumentation xml
/// file coming from an input stream.
///
/// @param in a pointer to the input stream.
///
/// @return a pointer to the resulting translation unit.
translation_unit_sptr
read_translation_unit_from_istream(istream* in)
{
  translation_unit_sptr result(new translation_unit(""));
  if (!read_translation_unit_from_istream(in, *result))
    return translation_unit_sptr();
  return result;
}

/// De-serialize a translation unit from an ABI Instrumentation XML
/// file at a given path.
///
/// @param file_path the path to the ABI Instrumentation XML file.
///
/// @return the deserialized translation or NULL if file_path could
/// not be read.  If file_path contains nothing, a non-null
/// translation_unit is returned, but with empty content.
translation_unit_sptr
read_translation_unit_from_file(const string& file_path)
{
  translation_unit_sptr result(new translation_unit(file_path));

  if (!xml_reader::read_translation_unit_from_file(result->get_path(), *result))
    return translation_unit_sptr();
  return result;
}

/// De-serialize a translation unit from an in-memory buffer
/// containing and ABI Instrumentation XML content.
///
/// @param buffer the buffer containing the ABI Instrumentation XML
/// content to parse.
///
/// @return the deserialized translation.
translation_unit_sptr
read_translation_unit_from_buffer(const std::string& buffer)
{
  translation_unit_sptr result(new translation_unit(""));

  if (!xml_reader::read_translation_unit_from_buffer(buffer, *result))
    return translation_unit_sptr();
  return result;
}

template<typename T>
struct array_deleter
{
  void
  operator()(T* a)
  {
    delete [] a;
  }
};//end array_deleter

#ifdef WITH_ZIP_ARCHIVE
/// Deserialize an ABI Instrumentation XML file at a given index in a
/// zip archive, and populate a given @ref translation_unit object
/// with the result of that de-serialization.
///
/// @param the @ref translation_unit to populate with the result of
/// the de-serialization.
///
/// @param ar the zip archive to read from.
///
/// @param file_index the index of the ABI Instrumentation XML file to
/// read from the zip archive.
///
/// @return true upon successful completion, false otherwise.
static bool
read_to_translation_unit(translation_unit& tu,
			 zip_sptr ar,
			 int file_index)
{
  if (!ar)
    return false;

  zip_file_sptr f = open_file_in_archive(ar, file_index);
  if (!f)
    return false;

  string input;
  {
    // Allocate a 64K byte buffer to read the archive.
    int buf_size = 64 * 1024;
    shared_ptr<char> buf(new char[buf_size + 1], array_deleter<char>());
    memset(buf.get(), 0, buf_size + 1);
    input.reserve(buf_size);

    while (zip_fread(f.get(), buf.get(), buf_size))
      {
	input.append(buf.get());
	memset(buf.get(), 0, buf_size + 1);
      }
  }

  if (!read_translation_unit_from_buffer(input, tu))
    return false;

  return true;
}

/// Read an ABI corpus from an archive file which is a ZIP archive of
/// several ABI Instrumentation XML files.
///
/// @param ar an object representing the archive file.
///
/// @param corp the ABI Corpus object to populate with the content of
/// the archive @ref ar.
///
/// @return the number of ABI Instrumentation file read from the
/// archive.
static int
read_corpus_from_archive(zip_sptr ar,
			 corpus_sptr& corp)
{
  if (!ar)
    return -1;

  int nb_of_tu_read = 0;
  int nb_entries = zip_get_num_entries(ar.get(), 0);
  if (nb_entries < 0)
    return -1;

  // TODO: ensure abi-info descriptor is present in the archive.  Read
  // it and ensure that version numbers match.
  for (int i = 0; i < nb_entries; ++i)
    {
      shared_ptr<translation_unit>
	tu(new translation_unit(zip_get_name(ar.get(), i, 0)));
      if (read_to_translation_unit(*tu, ar, i))
	{
	  if (!corp)
	    corp.reset(new corpus(""));
	  corp->add(tu);
	  ++nb_of_tu_read;
	}
    }
  if (nb_of_tu_read)
    corp->set_origin(corpus::NATIVE_XML_ORIGIN);
  return nb_of_tu_read;
}

/// Read an ABI corpus from an archive file which is a ZIP archive of
/// several ABI Instrumentation XML files.
///
/// @param corp the corpus to populate with the result of reading the
/// archive.
///
/// @param path the path to the archive file.
///
/// @return the number of ABI Instrument XML file read from the
/// archive, or -1 if the file could not read.
int
read_corpus_from_file(corpus_sptr& corp,
		      const string& path)
{
  if (path.empty())
    return -1;

  int error_code = 0;
  zip_sptr archive = open_archive(path, ZIP_CREATE|ZIP_CHECKCONS, &error_code);
  if (error_code)
    return -1;

  assert(archive);
  return read_corpus_from_archive(archive, corp);
}

/// Read an ABI corpus from an archive file which is a ZIP archive of
/// several ABI Instrumentation XML files.
///
/// @param corp the corpus to populate with the result of reading the
/// archive.  The archive file to consider is corp.get_path().
///
/// @return the number of ABI Instrument XML file read from the
/// archive.
int
read_corpus_from_file(corpus_sptr& corp)
{return read_corpus_from_file(corp, corp->get_path());}

/// Read an ABI corpus from an archive file which is a ZIP archive of
/// several ABI Instrumentation XML files.
///
/// @param path the path to the archive file.
///
/// @return the resulting corpus object, or NULL if the file could not
/// be read.
corpus_sptr
read_corpus_from_file(const string& path)
{
  if (path.empty())
    return corpus_sptr();

  corpus_sptr corp(new corpus(path));
  if (read_corpus_from_file(corp, path) < 0)
    return corpus_sptr();

  return corp;
}

#endif //WITH_ZIP_ARCHIVE

/// De-serialize an ABI corpus from an input XML document which root
/// node is 'abi-corpus'.
///
/// @param in the input stream to read the XML document from.
///
/// @param corp the corpus de-serialized from the parsing.  This is
/// set iff the function returns true.
///
/// @return the resulting corpus de-serialized from the parsing.  This
/// is non-null iff the parsing resulted in a valid corpus.
corpus_sptr
read_corpus_from_native_xml(std::istream* in)
{
  read_context read_ctxt(xml::new_reader_from_istream(in));
  corpus_sptr corp(new corpus(""));
  read_ctxt.set_corpus(corp);
  return read_corpus_from_input(read_ctxt);
}

/// De-serialize an ABI corpus from an XML document file which root
/// node is 'abi-corpus'.
///
/// @param path the path to the input file to read the XML document
/// from.
///
/// @param corp the corpus de-serialized from the parsing.  This is
/// set iff the function returns true.
///
/// @return the resulting corpus de-serialized from the parsing.  This
/// is non-null if the parsing successfully resulted in a corpus.
corpus_sptr
read_corpus_from_native_xml_file(const string& path)
{
  read_context read_ctxt(xml::new_reader_from_file(path));
  corpus_sptr corp = read_corpus_from_input(read_ctxt);
  if (corp)
    {
      if (corp->get_path().empty())
	corp->set_path(path);
    }
  return corp;
}

}//end namespace xml_reader

}//end namespace abigail
