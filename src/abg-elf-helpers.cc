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
/// This contains the definitions of the ELF utilities for the dwarf reader.

#include "abg-elf-helpers.h"

#include <elf.h>

#include "abg-tools-utils.h"

namespace abigail
{

namespace elf_helpers
{

/// Convert an elf symbol type (given by the ELF{32,64}_ST_TYPE
/// macros) into an elf_symbol::type value.
///
/// Note that this function aborts when given an unexpected value.
///
/// @param the symbol type value to convert.
///
/// @return the converted value.
elf_symbol::type
stt_to_elf_symbol_type(unsigned char stt)
{
  switch (stt)
    {
    case STT_NOTYPE:
      return elf_symbol::NOTYPE_TYPE;
    case STT_OBJECT:
      return elf_symbol::OBJECT_TYPE;
    case STT_FUNC:
      return elf_symbol::FUNC_TYPE;
    case STT_SECTION:
      return elf_symbol::SECTION_TYPE;
    case STT_FILE:
      return elf_symbol::FILE_TYPE;
    case STT_COMMON:
      return elf_symbol::COMMON_TYPE;
    case STT_TLS:
      return elf_symbol::TLS_TYPE;
    case STT_GNU_IFUNC:
      return elf_symbol::GNU_IFUNC_TYPE;
    default:
      // An unknown value that probably ought to be supported?  Let's
      // abort right here rather than yielding garbage.
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert an elf symbol binding (given by the ELF{32,64}_ST_BIND
/// macros) into an elf_symbol::binding value.
///
/// Note that this function aborts when given an unexpected value.
///
/// @param the symbol binding value to convert.
///
/// @return the converted value.
elf_symbol::binding
stb_to_elf_symbol_binding(unsigned char stb)
{
  switch (stb)
    {
    case STB_LOCAL:
      return elf_symbol::LOCAL_BINDING;
    case STB_GLOBAL:
      return elf_symbol::GLOBAL_BINDING;
    case STB_WEAK:
      return elf_symbol::WEAK_BINDING;
    case STB_GNU_UNIQUE:
      return elf_symbol::GNU_UNIQUE_BINDING;
    default:
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert an ELF symbol visiblity given by the symbols ->st_other
/// data member as returned by the GELF_ST_VISIBILITY macro into a
/// elf_symbol::visiblity value.
///
/// @param stv the value of the ->st_other data member of the ELF
/// symbol.
///
/// @return the converted elf_symbol::visiblity value.
elf_symbol::visibility
stv_to_elf_symbol_visibility(unsigned char stv)
{
  switch (stv)
    {
    case STV_DEFAULT:
      return elf_symbol::DEFAULT_VISIBILITY;
    case STV_INTERNAL:
      return elf_symbol::INTERNAL_VISIBILITY;
    case STV_HIDDEN:
      return elf_symbol::HIDDEN_VISIBILITY;
    case STV_PROTECTED:
      return elf_symbol::PROTECTED_VISIBILITY;
    default:
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert the value of the e_machine field of GElf_Ehdr into a
/// string.  This is to get a string representing the architecture of
/// the elf file at hand.
///
/// @param e_machine the value of GElf_Ehdr::e_machine.
///
/// @return the string representation of GElf_Ehdr::e_machine.
std::string
e_machine_to_string(GElf_Half e_machine)
{
  switch (e_machine)
    {
    case EM_NONE:
      return "elf-no-arch";
    case EM_M32:
      return "elf-att-we-32100";
    case EM_SPARC:
      return "elf-sun-sparc";
    case EM_386:
      return "elf-intel-80386";
    case EM_68K:
      return "elf-motorola-68k";
    case EM_88K:
      return "elf-motorola-88k";
    case EM_860:
      return "elf-intel-80860";
    case EM_MIPS:
      return "elf-mips-r3000-be";
    case EM_S370:
      return "elf-ibm-s370";
    case EM_MIPS_RS3_LE:
      return "elf-mips-r3000-le";
    case EM_PARISC:
      return "elf-hp-parisc";
    case EM_VPP500:
      return "elf-fujitsu-vpp500";
    case EM_SPARC32PLUS:
      return "elf-sun-sparc-v8plus";
    case EM_960:
      return "elf-intel-80960";
    case EM_PPC:
      return "elf-powerpc";
    case EM_PPC64:
      return "elf-powerpc-64";
    case EM_S390:
      return "elf-ibm-s390";
    case EM_V800:
      return "elf-nec-v800";
    case EM_FR20:
      return "elf-fujitsu-fr20";
    case EM_RH32:
      return "elf-trw-rh32";
    case EM_RCE:
      return "elf-motorola-rce";
    case EM_ARM:
      return "elf-arm";
    case EM_FAKE_ALPHA:
      return "elf-digital-alpha";
    case EM_SH:
      return "elf-hitachi-sh";
    case EM_SPARCV9:
      return "elf-sun-sparc-v9-64";
    case EM_TRICORE:
      return "elf-siemens-tricore";
    case EM_ARC:
      return "elf-argonaut-risc-core";
    case EM_H8_300:
      return "elf-hitachi-h8-300";
    case EM_H8_300H:
      return "elf-hitachi-h8-300h";
    case EM_H8S:
      return "elf-hitachi-h8s";
    case EM_H8_500:
      return "elf-hitachi-h8-500";
    case EM_IA_64:
      return "elf-intel-ia-64";
    case EM_MIPS_X:
      return "elf-stanford-mips-x";
    case EM_COLDFIRE:
      return "elf-motorola-coldfire";
    case EM_68HC12:
      return "elf-motorola-68hc12";
    case EM_MMA:
      return "elf-fujitsu-mma";
    case EM_PCP:
      return "elf-siemens-pcp";
    case EM_NCPU:
      return "elf-sony-ncpu";
    case EM_NDR1:
      return "elf-denso-ndr1";
    case EM_STARCORE:
      return "elf-motorola-starcore";
    case EM_ME16:
      return "elf-toyota-me16";
    case EM_ST100:
      return "elf-stm-st100";
    case EM_TINYJ:
      return "elf-alc-tinyj";
    case EM_X86_64:
      return "elf-amd-x86_64";
    case EM_PDSP:
      return "elf-sony-pdsp";
    case EM_FX66:
      return "elf-siemens-fx66";
    case EM_ST9PLUS:
      return "elf-stm-st9+";
    case EM_ST7:
      return "elf-stm-st7";
    case EM_68HC16:
      return "elf-motorola-68hc16";
    case EM_68HC11:
      return "elf-motorola-68hc11";
    case EM_68HC08:
      return "elf-motorola-68hc08";
    case EM_68HC05:
      return "elf-motorola-68hc05";
    case EM_SVX:
      return "elf-sg-svx";
    case EM_ST19:
      return "elf-stm-st19";
    case EM_VAX:
      return "elf-digital-vax";
    case EM_CRIS:
      return "elf-axis-cris";
    case EM_JAVELIN:
      return "elf-infineon-javelin";
    case EM_FIREPATH:
      return "elf-firepath";
    case EM_ZSP:
      return "elf-lsi-zsp";
    case EM_MMIX:
      return "elf-don-knuth-mmix";
    case EM_HUANY:
      return "elf-harvard-huany";
    case EM_PRISM:
      return "elf-sitera-prism";
    case EM_AVR:
      return "elf-atmel-avr";
    case EM_FR30:
      return "elf-fujistu-fr30";
    case EM_D10V:
      return "elf-mitsubishi-d10v";
    case EM_D30V:
      return "elf-mitsubishi-d30v";
    case EM_V850:
      return "elf-nec-v850";
    case EM_M32R:
      return "elf-mitsubishi-m32r";
    case EM_MN10300:
      return "elf-matsushita-mn10300";
    case EM_MN10200:
      return "elf-matsushita-mn10200";
    case EM_PJ:
      return "elf-picojava";
    case EM_OPENRISC:
      return "elf-openrisc-32";
    case EM_ARC_A5:
      return "elf-arc-a5";
    case EM_XTENSA:
      return "elf-tensilica-xtensa";

#ifdef HAVE_EM_AARCH64_MACRO
    case EM_AARCH64:
      return "elf-arm-aarch64";
#endif

#ifdef HAVE_EM_TILEPRO_MACRO
    case EM_TILEPRO:
      return "elf-tilera-tilepro";
#endif

#ifdef HAVE_EM_TILEGX_MACRO
    case EM_TILEGX:
      return "elf-tilera-tilegx";
#endif

    case EM_NUM:
      return "elf-last-arch-number";
    case EM_ALPHA:
      return "elf-non-official-alpha";
    default:
      {
	std::ostringstream o;
	o << "elf-unknown-arch-value-" << e_machine;
	return o.str();
      }
    }
}

/// Find and return a section by its name and its type.
///
/// @param elf_handle the elf handle to use.
///
/// @param name the name of the section.
///
/// @param section_type the type of the section.  This is the
/// Elf32_Shdr::sh_type (or Elf64_Shdr::sh_type) data member.
/// Examples of values of this parameter are SHT_PROGBITS or SHT_NOBITS.
///
/// @return the section found, nor nil if none was found.
Elf_Scn*
find_section(Elf* elf_handle, const std::string& name, Elf64_Word section_type)
{
  size_t section_header_string_index = 0;
  if (elf_getshdrstrndx (elf_handle, &section_header_string_index) < 0)
    return 0;

  Elf_Scn* section = 0;
  GElf_Shdr header_mem, *header;
  while ((section = elf_nextscn(elf_handle, section)) != 0)
    {
      header = gelf_getshdr(section, &header_mem);
      if (header == NULL || header->sh_type != section_type)
      continue;

      const char* section_name =
	elf_strptr(elf_handle, section_header_string_index, header->sh_name);
      if (section_name && name == section_name)
	return section;
    }

  return 0;
}

/// Find the symbol table.
///
/// If we are looking at a relocatable or executable file, this
/// function will return the .symtab symbol table (of type
/// SHT_SYMTAB).  But if we are looking at a DSO it returns the
/// .dynsym symbol table (of type SHT_DYNSYM).
///
/// @param elf_handle the elf handle to consider.
///
/// @param symtab the symbol table found.
///
/// @return true iff the symbol table is found.
bool
find_symbol_table_section(Elf* elf_handle, Elf_Scn*& symtab)
{
  Elf_Scn* section = 0, *dynsym = 0, *sym_tab = 0;
  while ((section = elf_nextscn(elf_handle, section)) != 0)
    {
      GElf_Shdr header_mem, *header;
      header = gelf_getshdr(section, &header_mem);
      if (header->sh_type == SHT_DYNSYM)
	dynsym = section;
      else if (header->sh_type == SHT_SYMTAB)
	sym_tab = section;
    }

  if (dynsym || sym_tab)
    {
      GElf_Ehdr eh_mem;
      GElf_Ehdr* elf_header = gelf_getehdr(elf_handle, &eh_mem);
      if (elf_header->e_type == ET_REL
	  || elf_header->e_type == ET_EXEC)
	symtab = sym_tab ? sym_tab : dynsym;
      else
	symtab = dynsym ? dynsym : sym_tab;
      return true;
    }
  return false;
}

/// Find the index (in the section headers table) of the symbol table
/// section.
///
/// If we are looking at a relocatable or executable file, this
/// function will return the index for the .symtab symbol table (of
/// type SHT_SYMTAB).  But if we are looking at a DSO it returns the
/// index for the .dynsym symbol table (of type SHT_DYNSYM).
///
/// @param elf_handle the elf handle to use.
///
/// @param symtab_index the index of the symbol_table, that was found.
///
/// @return true iff the symbol table section index was found.
bool
find_symbol_table_section_index(Elf* elf_handle, size_t& symtab_index)
{
  Elf_Scn* section = 0;
  if (!find_symbol_table_section(elf_handle, section))
    return false;

  symtab_index = elf_ndxscn(section);
  return true;
}

/// Get the offset offset of the hash table section.
///
/// @param elf_handle the elf handle to use.
///
/// @param ht_section_offset this is set to the resulting offset
/// of the hash table section.  This is set iff the function returns true.
///
/// @param symtab_section_offset the offset of the section of the
/// symbol table the hash table refers to.
hash_table_kind
find_hash_table_section_index(Elf*	elf_handle,
			      size_t&	ht_section_index,
			      size_t&	symtab_section_index)
{
  if (!elf_handle)
    return NO_HASH_TABLE_KIND;

  GElf_Shdr header_mem, *section_header;
  bool found_sysv_ht = false, found_gnu_ht = false;
  for (Elf_Scn* section = elf_nextscn(elf_handle, 0);
       section != 0;
       section = elf_nextscn(elf_handle, section))
    {
      section_header= gelf_getshdr(section, &header_mem);
      if (section_header->sh_type != SHT_HASH
	  && section_header->sh_type != SHT_GNU_HASH)
	continue;

      ht_section_index = elf_ndxscn(section);
      symtab_section_index = section_header->sh_link;

      if (section_header->sh_type == SHT_HASH)
	found_sysv_ht = true;
      else if (section_header->sh_type == SHT_GNU_HASH)
	found_gnu_ht = true;
    }

  if (found_gnu_ht)
    return GNU_HASH_TABLE_KIND;
  else if (found_sysv_ht)
    return SYSV_HASH_TABLE_KIND;
  else
    return NO_HASH_TABLE_KIND;
}

/// Find and return the .text section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .text section found.
Elf_Scn*
find_text_section(Elf* elf_handle)
{return find_section(elf_handle, ".text", SHT_PROGBITS);}

/// Find and return the .bss section.
///
/// @param elf_handle.
///
/// @return the .bss section found.
Elf_Scn*
find_bss_section(Elf* elf_handle)
{return find_section(elf_handle, ".bss", SHT_NOBITS);}

/// Find and return the .rodata section.
///
/// @param elf_handle.
///
/// @return the .rodata section found.
Elf_Scn*
find_rodata_section(Elf* elf_handle)
{return find_section(elf_handle, ".rodata", SHT_PROGBITS);}

/// Find and return the .data section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .data section found.
Elf_Scn*
find_data_section(Elf* elf_handle)
{return find_section(elf_handle, ".data", SHT_PROGBITS);}

/// Find and return the .data1 section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .data1 section found.
Elf_Scn*
find_data1_section(Elf* elf_handle)
{return find_section(elf_handle, ".data1", SHT_PROGBITS);}


} // end namespace elf_helpers
} // end namespace abigail
