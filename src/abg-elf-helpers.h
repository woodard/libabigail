// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020-2023 Google, Inc.

/// @file
///
/// This contains a set of ELF utilities used by the dwarf reader.

#ifndef __ABG_ELF_HELPERS_H__
#define __ABG_ELF_HELPERS_H__

#include "config.h"

#include <elfutils/libdwfl.h>
#include <gelf.h>
#include <string>

#include "abg-ir.h"

namespace abigail
{

namespace elf_helpers
{

/// A functor used by @ref dwfl_sptr.
struct dwfl_deleter
{
  void
  operator()(Dwfl* dwfl)
  {dwfl_end(dwfl);}
};//end struct dwfl_deleter

/// A convenience typedef for a shared pointer to a Dwfl.
typedef shared_ptr<Dwfl> dwfl_sptr;

/// Convenience typedef for a map which key is an elf address and
/// which value is an elf_symbol_sptr.
typedef unordered_map<GElf_Addr, elf_symbol_sptr> addr_elf_symbol_sptr_map_type;

/// Convenience typedef for a set of ELF addresses.
typedef unordered_set<GElf_Addr> address_set_type;

/// Convenience typedef for a shared pointer to an @ref address_set_type.
typedef shared_ptr<address_set_type> address_set_sptr;

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
find_section_by_name(Elf* elf_handle, const std::string& name);

Elf_Scn*
find_section(Elf* elf_handle, Elf64_Word section_type);

Elf_Scn*
find_symtab_section(Elf* elf_handle);

Elf_Scn*
find_dynsym_section(Elf* elf_handle);

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

Elf_Scn*
find_strtab_for_symtab_section(Elf*	elf_handle,
                               Elf_Scn*	symtab_section);

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

bool
get_crc_for_symbol(Elf* elf_handle, GElf_Sym* crc_symbol, uint32_t& crc_value);

//
// Architecture specific helpers
//
bool
architecture_is_ppc64(Elf* elf_handle);

bool
architecture_is_ppc32(Elf* elf_handle);

bool
architecture_is_arm32(Elf* elf_handle);

bool
architecture_is_arm64(Elf* elf_handle);

bool
architecture_is_big_endian(Elf* elf_handle);

GElf_Addr
lookup_ppc64_elf_fn_entry_point_address(Elf*	  elf_handle,
					GElf_Addr fn_desc_address);

//
// Helpers for Linux Kernel Binaries
//

bool
is_linux_kernel_module(Elf *elf_handle);

bool
is_linux_kernel(Elf *elf_handle);

//
// elfutils helpers
//

const Dwfl_Callbacks&
initialize_dwfl_callbacks(Dwfl_Callbacks&, char**);

dwfl_sptr
create_new_dwfl_handle(Dwfl_Callbacks&);

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

bool
address_is_in_opd_section(Elf* elf_handle, Dwarf_Addr addr);

bool
lookup_data_tag_from_dynamic_segment(Elf*                       elf,
                                     Elf64_Sxword               data_tag,
                                     vector<string>&            dt_tag_data);

bool
get_soname_of_elf_file(const string& path, string &soname);
} // end namespace elf_helpers
} // end namespace abigail

#endif // __ABG_ELF_HELPERS_H__
