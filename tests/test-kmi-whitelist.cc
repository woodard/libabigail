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

// Author: Matthias Maennich

/// @file
///
/// This program tests suppression generation from KMI whitelists.

#include <string>

#include "abg-fwd.h"
#include "abg-suppression.h"
#include "abg-tools-utils.h"
#include "test-utils.h"

using abigail::tools_utils::gen_suppr_spec_from_kernel_abi_whitelists;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::function_suppression_sptr;
using abigail::suppr::variable_suppression_sptr;
using abigail::suppr::is_function_suppression;
using abigail::suppr::is_variable_suppression;

const static std::string whitelist_with_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-single-entry";

const static std::string whitelist_with_another_single_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-another-single-entry";

const static std::string whitelist_with_two_sections
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-two-sections";

const static std::string whitelist_with_duplicate_entry
    = std::string(abigail::tests::get_src_dir())
      + "/tests/data/test-kmi-whitelist/whitelist-with-duplicate-entry";

bool
suppressions_are_consistent(const suppressions_type& suppr,
			    const std::string&	     expr)
{
  if (suppr.size() != 2)
    return false;

  function_suppression_sptr left = is_function_suppression(suppr[0]);
  variable_suppression_sptr right = is_variable_suppression(suppr[1]);

  return // correctly casted
      (left && right)
      // same label
      && (left->get_label() == right->get_label())
      // same mode
      && (left->get_drops_artifact_from_ir()
	  == right->get_drops_artifact_from_ir())
      // same regex
      && (left->get_symbol_name_not_regex_str()
	  == right->get_symbol_name_not_regex_str())
      // regex as expected
      && (left->get_symbol_name_not_regex_str() == expr);
}

bool
testNoWhitelist()
{
  const std::vector<std::string> abi_whitelist_paths;
  suppressions_type		 suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return suppr.empty();
}

bool
testSingleEntryWhitelist()
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return !suppr.empty() && suppressions_are_consistent(suppr, "^test_symbol$");
}

bool
testWhitelistWithDuplicateEntries()
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_duplicate_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return !suppr.empty() && suppressions_are_consistent(suppr, "^test_symbol$");
}

bool
testTwoWhitelists()
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_single_entry);
  abi_whitelist_paths.push_back(whitelist_with_another_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return !suppr.empty()
	 && suppressions_are_consistent(suppr,
					"^test_another_symbol$|^test_symbol$");
}

bool
testTwoWhitelistsWithDuplicates()
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_duplicate_entry);
  abi_whitelist_paths.push_back(whitelist_with_another_single_entry);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return !suppr.empty()
	 && suppressions_are_consistent(suppr,
					"^test_another_symbol$|^test_symbol$");
}

bool
testWhitelistWithTwoSections()
{
  std::vector<std::string> abi_whitelist_paths;
  abi_whitelist_paths.push_back(whitelist_with_two_sections);
  suppressions_type suppr
      = gen_suppr_spec_from_kernel_abi_whitelists(abi_whitelist_paths);
  return !suppr.empty()
	 && suppressions_are_consistent(suppr,
					"^test_symbol1$|^test_symbol2$");
}

int
main(int, char*[])
{
  bool is_ok = true;

  is_ok = is_ok && testNoWhitelist();
  is_ok = is_ok && testSingleEntryWhitelist();
  is_ok = is_ok && testWhitelistWithDuplicateEntries();
  is_ok = is_ok && testTwoWhitelists();
  is_ok = is_ok && testTwoWhitelistsWithDuplicates();
  is_ok = is_ok && testWhitelistWithTwoSections();

  return !is_ok;
}
