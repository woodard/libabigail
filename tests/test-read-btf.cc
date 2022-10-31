// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file is part of the BTF testsuite. It reads ELF binaries
/// containing BTF, save them in XML corpus files and diff the
/// corpus files against reference XML corpus files.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-btf-reader.h"
#include "test-read-common.h"

using std::string;
using std::cerr;
using std::vector;

using abigail::tests::read_common::InOutSpec;
using abigail::tests::read_common::test_task;
using abigail::tests::read_common::display_usage;
using abigail::tests::read_common::options;

using abigail::btf::create_reader;
using abigail::xml_writer::SEQUENCE_TYPE_ID_STYLE;
using abigail::xml_writer::HASH_TYPE_ID_STYLE;
using abigail::tools_utils::emit_prefix;

static InOutSpec in_out_specs[] =
{
  {
    "data/test-read-btf/test0.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-btf/test0.o.abi",
    "output/test-read-btf/test0.o.abi",
    "--btf",
  },
  {
    "data/test-read-btf/test1.o",
    "",
    "",
    SEQUENCE_TYPE_ID_STYLE,
    "data/test-read-btf/test1.o.abi",
    "output/test-read-btf/test1.o.abi",
    "--btf",
  },
  // This should be the last entry.
  {NULL, NULL, NULL, SEQUENCE_TYPE_ID_STYLE, NULL, NULL, NULL}
};

/// Task specialization to perform BTF tests.
struct test_task_btf : public test_task
{
  test_task_btf(const InOutSpec &s,
                string& a_out_abi_base,
                string& a_in_elf_base,
                string& a_in_abi_base);
  virtual void
  perform();

  virtual
  ~test_task_btf()
  {}
}; // end struct test_task_btf

/// Constructor.
///
/// Task to be executed for each BTF test entry in @ref
/// abigail::tests::read_common::InOutSpec.
/// @param InOutSpec the array containing set of tests.
///
/// @param a_out_abi_base the output base directory for abixml files.
///
/// @param a_in_elf_base the input base directory for object files.
///
/// @param a_in_elf_base the input base directory for expected
/// abixml files.
test_task_btf::test_task_btf(const InOutSpec &s,
                             string& a_out_abi_base,
                             string& a_in_elf_base,
                             string& a_in_abi_base)
        : test_task(s, a_out_abi_base, a_in_elf_base, a_in_abi_base)
  {}

/// The thread function to execute each BTF test entry in @ref
/// abigail::tests::read_common::InOutSpec.
///
/// This reads the corpus into memory, saves it to disk, loads it
/// again and compares the new in-memory representation against the
void
test_task_btf::perform()
{
  abigail::ir::environment env;

  set_in_elf_path();
  set_in_suppr_spec_path();

  abigail::fe_iface::status status =
    abigail::fe_iface::STATUS_UNKNOWN;
  vector<char**> di_roots;
  ABG_ASSERT(abigail::tools_utils::file_exists(in_elf_path));

  abigail::elf_based_reader_sptr rdr = abigail::btf::create_reader(in_elf_path,
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

  if (!(is_ok = run_abidw("--btf ")))
    return;

  if (!(is_ok = run_diff()))
      return;
}

/// Create a new BTF instance for task to be execute by the testsuite.
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
  return new test_task_btf(*s, a_out_abi_base,
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
