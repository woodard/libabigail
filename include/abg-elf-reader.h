// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations for the @ref fe_iface a.k.a
/// "Front End Interface".

#ifndef __ABG_ELF_READER_H__
#define __ABG_ELF_READER_H__

#include <memory>
#include <string>

#include <elfutils/libdwfl.h>

#include "abg-fe-iface.h"
#include "abg-ir.h"
#include "abg-suppression.h"

namespace abigail
{

/// The namespace for the ELF Reader.
namespace elf
{

/// The kind of ELF file we are looking at.
enum elf_type : unsigned
{
  /// A normal executable binary
  ELF_TYPE_EXEC,
  /// A Position Independant Executable binary
  ELF_TYPE_PI_EXEC,
  /// A dynamic shared object, a.k.a shared library binary.
  ELF_TYPE_DSO,
  /// A relocatalbe binary.
  ELF_TYPE_RELOCATABLE,
  /// An unknown kind of binary.
  ELF_TYPE_UNKNOWN
};

/// This is the interface an ELF reader.
///
/// It knows how to open an ELF file, read its content and expose an
/// interface for its symbol table and other properties.
///
/// Note that the ABI corpus returned by the elf::read_corpus()
/// member function doesn't contain any type representation.  It only
/// contains the representations of the the ELF symbols found in the
/// ELF file.
///
/// To construct the type representations for the functions and global
/// variables present in the ELF file, please use the implementations
/// of the @ref elf_based_reader interface.  Those know how to read
/// the debug information from the ELF file to build type
/// representation in the @ref abigail::ir::corpus instance.
class reader : public fe_iface
{
  struct priv;
  priv *priv_;

 public:

  reader(const std::string&	elf_path,
	 const vector<char**>&	debug_info_roots,
	 environment&		env);

  ~reader();

  void
  reset(const std::string&	elf_path,
	 const vector<char**>&	debug_info_roots);

  const vector<char**>&
  debug_info_root_paths() const;

  const Dwfl_Callbacks&
  dwfl_offline_callbacks() const;

  Dwfl_Callbacks&
  dwfl_offline_callbacks();

  Elf*
  elf_handle() const;

  const Dwarf*
  dwarf_debug_info() const;

  bool
  has_dwarf_debug_info() const;

  bool
  has_ctf_debug_info() const;

  const Dwarf*
  alternate_dwarf_debug_info() const;

  const string&
  alternate_dwarf_debug_info_path() const;

  bool
  refers_to_alt_debug_info(string& alt_di_path) const;

  const Elf_Scn*
  find_symbol_table_section() const;

  void
  reset_symbol_table_section();

  const Elf_Scn*
  find_ctf_section() const;

  const Elf_Scn*
  find_alternate_ctf_section() const;

  const vector<string>&
  dt_needed()const;

  const string&
  elf_architecture() const;

  symtab_reader::symtab_sptr&
  symtab() const;

  elf_symbol_sptr
  function_symbol_is_exported(GElf_Addr symbol_address) const;

  elf_symbol_sptr
  variable_symbol_is_exported(GElf_Addr symbol_address) const;

  elf_symbol_sptr
  function_symbol_is_exported(const string& name) const;

  elf_symbol_sptr
  variable_symbol_is_exported(const string& name) const;

  void
  load_dt_soname_and_needed();

  void
  load_elf_architecture();

  void
  load_elf_properties();

  virtual ir::corpus_sptr
  read_corpus(status& status);
};//end class reader.

/// A convenience typedef for a smart pointer to a
/// elf::reader.
typedef shared_ptr<elf::reader> reader_sptr;

bool
get_soname_of_elf_file(const string& path, string &soname);

bool
get_type_of_elf_file(const string& path, elf_type& type);
} // end namespace elf.
} // end namespace abigail

#endif // __ABG_ELF_READER_H__
