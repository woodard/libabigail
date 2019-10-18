// -*- mode: C++ -*-
//
// Copyright (C) 2013-2019 Red Hat, Inc.
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
/// ABI Instrumentation file in libabigail native XML format.  This
/// native XML format is named "abixml".

#include "config.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <deque>
#include <assert.h>
#include <sstream>
#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>

#include "abg-cxx-compat.h"
#include "abg-suppression-priv.h"

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-libxml-utils.h"
#include "abg-reader.h"
#include "abg-corpus.h"

#ifdef WITH_ZIP_ARCHIVE
#include "abg-libzip-utils.h"
#endif

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

using xml::xml_char_sptr;

/// The namespace for the native XML file format reader.
namespace xml_reader
{
using std::string;
using std::deque;
using abg_compat::shared_ptr;
using abg_compat::unordered_map;
using abg_compat::dynamic_pointer_cast;
using std::vector;
using std::istream;
#ifdef WITH_ZIP_ARCHIVE
using zip_utils::zip_sptr;
using zip_utils::zip_file_sptr;
using zip_utils::open_archive;
using zip_utils::open_file_in_archive;
#endif //WITH_ZIP_ARCHIVE

static bool	read_is_declaration_only(xmlNodePtr, bool&);

class read_context;

/// This abstracts the context in which the current ABI
/// instrumentation dump is being de-serialized.  It carries useful
/// information needed during the de-serialization, but that does not
/// make sense to be stored in the final resulting in-memory
/// representation of ABI Corpus.
class read_context
{
public:

  typedef unordered_map<string,
			vector<type_base_sptr> >::const_iterator
  const_types_map_it;

  typedef unordered_map<string,
			vector<type_base_sptr> >::iterator
  types_map_it;

  typedef unordered_map<string,
			shared_ptr<function_tdecl> >::const_iterator
  const_fn_tmpl_map_it;

  typedef unordered_map<string,
			shared_ptr<class_tdecl> >::const_iterator
  const_class_tmpl_map_it;

  typedef unordered_map<string, xmlNodePtr> string_xml_node_map;

  typedef unordered_map<xmlNodePtr, decl_base_sptr> xml_node_decl_base_sptr_map;

private:
  string						m_path;
  environment*						m_env;
  unordered_map<string, vector<type_base_sptr> >	m_types_map;
  unordered_map<string, shared_ptr<function_tdecl> >	m_fn_tmpl_map;
  unordered_map<string, shared_ptr<class_tdecl> >	m_class_tmpl_map;
  unordered_map<string, size_t>			m_wip_types_map;
  vector<type_base_sptr>				m_types_to_canonicalize;
  string_xml_node_map					m_id_xml_node_map;
  xml_node_decl_base_sptr_map				m_xml_node_decl_map;
  xml::reader_sptr					m_reader;
  xmlNodePtr						m_corp_node;
  deque<shared_ptr<decl_base> >			m_decls_stack;
  corpus_sptr						m_corpus;
  corpus_group_sptr					m_corpus_group;
  corpus::exported_decls_builder*			m_exported_decls_builder;
  suppr::suppressions_type				m_supprs;

  read_context();

public:
  read_context(xml::reader_sptr reader,
	       environment*	env)
    : m_env(env),
      m_reader(reader),
      m_corp_node(),
      m_exported_decls_builder()
  {}

  /// Getter of the path to the ABI file.
  ///
  /// @return the path to the native xml abi file.
  const string&
  get_path() const
  {return m_path;}

  /// Setter of the path to the ABI file.
  ///
  /// @param the new path to the native ABI file.
  void
  set_path(const string& s)
  {m_path = s;}

  /// Getter for the environment of this reader.
  ///
  /// @return the environment of this reader.
  const environment*
  get_environment() const
  {return m_env;}

  /// Getter for the environment of this reader.
  ///
  /// @return the environment of this reader.
  environment*
  get_environment()
  {return m_env;}

  /// Setter for the environment of this reader.
  ///
  /// @param env the environment of this reader.
  void
  set_environment(environment* env)
  {m_env = env;}

  xml::reader_sptr
  get_reader() const
  {return m_reader;}

  xmlNodePtr
  get_corpus_node() const
  {return m_corp_node;}

  void
  set_corpus_node(xmlNodePtr node)
  {m_corp_node = node;}

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

