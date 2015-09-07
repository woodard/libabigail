// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2015 Red Hat, Inc.
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
/// de-serialize an instance of @ref abigail::translation_unit from an
/// ABI Instrumentation file in libabigail native XML format.

#ifndef __ABG_READER_H__
#define __ABG_READER_H__

#include <istream>
#include "abg-corpus.h"

namespace abigail
{

namespace xml_reader
{

using namespace abigail::ir;

translation_unit_sptr
read_translation_unit_from_file(const std::string&	file_path,
				environment*		env);

translation_unit_sptr
read_translation_unit_from_buffer(const std::string&	file_path,
				  environment*		env);

translation_unit_sptr
read_translation_unit_from_istream(std::istream*	in,
				   environment*	env);

abigail::corpus_sptr
read_corpus_from_file(const string& path);

int
read_corpus_from_file(corpus_sptr& corp,
		      const string& path);

int
read_corpus_from_file(corpus_sptr& corp);

corpus_sptr
read_corpus_from_native_xml(std::istream* in,
			    environment*  env);

corpus_sptr
read_corpus_from_native_xml_file(const string& path,
				 environment*  env);

}//end xml_reader
}//end namespace abigail

#endif // __ABG_READER_H__
