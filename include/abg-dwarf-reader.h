// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref abigail::corpus from a file in
/// elf format, containing dwarf information.

#ifndef __ABG_DWARF_READER_H__
#define __ABG_DWARF_READER_H__

#include <ostream>
#include <elfutils/libdwfl.h>
#include "abg-corpus.h"
#include "abg-suppression.h"
#include "abg-elf-based-reader.h"

namespace abigail
{

/// The namespace for the DWARF reader.
namespace dwarf
{

using namespace abigail::ir;

elf_based_reader_sptr
create_reader(const std::string&	elf_path,
	      const vector<char**>& debug_info_root_paths,
	      environment&	environment,
	      bool		read_all_types = false,
	      bool		linux_kernel_mode = false);

void
reset_reader(elf_based_reader&		rdr,
	     const std::string&		elf_path,
	     const vector<char**>&	debug_info_root_paths,
	     bool			read_all_types = false,
	     bool			linux_kernel_mode = false);

corpus_sptr
read_corpus_from_elf(const std::string&	elf_path,
		     const vector<char**>&	debug_info_root_paths,
		     environment&		environment,
		     bool			load_all_types,
		     fe_iface::status&		status);

bool
lookup_symbol_from_elf(const environment&		env,
		       const string&			elf_path,
		       const string&			symbol_name,
		       bool				demangle,
		       vector<elf_symbol_sptr>&	symbols);

bool
lookup_public_function_symbol_from_elf(const environment&		env,
				       const string&			path,
				       const string&			symname,
				       vector<elf_symbol_sptr>&	func_syms);
}// end namespace dwarf

}// end namespace abigail

#endif //__ABG_DWARF_READER_H__