    string_xml_node_map::iterator i = get_id_xml_node_map().find(id);
    if (i != get_id_xml_node_map().end())
      {
	bool is_declaration = false;
	read_is_declaration_only(node, is_declaration);
	if (is_declaration)
	  i->second = node;
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
  get_scope_for_node(xmlNodePtr node,
		     access_specifier& access);

  // This is defined later, after build_type() is declared, because it
  // uses it.
  type_base_sptr
  build_or_get_type_decl(const string& id,
			 bool add_decl_to_scope);

  /// Return the first type already seen, that is identified by a
  /// given ID.
  ///
  /// Note that for a type to be "identified" by id, the function
  /// key_type_decl must have been previously called with that type
  /// and with id.
  ///
  /// @param id the id to consider.
  ///
  /// @return the type identified by the unique id id, or a null
  /// pointer if no type has ever been associated with id before.
  type_base_sptr
  get_type_decl(const string& id) const
  {
    const_types_map_it i = m_types_map.find(id);
    if (i == m_types_map.end())
      return type_base_sptr();
    type_base_sptr result = i->second[0];
    return result;
  }

  /// Return the vector of types already seen, that are identified by
  /// a given ID.
  ///
  /// Note that for a type to be "identified" by id, the function
  /// key_type_decl must have been previously called with that type
  /// and with id.
  ///
  /// @param id the id to consider.
  ///
  /// @return thevector of types already seen, that are identified by
  /// a given ID, or 0 if no type has ever been associated with @p id
  /// before.
  const vector<type_base_sptr>*
  get_all_type_decls(const string& id) const
  {
    const_types_map_it i = m_types_map.find(id);
    if (i == m_types_map.end())
      return 0;
    else
      return &i->second;
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
  get_cur_scope() const
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
    for (deque<shared_ptr<decl_base> >::reverse_iterator i =
	   m_decls_stack.rbegin();
	 i != m_decls_stack.rend();
	 ++i)
      if (decl_base_sptr d = *i)
	if ((global = get_global_scope(d)))
	  break;

    if (global)
      return global->get_translation_unit();

    return 0;
  }

  /// Test if a given type is from the current translation unit.
  ///
  /// @param type the type to consider.
  ///
  /// @return true iff the type is from the current translation unit.
  bool
  type_is_from_translation_unit(type_base_sptr type)
  {
    decl_base_sptr d = get_type_declaration(type);
    if (d)
      return (ir::get_translation_unit(d) == get_translation_unit());
    else if (function_type_sptr fn_type = is_function_type(type))
      return bool(lookup_function_type(fn_type, *get_translation_unit()));
    else
      return false;
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
  {ABG_ASSERT(pop_scope(scope));}

  void
  clear_decls_stack()
  {m_decls_stack.clear();}

  void
  clear_type_map()
  {m_types_map.clear();}

  /// Clean the vector of types to canonicalize after the translation
  /// unit has been read.
  void
  clear_types_to_canonicalize()
  {m_types_to_canonicalize.clear();}

  /// Clean the map of classes that are "Work In Progress"; that is,
  /// the map of the class that are currently being built, but at not
  /// yet fully built.
  void
  clear_wip_classes_map()
  {m_wip_types_map.clear();}

  /// Mark a given type as being "Work In Progress"; that is, mark it
  /// as being currently built.
  ///
  /// @param t the class to mark as being "Work In Progress".
  void
  mark_type_as_wip(const type_base_sptr t)
  {
    if (!t)
      return;
    string qname = get_type_name(t, /*qualified=*/true);
    unordered_map<string, size_t>::iterator it = m_wip_types_map.find(qname);
    if (it == m_wip_types_map.end())
      m_wip_types_map[qname] = 1;
    else
      ++it->second;
  }

  /// Mark a given class as being *NOT* "Work In Progress" anymore;
  /// that is, mark it as being fully built.
  ///
  /// @param t the type to mark as being built.
  void
  unmark_type_as_wip(const type_base_sptr t)
  {
    if (!t)
      return;

    string qname = get_type_name(t, /*qualified=*/true);
    unordered_map<string, size_t>::iterator it = m_wip_types_map.find(qname);
    if (it == m_wip_types_map.end())
      return;
    if (it->second)
      --it->second;
    if (it->second == 0)
      m_wip_types_map.erase(it);
  }

  /// Test if a type is being currently built; that is, if it's "Work
  /// In Progress".
  ///
  /// @param t the type to consider.
  bool
  is_wip_type(const type_base_sptr t) const
  {
    if (!t)
      return false;

    string qname = get_type_name(t, /*qualified=*/true);
    unordered_map<string, size_t>::const_iterator i =
      m_wip_types_map.find(qname);
    return i != m_wip_types_map.end();
  }

  /// Test if two types are equal, without comparing them structurally.
  ///
  /// This either tests that type pointers are equal, or it tests
  /// their names.  This is because it might be two early to compare
  /// types structurally because we are not necessarily done building
  /// them yet.
  ///
  /// @param t1 the first type to compare.
  ///
  /// @param t2 the second type to compare.
  ///
  /// @return true iff the types are equal.
  bool
  types_equal(type_base_sptr t1, type_base_sptr t2)
  {
    if (t1.get() == t2.get())
      return true;

    // We are going to test qualified names only if both types have
    // already been added to their scope.
    bool qualified = (get_type_scope(t1) && get_type_scope(t2));

    return (get_type_name(t1, qualified)
	    == get_type_name(t2, qualified));
  }

  /// Associate an ID with a type.
  ///
  /// @param type the type to associate witht he ID.
  ///
  /// @param id the ID to associate to the type.
  ///
  /// @return true upon successful completion.
  bool
  key_type_decl(shared_ptr<type_base> type, const string& id)
  {
    if (!type)
      return false;

    type_base_sptr t = type;

    types_map_it i = m_types_map.find(id);
    if (i != m_types_map.end())
      i->second.push_back(type);
    else
      {
	vector<type_base_sptr> types;
	types.push_back(type);
	m_types_map[id] = types;
      }

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
    ABG_ASSERT(fn_tmpl_decl);

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
    ABG_ASSERT(class_tmpl_decl);

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
  push_decl_to_current_scope(decl_base_sptr decl,
			     bool add_to_current_scope)
  {
    ABG_ASSERT(decl);

    if (add_to_current_scope)
      add_decl_to_scope(decl, get_cur_scope());
    if (!decl->get_translation_unit())
      decl->set_translation_unit(get_translation_unit());
    ABG_ASSERT(decl->get_translation_unit());
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
    ABG_ASSERT(decl);

    push_decl_to_current_scope(decl, add_to_current_scope);
    if (!t->get_translation_unit())
      t->set_translation_unit(get_translation_unit());
    ABG_ASSERT(t->get_translation_unit());
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

  /// Getter of the current corpus group.
  ///
  /// @return the current corpus group.n
  const corpus_group_sptr&
  get_corpus_group() const
  {return m_corpus_group;}

  /// Getter of the current corpus group.
  ///
  /// @return the current corpus group.
  corpus_group_sptr&
  get_corpus_group()
  {return m_corpus_group;}

  /// Setter of the corpus_group
  ///
  /// @param group the new corpus group.
  void
  set_corpus_group(const corpus_group_sptr& group)
  {m_corpus_group = group;}

  /// Getter for the object that determines if a given declaration
  /// ought to be put in the set of exported decls of the current
  /// corpus.
  ///
  /// @return the exported decls builder.
  corpus::exported_decls_builder*
  get_exported_decls_builder()
  {return m_exported_decls_builder;}

  /// Setter for the object that determines if a given declaration
  /// ought to be put in the set of exported decls of the current
  /// corpus.
  ///
  /// @param d the new exported decls builder.
  ///
  /// @return the exported decls builder.
  void
  set_exported_decls_builder(corpus::exported_decls_builder* d)
  {m_exported_decls_builder = d;}

  /// Getter of the vector of the suppression specifications
  /// associated to the current read context.
  ///
  /// @return the vector of suppression specifications.
  suppr::suppressions_type&
  get_suppressions()
  {return m_supprs;}

  /// Getter of the vector of the suppression specifications
  /// associated to the current read context.
  ///
  /// @return the vector of suppression specifications.
  const suppr::suppressions_type&
  get_suppressions() const
  {return const_cast<read_context*>(this)->get_suppressions();}

  /// Add a given function to the set of exported functions of the
  /// current corpus, if the function satisfies the different
  /// constraints requirements.
  ///
  /// @param fn the function to consider.
  void
  maybe_add_fn_to_exported_decls(function_decl* fn)
  {
    if (fn)
      if (corpus::exported_decls_builder* b = get_exported_decls_builder())
	b->maybe_add_fn_to_exported_fns(fn);
  }

  /// Add a given variable to the set of exported functions of the
  /// current corpus, if the function satisfies the different
  /// constraints requirements.
  ///
  /// @param var the variable to consider.
  void
  maybe_add_var_to_exported_decls(var_decl* var)
  {
    if (var)
      if (corpus::exported_decls_builder* b = get_exported_decls_builder())
	b->maybe_add_var_to_exported_vars(var);
  }

  /// Clear all the data that must absolutely be cleared at the end of
  /// the parsing of a translation unit.
  void
  clear_per_translation_unit_data()
  {
  }

  /// Clear all the data that must absolutely be cleared at the end of
  /// the parsing of an ABI corpus.
  void
  clear_per_corpus_data()
  {
    clear_type_map();
    clear_types_to_canonicalize();
    clear_xml_node_decl_map();
    clear_id_xml_node_map();
    clear_decls_stack();
  }

  /// Test if a type should be canonicalized early.  If so,
  /// canonicalize it right away.  Otherwise, schedule it for late
  /// canonicalizing; that is, schedule it so that it's going to be
  /// canonicalized when the translation unit is fully read.
  ///
  /// @param t the type to consider for canonicalizing.
  void
  maybe_canonicalize_type(type_base_sptr t,
			  bool force_delay = false)
  {
    if (!t)
      return;

    if (t->get_canonical_type())
      return;

    // If this class has some non-canonicalized sub type, then wait
    // for the when we've read all the translation unit to
    // canonicalize all of its non-canonicalized sub types and then we
    // can canonicalize this one.
    //
    // Also, if this is a declaration-only class, wait for the end of
    // the translation unit reading so that we have its definition and
    // then we'll use that for canonicalizing it.
    if (!force_delay
	&& !type_has_non_canonicalized_subtype(t)
	&& !is_class_type(t)
	&& !is_union_type(t)
	&& !is_wip_type(t)
	// Below are types that *must* be canonicalized only after
	// they are added to their context; but then this function
	// might be called to early, before they are actually added to
	// their context.
	//
	// TODO: make sure this function is called after types are
	// added to their context, so that we can try to
	// early-canonicalize some of these types, reducing the size
	// of the set of types to put on the side, waiting for being
	// canonicalized.
	&& !is_method_type(t)
	&& !is_reference_type(t)
	&& !is_pointer_type(t)
	&& !is_qualified_type(t)
	&& !is_typedef(t)
	&& !is_enum_type(t)
	&& !is_function_type(t))
      canonicalize(t);
    else
      {
	// We do not want to try to canonicalize a class type that
	// hasn't been properly added to its context.
	if (class_decl_sptr c = is_class_type(t))
	  ABG_ASSERT(c->get_scope());

	schedule_type_for_late_canonicalizing(t);
      }
  }

  /// Schedule a type for being canonicalized after the current
  /// translation unit is read.
  ///
  /// @param t the type to consider for canonicalization.
  void
  schedule_type_for_late_canonicalizing(type_base_sptr t)
  {m_types_to_canonicalize.push_back(t);}

  /// Perform the canonicalizing of types that ought to be done after
  /// the current translation unit is read.  This function is called
  /// when the current corpus is fully built.
  void
  perform_late_type_canonicalizing()
  {
    for (vector<type_base_sptr>::iterator i = m_types_to_canonicalize.begin();
	 i != m_types_to_canonicalize.end();
	 ++i)
      canonicalize(*i);
  }

  /// Test whether if a given function suppression matches a function
  /// designated by a regular expression that describes its name.
  ///
  /// @param s the suppression specification to evaluate to see if it
  /// matches a given function name.
  ///
  /// @param fn_name the name of the function of interest.  Note that
  /// this name must be *non* qualified.
  ///
  /// @return true iff the suppression specification @p s matches the
  /// function whose name is @p fn_name.
  bool
  suppression_matches_function_name(const suppr::function_suppression_sptr& s,
				    const string& fn_name) const
  {
    if (!s)
      return false;
    return suppression_matches_function_name(*s, fn_name);
  }

  /// Tests if a suppression specification can match ABI artifacts
  /// coming from the ABI corpus being analyzed.
  ///
  /// This tests if the suppression matches the soname of and binary
  /// name of the corpus being analyzed.
  ///
  /// @param s the suppression specification to consider.
  bool
  suppression_can_match(const suppr::suppression_base& s) const
  {
    corpus_sptr corp = get_corpus();
    if (s.priv_->matches_soname(corp->get_soname())
	&& s.priv_->matches_binary_name(corp->get_path()))
      return true;
    return false;
  }

  /// Test whether if a given function suppression matches a function
  /// designated by a regular expression that describes its name.
  ///
  /// @param s the suppression specification to evaluate to see if it
  /// matches a given function name.
  ///
  /// @param fn_name the name of the function of interest.  Note that
  /// this name must be *non* qualified.
  ///
  /// @return true iff the suppression specification @p s matches the
  /// function whose name is @p fn_name.
  bool
  suppression_matches_function_name(const suppr::function_suppression& s,
				    const string& fn_name) const
  {
    if (!s.get_drops_artifact_from_ir()
	|| !suppression_can_match(s))
      return false;

    return suppr::suppression_matches_function_name(s, fn_name);
  }

  /// Test whether if a given function suppression matches a function
  /// designated by a regular expression that describes its linkage
  /// name (symbol name).
  ///
  /// @param s the suppression specification to evaluate to see if it
  /// matches a given function linkage name
  ///
  /// @param fn_linkage_name the linkage name of the function of interest.
  ///
  /// @return true iff the suppression specification @p s matches the
  /// function whose linkage name is @p fn_linkage_name.
  bool
  suppression_matches_function_sym_name(const suppr::function_suppression_sptr& s,
					const string& fn_linkage_name) const
  {
    if (!s)
      return false;
    return suppression_matches_function_sym_name(*s, fn_linkage_name);
  }

  /// Test whether if a given function suppression matches a function
  /// designated by a regular expression that describes its linkage
  /// name (symbol name).
  ///
  /// @param s the suppression specification to evaluate to see if it
  /// matches a given function linkage name
  ///
  /// @param fn_linkage_name the linkage name of the function of interest.
  ///
  /// @return true iff the suppression specification @p s matches the
  /// function whose linkage name is @p fn_linkage_name.
  bool
  suppression_matches_function_sym_name(const suppr::function_suppression& s,
					const string& fn_linkage_name) const
  {
    if (!s.get_drops_artifact_from_ir()
	|| !suppression_can_match(s))
      return false;

    return suppr::suppression_matches_function_sym_name(s, fn_linkage_name);
  }

  /// Test whether if a given variable suppression specification
  /// matches a variable denoted by its name.
  ///
  /// @param s the variable suppression specification to consider.
  ///
  /// @param var_name the name of the variable to consider.
  ///
  /// @return true iff the suppression specification @p s matches the
  /// variable whose name is @p var_name.
  bool
  suppression_matches_variable_name(const suppr::variable_suppression& s,
				    const string& var_name) const
  {
    if (!s.get_drops_artifact_from_ir()
	|| !suppression_can_match(s))
      return false;

    return suppr::suppression_matches_variable_name(s, var_name);
  }

  /// Test whether if a given variable suppression specification
  /// matches a variable denoted by its linkage name.
  ///
  /// @param s the variable suppression specification to consider.
  ///
  /// @param var_linkage_name the linkage name of the variable to consider.
  ///
  /// @return true iff variable suppression specification @p s matches
  /// the variable denoted by linkage name @p var_linkage_name.
  bool
  suppression_matches_variable_sym_name(const suppr::variable_suppression& s,
					const string& var_linkage_name) const
  {
    if (!s.get_drops_artifact_from_ir()
	|| !suppression_can_match(s))
      return false;

    return suppr::suppression_matches_variable_sym_name(s, var_linkage_name);
  }

  /// Test if a given type suppression specification matches a type
  /// designated by its name and location.
  ///
  /// @param s the suppression specification to consider.
  ///
  /// @param type_name the fully qualified type name to consider.
  ///
  /// @param type_location the type location to consider.
  ///
  /// @return true iff the type suppression specification matches a
  /// type of a given name and location.
  bool
  suppression_matches_type_name_or_location(const suppr::type_suppression& s,
					    const string& type_name,
					    const location& type_location) const
  {
    if (!suppression_can_match(s))
      return false;

    return suppr::suppression_matches_type_name_or_location(s, type_name,
							    type_location);
  }
};// end class read_context

static int	advance_cursor(read_context&);
static bool
read_translation_unit(read_context&, translation_unit&, xmlNodePtr);
static translation_unit_sptr
get_or_read_and_add_translation_unit(read_context&, xmlNodePtr);
static translation_unit_sptr read_translation_unit_from_input(read_context&);
static bool	read_symbol_db_from_input(read_context&,
					  string_elf_symbols_map_sptr&,
					  string_elf_symbols_map_sptr&);
static bool	read_location(const read_context&, xmlNodePtr, location&);
static bool	read_visibility(xmlNodePtr, decl_base::visibility&);
static bool	read_binding(xmlNodePtr, decl_base::binding&);
static bool	read_access(xmlNodePtr, access_specifier&);
static bool	read_size_and_alignment(xmlNodePtr, size_t&, size_t&);
static bool	read_static(xmlNodePtr, bool&);
static bool	read_offset_in_bits(xmlNodePtr, size_t&);
static bool	read_cdtor_const(xmlNodePtr, bool&, bool&, bool&);
static bool	read_is_virtual(xmlNodePtr, bool&);
static bool	read_is_struct(xmlNodePtr, bool&);
static bool	read_is_anonymous(xmlNodePtr, bool&);
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
build_elf_symbol(read_context&, const xmlNodePtr, bool);

static elf_symbol_sptr
build_elf_symbol_from_reference(read_context&, const xmlNodePtr,
				bool);

static string_elf_symbols_map_sptr
build_elf_symbol_db(read_context&, const xmlNodePtr, bool);

static function_decl::parameter_sptr
build_function_parameter (read_context&, const xmlNodePtr);

static function_decl_sptr
build_function_decl(read_context&, const xmlNodePtr,
		    class_or_union_sptr, bool);

static function_decl_sptr
build_function_decl_if_not_suppressed(read_context&, const xmlNodePtr,
				      class_or_union_sptr, bool);

static bool
function_is_suppressed(const read_context& ctxt,
		       xmlNodePtr node);

static var_decl_sptr
build_var_decl_if_not_suppressed(read_context&, const xmlNodePtr, bool);

static var_decl_sptr
build_var_decl(read_context&, const xmlNodePtr, bool);

static bool
variable_is_suppressed(const read_context& ctxt,
		       xmlNodePtr node);

static shared_ptr<type_decl>
build_type_decl(read_context&, const xmlNodePtr, bool);

static qualified_type_def_sptr
build_qualified_type_decl(read_context&, const xmlNodePtr, bool);

static shared_ptr<pointer_type_def>
build_pointer_type_def(read_context&, const xmlNodePtr, bool);

static shared_ptr<reference_type_def>
build_reference_type_def(read_context&, const xmlNodePtr, bool);

static shared_ptr<function_type>
build_function_type(read_context&, const xmlNodePtr, bool);

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

static union_decl_sptr
build_union_decl(read_context&, const xmlNodePtr, bool);

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

static type_or_decl_base_sptr	handle_element_node(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_type_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_namespace_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_qualified_type_decl(read_context&,
						   xmlNodePtr, bool);
static decl_base_sptr	handle_pointer_type_def(read_context&,
						xmlNodePtr, bool);
static decl_base_sptr	handle_reference_type_def(read_context&,
						  xmlNodePtr, bool);
static type_base_sptr	handle_function_type(read_context&,
					     xmlNodePtr, bool);
static decl_base_sptr	handle_array_type_def(read_context&,
					      xmlNodePtr, bool);
static decl_base_sptr	handle_enum_type_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_typedef_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_var_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_function_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_class_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_union_decl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_function_tdecl(read_context&, xmlNodePtr, bool);
static decl_base_sptr	handle_class_tdecl(read_context&, xmlNodePtr, bool);

/// Get the IR node representing the scope for a given XML node.
///
/// This function might trigger the building of a full sub-tree of IR.
///
/// @param node the XML for which to return the scope decl.  If its
/// parent XML node has no corresponding IR node, that IR node is constructed.
///
/// @param access the access specifier of the node in its scope, if
/// applicable.  If the node doesn't have any access specifier
/// provided in its scope, then the parameter is set to no_access.
///
/// @return the IR node representing the scope of the IR node for the
/// XML node given in argument.
scope_decl_sptr
read_context::get_scope_for_node(xmlNodePtr node,
				 access_specifier& access)
{
  scope_decl_sptr nil, scope;
  if (!node)
    return nil;

  xmlNodePtr parent = node->parent;
  access = no_access;
  if (parent
      && (xmlStrEqual(parent->name, BAD_CAST("data-member"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-type"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-function"))
	  || xmlStrEqual(parent->name, BAD_CAST("member-template"))))
    {
      read_access(parent, access);
      parent = parent->parent;
    }

  xml_node_decl_base_sptr_map::const_iterator i =
    get_xml_node_decl_map().find(parent);
  if (i == get_xml_node_decl_map().end())
    {
      if (xmlStrEqual(parent->name, BAD_CAST("abi-instr")))
	{
	  translation_unit_sptr tu =
	    get_or_read_and_add_translation_unit(*this, parent);
	  return tu->get_global_scope();
	}

      access_specifier a = no_access;
      scope_decl_sptr parent_scope = get_scope_for_node(parent, a);
      push_decl(parent_scope);
      scope = dynamic_pointer_cast<scope_decl>
	(handle_element_node(*this, parent, /*add_decl_to_scope=*/true));
      ABG_ASSERT(scope);
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
      ABG_ASSERT(n);

      scope_decl_sptr scope;
      access_specifier access = no_access;
      if (add_decl_to_scope)
	{
	  scope = get_scope_for_node(n, access);
	  /// In some cases, if for instance the scope of 'n' is a
	  /// namespace, get_scope_for_node() can trigger the building
	  /// of what is underneath of the namespace, if that has not
	  /// already been done.  So after that, the IR node for 'n'
	  /// might have been built; let's try to see if we are in
	  /// that case.  Otherwise, we'll just build the IR node for
	  /// 'n' ourselves.
	  if ((t = get_type_decl(id)))
	    return t;
	  ABG_ASSERT(scope);
	  push_decl(scope);
	}

      t = build_type(*this, n, add_decl_to_scope);
      ABG_ASSERT(t);
      if (is_member_type(t) && access != no_access)
	{
	  ABG_ASSERT(add_decl_to_scope);
	  decl_base_sptr d = get_type_declaration(t);
	  ABG_ASSERT(d);
	  set_member_access_specifier(d, access);
	}
      map_xml_node_to_decl(n, get_type_declaration(t));

      if (add_decl_to_scope)
	pop_scope_or_abort(scope);

      maybe_canonicalize_type(t, !add_decl_to_scope);
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
///
/// @param ctxt the context of the reader.
///
/// @param node the XML sub-tree node to walk.  It must be an element
/// node.
static void
walk_xml_node_to_map_type_ids(read_context& ctxt,
			      xmlNodePtr node)
{
  xmlNodePtr n = node;

  if (!n || n->type != XML_ELEMENT_NODE)
    return;

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "id"))
    {
      string id = CHAR_STR(s);
      ctxt.map_id_and_node(id, n);
    }

  for (n = n->children; n; n = n->next)
    walk_xml_node_to_map_type_ids(ctxt, n);
}

static bool
read_translation_unit(read_context& ctxt, translation_unit& tu, xmlNodePtr node)
{
  tu.set_corpus(ctxt.get_corpus().get());

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

  xml::xml_char_sptr comp_dir_path_str =
    XML_NODE_GET_ATTRIBUTE(node, "comp-dir-path");
  if (comp_dir_path_str)
    tu.set_compilation_dir_path(reinterpret_cast<char*>
				(comp_dir_path_str.get()));

  xml::xml_char_sptr language_str = XML_NODE_GET_ATTRIBUTE(node, "language");
  if (language_str)
    tu.set_language(string_to_translation_unit_language
		     (reinterpret_cast<char*>(language_str.get())));


  // We are at global scope, as we've just seen the top-most
  // "abi-instr" element.
  ctxt.push_decl(tu.get_global_scope());
  ctxt.map_xml_node_to_decl(node, tu.get_global_scope());

  if (ctxt.get_id_xml_node_map().empty()
      || !ctxt.get_corpus())
    walk_xml_node_to_map_type_ids(ctxt, node);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;
      handle_element_node(ctxt, n, /*add_decl_to_scope=*/true);
    }

  ctxt.pop_scope_or_abort(tu.get_global_scope());

  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  ctxt.clear_per_translation_unit_data();

  return true;
}

/// Read a given xml node representing a tranlsation unit.
///
/// If the current corpus already contains a translation unit of the
/// path of the xml node we need to look at, then return that
/// translation unit.  Otherwise, read the translation unit, build a
/// @ref translation_unit out of it, add it to the current corpus and
/// return it.
///
/// @param ctxt the read context.
///
/// @param node the XML node to consider.
///
/// @return the resulting translation unit.
static translation_unit_sptr
get_or_read_and_add_translation_unit(read_context& ctxt, xmlNodePtr node)
{
  corpus_sptr corp = ctxt.get_corpus();

  translation_unit_sptr tu;
  string tu_path;
  xml::xml_char_sptr path_str = XML_NODE_GET_ATTRIBUTE(node, "path");

  if (path_str)
    {
      tu_path = reinterpret_cast<char*>(path_str.get());
      ABG_ASSERT(!tu_path.empty());

      if (corp)
	tu = corp->find_translation_unit(tu_path);

      if (tu)
	return tu;
    }

  tu.reset(new translation_unit(ctxt.get_environment(), tu_path));
  if (corp)
    corp->add(tu);

  if (read_translation_unit(ctxt, *tu, node))
    return tu;

  return translation_unit_sptr();
}

/// Parse the input XML document containing a translation_unit,
/// represented by an 'abi-instr' element node, associated to the current
/// context.
///
/// @param ctxt the current input context
///
/// @return the translation unit resulting from the parsing upon
/// successful completion, or nil.
static translation_unit_sptr
read_translation_unit_from_input(read_context&	ctxt)
{
  translation_unit_sptr tu, nil;

  xmlNodePtr node = ctxt.get_corpus_node();
  if (!node)
    {
      xml::reader_sptr reader = ctxt.get_reader();
      if (!reader)
	return nil;

      // The document must start with the abi-instr node.
      int status = 1;
      while (status == 1
	     && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
	status = advance_cursor (ctxt);

      if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				       BAD_CAST("abi-instr")))
	return nil;

      node = xmlTextReaderExpand(reader.get());
      if (!node)
	return nil;
    }
  else
    {
      node = 0;
      for (xmlNodePtr n = ctxt.get_corpus_node()->next; n; n = n->next)
	{
	  if (!n
	      || n->type != XML_ELEMENT_NODE)
	    continue;
	  if (!xmlStrEqual(n->name, BAD_CAST("abi-instr")))
	    return nil;
	  node = n;
	  break;
	}
    }

  if (node == 0)
    return nil;

  tu = get_or_read_and_add_translation_unit(ctxt, node);
  // So read_translation_unit() can trigger (under the hood) reading
  // from several translation units just because
  // read_context::get_scope_for_node() has been called.  In that
  // case, after that unexpected call to read_translation_unit(), the
  // current corpus node of the context is going to point to that
  // translation unit that has been read under the hood.  Let's set
  // the corpus node to the one we initially called
  // read_translation_unit() on here.
  ctxt.set_corpus_node(node);
  return tu;
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
read_symbol_db_from_input(read_context&		 ctxt,
			  string_elf_symbols_map_sptr& fn_symdb,
			  string_elf_symbols_map_sptr& var_symdb)
{
  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return false;

  bool found = false;

  if (!ctxt.get_corpus_node())
    for (;;)
      {
	int status = 1;
	while (status == 1
	       && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
	  status = advance_cursor (ctxt);

	if (status != 1)
	  return false;

	bool has_fn_syms = false, has_var_syms = false;
	if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
			 BAD_CAST("elf-function-symbols")))
	  has_fn_syms = true;
	else if (xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
			      BAD_CAST("elf-variable-symbols")))
	  has_var_syms = true;
	else
	  break;

	xmlNodePtr node = xmlTextReaderExpand(reader.get());
	if (!node)
	  return false;

	if (has_fn_syms)
	  {
	    fn_symdb = build_elf_symbol_db(ctxt, node, true);
	    if (fn_symdb)
	      found = true;
	  }
	else if (has_var_syms)
	  {
	    var_symdb = build_elf_symbol_db(ctxt, node, false);
	    if (var_symdb)
	      found = true;
	  }

	xmlTextReaderNext(reader.get());
      }
  else
    for (xmlNodePtr n = ctxt.get_corpus_node()->next; n; n = n->next)
      {
	if (!n || n->type != XML_ELEMENT_NODE)
	  continue;

	bool has_fn_syms = false, has_var_syms = false;
	if (xmlStrEqual(n->name, BAD_CAST("elf-function-symbols")))
	  has_fn_syms = true;
	else if (xmlStrEqual(n->name, BAD_CAST("elf-variable-symbols")))
	  has_var_syms = true;
	else
	  break;
	ctxt.set_corpus_node(n);
	if (has_fn_syms)
	  {
	    fn_symdb = build_elf_symbol_db(ctxt, n, true);
	    found = true;
	  }
	else if (has_var_syms)
	  {
	    var_symdb = build_elf_symbol_db(ctxt, n, false);
	    found = true;
	  }
	else
	  break;
      }

  return found;
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

  xmlNodePtr node = 0;

  if (ctxt.get_corpus_node() == 0)
    {
      int status = 1;
      while (status == 1
	     && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
	status = advance_cursor (ctxt);

      if (status != 1)
	return false;

      if (!xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
			BAD_CAST("elf-needed")))
	return false;

      node = xmlTextReaderExpand(reader.get());
      if (!node)
	return false;
    }
  else
    {
      for (xmlNodePtr n = ctxt.get_corpus_node()->next; n; n = n->next)
	{
	  if (!n || n->type != XML_ELEMENT_NODE)
	    continue;
	  if (!xmlStrEqual(n->name, BAD_CAST("elf-needed")))
	    return false;
	  node = n;
	  break;
	}
    }

  bool result = false;
  if (node)
    {
      result = build_needed(node, needed);
      ctxt.set_corpus_node(node);
    }

  return result;
}

/// Add suppressions specifications to the set of suppressions to be
/// used during the construction of the ABI internal representation
/// (the ABI corpus) from ELF and DWARF.
///
/// During the construction of the ABI corpus, ABI artifacts that
/// match the a given suppression specification are dropped on the
/// floor; that is, they are discarded and won't be part of the final
/// ABI corpus.  This is a way to reduce the amount of data held by
/// the final ABI corpus.
///
/// Note that the suppression specifications provided to this function
/// are only considered during the construction of the ABI corpus.
/// For instance, they are not taken into account during e.g
/// comparisons of two ABI corpora that might happen later.  If you
/// want to apply suppression specificatins to the comparison (or
/// reporting) of ABI corpora please refer to the documentation of the
/// @ref diff_context type to learn how to set suppressions that are
/// to be used in that context.
///
/// @param ctxt the context that is going to be used by functions that
/// read types and declarations information to construct and ABI
/// corpus.
///
/// @param supprs the suppression specifications to be applied during
/// the construction of the ABI corpus.
void
add_read_context_suppressions(read_context& ctxt,
			      const suppr::suppressions_type& supprs)
{
  for (suppr::suppressions_type::const_iterator i = supprs.begin();
       i != supprs.end();
       ++i)
    if ((*i)->get_drops_artifact_from_ir())
      ctxt.get_suppressions().push_back(*i);
}

/// Parse the input XML document containing an ABI corpus, represented
/// by an 'abi-corpus' element node, associated to the current
/// context.
///
/// @param ctxt the current input context.
///
/// @return the corpus resulting from the parsing
corpus_sptr
read_corpus_from_input(read_context& ctxt)
{
  corpus_sptr nil;

  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return nil;

  bool call_reader_next = false;

  xmlNodePtr node = ctxt.get_corpus_node();
  if (!node)
    {
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
	  corpus_sptr c(new corpus(ctxt.get_environment(), ""));
	  ctxt.set_corpus(c);
	}

      if (!ctxt.get_corpus_group())
	ctxt.clear_per_corpus_data();

      corpus& corp = *ctxt.get_corpus();
      ctxt.set_exported_decls_builder(corp.get_exported_decls_builder().get());

      xml::xml_char_sptr path_str = XML_READER_GET_ATTRIBUTE(reader, "path");
      if (path_str)
	corp.set_path(reinterpret_cast<char*>(path_str.get()));

      xml::xml_char_sptr architecture_str =
	XML_READER_GET_ATTRIBUTE(reader, "architecture");
      if (architecture_str)
	corp.set_architecture_name
	  (reinterpret_cast<char*>(architecture_str.get()));

      xml::xml_char_sptr soname_str =
	XML_READER_GET_ATTRIBUTE(reader, "soname");
      if (soname_str)
	corp.set_soname(reinterpret_cast<char*>(soname_str.get()));

      node = xmlTextReaderExpand(reader.get());
      if (!node)
	return nil;

      call_reader_next = true;

      ctxt.set_corpus_node(node->children);
    }
  else
    {
      if (!ctxt.get_corpus())
	{
	  corpus_sptr c(new corpus(ctxt.get_environment(), ""));
	  ctxt.set_corpus(c);
	}

      if (!ctxt.get_corpus_group())
	ctxt.clear_per_corpus_data();

      corpus& corp = *ctxt.get_corpus();
      ctxt.set_exported_decls_builder(corp.get_exported_decls_builder().get());

      xml::xml_char_sptr path_str = XML_NODE_GET_ATTRIBUTE(node, "path");
      if (path_str)
	corp.set_path(reinterpret_cast<char*>(path_str.get()));

      xml::xml_char_sptr architecture_str =
	XML_NODE_GET_ATTRIBUTE(node, "architecture");
      if (architecture_str)
	corp.set_architecture_name
	  (reinterpret_cast<char*>(architecture_str.get()));

      xml::xml_char_sptr soname_str =
	XML_NODE_GET_ATTRIBUTE(node, "soname");
      if (soname_str)
	corp.set_soname(reinterpret_cast<char*>(soname_str.get()));

      ctxt.set_corpus_node(node->children);
    }

  corpus& corp = *ctxt.get_corpus();

  walk_xml_node_to_map_type_ids(ctxt, node);

  // Read the needed element
  vector<string> needed;
  read_elf_needed_from_input(ctxt, needed);
  if (!needed.empty())
    corp.set_needed(needed);

  string_elf_symbols_map_sptr fn_sym_db, var_sym_db;

  // Read the symbol databases.
  bool is_ok = read_symbol_db_from_input(ctxt, fn_sym_db, var_sym_db);
  if (is_ok)
    {
      // Note that it's possible that both fn_sym_db and var_sym_db
      // are nil, due to potential suppression specifications.  That's
      // fine.
      if (fn_sym_db)
	{
	  corp.set_fun_symbol_map(fn_sym_db);
	  fn_sym_db.reset();
	}
      if (var_sym_db)
	{
	  corp.set_var_symbol_map(var_sym_db);
	  var_sym_db.reset();
	}
    }

  ctxt.get_environment()->canonicalization_is_done(false);

  // Read the translation units.
  do
    {
      translation_unit_sptr tu = read_translation_unit_from_input(ctxt);
      is_ok = bool(tu);
    }
  while (is_ok);

  ctxt.perform_late_type_canonicalizing();

  ctxt.get_environment()->canonicalization_is_done(true);

  corp.set_origin(corpus::NATIVE_XML_ORIGIN);

  if (call_reader_next)
    {
      // This is the necessary counter-part of the xmlTextReaderExpand()
      // call at the beginning of the function.
      xmlTextReaderNext(reader.get());
    }
  else
    {
      node = ctxt.get_corpus_node();
      node = xml::advance_to_next_sibling_element(node);
      if (!node)
	{
	  node = ctxt.get_corpus_node();
	  node = xml::advance_to_next_sibling_element(node->parent);
	}
      ctxt.set_corpus_node(node);
    }

  return ctxt.get_corpus();;
}

/// Parse the input XML document containing an ABI corpus group,
/// represented by an 'abi-corpus-group' element node, associated to
/// the current context.
///
/// @param ctxt the current input context.
///
/// @return the corpus group resulting from the parsing
corpus_group_sptr
read_corpus_group_from_input(read_context& ctxt)
{
  corpus_group_sptr nil;

  xml::reader_sptr reader = ctxt.get_reader();
  if (!reader)
    return nil;

  // The document must start with the abi-corpus-group node.
  int status = 1;
  while (status == 1
	 && XML_READER_GET_NODE_TYPE(reader) != XML_READER_TYPE_ELEMENT)
    status = advance_cursor (ctxt);

  if (status != 1 || !xmlStrEqual (XML_READER_GET_NODE_NAME(reader).get(),
				   BAD_CAST("abi-corpus-group")))
    return nil;

  if (!ctxt.get_corpus_group())
    {
      corpus_group_sptr g(new corpus_group(ctxt.get_environment(),
					   ctxt.get_path()));
      ctxt.set_corpus_group(g);
    }

  corpus_group_sptr group = ctxt.get_corpus_group();
  xml::xml_char_sptr path_str = XML_READER_GET_ATTRIBUTE(reader, "path");
  if (path_str)
    group->set_path(reinterpret_cast<char*>(path_str.get()));

  xmlNodePtr node = xmlTextReaderExpand(reader.get());
  if (!node)
    return nil;

  //node = xml::get_first_element_sibling_if_text(node->children);
  node = xml::advance_to_next_sibling_element(node->children);
  ctxt.set_corpus_node(node);

  corpus_sptr corp;
  while ((corp = read_corpus_from_input(ctxt)))
    ctxt.get_corpus_group()->add_corpus(corp);

  xmlTextReaderNext(reader.get());

  return ctxt.get_corpus_group();
}

/// De-serialize an ABI corpus group from an input XML document which
/// root node is 'abi-corpus-group'.
///
/// @param in the input stream to read the XML document from.
///
/// @param env the environment to use.  Note that the life time of
/// this environment must be greater than the lifetime of the
/// resulting corpus as the corpus uses resources that are allocated
/// in the environment.
///
/// @return the resulting corpus group de-serialized from the parsing.
/// This is non-null iff the parsing resulted in a valid corpus group.
corpus_group_sptr
read_corpus_group_from_native_xml(std::istream* in,
				  environment*  env)
{
  read_context_sptr read_ctxt = create_native_xml_read_context(in, env);
  return read_corpus_group_from_input(*read_ctxt);
}

/// De-serialize an ABI corpus group from an XML document file which
/// root node is 'abi-corpus-group'.
///
/// @param path the path to the input file to read the XML document
/// from.
///
/// @param env the environment to use.  Note that the life time of
/// this environment must be greater than the lifetime of the
/// resulting corpus as the corpus uses resources that are allocated
/// in the environment.
///
/// @return the resulting corpus group de-serialized from the parsing.
/// This is non-null if the parsing successfully resulted in a corpus
/// group.
corpus_group_sptr
read_corpus_group_from_native_xml_file(const string& path,
				       environment*  env)
{
    read_context_sptr read_ctxt = create_native_xml_read_context(path, env);
    corpus_group_sptr group = read_corpus_group_from_input(*read_ctxt);
    return group;
}

/// Parse an ABI instrumentation file (in XML format) at a given path.
///
/// @param input_file a path to the file containing the xml document
/// to parse.
///
/// @param env the environment to use.
///
/// @return the translation unit resulting from the parsing upon
/// successful completion, or nil.
translation_unit_sptr
read_translation_unit_from_file(const string&	input_file,
				environment*	env)
{
  read_context ctxt(xml::new_reader_from_file(input_file), env);
  translation_unit_sptr tu = read_translation_unit_from_input(ctxt);
  ctxt.get_environment()->canonicalization_is_done(false);
  ctxt.perform_late_type_canonicalizing();
  ctxt.get_environment()->canonicalization_is_done(true);
  return tu;
}

/// Parse an ABI instrumentation file (in XML format) from an
/// in-memory buffer.
///
/// @param buffer the in-memory buffer containing the xml document to
/// parse.
///
/// @param env the environment to use.
///
/// @return the translation unit resulting from the parsing upon
/// successful completion, or nil.
translation_unit_sptr
read_translation_unit_from_buffer(const string&	buffer,
				  environment*		env)
{
  read_context ctxt(xml::new_reader_from_buffer(buffer), env);
  translation_unit_sptr tu = read_translation_unit_from_input(ctxt);
  ctxt.get_environment()->canonicalization_is_done(false);
  ctxt.perform_late_type_canonicalizing();
  ctxt.get_environment()->canonicalization_is_done(true);
  return tu;
}

/// This function is called by @ref read_translation_unit_from_input.
/// It handles the current xml element node of the reading context.
/// The result of the "handling" is to build the representation of the
/// xml node and tied it to the current translation unit.
///
/// @param ctxt the current parsing context.
///
/// @return true upon successful completion, false otherwise.
static type_or_decl_base_sptr
handle_element_node(read_context& ctxt, xmlNodePtr node,
		    bool add_to_current_scope)
{
  type_or_decl_base_sptr decl;
  if (!node)
    return decl;

  ((decl = handle_namespace_decl(ctxt, node, add_to_current_scope))
   ||(decl = handle_type_decl(ctxt, node, add_to_current_scope))
   ||(decl = handle_qualified_type_decl(ctxt, node,
					add_to_current_scope))
   ||(decl = handle_pointer_type_def(ctxt, node,
				     add_to_current_scope))
   || (decl = handle_reference_type_def(ctxt, node, add_to_current_scope))
   || (decl = handle_function_type(ctxt, node, add_to_current_scope))
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
   || (decl = handle_union_decl(ctxt, node,
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
read_location(const read_context&	ctxt,
	      xmlNodePtr		node,
	      location&		loc)
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

  read_context& c = const_cast<read_context&>(ctxt);
  loc = c.get_translation_unit()->get_loc_mgr().create_new_location(file_path,
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
	/// If there is an access specifier of an unsupported value,
	/// we should not assume anything and abort.
	abort();

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
/// @param is_struct is set to true iff the "is-struct" attribute is
/// present and set to "yes".
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

/// Read the 'is-anonymous' attribute.
///
/// @param node the xml node to read the attribute from.
///
/// @param is_anonymous is set to true iff the "is-anonymous" is present
/// and set to "yes".
///
/// @return true iff the "is-anonymous" attribute is present.
static bool
read_is_anonymous(xmlNodePtr node, bool& is_anonymous)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-anonymous"))
    {
      string str = CHAR_STR(s);
      is_anonymous = (str == "yes");
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

/// Read the 'visibility' attribute of the of the 'elf-symbol'
/// element.
///
/// @param node the XML node to read the attribute from.
///
/// @param b the XML the resulting elf_symbol::visibility.
///
/// @return true iff the function completed successfully.
static bool
read_elf_symbol_binding(xmlNodePtr node, elf_symbol::visibility& v)
{
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "visibility"))
    {
      string str;
      xml::xml_char_sptr_to_string(s, str);
      if (!string_to_elf_symbol_visibility(str, v))
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
      ABG_ASSERT(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  const environment* env = ctxt.get_environment();
  namespace_decl_sptr decl(new namespace_decl(env, name, loc));
  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);
  ctxt.map_xml_node_to_decl(node, decl);

  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;
      handle_element_node(ctxt, n, /*add_to_current_scope=*/true);
    }

  ctxt.pop_scope_or_abort(decl);

  return decl;
}

/// Build an instance of @ref elf_symbol from an XML element node
/// which name is 'elf-symbol'.
///
/// @param ctxt the context used for reading the XML input.
///
/// @param node the XML node to read.
///
/// @param drop_if_suppressed if the elf symbol was suppressed by a
/// suppression specification then do not build it.
///
/// @return the @ref elf_symbol built, or nil if it couldn't be built.
static elf_symbol_sptr
build_elf_symbol(read_context& ctxt, const xmlNodePtr node,
		 bool drop_if_suppressed)
{
  elf_symbol_sptr nil;

  if (!node
      || node->type != XML_ELEMENT_NODE
      || !xmlStrEqual(node->name, BAD_CAST("elf-symbol")))
    return nil;

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    xml::xml_char_sptr_to_string(s, name);

  size_t size = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size"))
    size = strtol(CHAR_STR(s), NULL, 0);

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

  bool is_common = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "is-common"))
    {
      string value;
      xml::xml_char_sptr_to_string(s, value);
      if (value == "true" || value == "yes")
	is_common = true;
      else
	is_common = false;
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

  elf_symbol::binding binding = elf_symbol::GLOBAL_BINDING;
  read_elf_symbol_binding(node, binding);

  elf_symbol::visibility visibility = elf_symbol::DEFAULT_VISIBILITY;
  read_elf_symbol_binding(node, visibility);

  elf_symbol::version version(version_string, is_default_version);

  if (drop_if_suppressed && suppr::is_elf_symbol_suppressed(ctxt, name, type))
    return elf_symbol_sptr();

  const environment* env = ctxt.get_environment();
  elf_symbol_sptr e = elf_symbol::create(env, /*index=*/0,
					 size, name, type, binding,
					 is_defined, is_common,
					 version, visibility,
					 /*is_linux_string_cst=*/false);
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

  ctxt.set_corpus_node(node);

  typedef abg_compat::unordered_map<xmlNodePtr, elf_symbol_sptr>
    xml_node_ptr_elf_symbol_sptr_map_type;
  xml_node_ptr_elf_symbol_sptr_map_type xml_node_ptr_elf_symbol_map;

  elf_symbol_sptr sym;
  for (xmlNodePtr n = node->children; n; n = n->next)
    {
      if ((sym = build_elf_symbol(ctxt, n, /*drop_if_suppress=*/true)))
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
           alias != elems.end(); ++alias)
        {
          string_elf_symbol_sptr_map_type::const_iterator i =
          id_sym_map.find(*alias);
          ABG_ASSERT(i != id_sym_map.end());
          ABG_ASSERT(i->second->is_main_symbol());

          x->second->get_main_symbol()->add_alias(i->second);
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

  ABG_ASSERT(ctxt.get_environment());

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

  type_base_sptr type;
  if (is_variadic)
    type = ctxt.get_environment()->get_variadic_parameter_type();
  else
    {
      ABG_ASSERT(!type_id.empty());
      type = ctxt.build_or_get_type_decl(type_id, true);
    }
  ABG_ASSERT(type);
  ABG_ASSERT(type->get_environment() == ctxt.get_environment());

  string name;
  if (xml_char_sptr a = xml::build_sptr(xmlGetProp(node, BAD_CAST("name"))))
    name = CHAR_STR(a);

  location loc;
  read_location(ctxt, node, loc);

  function_decl::parameter_sptr p
    (new function_decl::parameter(type, name, loc,
				  is_variadic, is_artificial));

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
/// shared_ptr<method_decl>.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly created function_decl upon successful
/// completion, a null pointer otherwise.
static function_decl_sptr
build_function_decl(read_context&	ctxt,
		    const xmlNodePtr	node,
		    class_or_union_sptr as_method_decl,
		    bool		add_to_current_scope)
{
  function_decl_sptr nil;

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

  environment* env = ctxt.get_environment();
  ABG_ASSERT(env);
  std::vector<function_decl::parameter_sptr> parms;
  type_base_sptr return_type = env->get_void_type();

  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      else if (xmlStrEqual(n->name, BAD_CAST("parameter")))
	{
	  if (function_decl::parameter_sptr p =
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
	    return_type = ctxt.build_or_get_type_decl(type_id, true);
	}
    }

  function_type_sptr fn_type(as_method_decl
			     ? new method_type(return_type, as_method_decl,
					       parms, /*is_const=*/false,
					       size, align)
			     : new function_type(return_type,
						 parms, size, align));

  ABG_ASSERT(fn_type);

  function_decl_sptr fn_decl(as_method_decl
			     ? new method_decl (name, fn_type,
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

  if (fn_decl->get_symbol() && fn_decl->get_symbol()->is_public())
    fn_decl->set_is_in_public_symbol_table(true);

  ctxt.get_translation_unit()->bind_function_type_life_time(fn_type);

  ctxt.maybe_canonicalize_type(fn_type, !add_to_current_scope);

  ctxt.maybe_add_fn_to_exported_decls(fn_decl.get());

  return fn_decl;
}

/// Build a function_decl from a 'function-decl' xml node if it's not
/// been suppressed by a suppression specification that is in the
/// context.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the function_decl from.
///
/// @param as_method_decl if this is set to a class_or_union pointer,
/// it means that the 'function-decl' xml node should be parsed as a
/// method_decl.  The class_or_union pointer is the class or union the
/// resulting method_decl is a member function of.  The resulting @ref
/// function_decl_sptr that is returned is then really a @ref
/// method_decl_sptr.
///
/// @param add_to_current_scope if set to yes, the resulting of
/// this function is added to its current scope.
///
/// @return a pointer to a newly created function_decl upon successful
/// completion.  If the function was suppressed by a suppression
/// specification then returns nil.
static function_decl_sptr
build_function_decl_if_not_suppressed(read_context&	ctxt,
				      const xmlNodePtr	node,
				      class_or_union_sptr as_method_decl,
				      bool		add_to_current_scope)
{
    function_decl_sptr fn;

  if (function_is_suppressed(ctxt, node))
    // The function was suppressed by at least one suppression
    // specification associated to the current read context.  So
    // don't build any IR for it.
    ;
  else
    fn = build_function_decl(ctxt, node, as_method_decl,
			     add_to_current_scope);
  return fn;
}

/// Test if a given function denoted by its name and linkage name is
/// suppressed by any of the suppression specifications associated to
/// a given context of native xml reading.
///
/// @param ctxt the native xml reading context of interest.
///
/// @param note the XML node that represents the fucntion.
/// match.
///
/// @return true iff at least one function specification matches the
/// function denoted by the node @p node.
static bool
function_is_suppressed(const read_context& ctxt, xmlNodePtr node)
{
  string fname;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    fname = xml::unescape_xml_string(CHAR_STR(s));

  string flinkage_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    flinkage_name = xml::unescape_xml_string(CHAR_STR(s));

  scope_decl* scope = ctxt.get_cur_scope();

  string qualified_name = build_qualified_name(scope, fname);

  return suppr::function_is_suppressed(ctxt, qualified_name, flinkage_name);
}

/// Test if a type denoted by its name, context and location is
/// suppressed by the suppression specifications that are associated
/// to a given read context.
///
/// @param ctxt the read context to consider.
///
/// @param note the XML node that represents the type.
///
/// @return true iff the type designated by @p node is suppressed by
///  at least of suppression specifications associated to the current
///  read context.
static bool
type_is_suppressed(const read_context& ctxt, xmlNodePtr node)
{
  string type_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    type_name = xml::unescape_xml_string(CHAR_STR(s));

  location type_location;
  read_location(ctxt, node, type_location);

  scope_decl* scope = ctxt.get_cur_scope();

  string qualified_name = build_qualified_name(scope, type_name);

  bool type_is_private = false;
  return suppr::type_is_suppressed(ctxt, qualified_name, type_location,
				   type_is_private,
				   /*require_drop_property=*/true);
}

/// Build a @ref var_decl out of a an XML node that describes it iff
/// the variable denoted by the XML node is not suppressed by a
/// suppression specification associated to the current read context.
///
/// @param ctxt the read context to use.
///
/// @param node the XML node for the variable to consider.
///
/// @parm add_to_current_scope whether to add the built @ref var_decl
/// to the current scope or not.
///
/// @return true iff the @ref var_decl was built.
static var_decl_sptr
build_var_decl_if_not_suppressed(read_context&		ctxt,
				 const xmlNodePtr	node,
				 bool			add_to_current_scope)
{
  var_decl_sptr var;
  if (!variable_is_suppressed(ctxt, node))
    var = build_var_decl(ctxt, node, add_to_current_scope);
  return var;
}

/// Test if a variable denoted by its XML node is suppressed by a
/// suppression specification that is present in a given read context.
///
/// @param ctxt the read context to consider.
///
/// @param node the XML node of the variable to consider.
///
/// @return true iff the variable denoted by @p node is suppressed.
static bool
variable_is_suppressed(const read_context& ctxt, xmlNodePtr node)
{
  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string linkage_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    linkage_name = xml::unescape_xml_string(CHAR_STR(s));

  scope_decl* scope = ctxt.get_cur_scope();

  string qualified_name = build_qualified_name(scope, name);

  return suppr::variable_is_suppressed(ctxt, qualified_name, linkage_name);
}

/// Test if a variable in a particular scope is suppressed by a
/// suppression specification that is present in a given read context.
///
/// @parm ctxt the read context to consider.
///
/// @param scope the scope of the variable to consider.
///
/// @param v the variable to consider.
///
/// @return true iff the variable @p v is suppressed.
static bool
variable_is_suppressed(const read_context& ctxt,
		       const scope_decl* scope,
		       const var_decl& v)
{
  string qualified_name = build_qualified_name(scope, v.get_name());
  return suppr::variable_is_suppressed(ctxt, qualified_name,
				       v.get_linkage_name());
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
  type_base_sptr underlying_type = ctxt.build_or_get_type_decl(type_id,
							       true);
  ABG_ASSERT(underlying_type);

  string mangled_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "mangled-name"))
    mangled_name = xml::unescape_xml_string(CHAR_STR(s));

  decl_base::visibility vis = decl_base::VISIBILITY_NONE;
  read_visibility(node, vis);

  decl_base::binding bind = decl_base::BINDING_NONE;
  read_binding(node, bind);

  location locus;
  read_location(ctxt, node, locus);

  var_decl_sptr decl(new var_decl(name, underlying_type,
				  locus, mangled_name,
				  vis, bind));

  elf_symbol_sptr sym = build_elf_symbol_from_reference(ctxt, node,
							/*function_sym=*/false);
  if (sym)
    decl->set_symbol(sym);

  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);

  if (decl->get_symbol() && decl->get_symbol()->is_public())
    decl->set_is_in_public_symbol_table(true);

  ctxt.maybe_add_var_to_exported_decls(decl.get());

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
      ABG_ASSERT(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  ABG_ASSERT(!id.empty());

  size_t size_in_bits= 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "size-in-bits"))
    size_in_bits = atoi(CHAR_STR(s));

  size_t alignment_in_bits = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "alignment-in-bits"))
    alignment_in_bits = atoi(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  bool is_anonymous = false;
  read_is_anonymous(node, is_anonymous);

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      // I've seen instances of DSOs where a type_decl would appear
      // several times.  Hugh.
      type_decl_sptr ty = dynamic_pointer_cast<type_decl>(d);
      ABG_ASSERT(ty);
      ABG_ASSERT(name == ty->get_name());
      ABG_ASSERT(ty->get_size_in_bits() == size_in_bits);
      ABG_ASSERT(ty->get_alignment_in_bits() == alignment_in_bits);
      return ty;
    }

  const environment* env = ctxt.get_environment();
  type_decl_sptr decl(new type_decl(env, name, size_in_bits,
				    alignment_in_bits, loc));
  decl->set_is_anonymous(is_anonymous);
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
      ABG_ASSERT(result);
      return result;
    }

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> underlying_type =
    ctxt.build_or_get_type_decl(type_id, true);
  ABG_ASSERT(underlying_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      qualified_type_def_sptr result =
	dynamic_pointer_cast<qualified_type_def>(d);
      ABG_ASSERT(result);
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

  ABG_ASSERT(!id.empty());

  qualified_type_def_sptr decl;

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      qualified_type_def_sptr ty = is_qualified_type(d);
      ABG_ASSERT(ty);
      string pr1 = get_pretty_representation(ty->get_underlying_type()),
	pr2 = get_pretty_representation(underlying_type);
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
static pointer_type_def_sptr
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
      ABG_ASSERT(result);
      return result;
    }

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  shared_ptr<type_base> pointed_to_type =
    ctxt.build_or_get_type_decl(type_id, true);
  ABG_ASSERT(pointed_to_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      pointer_type_def_sptr result =
	dynamic_pointer_cast<pointer_type_def>(d);
      ABG_ASSERT(result);
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
  ABG_ASSERT(!id.empty());
  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      pointer_type_def_sptr ty = is_pointer_type(d);
      ABG_ASSERT(ty);
      ABG_ASSERT(ctxt.types_equal(pointed_to_type,
			      ty->get_pointed_to_type()));
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
      ABG_ASSERT(result);
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
  ABG_ASSERT(pointed_to_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      reference_type_def_sptr result =
	dynamic_pointer_cast<reference_type_def>(d);
      ABG_ASSERT(result);
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
  ABG_ASSERT(!id.empty());

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      reference_type_def_sptr ty = is_reference_type(d);
      ABG_ASSERT(ty);
      ABG_ASSERT(ctxt.types_equal(pointed_to_type, ty->get_pointed_to_type()));
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

/// Build a function_type from a pointer to 'function-type'
/// xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the function_type from.
///
/// @param add_to_current_scope if set to yes, the result of
/// this function is added to its current scope.
///
/// @return a pointer to a newly built function_type upon
/// successful completion, a null pointer otherwise.
static function_type_sptr
build_function_type(read_context&	ctxt,
		    const xmlNodePtr	node,
		    bool /*add_to_current_scope*/)
{
  function_type_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("function-type")))
    return nil;

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  ABG_ASSERT(!id.empty());

  string method_class_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "method-class-id"))
    method_class_id = CHAR_STR(s);

