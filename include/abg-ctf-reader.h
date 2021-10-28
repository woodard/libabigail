// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2021 Oracle, Inc.
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

namespace abigail
{
namespace ctf_reader
{

class read_context;
read_context *create_read_context (std::string elf_path,
                                   ir::environment *env);
corpus_sptr read_corpus (read_context *ctxt);

} // end namespace ctf_reader
} // end namespace abigail

#endif // ! __ABG_CTF_READER_H__
