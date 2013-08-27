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
  vector<string> serialized_tus;
  mutable zip_sptr		archive;

   impl(const string &p)
     : path(p)
   {}

  zip_sptr
  get_archive() const
  {
    int error_code = 0;
    if (!archive)
      {
	// Open the zip archive.  If no archive at that path existed,
	// create a new archive.
	archive = open_archive(path, ZIP_CREATE|ZIP_CHECKCONS, &error_code);
	if (error_code)
	  {
	    std::ostringstream o;
	    o << "zip_create returned error code '" << error_code << "'";
	    if (archive)
	      o << " and returned a non-null archive";
	    throw std::runtime_error(o.str());
	  }
      }
    return archive;
  }

  /// Closes the zip archive associated to the current corpus, if any.
  ///
  /// Note that closing the archive writes the content (that was added
  /// to it) to the disk and frees the memory associated to the
  /// ressources held by the archive.
  void
  close_archive()
  {
    if (archive)
      archive.reset();
  }

  /// @brief Write a translation unit to the current zip archive
  /// associated to to the current corpus.
  ///
  /// If the translation unit was already present (and loaded) in the
  /// current archive, this function replaces the version in the
  /// archive with the translation unit given in parameter, otherwise,
  /// the translation unit given in parameter is just added to the
  /// current archive.  Note that the updated archive is not saved to
  /// disk until the fonction close_archive() is invoked.
  bool
  write_tu_to_archive(const translation_unit& tu)
  {
    ostringstream os;

    zip_sptr ar = get_archive();
    if (!archive)
      return false;

    if (!tu.write(os))
      return false;

    serialized_tus.push_back(os.str());

    zip_source *source;
    if ((source = zip_source_buffer(ar.get(),
				    serialized_tus.back().c_str(),
				    serialized_tus.back().size(),
				    false)) == 0)
      return false;



    int index = zip_name_locate(ar.get(), tu.get_path().c_str(), 0);
    if ( index == -1)
      {
	if (zip_add(ar.get(), tu.get_path().c_str(), source) < 0)
	  {
	    zip_source_free(source);
	    return false;
	  }
      }
    else
      {
	if (zip_replace(ar.get(), index, source) != 0)
	  {
	    zip_source_free(source);
	    return false;
	  }
      }

    return true;
  }

  /// Read a file that is at a particular index in the archive, into a
  /// translation_unit.
  ///
  /// @param tu the translation unit to read the content of the file
  /// into.
  ///
  /// @param file_index the index of the file to read in.
  ///
  /// @return true upon successful completion, false otherwise.
  bool
  read_to_translation_unit(translation_unit& tu,
			   int file_index)
  {
    zip_sptr ar = get_archive();
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

    if (!tu.read(input))
      return false;

    return true;
  }

 private:
   impl();
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
 const corpus::translation_units&
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
 corpus::get_file_path() const
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
corpus::set_file_path(const string& path)
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

 /// Serialize the current corpus to disk in a file which path is given
 /// by corpus::get_file_path.
 ///
 /// @return true upon successful completion, false otherwise.
bool
corpus::write() const
{
  for (translation_units::const_iterator i = get_translation_units().begin();
       i != get_translation_units().end();
       ++i)
    {
      if (! m_priv->write_tu_to_archive(**i))
	return false;
    }
  // TODO: ensure abi-info descriptor is added to the archive.
  m_priv->close_archive();
  return true;
}

/// Open the archive which path is given by corpus::get_file_path and
/// de-serialize each of the translation units it contains.
///
/// @return the number of entries read and properly de-serialized,
/// zero if none was red.
int
corpus::read()
{
  zip_sptr ar = m_priv->get_archive();
  if (!ar)
    return false;

  int nb_of_tu_read = 0;
  int nb_entries = zip_get_num_entries(ar.get(), 0);
  if (nb_entries < 0)
    return 0;

  // TODO: ensure abi-info descriptor is present in the archive.  Read
  // it and ensure that version numbers match.
  for (int i = 0; i < nb_entries; ++i)
    {
      shared_ptr<translation_unit>
	tu(new translation_unit(zip_get_name(ar.get(), i, 0)));
      if (m_priv->read_to_translation_unit(*tu, i))
	{
	  add(tu);
	  ++nb_of_tu_read;
	}
    }
  return nb_of_tu_read;
}

}// end namespace abigail