  bool is_method_t = !method_class_id.empty();

  size_t size = 0, align = 0;
  read_size_and_alignment(node, size, align);

  environment* env = ctxt.get_environment();
  ABG_ASSERT(env);
  std::vector<shared_ptr<function_decl::parameter> > parms;
  type_base_sptr return_type = env->get_void_type();;

  class_decl_sptr method_class_type;
  if (is_method_t)
    {
      method_class_type =
	is_class_type(ctxt.build_or_get_type_decl(method_class_id,
						  /*add_decl_to_scope=*/true));
      ABG_ASSERT(method_class_type);
    }


 function_type_sptr fn_type(is_method_t
			    ? new method_type(method_class_type,
					      ctxt.get_environment(),
					      size, align)
			    : new function_type(return_type,
						parms, size, align));

  ctxt.get_translation_unit()->bind_function_type_life_time(fn_type);
  ctxt.mark_type_as_wip(fn_type);
  ctxt.key_type_decl(fn_type, id);

  for (xmlNodePtr n = node->children; n ; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      else if (xmlStrEqual(n->name, BAD_CAST("parameter")))
	{
	  if (function_decl::parameter_sptr p =
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
	    fn_type->set_return_type(ctxt.build_or_get_type_decl
				     (type_id, true));
	}
    }

  fn_type->set_parameters(parms);
  ctxt.unmark_type_as_wip(fn_type);

  return fn_type;
}

/// Build a array_type_def::subrange_type from a 'subrange' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the
/// array_type_def::subrange_type from.
///
///
/// @return a pointer to a newly built array_type_def::subrange_type
/// upon successful completion, a null pointer otherwise.
static array_type_def::subrange_sptr
build_subrange_type(read_context&	ctxt,
		    const xmlNodePtr	node)
{
  array_type_def::subrange_sptr nil;

  if (!node || !xmlStrEqual(node->name, BAD_CAST("subrange")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      array_type_def::subrange_sptr result =
	dynamic_pointer_cast<array_type_def::subrange_type>(d);
      ABG_ASSERT(result);
      return result;
    }

  string id;
  // Note that in early implementations, the subrange didn't carry its
  // own ID as the subrange was just a detail of an array.  So we
  // still need to support the abixml emitted by those early
  // implementations.
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  if (!id.empty())
    if (type_base_sptr d = ctxt.get_type_decl(id))
      {
	array_type_def::subrange_sptr ty = is_subrange_type(d);
	ABG_ASSERT(ty);
	return ty;
      }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = CHAR_STR(s);

  size_t length = 0;
  string length_str;
  bool is_infinite = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "length"))
    {
      if (string(CHAR_STR(s)) == "infinite")
	is_infinite = true;
      else
	length = atoi(CHAR_STR(s));
    }

