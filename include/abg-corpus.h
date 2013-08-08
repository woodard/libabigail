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

#ifndef __ABG_CORPUS_H__
#define __ABG_CORPUS_H__

#include <tr1/memory>
#include "abg-fwd.h"
#include "abg-traverse.h"

namespace abigail
{

/// @brief The source location of a token.
///
/// This represents the location of a token coming from a given ABI
/// Corpus.  This location is actually an abstraction of cursor in the
/// table of all the locations of all the tokens of the ABI Corpus.
/// That table is managed by the location_manager type.
class location
{
  unsigned		m_value;

  location(unsigned v) : m_value(v) { }

public:

  location() : m_value(0) { }

  unsigned
  get_value() const
  { return m_value; }

  operator bool() const
  { return !!m_value; }

  bool
  operator==(const location other) const
  { return m_value == other.m_value; }

  bool
  operator<(const location other) const
  { return m_value < other.m_value; }

  friend class location_manager;
};

/// @brief The entry point to manage locations.
///
/// This type keeps a table of all the locations for tokens of a
/// given ABI Corpus
class location_manager
{
  struct _Impl;

  /// Pimpl.
  shared_ptr<_Impl>			m_priv;

public:

  location_manager();

  /// Insert the triplet representing a source locus into our internal
  /// vector of location triplet.  Return an instance of location type,
  /// built from an integral type that represents the index of the
  /// source locus triplet into our source locus table.
  ///
  /// @param fle the file path of the source locus
  /// @param lne the line number of the source location
  /// @param col the column number of the source location
  location
  create_new_location(const std::string& fle, size_t lne, size_t col);

  /// Given an instance of location type, return the triplet
  /// {path,line,column} that represents the source locus.  Note that
  /// the location must have been previously created from the function
  /// location_manager::expand_location otherwise this function yields
  /// unexpected results, including possibly a crash.
  ///
  /// @param location the instance of location type to expand
  /// @param path the resulting path of the source locus
  /// @param line the resulting line of the source locus
  /// @param column the resulting colum of the source locus
  void
  expand_location(const location location, std::string& path,
		  unsigned& line, unsigned& column) const;
};


/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// into a translation unit.
class translation_unit : public traversable_base
{
public:

  typedef shared_ptr<global_scope>     	global_scope_sptr;

private:
  std::string 				m_path;
  location_manager 			m_loc_mgr;
  mutable global_scope_sptr 		m_global_scope;

  // Forbidden
  translation_unit();

public:

  /// Constructor of translation_unit.
  ///
  /// @param path the location of the translation unit.
  translation_unit(const std::string& path);

  virtual ~translation_unit();

  /// @return the path of the compilation unit that gave birth to this
  /// instance of tranlation_unit.
  const std::string&
  get_path() const;

  /// Getter of the the global scope of the translation unit.
  ///
  /// @return the global scope of the current translation unit.  If
  /// there is not global scope allocated yet, this function creates one
  /// and returns it.
  const global_scope_sptr
  get_global_scope() const;

  /// Getter of the location manager for the current translation unit.
  ///
  /// @return a reference to the location manager for the current
  /// translation unit.
  location_manager&
  get_loc_mgr();

  /// const Getter of the location manager.
  ///
  /// @return a const reference to the location manager for the current
  /// translation unit.
  const location_manager&
  get_loc_mgr() const;

  /// Tests whether if the current translation unit contains ABI
  /// artifacts or not.
  ///
  /// @return true iff the current translation unit is empty.
  bool
  is_empty() const;

  /// Deserialize the contents of the external file into this
  /// translation_unit object.
  bool
  read();

  /// Serialize the contents of this translation unit object into an
  /// external file.
  ///
  /// @param out the ostream output object.
  bool
  write(std::ostream& out); 

  /// This implements the traversable_base::traverse pure virtual
  /// function.
  ///
  /// @param v the visitor used on the member nodes of the translation
  /// unit during the traversal.
  void
  traverse(ir_node_visitor& v);
};


/// This is the abstraction of a set of translation units (themselves
/// seen as bundles of unitary abi artefacts like types and decls)
/// bundled together as a corpus.  A corpus is thus the Application
/// binary interface of a program, a library or just a set of modules
/// put together.
class corpus
{
public:
  typedef std::string					string;
  typedef std::tr1::shared_ptr<translation_unit>	translation_unit_sptr;
  typedef std::list<translation_unit_sptr>		translation_units;

private:
  string 			m_name;
  translation_units 		m_members;

  corpus();

public:

  corpus(const string&);

  void
  add(const translation_unit_sptr);

  const translation_units&
  get_translation_units() const;

  bool
  is_empty() const;
};
}//end namespace abigail
#endif //__ABG_CORPUS_H__
