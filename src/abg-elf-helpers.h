// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.
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
/// This contains a set of ELF utilities used by the dwarf reader.

#ifndef __ABG_ELF_HELPERS_H__
#define __ABG_ELF_HELPERS_H__

#include "config.h"

#include <gelf.h>
#include <string>

#include "abg-ir.h"

namespace abigail
{

namespace elf_helpers
{

//
// ELF Value Converters
//

elf_symbol::type
stt_to_elf_symbol_type(unsigned char stt);

elf_symbol::binding
stb_to_elf_symbol_binding(unsigned char stb);

elf_symbol::visibility
stv_to_elf_symbol_visibility(unsigned char stv);

std::string
e_machine_to_string(GElf_Half e_machine);

//
// ELF section helpers
//

Elf_Scn*
find_section(Elf*		elf_handle,
	     const std::string& name,
	     Elf64_Word		section_type);

Elf_Scn*
find_symbol_table_section(Elf* elf_handle);

bool
find_symbol_table_section_index(Elf* elf_handle, size_t& symtab_index);

enum hash_table_kind
{
  NO_HASH_TABLE_KIND = 0,
  SYSV_HASH_TABLE_KIND,
  GNU_HASH_TABLE_KIND
};

hash_table_kind
find_hash_table_section_index(Elf*	elf_handle,
			      size_t&	ht_section_index,
			      size_t&	symtab_section_index);

Elf_Scn*
find_text_section(Elf* elf_handle);

Elf_Scn*
find_bss_section(Elf* elf_handle);

Elf_Scn*
find_rodata_section(Elf* elf_handle);

Elf_Scn*
find_data_section(Elf* elf_handle);

Elf_Scn*
find_data1_section(Elf* elf_handle);

Elf_Scn*
find_opd_section(Elf* elf_handle);

bool
get_symbol_versionning_sections(Elf*		elf_handle,
				Elf_Scn*&	versym_section,
				Elf_Scn*&	verdef_section,
				Elf_Scn*&	verneed_section);

Elf_Scn*
find_ksymtab_section(Elf* elf_handle);

Elf_Scn*
find_ksymtab_gpl_section(Elf* elf_handle);

Elf_Scn*
find_ksymtab_strings_section(Elf *elf_handle);

Elf_Scn*
find_relocation_section(Elf* elf_handle, Elf_Scn* target_section);

//
// Helpers for symbol versioning
//

bool
get_version_definition_for_versym(Elf*			 elf_handle,
				  GElf_Versym*		 versym,
				  Elf_Scn*		 verdef_section,
				  elf_symbol::version&	 version);

bool
get_version_needed_for_versym(Elf*			elf_handle,
			      GElf_Versym*		versym,
			      Elf_Scn*			verneed_section,
			      elf_symbol::version&	version);

bool
get_version_for_symbol(Elf*			elf_handle,
		       size_t			symbol_index,
		       bool			get_def_version,
		       elf_symbol::version&	version);

//
// Architecture specific helpers
//
bool
architecture_is_ppc64(Elf* elf_handle);

bool
architecture_is_big_endian(Elf* elf_handle);

//
// Helpers for Linux Kernel Binaries
//

bool
is_linux_kernel_module(Elf *elf_handle);

bool
is_linux_kernel(Elf *elf_handle);

//
// Misc Helpers
//

bool
get_binary_load_address(Elf* elf_handle, GElf_Addr& load_address);

unsigned char
get_architecture_word_size(Elf* elf_handle);

bool
is_executable(Elf* elf_handle);

bool
is_dso(Elf* elf_handle);

GElf_Addr
maybe_adjust_et_rel_sym_addr_to_abs_addr(Elf* elf_handle, GElf_Sym* sym);

} // end namespace elf_helpers
} // end namespace abigail

#endif // __ABG_ELF_HELPERS_H__