  string underlying_type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    underlying_type_id = CHAR_STR(s);

  type_base_sptr underlying_type;
  if (!underlying_type_id.empty())
    {
      underlying_type = ctxt.build_or_get_type_decl(underlying_type_id, true);
      ABG_ASSERT(underlying_type);
    }

  location loc;
  read_location(ctxt, node, loc);

  // Note that DWARF would actually have a lower_bound of -1 for an
  // array of length 0.
  array_type_def::subrange_type::bound_value max_bound;
  array_type_def::subrange_type::bound_value min_bound;
  if (!is_infinite)
    if (length > 0)
      max_bound.set_signed(length - 1);

  array_type_def::subrange_sptr p
    (new array_type_def::subrange_type(ctxt.get_environment(),
				       name, min_bound, max_bound,
				       underlying_type, loc));
  p->is_infinite(is_infinite);

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
      ABG_ASSERT(result);
      return result;
    }

  int dimensions = 0;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "dimensions"))
    dimensions = atoi(CHAR_STR(s));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);

  // The type of array elements.
  type_base_sptr type =
    ctxt.build_or_get_type_decl(type_id, true);
  ABG_ASSERT(type);

  // maybe building the type of array elements triggered building this
  // one in the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      array_type_def_sptr result =
	dynamic_pointer_cast<array_type_def>(d);
      ABG_ASSERT(result);
      return result;
    }

  size_t size_in_bits = 0, alignment_in_bits = 0;
  bool has_size_in_bits = false;
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
      has_size_in_bits = true;
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
  ABG_ASSERT(!id.empty());

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      array_type_def_sptr ty = is_array_type(d);
      ABG_ASSERT(ty);
      ABG_ASSERT(*type == *ty->get_element_type());
      ABG_ASSERT(type->get_alignment_in_bits() == alignment_in_bits);
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

  if (has_size_in_bits)
    if (size_in_bits != ar_type->get_size_in_bits())
      {
	ABG_ASSERT(size_in_bits == (size_t) -1
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

/// Build an @ref enum_type_decl from the XML node that represents it,
/// if it was not suppressed by a supression specification present in
/// the current read_context.
///
/// @param ctxt the read_context to take into account.
///
/// @param node the XML node representing the @ref enum_type_decl to
/// build.
///
/// @param add_to_current_scope whether to add the built @ref
/// enum_type_decl to the current scope.
///
/// @return the newly built @ref enum_type_decl iff it was effectively
/// built.
static enum_type_decl_sptr
build_enum_type_decl_if_not_suppressed(read_context&	ctxt,
				       const xmlNodePtr node,
				       bool		add_to_current_scope)
{
  enum_type_decl_sptr enum_type;
  if (!type_is_suppressed(ctxt, node))
    enum_type = build_enum_type_decl(ctxt, node, add_to_current_scope);
  return enum_type;
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
  enum_type_decl_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("enum-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      enum_type_decl_sptr result =
	dynamic_pointer_cast<enum_type_decl>(d);
      ABG_ASSERT(result);
      return result;
    }

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string linkage_name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "linkage-name"))
    linkage_name = xml::unescape_xml_string(CHAR_STR(s));

  location loc;
  read_location(ctxt, node, loc);

  bool is_anonymous = false;
  read_is_anonymous(node, is_anonymous);

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);

  ABG_ASSERT(!id.empty());

  const environment* env = ctxt.get_environment();
  ABG_ASSERT(env);

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
	  int64_t value = 0;

	  xml_char_sptr a = xml::build_sptr(xmlGetProp(n, BAD_CAST("name")));
	  if (a)
	    name = xml::unescape_xml_string(CHAR_STR(a));

	  a = xml::build_sptr(xmlGetProp(n, BAD_CAST("value")));
	  if (a)
	    {
	      value = strtoll(CHAR_STR(a), NULL, 0);
	      if (value == LLONG_MIN || value == LLONG_MAX)
		return nil;
	    }

	  enums.push_back(enum_type_decl::enumerator(env, name, value));
	}
    }

  type_base_sptr underlying_type =
    ctxt.build_or_get_type_decl(base_type_id, true);
  ABG_ASSERT(underlying_type);

  enum_type_decl_sptr t(new enum_type_decl(name, loc,
					   underlying_type,
					   enums, linkage_name));
  t->set_is_anonymous(is_anonymous);
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
      typedef_decl_sptr result = is_typedef(d);
      ABG_ASSERT(result);
      return result;
    }

  string id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "id"))
    id = CHAR_STR(s);
  ABG_ASSERT(!id.empty());

  string name;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "name"))
    name = xml::unescape_xml_string(CHAR_STR(s));

  string type_id;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "type-id"))
    type_id = CHAR_STR(s);
  shared_ptr<type_base> underlying_type(ctxt.build_or_get_type_decl(type_id,
								    true));
  ABG_ASSERT(underlying_type);

  // maybe building the underlying type triggered building this one in
  // the mean time ...
  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      typedef_decl_sptr result = dynamic_pointer_cast<typedef_decl>(d);
      ABG_ASSERT(result);
      return result;
    }

  location loc;
  read_location(ctxt, node, loc);

  if (type_base_sptr d = ctxt.get_type_decl(id))
    {
      typedef_decl_sptr ty = dynamic_pointer_cast<typedef_decl>(d);
      ABG_ASSERT(ty);
      ABG_ASSERT(name == ty->get_name());
      ABG_ASSERT(get_type_name(underlying_type)
	     == get_type_name(ty->get_underlying_type()));
      // it's possible to have the same typedef several times.
    }
  typedef_decl_sptr t(new typedef_decl(name, underlying_type, loc));

  if (ctxt.push_and_key_type_decl(t, id, add_to_current_scope))
    {
      ctxt.map_xml_node_to_decl(node, t);
      return t;
    }

  return nil;
}

