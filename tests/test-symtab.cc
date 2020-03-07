// SPDX-License-Identifier: LGPL-3.0-or-later
// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This program tests symtab invariants through abg-corpus.

#include <iostream>
#include <limits>
#include <vector>

#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-ir.h"
#include "lib/catch.hpp"
#include "test-utils.h"

using namespace abigail;

using dwarf_reader::create_read_context;
using dwarf_reader::read_context_sptr;
using dwarf_reader::read_corpus_from_elf;
using ir::environment;
using ir::environment_sptr;

static const std::string test_data_dir =
    std::string(abigail::tests::get_src_dir()) + "/tests/data/test-symtab/";

dwarf_reader::status
read_corpus(const std::string path, corpus_sptr& result)
{
  const std::string& absolute_path = test_data_dir + path;

  environment_sptr	    env(new environment);
  const std::vector<char**> debug_info_root_paths;
  read_context_sptr	    ctxt = create_read_context(
      absolute_path, debug_info_root_paths, env.get(),
      /* load_all_type = */ true, /* linux_kernel_mode = */ true);

  dwarf_reader::status status = dwarf_reader::STATUS_UNKNOWN;
  result = read_corpus_from_elf(*ctxt, status);

  REQUIRE(status != dwarf_reader::STATUS_UNKNOWN);
  return status;
}

TEST_CASE("Symtab::Empty", "[symtab, basic]")
{
  const std::string	     binary = "basic/empty.so";
  corpus_sptr		     corpus_ptr;
  const dwarf_reader::status status = read_corpus(binary, corpus_ptr);
  REQUIRE(corpus_ptr);

  REQUIRE((status & dwarf_reader::STATUS_OK));

  // TODO: Those two assertions are currently not met. Empty symtabs are
  //       currently treated like the error case.
  // REQUIRE((status & dwarf_reader::STATUS_OK));
  // REQUIRE((status & dwarf_reader::STATUS_NO_SYMBOLS_FOUND));
}

TEST_CASE("Symtab::NoDebugInfo", "[symtab, basic]")
{
  const std::string	     binary = "basic/no_debug_info.so";
  corpus_sptr		     corpus_ptr;
  const dwarf_reader::status status = read_corpus(binary, corpus_ptr);
  REQUIRE(corpus_ptr);

  REQUIRE(status
	  == (dwarf_reader::STATUS_OK
	      | dwarf_reader::STATUS_DEBUG_INFO_NOT_FOUND));
}

// this value indicates in the following helper method, that we do not want to
// assert for this particular value. In other words, N is a placeholder for an
// arbitrary value.
#define N std::numeric_limits<size_t>::max()

corpus_sptr
assert_symbol_count(const std::string& path,
		    size_t	       function_symbols = 0,
		    size_t	       variable_symbols = 0,
		    size_t	       undefined_function_symbols = 0,
		    size_t	       undefined_variable_symbols = 0)
{
  corpus_sptr		     corpus_ptr;
  const dwarf_reader::status status = read_corpus(path, corpus_ptr);
  REQUIRE(corpus_ptr);

  REQUIRE((status & dwarf_reader::STATUS_OK));
  const corpus& corpus = *corpus_ptr;

  if (function_symbols != N)
    {
      CHECK(corpus.get_sorted_fun_symbols().size() == function_symbols);
      CHECK(corpus.get_fun_symbol_map().size() == function_symbols);
      CHECK(corpus.get_fun_symbol_map_sptr()->size() == function_symbols);
    }
  if (variable_symbols != N)
    {
      CHECK(corpus.get_sorted_var_symbols().size() == variable_symbols);
      CHECK(corpus.get_var_symbol_map().size() == variable_symbols);
      CHECK(corpus.get_var_symbol_map_sptr()->size() == variable_symbols);
    }
  if (undefined_variable_symbols != N)
    {
      CHECK(corpus.get_sorted_undefined_fun_symbols().size()
	    == undefined_function_symbols);
      CHECK(corpus.get_undefined_fun_symbol_map().size()
	    == undefined_function_symbols);
      CHECK(corpus.get_undefined_fun_symbol_map_sptr()->size()
	    == undefined_function_symbols);
    }
  if (undefined_function_symbols != N)
    {
      CHECK(corpus.get_sorted_undefined_var_symbols().size()
	    == undefined_variable_symbols);
      CHECK(corpus.get_undefined_var_symbol_map().size()
	    == undefined_variable_symbols);
      CHECK(corpus.get_undefined_var_symbol_map_sptr()->size()
	    == undefined_variable_symbols);
    }

  return corpus_ptr;
}

