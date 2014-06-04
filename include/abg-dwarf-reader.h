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
/// de-serialize an instance of @ref abigail::corpus from a file in
/// elf format, containing dwarf information.

#include <ostream>
#include "abg-corpus.h"

#ifndef __ABG_DWARF_READER_H__
#define __ABG_DWARF_READER_H__

namespace abigail
{

namespace dwarf_reader
{

/// The status of the @ref read_corpus_from_elf() call.
enum status
{
  /// This status is for when the call went OK.
  STATUS_OK,

  /// This satus is for when the debug info could not be read.
  STATUS_DEBUG_INFO_NOT_FOUND,

  /// This status is for when the symbols of the ELF binaries could
  /// not be read.
  STATUS_NO_SYMBOLS_FOUND,
};

status
read_corpus_from_elf(const std::string& elf_path,
		     char** debug_info_root_path,
		     corpus_sptr& resulting_corp);

bool
lookup_symbol_from_elf(const string&		elf_path,
		       const string&		symbol_name,
		       bool			demangle,
		       vector<elf_symbol>&	symbols);

bool
lookup_public_function_symbol_from_elf(const string&		path,
				       const string&		symname,
				       vector<elf_symbol>&	func_syms);

}// end namespace dwarf_reader

}// end namespace abigail

#endif //__ABG_DWARF_READER_H__
