// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations for an elf-based.  DWARF and
/// CTF readers can inherit this one.


#ifndef __ABG_ELF_BASED_READER_H__
#define __ABG_ELF_BASED_READER_H__

#include <memory>
#include <string>

#include "abg-elf-reader.h"
#include "abg-corpus.h"

namespace abigail
{

/// The common interface of readers based on ELF.
///
/// These are for readers like DWARF and CTF readers that read debug
/// information describing binaries in the ELF format.
///
/// This interface extends the elf_reader::reader interface and thus
/// also provides facilities for reading ELF binaries.
class elf_based_reader : public elf::reader
{
  struct priv;
  priv* priv_;

  elf_based_reader() = delete;

protected:

  /// Readers that implement this interface must provide a factory
  /// method to create a reader instance as this constructor is
  /// protected.
  elf_based_reader(const std::string& elf_path,
		   const vector<char**>& debug_info_root_paths,
		   environment& env);
public:

  ~elf_based_reader();

  virtual void
  reset(const std::string& elf_path,
	const vector<char**>& debug_info_root_paths);

  virtual ir::corpus_sptr
  read_and_add_corpus_to_group(ir::corpus_group& group,
			       fe_iface::status& status);
};//end class elf_based_reader

typedef std::shared_ptr<elf_based_reader> elf_based_reader_sptr;
}// namespace
#endif
