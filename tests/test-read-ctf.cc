// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2021-2023 Oracle, Inc.
//
// Author: Guillermo E. Martinez

/// @file
///
/// This file implement the CTF testsuite. It reads ELF binaries
/// containing CTF, save them in XML corpus files and diff the
/// corpus files against reference XML corpus files.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-ctf-reader.h"
#include "test-read-common.h"

using std::string;
using std::cerr;
using std::vector;

using abigail::tests::read_common::InOutSpec;
using abigail::tests::read_common::test_task;
using abigail::tests::read_common::display_usage;
using abigail::tests::read_common::options;

using abigail::xml_writer::SEQUENCE_TYPE_ID_STYLE;
using abigail::xml_writer::HASH_TYPE_ID_STYLE;
using abigail::tools_utils::emit_prefix;

using namespace abigail;

static InOutSpec in_out_specs[] =
{
  {
    "data/test-read-ctf/test0",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test0.abi",
    "output/test-read-ctf/test0.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test0",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test0.hash.abi",
    "output/test-read-ctf/test0.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test1.so",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test1.so.abi",
    "output/test-read-ctf/test1.so.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test1.so",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test1.so.hash.abi",
    "output/test-read-ctf/test1.so.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test2.so",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test2.so.abi",
    "output/test-read-ctf/test2.so.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test2.so",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test2.so.hash.abi",
    "output/test-read-ctf/test2.so.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test3.so",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test3.so.abi",
    "output/test-read-ctf/test3.so.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test3.so",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test3.so.hash.abi",
    "output/test-read-ctf/test3.so.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-enum-many.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-enum-many.o.hash.abi",
    "output/test-read-ctf/test-enum-many.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-ambiguous-struct-A.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-ambiguous-struct-A.o.hash.abi",
    "output/test-read-ctf/test-ambiguous-struct-A.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-ambiguous-struct-B.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-ambiguous-struct-B.o.hash.abi",
    "output/test-read-ctf/test-ambiguous-struct-B.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-conflicting-type-syms-a.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-conflicting-type-syms-a.o.hash.abi",
    "output/test-read-ctf/test-conflicting-type-syms-a.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-conflicting-type-syms-b.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-conflicting-type-syms-b.o.hash.abi",
    "output/test-read-ctf/test-conflicting-type-syms-b.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test4.so",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test4.so.abi",
    "output/test-read-ctf/test4.so.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test4.so",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test4.so.hash.abi",
    "output/test-read-ctf/test4.so.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test5.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test5.o.abi",
    "output/test-read-ctf/test5.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test7.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test7.o.abi",
    "output/test-read-ctf/test7.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test8.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test8.o.abi",
    "output/test-read-ctf/test8.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test9.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test9.o.abi",
    "output/test-read-ctf/test9.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-enum.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-enum.o.abi",
    "output/test-read-ctf/test-enum.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-enum-symbol.o",
    "",
    "",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/test-enum-symbol.o.hash.abi",
    "output/test-read-ctf/test-enum-symbol.o.hash.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-dynamic-array.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-dynamic-array.o.abi",
    "output/test-read-ctf/test-dynamic-array.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-anonymous-fields.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-anonymous-fields.o.abi",
    "output/test-read-ctf/test-anonymous-fields.o.abi",
    "--ctf"
  },
  {
    "data/test-read-common/PR27700/test-PR27700.o",
    "",
    "data/test-read-common/PR27700/pub-incdir",
    HASH_TYPE_ID_STYLE,
    "data/test-read-ctf/PR27700/test-PR27700.abi",
    "output/test-read-ctf/PR27700/test-PR27700.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-callback.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-callback.abi",
    "output/test-read-ctf/test-callback.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-array-of-pointers.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-array-of-pointers.abi",
    "output/test-read-ctf/test-array-of-pointers.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-functions-declaration.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-functions-declaration.abi",
    "output/test-read-ctf/test-functions-declaration.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-forward-type-decl.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-forward-type-decl.abi",
    "output/test-read-ctf/test-forward-type-decl.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-list-struct.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-list-struct.abi",
    "output/test-read-ctf/test-list-struct.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test-PR26568-1.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-PR26568-1.o.abi",
    "output/test-read-ctf/test-PR26568-1.o.abi",
    "--ctf"
  },
  {
    "data/test-read-common/test-PR26568-2.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-PR26568-2.o.abi",
    "output/test-read-ctf/test-PR26568-2.o.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-callback2.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-callback2.abi",
    "output/test-read-ctf/test-callback2.abi",
    "--ctf"
  },
  // out-of-tree kernel module.
  {
    "data/test-read-ctf/test-linux-module.ko",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-linux-module.abi",
    "output/test-read-ctf/test-linux-module.abi",
    "--ctf"
  },
  {
    "data/test-read-ctf/test-alias.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-alias.o.abi",
    "output/test-read-ctf/test-alias.o.abi",
    "--ctf"
  },
  // CTF fallback feature.
  {
    "data/test-read-ctf/test-fallback.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-fallback.abi",
    "output/test-read-ctf/test-fallback.abi",
    NULL,
  },
  {
    "data/test-read-ctf/test-bitfield.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-bitfield.abi",
    "output/test-read-ctf/test-bitfield.abi",
    "--ctf",
  },
  {
    "data/test-read-ctf/test-bitfield-enum.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-bitfield-enum.abi",
    "output/test-read-ctf/test-bitfield-enum.abi",
    "--ctf",
  },
  {
    "data/test-read-ctf/test-const-array.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-const-array.abi",
    "output/test-read-ctf/test-const-array.abi",
    "--ctf",
  },
  {
    "data/test-read-ctf/test-array-mdimension.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-array-mdimension.abi",
    "output/test-read-ctf/test-array-mdimension.abi",
    "--ctf",
  },
  {
    "data/test-read-ctf/test-array-size.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-ctf/test-array-size.abi",
    "output/test-read-ctf/test-array-size.abi",
    "--ctf",
  },
  // This should be the last entry.
  {NULL, NULL, NULL, SEQUENCE_TYPE_ID_STYLE, NULL, NULL, NULL}
};

