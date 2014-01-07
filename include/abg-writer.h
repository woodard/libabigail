// -*- Mode: C++ -*-
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
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref translation_unit to an ABI
/// Instrumentation file in libabigail native XML format.

#ifndef __ABG_WRITER_H__
#define __ABG_WRITER_H__

namespace abigail
{
namespace xml_writer
{
bool
write_translation_unit(const translation_unit&	tu,
		       unsigned		indent,
		       std::ostream&		out);

bool
write_translation_unit(const translation_unit&	tu,
		       unsigned		indent,
		       const string&		path);

bool
write_corpus_to_archive(const corpus& corp,
			const string& path);

bool
write_corpus_to_archive(const corpus& corp);

bool
write_corpus_to_archive(const corpus_sptr corp);

}// end namespace xml_writer
}// end namespace abigail

#endif //  __ABG_WRITER_H__
