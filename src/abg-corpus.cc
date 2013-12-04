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

#include <cstdio>
#include <cstring>
#include <ext/stdio_filebuf.h>
#include <sstream>
#include <stdexcept>
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-writer.h"
#include "abg-libzip-utils.h"

namespace abigail
{

using std::ostringstream;
using std::list;
using std::vector;
using zip_utils::zip_sptr;
using zip_utils::zip_file_sptr;
using zip_utils::open_archive;
using zip_utils::open_file_in_archive;

template<typename T>
struct array_deleter
{
  void
  operator()(T* a)
  {
    delete [] a;
  }
};//end array_deleter

struct corpus::impl
{
  string			path;
  translation_units		members;

private:
   impl();

public:
   impl(const string &p)
     : path(p)
   {}
 };

 /// @param path the path to the file containing the ABI corpus.
 corpus::corpus(const string& path)
 {
   m_priv.reset(new impl(path));
 }

 /// Add a translation unit to the current ABI Corpus.	Next time
 /// corpus::save is called, all the translation unit that got added
 /// to the corpus are going to be serialized on disk in the file
 /// associated to the current corpus.
 ///
 /// @param tu
 void
 corpus::add(const translation_unit_sptr tu)
 {
   m_priv->members.push_back(tu);
 }

 /// Return the list of translation units of the current corpus.
 ///
 /// @return the list of translation units of the current corpus.
 const translation_units&
 corpus::get_translation_units() const
 {
   return m_priv->members;
 }

/// Erase the translation units contained in this in-memory object.
///
/// Note that the on-disk archive file that contains the serialized
/// representation of this object is not modified.
void
corpus::drop_translation_units()
{
  m_priv->members.clear();
}

 /// Get the file path associated to the corpus file.
 ///
 /// A subsequent call to corpus::read will deserialize the content of
 /// the abi file expected at this path; likewise, a call to
 /// corpus::write will serialize the translation units contained in
 /// the corpus object into the on-disk file at this path.

 /// @return the file path associated to the current corpus.
 string&
 corpus::get_path() const
 {
   return m_priv->path;
 }

/// Set the file path associated to the corpus file.
///
/// A subsequent call to corpus::read will deserialize the content of
/// the abi file expected at this path; likewise, a call to
/// corpus::write will serialize the translation units contained in
/// the corpus object into the on-disk file at this path.
/// @param the new file path to assciate to the current corpus.
void
corpus::set_path(const string& path)
{
  m_priv->path = path;
}

 /// Tests if the corpus contains no translation unit.
 ///
 /// @return true if the corpus contains no translation unit.
 bool
 corpus::is_empty() const
 {
   return m_priv->members.empty();
 }

}// end namespace abigail
