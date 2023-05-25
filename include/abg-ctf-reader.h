// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2021-2023 Oracle, Inc.
//
// Author: Jose E. Marchesi

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref abigail::corpus from a file in
/// elf format, containing CTF information.

#ifndef __ABG_CTF_READER_H__
#define __ABG_CTF_READER_H__

#include <ostream>
#include "abg-corpus.h"
#include "abg-suppression.h"
#include "abg-elf-based-reader.h"

#include "ctf-api.h"

namespace abigail
{
namespace ctf
{

elf_based_reader_sptr
create_reader(const std::string& elf_path,
	      const vector<char**>& debug_info_root_paths,
	      environment& env);

void
reset_reader(elf_based_reader&		ctxt,
	     const std::string&	elf_path,
	     const vector<char**>&	debug_info_root_path);
} // end namespace ctf_reader
} // end namespace abigail

#endif // ! __ABG_CTF_READER_H__
