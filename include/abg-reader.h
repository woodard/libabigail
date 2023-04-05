// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations of the entry points to
/// de-serialize an instance of @ref abigail::translation_unit from an
/// ABI Instrumentation file in libabigail native XML format.

#ifndef __ABG_READER_H__
#define __ABG_READER_H__

#include <istream>
#include "abg-corpus.h"
#include "abg-suppression.h"
#include "abg-fe-iface.h"

namespace abigail
{

namespace abixml
{

using namespace abigail::ir;

translation_unit_sptr
read_translation_unit_from_file(const std::string&	file_path,
				environment&		env);

translation_unit_sptr
read_translation_unit_from_buffer(const std::string&	file_path,
				  environment&		env);

translation_unit_sptr
read_translation_unit_from_istream(std::istream*	in,
				   environment&	env);

translation_unit_sptr
read_translation_unit(fe_iface&);

 abigail::fe_iface_sptr
create_reader(const string& path, environment& env);

fe_iface_sptr
create_reader(std::istream* in, environment& env);

corpus_sptr
read_corpus_from_abixml(std::istream* in,
			environment&  env);

corpus_sptr
read_corpus_from_abixml_file(const string& path,
			     environment&  env);

corpus_group_sptr
read_corpus_group_from_input(fe_iface& ctxt);

corpus_group_sptr
read_corpus_group_from_abixml(std::istream* in,
			      environment&  env);

corpus_group_sptr
read_corpus_group_from_abixml_file(const string& path,
				   environment&  env);

void
consider_types_not_reachable_from_public_interfaces(fe_iface& ctxt,
						    bool flag);

#ifdef WITH_SHOW_TYPE_USE_IN_ABILINT
vector<type_base_sptr>*
get_types_from_type_id(fe_iface&, const string&);

unordered_map<type_or_decl_base*, vector<type_or_decl_base*>>*
get_artifact_used_by_relation_map(fe_iface&);
#endif
}//end abixml

#ifdef WITH_DEBUG_SELF_COMPARISON
bool
load_canonical_type_ids(fe_iface& ctxt,
			const string& file_path);
#endif
}//end namespace abigail

#endif // __ABG_READER_H__
