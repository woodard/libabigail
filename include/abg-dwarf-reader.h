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

#include <elf.h>
#include <ostream>
#include "abg-corpus.h"

#ifndef __ABG_DWARF_READER_H__
#define __ABG_DWARF_READER_H__

namespace abigail
{

namespace dwarf_reader
{

corpus_sptr
read_corpus_from_elf(const std::string& elf_path);

/// The type of a symbol as returned by lookup_symbol_from_elf().
enum symbol_type
{
  NOTYPE_TYPE = STT_NOTYPE,
  OBJECT_TYPE = STT_OBJECT,
  FUNC_TYPE = STT_FUNC,
  SECTION_TYPE = STT_SECTION,
  FILE_TYPE = STT_FILE,
  COMMON_TYPE = STT_COMMON,
  TLS_TYPE = STT_TLS,
  GNU_IFUNC_TYPE = STT_GNU_IFUNC
};

/// The binding of a symbol as returned by lookup_symbol_from_elf.
enum symbol_binding
{
  LOCAL_BINDING = STB_LOCAL,
  GLOBAL_BINDING = STB_GLOBAL,
  WEAK_BINDING = STB_WEAK,
  GNU_UNIQUE_BINDING = STB_GNU_UNIQUE
};

std::ostream&
operator<<(std::ostream& o, symbol_type t);

std::ostream&
operator<<(std::ostream& o, symbol_binding t);

bool
lookup_symbol_from_elf(const string&	elf_path,
		       const string&	symbol_name,
		       bool		demangle,
		       string&		symbol_name_found,
		       symbol_type&	sym_type,
		       symbol_binding&	symb_binding);

bool
lookup_public_function_symbol_from_elf(const string&	elf_path,
				       const string&	symbol_name);

}// end namespace dwarf_reader

}// end namespace abigail

#endif //__ABG_DWARF_READER_H__
