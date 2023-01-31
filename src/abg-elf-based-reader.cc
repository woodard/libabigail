// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions of the entry points of the
/// generic interface for ELF-based front-ends.  The generic interface
/// for ELF-based front-ends is named @ref elf_based_reader.  Examples
/// of front-ends that implement that interface are @ref
/// abigail::dwarf_reader::reader and abigail::ctf_raeder::reader.

#include "abg-internal.h"

// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-elf-based-reader.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>
namespace abigail
{

/// The private data of the @ref elf_based_reader type.
struct elf_based_reader::priv
{

  void
  initialize()
  {
  }

  priv()
  {
    initialize();
  }
}; // end struct elf_based_reader::priv

/// Constructor of the @erf elf_based_reader type.
///
/// @param elf_path the path the ELF file to read.
///
/// @param debug_info_root_paths a vector of paths to look into for
/// split debug info files.
///
/// @param env the environment used by the reader.
elf_based_reader::elf_based_reader(const std::string& elf_path,
				   const vector<char**>& debug_info_root_paths,
				   environment& env)
  : elf::reader(elf_path, debug_info_root_paths, env),
    priv_(new priv)
{
  priv_->initialize();
}

/// Destructor of the reader.
elf_based_reader::~elf_based_reader()
{delete priv_;}

/// Reset (re-initialize) the resources used by the current reader.
///
/// This frees the resources of the current reader and gets it ready
/// to read data from another ELF file.
///
/// @param elf_path the path to the new ELF file to consider.
///
/// @param debug_info_root_paths a vector of paths to look into for
/// split debug info files.
void
elf_based_reader::reset(const std::string& elf_path,
			const vector<char**>& debug_info_root_paths)
{
  elf::reader::reset(elf_path, debug_info_root_paths);
  priv_->initialize();
}

/// Read an ABI corpus and add it to a given corpus group.
///
/// @param group the corpus group to consider.  The new corpus is
/// added to this group.
///
/// @param status output parameter.  This is the status of the
/// creation of the current ABI corpus.  It's set by this function iff
/// a non-nil @ref corpus_sptr is returned.
///
/// @return the resulting ABI corpus.
ir::corpus_sptr
elf_based_reader::read_and_add_corpus_to_group(ir::corpus_group& group,
					       fe_iface::status& status)
{
  group.add_corpus(corpus());
  ir::corpus_sptr corp = read_corpus(status);
  return corp;
}

} // end namespace abigail