/// Task specialization to perform CTF tests.
struct test_task_ctf : public test_task
{
  test_task_ctf(const InOutSpec &s,
                string& a_out_abi_base,
                string& a_in_elf_base,
                string& a_in_abi_base);
  virtual void
  perform();

  virtual
  ~test_task_ctf()
  {}
}; // end struct test_task_ctf

/// Constructor.
///
/// Task to be executed for each CTF test entry in @ref
/// abigail::tests::read_common::InOutSpec.
/// @param InOutSpec the array containing set of tests.
///
/// @param a_out_abi_base the output base directory for abixml files.
///
/// @param a_in_elf_base the input base directory for object files.
///
/// @param a_in_elf_base the input base directory for expected
/// abixml files.
test_task_ctf::test_task_ctf(const InOutSpec &s,
                             string& a_out_abi_base,
                             string& a_in_elf_base,
                             string& a_in_abi_base)
        : test_task(s, a_out_abi_base, a_in_elf_base, a_in_abi_base)
  {}

/// The thread function to execute each CTF test entry in @ref
/// abigail::tests::read_common::InOutSpec.
///
/// This reads the corpus into memory, saves it to disk, loads it
/// again and compares the new in-memory representation against the
void
test_task_ctf::perform()
{
  abigail::ir::environment env;

  set_in_elf_path();
  set_in_suppr_spec_path();

  abigail::fe_iface::status status =
    abigail::fe_iface::STATUS_UNKNOWN;
  vector<char**> di_roots;
  ABG_ASSERT(abigail::tools_utils::file_exists(in_elf_path));

  abigail::elf_based_reader_sptr rdr = ctf::create_reader(in_elf_path,
							  di_roots, env);
  ABG_ASSERT(rdr);

  corpus_sptr corp = rdr->read_corpus(status);

  // if there is no output and no input, assume that we do not care about the
  // actual read result, just that it succeeded.
  if (!spec.in_abi_path && !spec.out_abi_path)
    {
        // Phew! we made it here and we did not crash! yay!
        return;
    }
  if (!corp)
    {
        error_message = string("failed to read ") + in_elf_path  + "\n";
        is_ok = false;
        return;
    }
  corp->set_path(spec.in_elf_path);
  // Do not take architecture names in comparison so that these
  // test input binaries can come from whatever arch the
  // programmer likes.
  corp->set_architecture_name("");

  if (!(is_ok = set_out_abi_path()))
      return;

  if (!(is_ok = serialize_corpus(out_abi_path, corp)))
       return;

  if (!(is_ok = run_abidw()))
    return;

  if (!(is_ok = run_diff()))
      return;
}

/// Create a new CTF instance for task to be execute by the testsuite.
///
/// @param s the @ref abigail::tests::read_common::InOutSpec
/// tests container.
///
/// @param a_out_abi_base the output base directory for abixml files.
///
/// @param a_in_elf_base the input base directory for object files.
///
/// @param a_in_abi_base the input base directory for abixml files.
///
/// @return abigail::tests::read_common::test_task instance.
static test_task*
new_task(const InOutSpec* s, string& a_out_abi_base,
         string& a_in_elf_base, string& a_in_abi_base)
{
  return new test_task_ctf(*s, a_out_abi_base,
                           a_in_elf_base, a_in_abi_base);
}

int
main(int argc, char *argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      if (!opts.wrong_option.empty())
        emit_prefix(argv[0], cerr)
          << "unrecognized option: " << opts.wrong_option << "\n";
      display_usage(argv[0], cerr);
      return 1;
    }

  // compute number of tests to be executed.
  const size_t num_tests = sizeof(in_out_specs) / sizeof(InOutSpec) - 1;

  return run_tests(num_tests, in_out_specs, opts, new_task);
}
