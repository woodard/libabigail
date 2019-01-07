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
/// Utilities to ease the wrapping of C types into std::tr1::shared_ptr

#ifndef __ABG_SPTR_UTILS_H__
#define __ABG_SPTR_UTILS_H__

#include <tr1/memory>
#include <regex.h>
#include <libxml/xmlreader.h>

namespace abigail
{

/// Namespace for the utilities to wrap C types into std::tr1::shared_ptr.
namespace sptr_utils
{

using std::tr1::shared_ptr;

/// This is to be specialized for the diverse C types that needs
/// wrapping in shared_ptr.
///
/// @tparam T the type of the C type to wrap in a shared_ptr.
///
/// @param p a pointer to wrap in a shared_ptr.
///
/// @return then newly created shared_ptr<T>
template<class T>
shared_ptr<T>
build_sptr(T* p);

/// This is to be specialized for the diverse C types that needs
/// wrapping in shared_ptr.
///
/// This variant creates a pointer to T and wraps it into a
/// shared_ptr<T>.
///
/// @tparam T the type of the C type to wrap in a shared_ptr.
///
/// @return then newly created shared_ptr<T>
template<class T>
shared_ptr<T>
build_sptr();

/// A convenience typedef for a shared pointer of xmlTextReader.
typedef shared_ptr<xmlTextReader> reader_sptr;

/// Specialization of sptr_utils::build_sptr for xmlTextReader
template<>
reader_sptr
build_sptr<xmlTextReader>(xmlTextReader *p);

/// A convenience typedef for a shared pointer of xmlChar.
typedef shared_ptr<xmlChar> xml_char_sptr;

/// Specialization of build_str for xmlChar.
template<>
xml_char_sptr build_sptr<xmlChar>(xmlChar *p);

/// A convenience typedef for a shared pointer of regex_t.
typedef shared_ptr<regex_t> regex_t_sptr;

/// Specialization of sptr_utils::build_sptr for regex_t.
template<>
regex_t_sptr
build_sptr<regex_t>(regex_t *p);

/// Specialization of sptr_utils::build_sptr for regex_t.
template<>
regex_t_sptr
build_sptr<regex_t>();

/// A deleter for shared pointers that ... doesn't delete the object
/// managed by the shared pointer.
struct noop_deleter
{
  template<typename T>
  void
  operator()(const T*)
  {}
};

/// A delete functor for a shared_ptr of regex_t.
struct regex_t_deleter
{
  /// The operator called to de-allocate the pointer to regex_t
  /// embedded in a shared_ptr<regex_t>
  ///
  /// @param r the pointer to regex_t to de-allocate.
  void
  operator()(::regex_t* r)
  {
    regfree(r);
    delete r;
  }
};//end struct regex_deleter

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This is used to wrap a pointer to regex_t into a
/// shared_ptr<regex_t>.
///
/// @param p the bare pointer to regex_t to wrap into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> that wraps @p p.
template<>
regex_t_sptr
build_sptr<regex_t>(regex_t *p);

/// Specialization of sptr_utils::build_sptr for regex_t.
///
/// This creates a pointer to regex_t and wraps it into a shared_ptr<regex_t>.
///
/// @return the shared_ptr<regex_t> wrapping the newly created regex_t*
template<>
regex_t_sptr
build_sptr<regex_t>();

}// end namespace sptr_utils
}// end namespace abigail

#endif //__ABG_SPTR_UTILS_H__