TEST_CASE("Symtab::SimpleSymtabs", "[symtab, basic]")
{
  GIVEN("a binary with no exported symbols")
  {
    // TODO: should pass, but does currently not as empty tables are treated
    //       like the error case, but this is an edge case anyway.
    // assert_symbol_count("empty.so");
  }

  GIVEN("a binary with a single exported function")
  {
    const std::string	   binary = "basic/single_function.so";
    const corpus_sptr&	   corpus = assert_symbol_count(binary, 1, 0);
    const elf_symbol_sptr& symbol =
	corpus->lookup_function_symbol("exported_function");
    REQUIRE(symbol);
    CHECK(!corpus->lookup_variable_symbol("exported_function"));
    CHECK(symbol == corpus->lookup_function_symbol(*symbol));
    CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
  }

  GIVEN("a binary with a single exported variable")
  {
    const std::string	   binary = "basic/single_variable.so";
    const corpus_sptr&	   corpus = assert_symbol_count(binary, 0, 1);
    const elf_symbol_sptr& symbol =
	corpus->lookup_variable_symbol("exported_variable");
    REQUIRE(symbol);
    CHECK(!corpus->lookup_function_symbol("exported_variable"));
    CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
    CHECK(symbol != corpus->lookup_function_symbol(*symbol));
  }

  GIVEN("a binary with one function and one variable exported")
  {
    const std::string  binary = "basic/one_function_one_variable.so";
    const corpus_sptr& corpus = assert_symbol_count(binary, 1, 1);
    CHECK(corpus->lookup_function_symbol("exported_function"));
    CHECK(!corpus->lookup_variable_symbol("exported_function"));
    CHECK(corpus->lookup_variable_symbol("exported_variable"));
    CHECK(!corpus->lookup_function_symbol("exported_variable"));
  }

  GIVEN("a binary with a single undefined function")
  {
    const std::string  binary = "basic/single_undefined_function.so";
    const corpus_sptr& corpus = assert_symbol_count(binary, 0, 0, 1, 0);
  }

  GIVEN("a binary with a single undefined variable")
  {
    const std::string  binary = "basic/single_undefined_variable.so";
    const corpus_sptr& corpus = assert_symbol_count(binary, 0, 0, 0, 1);
  }

  GIVEN("a binary with one function and one variable undefined")
  {
    const std::string  binary = "basic/one_function_one_variable_undefined.so";
    const corpus_sptr& corpus = assert_symbol_count(binary, 0, 0, 1, 1);
  }
}

static const char* kernel_versions[] = { "4.14", "4.19", "5.4", "5.6" };
static const size_t nr_kernel_versions =
    sizeof(kernel_versions) / sizeof(kernel_versions[0]);

TEST_CASE("Symtab::SimpleKernelSymtabs", "[symtab, basic, kernel, ksymtab]")
{
  for (size_t i = 0; i < nr_kernel_versions; ++i)
    {
      const std::string base_path =
	  "kernel-" + std::string(kernel_versions[i]) + "/";

      GIVEN("The binaries in " + base_path)
      {

	GIVEN("a kernel module with no exported symbols")
	{
	  // TODO: should pass, but does currently not as empty tables are
	  // treated
	  //       like the error case, but this is an edge case anyway.
	  // assert_symbol_count(base_path + "empty.so");
	}

	GIVEN("a kernel module with a single exported function")
	{
	  const std::string	 binary = base_path + "single_function.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 1, 0);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_function_symbol("exported_function");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_variable_symbol("exported_function"));
	  CHECK(symbol == corpus->lookup_function_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
	}

	GIVEN("a kernel module with a single GPL exported function")
	{
	  const std::string	 binary = base_path + "single_function_gpl.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 1, 0);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_function_symbol("exported_function_gpl");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_variable_symbol("exported_function_gpl"));
	  CHECK(symbol == corpus->lookup_function_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
	}

	GIVEN("a binary with a single exported variable")
	{
	  const std::string	 binary = base_path + "single_variable.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 0, 1);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_variable_symbol("exported_variable");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_function_symbol("exported_variable"));
	  CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_function_symbol(*symbol));
	}

	GIVEN("a binary with a single GPL exported variable")
	{
	  const std::string	 binary = base_path + "single_variable_gpl.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 0, 1);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_variable_symbol("exported_variable_gpl");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_function_symbol("exported_variable_gpl"));
	  CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_function_symbol(*symbol));
	}

	GIVEN("a binary with one function and one variable (GPL) exported")
	{
	  const std::string  binary = base_path + "one_of_each.ko";
	  const corpus_sptr& corpus = assert_symbol_count(binary, 2, 2);
	  CHECK(corpus->lookup_function_symbol("exported_function"));
	  CHECK(!corpus->lookup_variable_symbol("exported_function"));
	  CHECK(corpus->lookup_function_symbol("exported_function_gpl"));
	  CHECK(!corpus->lookup_variable_symbol("exported_function_gpl"));
	  CHECK(corpus->lookup_variable_symbol("exported_variable"));
	  CHECK(!corpus->lookup_function_symbol("exported_variable"));
	  CHECK(corpus->lookup_variable_symbol("exported_variable_gpl"));
	  CHECK(!corpus->lookup_function_symbol("exported_variable_gpl"));
	}
      }
    }
}