/// Build a class from its XML node if it is not suppressed by a
/// suppression specification that is present in the read context.
///
/// @param ctxt the read context to consider.
///
/// @param node the XML node to consider.
///
/// @param add_to_current_scope whether to add the built class to the
/// current context or not.
///
/// @return true iff the class was built.
static class_decl_sptr
build_class_decl_if_not_suppressed(read_context&	ctxt,
				   const xmlNodePtr	node,
				   bool		add_to_current_scope)
{
  class_decl_sptr class_type;
  if (!type_is_suppressed(ctxt, node))
    class_type = build_class_decl(ctxt, node, add_to_current_scope);
  return class_type;
}

/// Build a @ref union_decl from its XML node if it is not suppressed
/// by a suppression specification that is present in the read
/// context.
///
/// @param ctxt the read context to consider.
///
/// @param node the XML node to consider.
///
/// @param add_to_current_scope whether to add the built @ref
/// union_decl to the current context or not.
///
/// @return true iff the @ref union_decl was built.
static union_decl_sptr
build_union_decl_if_not_suppressed(read_context&	ctxt,
				   const xmlNodePtr	node,
				   bool		add_to_current_scope)
{
  union_decl_sptr union_type;
  if (!type_is_suppressed(ctxt, node))
    union_type = build_union_decl(ctxt, node, add_to_current_scope);
  return union_type;
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
  class_decl_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("class-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      class_decl_sptr result = dynamic_pointer_cast<class_decl>(d);
      ABG_ASSERT(result);
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

  class_decl_sptr decl;

  bool is_decl_only = false;
  read_is_declaration_only(node, is_decl_only);

  bool is_struct = false;
  read_is_struct(node, is_struct);

  bool is_anonymous = false;
  read_is_anonymous(node, is_anonymous);

  string naming_typedef_id;

  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "naming-typedef-id"))
    naming_typedef_id = xml::unescape_xml_string(CHAR_STR(s));

  ABG_ASSERT(!id.empty());
  class_decl_sptr previous_definition, previous_declaration;
  const vector<type_base_sptr> *types_ptr = 0;
  if (!is_anonymous)
    types_ptr = ctxt.get_all_type_decls(id);
  if (types_ptr)
    {
      // Lets look at the previous declarations and the first previous
      // definition of this type that we've already seen while parsing
      // this corpus.
      for (vector<type_base_sptr>::const_iterator i = types_ptr->begin();
	   i != types_ptr->end();
	   ++i)
	{
	  class_decl_sptr klass = is_class_type(*i);
	  ABG_ASSERT(klass);
	  if (klass->get_is_declaration_only()
	      && !klass->get_definition_of_declaration())
	    previous_declaration = klass;
	  else if (!klass->get_is_declaration_only()
		   && !previous_definition)
	    previous_definition = klass;
	  if (previous_definition && previous_declaration)
	    break;
	}

      if (previous_declaration)
	ABG_ASSERT(previous_declaration->get_name() == name);

      if (previous_definition)
	ABG_ASSERT(previous_definition->get_name() == name);

      if (is_decl_only && previous_declaration)
	return previous_declaration;
    }

  const environment* env = ctxt.get_environment();
  ABG_ASSERT(env);

  if (!is_decl_only && previous_definition)
    // We are in the case where we've read this class definition
    // before, but we might need to update it to add some new stuff to
    // it; we might thus find the new stuff to add in the current
    // (new) incarnation of that definition that we are currently
    // reading.
    decl = previous_definition;
  else
    {
      if (is_decl_only)
	{
	  decl.reset(new class_decl(env, name, is_struct));
	  if (size_in_bits)
	    decl->set_size_in_bits(size_in_bits);
	}
      else
	decl.reset(new class_decl(env, name, size_in_bits, alignment_in_bits,
				  is_struct, loc, vis, bases, mbrs,
				  data_mbrs, mbr_functions));
      decl->set_is_anonymous(is_anonymous);
    }

  string def_id;
  bool is_def_of_decl = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "def-of-decl-id"))
    def_id = CHAR_STR(s);

  if (!def_id.empty())
    {
      class_decl_sptr d =
	dynamic_pointer_cast<class_decl>(ctxt.get_type_decl(def_id));
      if (d && d->get_is_declaration_only())
	{
	  is_def_of_decl = true;
	  decl->set_earlier_declaration(d);
	  d->set_definition_of_declaration(decl);
	}
    }

  if (!is_decl_only
      && decl
      && !decl->get_is_declaration_only()
      && previous_declaration)
    {
      // decl is the definition of the previous declaration
      // previous_declaration.
      //
      // Let's link them.
      decl->set_earlier_declaration(previous_declaration);
      for (vector<type_base_sptr>::const_iterator i = types_ptr->begin();
	   i != types_ptr->end();
	   ++i)
	{
	  class_decl_sptr d = is_class_type(*i);
	  ABG_ASSERT(d);
	  if (d->get_is_declaration_only()
	      && !d->get_definition_of_declaration())
	    {
	      previous_declaration->set_definition_of_declaration(decl);
	      is_def_of_decl = true;
	    }
	}
    }

  if (is_decl_only && previous_definition)
    {
      // decl is a declaration of the previous definition
      // previous_definition.  Let's link them.
      ABG_ASSERT(decl->get_is_declaration_only()
	     && !decl->get_definition_of_declaration());
      decl->set_definition_of_declaration(previous_definition);
    }

  ABG_ASSERT(!is_decl_only || !is_def_of_decl);

  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);

  ctxt.map_xml_node_to_decl(node, decl);
  ctxt.mark_type_as_wip(decl);
  ctxt.key_type_decl(decl, id);

  // If this class has a naming typedef, get it and refer to it.
  if (!naming_typedef_id.empty())
    {
      typedef_decl_sptr naming_typedef =
	is_typedef(ctxt.build_or_get_type_decl(naming_typedef_id, true));
      ABG_ASSERT(naming_typedef);
      decl->set_naming_typedef(naming_typedef);
    }

  for (xmlNodePtr n = node->children; !is_decl_only && n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("base-class")))
	{
	  access_specifier access =
	    is_struct
	    ? public_access
	    : private_access;
	  read_access(n, access);

	  string type_id;
	  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(n, "type-id"))
	    type_id = CHAR_STR(s);
	  shared_ptr<class_decl> b =
	    dynamic_pointer_cast<class_decl>
	    (ctxt.build_or_get_type_decl(type_id, true));
	  ABG_ASSERT(b);

	  if (decl->find_base_class(b->get_qualified_name()))
	    // We are in updating mode for this class.  The version of
	    // the class we have already has this base class, so we
	    // are not going to add it again.
	    continue;

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
	  access_specifier access =
	    is_struct
	    ? public_access
	    : private_access;
	  read_access(n, access);

	  ctxt.map_xml_node_to_decl(n, decl);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (type_base_sptr t =
		  build_type(ctxt, p, /*add_to_current_scope=*/true))
		{
		  decl_base_sptr td = get_type_declaration(t);
		  ABG_ASSERT(td);
		  set_member_access_specifier(td, access);
		  ctxt.maybe_canonicalize_type(t, !add_to_current_scope);
		  xml_char_sptr i= XML_NODE_GET_ATTRIBUTE(p, "id");
		  string id = CHAR_STR(i);
		  ABG_ASSERT(!id.empty());
		  ctxt.key_type_decl(t, id);
		  ctxt.map_xml_node_to_decl(p, td);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("data-member")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access =
	    is_struct
	    ? public_access
	    : private_access;
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

	      if (var_decl_sptr v =
		  build_var_decl(ctxt, p, /*add_to_cur_scope=*/false))
		{
		  if (decl->find_data_member(v->get_name()))
		    {
		      // We are in updating mode and the current
		      // version of this class already has this data
		      // member, so we are not going to add it again.
		      // So we need to discard the data member we have
		      // built (and that was pushed to the current
		      // stack of decls built) and move on.
		      decl_base_sptr d = ctxt.pop_decl();
		      ABG_ASSERT(is_var_decl(d));
		      continue;
		    }
		  if (!is_static
		      || !variable_is_suppressed(ctxt, decl.get(), *v))
		    decl->add_data_member(v, access,
					  is_laid_out,
					  is_static,
					  offset_in_bits);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-function")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access =
	    is_struct
	    ? public_access
	    : private_access;
	  read_access(n, access);

	  bool is_virtual = false;
	  ssize_t vtable_offset = -1;
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
		  build_function_decl_if_not_suppressed(ctxt, p, decl,
							/*add_to_cur_sc=*/true))
		{
		  method_decl_sptr m = is_method_decl(f);
		  ABG_ASSERT(m);
		  set_member_access_specifier(m, access);
		  set_member_is_static(m, is_static);
		  if (vtable_offset != -1)
		    set_member_function_vtable_offset(m, vtable_offset);
		  set_member_function_is_virtual(m, is_virtual);
		  set_member_function_is_ctor(m, is_ctor);
		  set_member_function_is_dtor(m, is_dtor);
		  set_member_function_is_const(m, is_const);
		  break;
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-template")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access =
	    is_struct
	    ? public_access
	    : private_access;
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
				       /*add_to_current_scope=*/true))
		{
		  shared_ptr<member_function_template> m
		    (new member_function_template(f, access, is_static,
						  is_ctor, is_const));
		  ABG_ASSERT(f->get_scope());
		  decl->add_member_function_template(m);
		}
	      else if (shared_ptr<class_tdecl> c =
		       build_class_tdecl(ctxt, p,
					 /*add_to_current_scope=*/true))
		{
		  member_class_template_sptr m(new member_class_template(c,
									 access,
									 is_static));
		  ABG_ASSERT(c->get_scope());
		  decl->add_member_class_template(m);
		}
	    }
	}
    }

  ctxt.pop_scope_or_abort(decl);
  ctxt.unmark_type_as_wip(decl);

  return decl;
}

/// Build a union_decl from a 'union-decl' xml node.
///
/// @param ctxt the context of the parsing.
///
/// @param node the xml node to build the union_decl from.
///
/// @param add_to_current_scope if yes, the resulting union node
/// hasn't triggered voluntarily the adding of the resulting
/// union_decl_sptr to the current scope.
///
/// @return a pointer to union_decl upon successful completion, a null
/// pointer otherwise.
static union_decl_sptr
build_union_decl(read_context& ctxt,
		 const xmlNodePtr node,
		 bool add_to_current_scope)
{
  union_decl_sptr nil;

  if (!xmlStrEqual(node->name, BAD_CAST("union-decl")))
    return nil;

  if (decl_base_sptr d = ctxt.get_decl_for_xml_node(node))
    {
      union_decl_sptr result = dynamic_pointer_cast<union_decl>(d);
      ABG_ASSERT(result);
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

  union_decl::member_types mbrs;
  union_decl::data_members data_mbrs;
  union_decl::member_functions mbr_functions;

  union_decl_sptr decl;

  bool is_decl_only = false;
  read_is_declaration_only(node, is_decl_only);

  bool is_anonymous = false;
  read_is_anonymous(node, is_anonymous);

  ABG_ASSERT(!id.empty());
  union_decl_sptr previous_definition, previous_declaration;
  const vector<type_base_sptr> *types_ptr = 0;
  if (!is_anonymous)
    types_ptr = ctxt.get_all_type_decls(id);
  if (types_ptr)
    {
      // Lets look at the previous declarations and the first previous
      // definition of this type that we've already seen while parsing
      // this corpus.
      for (vector<type_base_sptr>::const_iterator i = types_ptr->begin();
	   i != types_ptr->end();
	   ++i)
	{
	  union_decl_sptr onion = is_union_type(*i);
	  ABG_ASSERT(onion);
	  if (onion->get_is_declaration_only()
	      && !onion->get_definition_of_declaration())
	    previous_declaration = onion;
	  else if (!onion->get_is_declaration_only()
		   && !previous_definition)
	    previous_definition = onion;
	  if (previous_definition && previous_declaration)
	    break;
	}

      if (previous_declaration)
	ABG_ASSERT(previous_declaration->get_name() == name);

      if (previous_definition)
	ABG_ASSERT(previous_definition->get_name() == name);

      if (is_decl_only && previous_declaration)
	return previous_declaration;
    }

  const environment* env = ctxt.get_environment();
  ABG_ASSERT(env);

  if (!is_decl_only && previous_definition)
    // We are in the case where we've read this class definition
    // before, but we might need to update it to add some new stuff to
    // it; we might thus find the new stuff to add in the current
    // (new) incarnation of that definition that we are currently
    // reading.
    decl = previous_definition;
  else
    {
      if (is_decl_only)
	decl.reset(new union_decl(env, name));
      else
	decl.reset(new union_decl(env, name,
				  size_in_bits,
				  loc, vis, mbrs,
				  data_mbrs,
				  mbr_functions));
      decl->set_is_anonymous(is_anonymous);
    }

  string def_id;
  bool is_def_of_decl = false;
  if (xml_char_sptr s = XML_NODE_GET_ATTRIBUTE(node, "def-of-decl-id"))
    def_id = CHAR_STR(s);

  if (!def_id.empty())
    {
      class_decl_sptr d =
	dynamic_pointer_cast<class_decl>(ctxt.get_type_decl(def_id));
      if (d && d->get_is_declaration_only())
	{
	  is_def_of_decl = true;
	  decl->set_earlier_declaration(d);
	  d->set_definition_of_declaration(decl);
	}
    }

  if (!is_decl_only
      && decl
      && !decl->get_is_declaration_only()
      && previous_declaration)
    {
      // decl is the definition of the previous declaration
      // previous_declaration.
      //
      // Let's link them.
      decl->set_earlier_declaration(previous_declaration);
      for (vector<type_base_sptr>::const_iterator i = types_ptr->begin();
	   i != types_ptr->end();
	   ++i)
	{
	  union_decl_sptr d = is_union_type(*i);
	  ABG_ASSERT(d);
	  if (d->get_is_declaration_only()
	      && !d->get_definition_of_declaration())
	    {
	      previous_declaration->set_definition_of_declaration(decl);
	      is_def_of_decl = true;
	    }
	}
    }

  if (is_decl_only && previous_definition)
    {
      // decl is a declaration of the previous definition
      // previous_definition.  Let's link them.
      ABG_ASSERT(decl->get_is_declaration_only()
	     && !decl->get_definition_of_declaration());
      decl->set_definition_of_declaration(previous_definition);
    }

  ABG_ASSERT(!is_decl_only || !is_def_of_decl);

  ctxt.push_decl_to_current_scope(decl, add_to_current_scope);

  ctxt.map_xml_node_to_decl(node, decl);
  ctxt.mark_type_as_wip(decl);
  ctxt.key_type_decl(decl, id);

  for (xmlNodePtr n = node->children; !is_decl_only && n; n = n->next)
    {
      if (n->type != XML_ELEMENT_NODE)
	continue;

      if (xmlStrEqual(n->name, BAD_CAST("member-type")))
	{
	  access_specifier access = private_access;
	  read_access(n, access);

	  ctxt.map_xml_node_to_decl(n, decl);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (type_base_sptr t =
		  build_type(ctxt, p, /*add_to_current_scope=*/true))
		{
		  decl_base_sptr td = get_type_declaration(t);
		  ABG_ASSERT(td);
		  set_member_access_specifier(td, access);
		  ctxt.maybe_canonicalize_type(t, !add_to_current_scope);
		  xml_char_sptr i= XML_NODE_GET_ATTRIBUTE(p, "id");
		  string id = CHAR_STR(i);
		  ABG_ASSERT(!id.empty());
		  ctxt.key_type_decl(t, id);
		  ctxt.map_xml_node_to_decl(p, td);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("data-member")))
	{
	  ctxt.map_xml_node_to_decl(n, decl);

	  access_specifier access = private_access;
	  read_access(n, access);

	  bool is_laid_out = true;
	  size_t offset_in_bits = 0;
	  bool is_static = false;
	  read_static(n, is_static);

	  for (xmlNodePtr p = n->children; p; p = p->next)
	    {
	      if (p->type != XML_ELEMENT_NODE)
		continue;

	      if (var_decl_sptr v =
		  build_var_decl(ctxt, p, /*add_to_cur_scope=*/false))
		{
		  if (decl->find_data_member(v->get_name()))
		    {
		      // We are in updating mode and the current
		      // version of this class already has this data
		      // member, so we are not going to add it again.
		      // So we need to discard the data member we have
		      // built (and that was pushed to the current
		      // stack of decls built) and move on.
		      decl_base_sptr d = ctxt.pop_decl();
		      ABG_ASSERT(is_var_decl(d));
		      continue;
		    }
		  if (!is_static
		      || !variable_is_suppressed(ctxt, decl.get(), *v))
		    decl->add_data_member(v, access,
					  is_laid_out,
					  is_static,
					  offset_in_bits);
		}
	    }
	}
      else if (xmlStrEqual(n->name, BAD_CAST("member-function")))
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

	      if (function_decl_sptr f =
		  build_function_decl_if_not_suppressed(ctxt, p, decl,
							/*add_to_cur_sc=*/true))
		{
		  method_decl_sptr m = is_method_decl(f);
		  ABG_ASSERT(m);
		  set_member_access_specifier(m, access);
		  set_member_is_static(m, is_static);
		  set_member_function_is_ctor(m, is_ctor);
		  set_member_function_is_dtor(m, is_dtor);
		  set_member_function_is_const(m, is_const);
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

	      if (function_tdecl_sptr f =
		  build_function_tdecl(ctxt, p,
				       /*add_to_current_scope=*/true))
		{
		  member_function_template_sptr m
		    (new member_function_template(f, access, is_static,
						  is_ctor, is_const));
		  ABG_ASSERT(f->get_scope());
		  decl->add_member_function_template(m);
		}
	      else if (class_tdecl_sptr c =
		       build_class_tdecl(ctxt, p,
					 /*add_to_current_scope=*/true))
		{
		  member_class_template_sptr m(new member_class_template(c,
									 access,
									 is_static));
		  ABG_ASSERT(c->get_scope());
		  decl->add_member_class_template(m);
		}
	    }
	}
    }

  ctxt.pop_scope_or_abort(decl);
  ctxt.unmark_type_as_wip(decl);

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

  const environment* env = ctxt.get_environment();
  ABG_ASSERT(env);

  function_tdecl_sptr fn_tmpl_decl(new function_tdecl(env, loc, vis, bind));

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
      else if (function_decl_sptr f =
	       build_function_decl_if_not_suppressed(ctxt, n, class_decl_sptr(),
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

  const environment* env = ctxt.get_environment();
  ABG_ASSERT(env);

  class_tdecl_sptr class_tmpl (new class_tdecl(env, loc, vis));

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
      else if (class_decl_sptr c =
	       build_class_decl_if_not_suppressed(ctxt, n,
						  add_to_current_scope))
	{
	  if (c->get_scope())
	    ctxt.maybe_canonicalize_type(c, /*force_delay=*/false);
	  class_tmpl->set_pattern(c);
	}
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
    ABG_ASSERT(!ctxt.get_type_decl(id));

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

  ABG_ASSERT(result->get_environment());

  ctxt.maybe_canonicalize_type(result, /*force_delay=*/false);

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
	  ctxt.maybe_canonicalize_type(composed_type,
				       /*force_delay=*/true);
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
  ABG_ASSERT(!id.empty());

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
    {
      ctxt.key_type_decl(result, id);
      ctxt.maybe_canonicalize_type(result, /*force_delay=*/false);
    }

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
static type_base_sptr
build_type(read_context&	ctxt,
	   const xmlNodePtr	node,
	   bool		add_to_current_scope)
{
  type_base_sptr t;

  ((t = build_type_decl(ctxt, node, add_to_current_scope))
   || (t = build_qualified_type_decl(ctxt, node, add_to_current_scope))
   || (t = build_pointer_type_def(ctxt, node, add_to_current_scope))
   || (t = build_reference_type_def(ctxt, node , add_to_current_scope))
   || (t = build_function_type(ctxt, node, add_to_current_scope))
   || (t = build_array_type_def(ctxt, node, add_to_current_scope))
   || (t = build_enum_type_decl_if_not_suppressed(ctxt, node,
						  add_to_current_scope))
   || (t = build_typedef_decl(ctxt, node, add_to_current_scope))
   || (t = build_class_decl_if_not_suppressed(ctxt, node,
					      add_to_current_scope))
   || (t = build_union_decl_if_not_suppressed(ctxt, node,
					      add_to_current_scope)));

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
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
  return decl;
}

/// Parse a function-type element.
///
/// @param ctxt the context of the parsing.
///
/// function_type is added to.
static type_base_sptr
handle_function_type(read_context&	ctxt,
		     xmlNodePtr		node,
		     bool		add_to_current_scope)
{
  function_type_sptr type = build_function_type(ctxt, node,
						add_to_current_scope);
  ctxt.maybe_canonicalize_type(type, /*force_delay=*/true);
  return type;
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
  ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  enum_type_decl_sptr decl =
    build_enum_type_decl_if_not_suppressed(ctxt, node,
					   add_to_current_scope);
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
  decl_base_sptr decl = build_var_decl_if_not_suppressed(ctxt, node,
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
  return build_function_decl_if_not_suppressed(ctxt, node, class_decl_sptr(),
					       add_to_current_scope);
}

/// Parse a 'class-decl' xml element.
///
/// @param ctxt the context of the parsing.
///
/// @return the resulting @ref class_decl built from the XML element
/// upon successful completion of the parsing, nil otherwise.
static decl_base_sptr
handle_class_decl(read_context& ctxt,
		  xmlNodePtr	node,
		  bool		add_to_current_scope)
{
  class_decl_sptr decl =
    build_class_decl_if_not_suppressed(ctxt, node, add_to_current_scope);
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
  return decl;
}

/// Parse a 'union-decl' xml element.
///
/// @param ctxt the context of the parsing.
///
/// @return the resulting @ref union_decl built from the XML element
/// upon successful completion of the parsing, nil otherwise.
static decl_base_sptr
handle_union_decl(read_context& ctxt,
		  xmlNodePtr	node,
		  bool		add_to_current_scope)
{
  union_decl_sptr decl =
    build_union_decl_if_not_suppressed(ctxt, node, add_to_current_scope);
  if (decl && decl->get_scope())
    ctxt.maybe_canonicalize_type(decl, /*force_delay=*/false);
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
/// @param env the environment to use.
///
/// @return the translation unit resulting from the parsing upon
/// successful completion, or nil.
translation_unit_sptr
read_translation_unit_from_istream(istream* in, environment* env)
{
  read_context read_ctxt(xml::new_reader_from_istream(in), env);
  return read_translation_unit_from_input(read_ctxt);
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
static translation_unit_sptr
read_to_translation_unit(zip_sptr ar,
			 int file_index)
{
  translation_unit_sptr nil;
  if (!ar)
    return nil;

  zip_file_sptr f = open_file_in_archive(ar, file_index);
  if (!f)
    return nil;

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

  return read_translation_unit_from_buffer(input);
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

  ABG_ASSERT(archive);
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

/// Create an xml_reader::read_context to read a native XML ABI file.
///
/// @param path the path to the native XML file to read.
///
/// @param env the environment to use.
///
/// @return the created context.
read_context_sptr
create_native_xml_read_context(const string& path, environment *env)
{
  read_context_sptr result(new read_context(xml::new_reader_from_file(path),
					    env));
  corpus_sptr corp(new corpus(env));
  result->set_corpus(corp);
  result->set_path(path);
  return result;
}

/// Create an xml_reader::read_context to read a native XML ABI from
/// an input stream..
///
/// @param in the input stream that contains the native XML file to read.
///
/// @param env the environment to use.
///
/// @return the created context.
read_context_sptr
create_native_xml_read_context(std::istream* in, environment* env)
{
  read_context_sptr result(new read_context(xml::new_reader_from_istream(in),
					    env));
  corpus_sptr corp(new corpus(env, ""));
  result->set_corpus(corp);
  return result;
}

/// Getter for the path to the binary this @ref read_context is for.
///
/// @return the path to the binary the @ref read_context is for.
const string&
read_context_get_path(const read_context& ctxt)
{return ctxt.get_path();}

/// De-serialize an ABI corpus from an input XML document which root
/// node is 'abi-corpus'.
///
/// @param in the input stream to read the XML document from.
///
/// @param env the environment to use.  Note that the life time of
/// this environment must be greater than the lifetime of the
/// resulting corpus as the corpus uses resources that are allocated
/// in the environment.
///
/// @return the resulting corpus de-serialized from the parsing.  This
/// is non-null iff the parsing resulted in a valid corpus.
corpus_sptr
read_corpus_from_native_xml(std::istream* in,
			    environment* env)
{
  read_context_sptr read_ctxt = create_native_xml_read_context(in, env);
  return read_corpus_from_input(*read_ctxt);
}

/// De-serialize an ABI corpus from an XML document file which root
/// node is 'abi-corpus'.
///
/// @param path the path to the input file to read the XML document
/// from.
///
/// @param env the environment to use.  Note that the life time of
/// this environment must be greater than the lifetime of the
/// resulting corpus as the corpus uses resources that are allocated
/// in the environment.
///
/// @return the resulting corpus de-serialized from the parsing.  This
/// is non-null if the parsing successfully resulted in a corpus.
corpus_sptr
read_corpus_from_native_xml_file(const string& path,
				 environment* env)
{
  read_context_sptr read_ctxt = create_native_xml_read_context(path, env);
  corpus_sptr corp = read_corpus_from_input(*read_ctxt);
  return corp;
}

}//end namespace xml_reader

}//end namespace abigail
