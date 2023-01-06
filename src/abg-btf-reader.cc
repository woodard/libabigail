// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions of the front-end to analyze the
/// BTF information contained in an ELF file.

#include "abg-internal.h"

#ifdef WITH_BTF

#include <bpf/btf.h>
#include <iostream>
#include <unordered_map>

#include "abg-elf-helpers.h"

// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-btf-reader.h"
#include "abg-ir.h"
#include "abg-tools-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{
using namespace ir;

namespace btf
{

class reader;

/// A convenience typedef for a shared pointer to
/// abigail::btf::reader.
typedef shared_ptr<reader> reader_sptr;

static const char*
btf_offset_to_string(const ::btf* btf, uint32_t offset)
{
  if (!offset)
    return "__anonymous__";
  return btf__name_by_offset(btf, offset) ?: "(invalid string offset)";
}

/// A convenience typedef of a map that associates a btf type id to a
/// libabigail ABI artifact.
typedef std::unordered_map<int, type_or_decl_base_sptr>
btf_type_id_to_abi_artifact_map_type;

/// The BTF front-end abstraction type.
class reader : public elf_based_reader
{
  ::btf*				btf_handle_ = nullptr;
  translation_unit_sptr		cur_tu_;
  vector<type_base_sptr>		types_to_canonicalize_;
  btf_type_id_to_abi_artifact_map_type	btf_type_id_to_artifacts_;

  /// Getter of the handle to the BTF data as returned by libbpf.
  ///
  /// @return the handle to the BTF data as returned by libbpf.
  ::btf*
  btf_handle()
  {
    if (btf_handle_ == nullptr)
      {
	btf_handle_ = btf__parse(corpus_path().c_str(), nullptr);
	if (!btf_handle_)
	  std::cerr << "Could not parse BTF information from file '"
		    << corpus_path().c_str() << "'" << std::endl;
      }
    return btf_handle_;
  }

  /// Getter of the environment of the current front-end.
  ///
  /// @return The environment of the current front-end.
  environment&
  env()
  {return options().env;}

  /// Getter of the environment of the current front-end.
  ///
  /// @return The environment of the current front-end.
  const environment&
  env() const
  {return const_cast<reader*>(this)->env();}

  /// Getter of the current translation unit being built.
  ///
  /// Actually, BTF doesn't keep track of the translation unit each
  /// ABI artifact originates from.  So an "artificial" translation
  /// unit is built.  It contains all the ABI artifacts of the binary.
  ///
  /// @return The current translation unit being built.
  translation_unit_sptr&
  cur_tu()
  {return cur_tu_;}

  /// Getter of the current translation unit being built.
  ///
  /// Actually, BTF doesn't keep track of the translation unit each
  /// ABI artifact originates from.  So an "artificial" translation
  /// unit is built.  It contains all the ABI artifacts of the binary.
  ///
  /// @return The current translation unit being built.
  const translation_unit_sptr&
  cur_tu() const
  {return cur_tu_;}

  /// Getter of the current translation unit being built.
  ///
  /// Actually, BTF doesn't keep track of the translation unit each
  /// ABI artifact originates from.  So an "artificial" translation
  /// unit is built.  It contains all the ABI artifacts of the binary.
  ///
  /// @return The current translation unit being built.
  void
  cur_tu(const translation_unit_sptr& tu)
  {cur_tu_ = tu;}

  /// Getter of the map that associates a BTF type ID to an ABI
  /// artifact.
  ///
  /// @return The map that associates a BTF type ID to an ABI
  /// artifact.
  btf_type_id_to_abi_artifact_map_type&
  btf_type_id_to_artifacts()
  {return btf_type_id_to_artifacts_;}

  /// Getter of the map that associates a BTF type ID to an ABI
  /// artifact.
  ///
  /// @return The map that associates a BTF type ID to an ABI
  /// artifact.
  const btf_type_id_to_abi_artifact_map_type&
  btf_type_id_to_artifacts() const
  {return btf_type_id_to_artifacts_;}

  /// Get the ABI artifact that is associated to a given BTF type ID.
  ///
  /// If no ABI artifact is associated to the BTF type id, then return
  /// nil.
  ///
  /// @return the ABI artifact that is associated to a given BTF type
  /// id.
  type_or_decl_base_sptr
  lookup_artifact_from_btf_id(int btf_id)
  {
    auto i = btf_type_id_to_artifacts().find(btf_id);
    if (i != btf_type_id_to_artifacts().end())
      return i->second;
    return type_or_decl_base_sptr();
  }

  /// Associate an ABI artifact to a given BTF type ID.
  ///
  /// @param artifact the ABI artifact to consider.
  ///
  /// @param btf_type_id the BTF type ID to associate to @p artifact.
  void
  associate_artifact_to_btf_type_id(const type_or_decl_base_sptr& artifact,
				    int btf_type_id)
  {btf_type_id_to_artifacts()[btf_type_id] = artifact;}

  /// Schecule a type for canonicalization at the end of the debug
  /// info loading.
  ///
  /// @param t the type to schedule.
  void
  schedule_type_for_canonocalization(const type_base_sptr& t)
  {types_to_canonicalize_.push_back(t);}

  /// Canonicalize all the types scheduled for canonicalization using
  /// schedule_type_for_canonocalization().
  void
  canonicalize_types()
  {
    for (auto t : types_to_canonicalize_)
      canonicalize(t);
  }

  uint64_t
  nr_btf_types() const
  {
#ifdef WITH_BTF__GET_NR_TYPES
#define GET_NB_TYPES btf__get_nr_types
#endif

#ifdef WITH_BTF__TYPE_CNT
#undef GET_NB_TYPES
#define GET_NB_TYPES btf__type_cnt
#endif

#ifndef GET_NB_TYPES
    ABG_ASSERT_NOT_REACHED;
    return 0;
#endif

    return GET_NB_TYPES(const_cast<reader*>(this)->btf_handle());
  }

protected:
  reader() = delete;

  /// Initializer of the current instance of @ref btf::reader.
  ///
  /// This frees the resources used by the current instance of @ref
  /// btf::reader and gets it ready to analyze another ELF
  /// file.
  ///
  /// @param elf_path the path to the ELF file to read from.
  ///
  /// @param debug_info_root_paths the paths where to look for
  /// seperate debug info.
  ///
  /// @param load_all_types if true, then load all the types described
  /// in the binary, rather than loading only the types reachable from
  /// the exported decls.
  ///
  /// @param linux_kernel_mode
  void
  initialize(const string&		elf_path,
	     const vector<char**>&	debug_info_root_paths,
	     bool			load_all_types,
	     bool			linux_kernel_mode)
  {
    reset(elf_path, debug_info_root_paths);
    btf__free(btf_handle_);
    options().load_all_types = load_all_types;
    options().load_in_linux_kernel_mode = linux_kernel_mode;
  }

  /// Constructor of the btf::reader type.
  ///
  /// @param elf_path the path to the ELF file to analyze.
  ///
  /// @param debug_info_root_paths the set of directory where to look
  /// debug info from, for cases where the debug is split.
  ///
  /// @param environment the environment of the current front-end.
  ///
  /// @param load_all_types if true load all the types described by
  /// the BTF debug info, as opposed to loading only the types
  /// reachable from the decls that are defined and exported.
  ///
  /// @param linux_kernel_mode if true, then consider the binary being
  /// analyzed as a linux kernel binary.
  reader(const string&		elf_path,
	 const vector<char**>&	debug_info_root_paths,
	 environment&		environment,
	 bool			load_all_types,
	 bool			linux_kernel_mode)
    : elf_based_reader(elf_path,
		       debug_info_root_paths,
		       environment)
  {
    initialize(elf_path, debug_info_root_paths,
	       load_all_types, linux_kernel_mode);
  }

public:

  /// Constructor of the btf::reader type.
  ///
  /// @param elf_path the path to the ELF file to analyze.
  ///
  /// @param debug_info_root_paths the set of directory where to look
  /// debug info from, for cases where the debug is split.
  ///
  /// @param environment the environment of the current front-end.
  ///
  /// @param load_all_types if true load all the types described by
  /// the BTF debug info, as opposed to loading only the types
  /// reachable from the decls that are defined and exported.
  ///
  /// @param linux_kernel_mode if true, then consider the binary being
  /// analyzed as a linux kernel binary.
  static btf::reader_sptr
  create(const string&		elf_path,
	 const vector<char**>&	debug_info_root_paths,
	 environment&		environment,
	 bool			load_all_types,
	 bool			linux_kernel_mode)
  {
    reader_sptr result(new reader(elf_path, debug_info_root_paths, environment,
				  load_all_types, linux_kernel_mode));
    return result;
  }

  /// Destructor of the btf::reader type.
  ~reader()
  {
    btf__free(btf_handle_);
  }

  /// Read the ELF information as well as the BTF type information to
  /// build an ABI corpus.
  ///
  /// @param status output parameter.  The status of the analysis.
  ///
  /// @return the resulting ABI corpus.
  corpus_sptr
  read_corpus(status& status)
  {
    // Read the properties of the ELF file.
    elf::reader::read_corpus(status);

    corpus::origin origin = corpus()->get_origin();
    origin |= corpus::BTF_ORIGIN;
    corpus()->set_origin(origin);

    if ((status & STATUS_NO_SYMBOLS_FOUND)
	|| !(status & STATUS_OK))
      // Either we couldn't find ELF symbols or something went badly
      // wrong.  There is nothing we can do with this ELF file.  Bail
      // out.
      return corpus_sptr();

    if (find_btf_section() == nullptr)
      status |= STATUS_DEBUG_INFO_NOT_FOUND;

    read_debug_info_into_corpus();

    status |= STATUS_OK;

    return corpus();
  }

  /// Read the BTF debug info to construct the ABI corpus.
  ///
  /// @return the resulting ABI corpus.
  corpus_sptr
  read_debug_info_into_corpus()
  {
    btf_handle();

    translation_unit_sptr artificial_tu
      (new translation_unit(env(), "", /*address_size=*/64));
    corpus()->add(artificial_tu);
    cur_tu(artificial_tu);

    int number_of_types = nr_btf_types();
    int first_type_id = 1;

    // Let's cycle through whatever is described in the BTF section
    // and emit libabigail IR for it.
    for (int type_id = first_type_id;
	 type_id < number_of_types;
	 ++type_id)
      {
	// Build IR nodes only for decls (functions and variables)
	// that have associated ELF symbols that are publicly defined
	// and exported, unless the user asked to load all types.

	bool do_construct_ir_node = false;

	const btf_type* t = btf__type_by_id(btf_handle(), type_id);
	string name;
	if (t->name_off)
	  name = btf_offset_to_string(btf_handle(), t->name_off);

	int kind = btf_kind(t);
	if (kind == BTF_KIND_FUNC)
	  {
	    ABG_ASSERT(!name.empty());
	    if (btf_vlen(t) == BTF_FUNC_GLOBAL
		|| btf_vlen(t) == BTF_FUNC_EXTERN
		|| function_symbol_is_exported(name))
	      do_construct_ir_node = true;
	  }
	else if (kind == BTF_KIND_VAR)
	  {
	    ABG_ASSERT(!name.empty());
	    if (btf_vlen(t) == BTF_VAR_GLOBAL_ALLOCATED
		|| btf_vlen(t) == BTF_VAR_GLOBAL_EXTERN
		|| variable_symbol_is_exported(name))
	      do_construct_ir_node = true;
	  }
	else if (options().load_all_types)
	  do_construct_ir_node = true;

	if (do_construct_ir_node)
	  build_ir_node_from_btf_type(type_id);
      }

    canonicalize_types();

    return corpus();
  }

  /// Build an abigail IR node for a given type described by a BTF
  /// type ID.  The node is added to the ABI corpus.
  ///
  /// @param type_id the ID of the type to build and IR node for.
  ///
  /// @return the IR node representing the type @p type_id.
  type_or_decl_base_sptr
  build_ir_node_from_btf_type(int type_id)
  {
    type_or_decl_base_sptr result;
    const btf_type *t = nullptr;

    if ((result = lookup_artifact_from_btf_id(type_id)))
      return result;

    if (type_id == 0)
      result = build_ir_node_for_void_type();
    else
      t = btf__type_by_id(btf_handle(), type_id);

    if (!result)
      {
	ABG_ASSERT(t);
	int type_kind = btf_kind(t);

	switch(type_kind)
	  {
	  case BTF_KIND_INT/* Integer */:
	    result = build_int_type(type_id);
	    break;

	  case BTF_KIND_FLOAT/* Floating point */:
	    result = build_float_type(type_id);
	    break;

	  case BTF_KIND_TYPEDEF/* Typedef*/:
	    result = build_typedef_type(type_id);
	    break;

	  case BTF_KIND_PTR/* Pointer */:
	    result = build_pointer_type(type_id);
	    break;

	  case BTF_KIND_ARRAY/* Array */:
	    result = build_array_type(type_id);
	    break;

	  case BTF_KIND_ENUM/* Enumeration up to 32-bit values */:
#ifdef WITH_BTF_ENUM64
	  case BTF_KIND_ENUM64/* Enumeration up to 64-bit values */:
#endif
	    result = build_enum_type(type_id);
	    break;

	  case BTF_KIND_STRUCT/* Struct */:
	  case BTF_KIND_UNION/* Union */:
	    result = build_class_or_union_type(type_id);
	    break;

	  case BTF_KIND_FWD/* Forward */:
	    result = build_class_or_union_type(type_id);
	    break;

	  case BTF_KIND_CONST/* Const	*/:
	  case BTF_KIND_VOLATILE/* Volatile */:
	  case BTF_KIND_RESTRICT/* Restrict */:
	    result = build_qualified_type(type_id);
	    break;

	  case BTF_KIND_FUNC/* Function */:
	    result = build_function_decl(type_id);
	    break;

	  case BTF_KIND_FUNC_PROTO/* Function Proto */:
	    result = build_function_type(type_id);
	    break;

	  case BTF_KIND_VAR/* Variable */:
	    result = build_variable_decl(type_id);
	    break;

#ifdef WITH_BTF_KIND_TYPE_TAG
	  case BTF_KIND_TYPE_TAG/* Type Tag */:
#endif
#ifdef WITH_BTF_KIND_DECL_TAG
	  case BTF_KIND_DECL_TAG/* Decl Tag */:
#endif
	  case BTF_KIND_DATASEC/* Section */:
	  case BTF_KIND_UNKN/* Unknown	*/:
	  default:
	    ABG_ASSERT_NOT_REACHED;
	    break;
	  }
      }

    add_decl_to_scope(is_decl(result), cur_tu()->get_global_scope());

    if (type_base_sptr type = is_type(result))
      schedule_type_for_canonocalization(type);

    associate_artifact_to_btf_type_id(result, type_id);

    if (function_decl_sptr fn = is_function_decl(result))
      {
	if (fn->get_is_in_public_symbol_table())
	  maybe_add_fn_to_exported_decls(fn.get());
      }
    else if (var_decl_sptr var = is_var_decl(result))
      {
	if (var->get_is_in_public_symbol_table())
	  maybe_add_var_to_exported_decls(var.get());
      }

    return result;
  }

  /// Build an IR node for the "void" type.
  ///
  /// @return the IR node for the void type.
  type_base_sptr
  build_ir_node_for_void_type()
  {
    type_base_sptr t = env().get_void_type();
    decl_base_sptr type_declaration = get_type_declaration(t);
    if (!has_scope(type_declaration))
      {
	add_decl_to_scope(type_declaration, cur_tu()->get_global_scope());
	canonicalize(t);
      }

    return t;
  }

  /// Build an IR node for the "variadic parameter" type.
  ///
  /// @return the IR node for the "variadic parameter" type.
  type_base_sptr
  build_ir_node_for_variadic_parameter_type()
  {
    type_base_sptr t = env().get_variadic_parameter_type();
    decl_base_sptr t_decl = get_type_declaration(t);
    if (!has_scope(t_decl))
      {
	add_decl_to_scope(t_decl, cur_tu()->get_global_scope());
	canonicalize(t);
      }
    return t;
  }

  /// Build an IR node for an integer type expressed in BTF.
  ///
  /// @param t a pointer a BTF type describing an integer.
  ///
  /// @return a pointer to @ref type_decl representing an integer
  /// type.
  type_or_decl_base_sptr
  build_int_type(int type_id)
  {
    type_decl_sptr result;

    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    ABG_ASSERT(btf_kind(t) == BTF_KIND_INT);

    uint32_t info = *reinterpret_cast<const uint32_t*>(t + 1);
    uint64_t byte_size = 0, bit_size = 0;
    string type_name;

    byte_size = t->size;
    bit_size = byte_size * 8;

    if (BTF_INT_ENCODING(info) & BTF_INT_CHAR)
      {
	if (!(BTF_INT_ENCODING(info) & BTF_INT_SIGNED))
	  type_name = "unsigned ";
	type_name += "char";
      }
    else if (BTF_INT_ENCODING(info) & BTF_INT_BOOL)
      type_name = "bool";
    else if (!(BTF_INT_ENCODING(info) & BTF_INT_SIGNED))
      {
	type_name = "unsigned ";
	type_name += btf_offset_to_string(btf_handle(), t->name_off);
      }
    else
      type_name = btf_offset_to_string(btf_handle(), t->name_off);

    location loc;
    result.reset(new type_decl(env(), type_name,
			       bit_size, /*alignment=*/0,
			       loc, type_name));

    return result;
  }

  /// Build an IR node for a float type expressed in BTF.
  ///
  /// @return a pointer to @ref type_decl representing a float type.
  type_or_decl_base_sptr
  build_float_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    ABG_ASSERT(btf_kind(t) == BTF_KIND_FLOAT);

    string type_name = btf_offset_to_string(btf_handle(), t->name_off);;
    uint64_t byte_size = t->size, bit_size = byte_size * 8;
    location loc;
    type_decl_sptr result(new type_decl(env(), type_name, bit_size,
					/*alignment=*/0, loc, type_name));

    return result;
  }

  /// Build an IR type that represents the underlying type of an enum type.
  ///
  /// This is a sub-routine of the build_enum_type() function.
  ///
  /// @param enum_name the name of the enum type this type is an
  /// underlying type for.
  ///
  /// @param enum_size the size of the enum.
  ///
  /// @param is_anonymous if true, the enum type is anonymous.
  ///
  /// @return a pointer to type_decl that represents a integer type
  /// that is the underlying type of an enum type.
  type_decl_sptr
  build_enum_underlying_type(const string enum_name, uint64_t enum_size,
			     bool is_anonymous = true)
  {
    string underlying_type_name =
      build_internal_underlying_enum_type_name(enum_name,
					       is_anonymous,
					       enum_size);
    type_decl_sptr result(new type_decl(env(), underlying_type_name,
					enum_size, enum_size, location()));
    result->set_is_anonymous(is_anonymous);
    result->set_is_artificial(true);
    add_decl_to_scope(result, cur_tu()->get_global_scope());
    canonicalize(result);
    return result;
  }

  /// Build an IR node that represents an enum type expressed in BTF.
  ///
  /// @param type_id the ID of the BTF representation of the enum.
  ///
  /// @return a pointer to @ref enum_type_decl representing @p t.
  type_or_decl_base_sptr
  build_enum_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
#ifdef WITH_BTF_ENUM64
    ABG_ASSERT(kind == BTF_KIND_ENUM || kind == BTF_KIND_ENUM64);
#else
    ABG_ASSERT(kind == BTF_KIND_ENUM);
#endif

    int byte_size = t->size, bit_size = byte_size * 8;

    string enum_name;
    if (t->name_off)
      enum_name = btf_offset_to_string(btf_handle(), t->name_off);
    bool is_anonymous = enum_name.empty();

    int num_enms = btf_vlen(t);
    enum_type_decl::enumerators enms;
    string e_name;
    if (kind == BTF_KIND_ENUM)
      {
	const struct btf_enum* e = btf_enum(t);
	uint32_t e_value = 0;
	for (int i = 0; i < num_enms; ++i, ++e)
	  {
	    e_name = btf_offset_to_string(btf_handle(), e->name_off);
	    e_value = e->val;
	    enms.push_back(enum_type_decl::enumerator(e_name, e_value));
	  }
      }
#ifdef WITH_BTF_ENUM64
    else if (kind == BTF_KIND_ENUM64)
      {
	const struct btf_enum64* e =
	  reinterpret_cast<const struct btf_enum64*>(t + 1);
	uint64_t e_value = 0;
	for (int i = 0; i < num_enms; ++i, ++e)
	  {
	    e_name = btf_offset_to_string(btf_handle(), e->name_off);
	    e_value = (static_cast<uint64_t>(e->val_hi32) << 32) | e->val_lo32;
	    enms.push_back(enum_type_decl::enumerator(e_name, e_value));
	  }
      }
#endif
    else
      ABG_ASSERT_NOT_REACHED;

    type_decl_sptr underlying_type =
      build_enum_underlying_type(enum_name, bit_size, is_anonymous);
    enum_type_decl_sptr result(new enum_type_decl(enum_name,
						  location(),
						  underlying_type,
						  enms, enum_name));
    result->set_is_anonymous(is_anonymous);
    return result;
  }

  /// Build an IR node for a typedef that is expressed in BTF.
  ///
  /// @param type_id the ID of the BTF representation of a typedef.
  ///
  /// @return a pointer to @ref typedef_decl representing @p t.
  type_or_decl_base_sptr
  build_typedef_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_TYPEDEF);

    string type_name = btf_offset_to_string(btf_handle(), t->name_off);
    type_base_sptr underlying_type =
      is_type(build_ir_node_from_btf_type(t->type));
    if (!underlying_type)
      return type_or_decl_base_sptr();

    typedef_decl_sptr result(new typedef_decl(type_name, underlying_type,
					      location(),
					      /*linkage_name=*/type_name));
    if ((is_class_or_union_type(underlying_type)
	 || is_enum_type(underlying_type))
	&& is_anonymous_type(underlying_type))
      get_type_declaration(underlying_type)->set_naming_typedef(result);

    return result;
  }

  /// Build an IR node representing a pointer described in BTF.
  ///
  /// @param type_id the ID of a BTF representation of a pointer type.
  ///
  /// @return a pointer to pointer_type_def that represents @p t.
  type_or_decl_base_sptr
  build_pointer_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_PTR);

    type_base_sptr underlying_type =
      is_type(build_ir_node_from_btf_type(t->type));
    if (!underlying_type)
      return type_or_decl_base_sptr();

    int size = elf_helpers::get_architecture_word_size(elf_handle());
    size *= 8;
    pointer_type_def_sptr result(new pointer_type_def(underlying_type, size,
						      /*alignment=*/0,
						      location()));
    return result;
  }

  /// Build an IR node representing an array type described in BTF.
  ///
  /// @param type_id the ID of the BTF representation of an array
  /// type.
  ///
  /// return a pointer to @ref array_type_def representing @p t.
  type_or_decl_base_sptr
  build_array_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_ARRAY);

    const struct btf_array* arr = btf_array(t);

    type_base_sptr underlying_type =
      is_type(build_ir_node_from_btf_type(arr->type));
    if (!underlying_type)
      return type_or_decl_base_sptr();

    uint64_t lower_boud = 0;
    // Note that arr->nelems can be 0;
    uint64_t upper_bound = arr->nelems ? arr->nelems - 1: 0;

    array_type_def::subrange_sptr subrange(new array_type_def::subrange_type
					   (env(), /*name=*/"",
					    lower_boud, upper_bound,
					    location()));
    add_decl_to_scope(subrange, cur_tu()->get_global_scope());
    canonicalize(subrange);
    array_type_def::subranges_type subranges = {subrange};
    array_type_def_sptr result(new array_type_def(underlying_type,
						  subranges, location()));

    return result;
  }

  /// Build an IR node representing a qualified type described in BTF.
  ///
  /// @param type_id the ID of the BTF representation of an array
  /// type.
  ///
  /// @return a pointer to a qualified_type_def representing @ t.
  type_or_decl_base_sptr
  build_qualified_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_CONST
	       || kind == BTF_KIND_VOLATILE
	       || kind == BTF_KIND_RESTRICT);

    type_base_sptr underlying_type =
      is_type(build_ir_node_from_btf_type(t->type));
    if (!underlying_type)
      return type_or_decl_base_sptr();

    qualified_type_def::CV qual = qualified_type_def::CV_NONE;
    if (kind == BTF_KIND_CONST)
      qual |= qualified_type_def::CV_CONST;
    else if (kind == BTF_KIND_VOLATILE)
      qual |= qualified_type_def::CV_VOLATILE;
    else if (kind == BTF_KIND_RESTRICT)
      qual |= qualified_type_def::CV_RESTRICT;
    else
      ABG_ASSERT_NOT_REACHED;

    qualified_type_def_sptr result(new qualified_type_def(underlying_type,
							  qual, location()));
    return result;
  }

  /// Build an IR node for a class or union type expressed in BTF.
  ///
  /// @param type_id the ID of a pointer to a BTF type describing a
  /// class or union type.
  ///
  /// @return a pointer to either a @ref class_decl or a @ref
  /// union_decl type representing the type expressed by @p t.
  type_or_decl_base_sptr
  build_class_or_union_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);

    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_STRUCT
	       || kind == BTF_KIND_UNION
	       || kind == BTF_KIND_FWD);

    string type_name;
    if (t->name_off)
      type_name = btf_offset_to_string(btf_handle(), t->name_off);

    bool is_anonymous = type_name.empty();
    uint64_t size = t->size;
    size *= 8;

    bool is_decl_only = (kind == BTF_KIND_FWD);

    class_or_union_sptr result;
    if (kind == BTF_KIND_STRUCT
	|| (kind == BTF_KIND_FWD
	    && BTF_INFO_KFLAG(t->info) == 0 /*struct*/))
    result.reset(new class_decl(env(), type_name, size,
				/*alignment=*/0,
				/*is_struct=*/true,
				location(),
				decl_base::VISIBILITY_DEFAULT,
				is_anonymous));
    else if (kind == BTF_KIND_UNION
	     || (kind == BTF_KIND_FWD
		 && BTF_INFO_KFLAG(t->info) == 1/*union*/))
      result.reset(new union_decl(env(), type_name, size, location(),
				  decl_base::VISIBILITY_DEFAULT,
				  is_anonymous));
    else
      ABG_ASSERT_NOT_REACHED;

    if (is_decl_only)
      result->set_is_declaration_only(is_decl_only);

    add_decl_to_scope(result, cur_tu()->get_global_scope());

    associate_artifact_to_btf_type_id(result, type_id);

    // For defined classes and unions, add data members to the type
    // being built.
    if (!is_decl_only)
      {
	const struct btf_member *m =
	  reinterpret_cast<const struct btf_member*>(t + 1);
	uint64_t nb_members = btf_vlen(t);

	for (uint64_t i = 0; i < nb_members; ++i, ++m)
	  {
	    type_base_sptr member_type =
	      is_type(build_ir_node_from_btf_type(m->type));
	    if (!member_type)
	      continue;

	    string member_name;
	    if (m->name_off)
	      member_name = btf_offset_to_string(btf_handle(), m->name_off);
	    var_decl_sptr data_member(new var_decl(member_name,
						   member_type,
						   location(),
						   /*linkage_name=*/""));
	    uint64_t offset_in_bits =
	      BTF_INFO_KFLAG(t->info)
	      ? BTF_MEMBER_BIT_OFFSET(m->offset)
	      : m->offset;

	    result->add_data_member(data_member,
				    public_access,
				    /*is_laid_out=*/true,
				    /*is_static=*/false,
				    offset_in_bits);
	  }
      }
    return result;
  }

  /// Build an IR node for a function type expressed in BTF.
  ///
  /// @param type_id the ID of a pointer to a BTF type describing a
  /// function type.
  ///
  /// @return a pointer to a @ref function_type representing the
  /// function type expressed by @p t.
  type_or_decl_base_sptr
  build_function_type(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_FUNC_PROTO);

    type_base_sptr return_type = is_type(build_ir_node_from_btf_type(t->type));
    if (return_type == nullptr)
      return type_or_decl_base_sptr();

    int address_size = elf_helpers::get_architecture_word_size(elf_handle());
    address_size *= 8;
    function_type_sptr result(new function_type(env(), address_size,
						/*alignment=*/0));
    result->set_return_type(return_type);

    associate_artifact_to_btf_type_id(result, type_id);

    uint16_t nb_parms = btf_vlen(t);
    const struct btf_param* parm =
      reinterpret_cast<const struct btf_param*>(t + 1);

    function_decl::parameters function_parms;
    for (uint16_t i = 0; i < nb_parms; ++i, ++parm)
      {
	type_base_sptr parm_type;
	string parm_name;
	bool is_variadic = false;

	if (parm->name_off == 0 && parm->type == 0)
	  {
	    is_variadic = true;
	    parm_type = build_ir_node_for_variadic_parameter_type();
	  }
	else
	  {
	    parm_name = btf_offset_to_string(btf_handle(), parm->name_off);
	    parm_type = is_type(build_ir_node_from_btf_type(parm->type));
	  }

	if (!parm_type)
	  continue;

	function_decl::parameter_sptr p
	  (new function_decl::parameter(parm_type, parm_name,
					location(), is_variadic));
	function_parms.push_back(p);
      }
    result->set_parameters(function_parms);

    cur_tu()->bind_function_type_life_time(result);

    return result;
  }

  /// Build an IR node for a function declaration expressed in BTF.
  ///
  /// @param type_id the ID of a pointer to a BTF "type" which realy
  /// describes a function declaration.
  ///
  /// @return a pointer to a @ref function_decl representing the
  /// function declaration expressed by @p t.
  type_or_decl_base_sptr
  build_function_decl(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_FUNC);

    function_decl_sptr result;

    string fn_name = btf_offset_to_string(btf_handle(), t->name_off);

    type_base_sptr fn_type = is_type(build_ir_node_from_btf_type(t->type));
    if (!fn_type)
      return result;

    result.reset(new function_decl(fn_name, fn_type, /*is_inline=*/false,
				   location(), /*linkage_name=*/fn_name));

    elf_symbol_sptr fn_sym;
    if ((fn_sym = function_symbol_is_exported(fn_name)))
      {
	result->set_symbol(fn_sym);
	result->set_is_in_public_symbol_table(true);
      }
    return result;
  }

  /// Build an IR node for a variable declaration expressed in BTF.
  ///
  /// @param t a pointer to a BTF "type" describing a variable
  /// declaration.
  ///
  /// @return a pointer to @ref var_decl representing the variable
  /// declaration expressed by @p t.
  type_or_decl_base_sptr
  build_variable_decl(int type_id)
  {
    const btf_type *t = btf__type_by_id(btf_handle(), type_id);
    int kind = btf_kind(t);
    ABG_ASSERT(kind == BTF_KIND_VAR);

    var_decl_sptr result;

    string var_name = btf_offset_to_string(btf_handle(), t->name_off);

    type_base_sptr var_type = is_type(build_ir_node_from_btf_type(t->type));
    if (!var_type)
      return result;

    result.reset(new var_decl(var_name, var_type, location(),
			      /*linkage_name=*/var_name));

    elf_symbol_sptr var_sym;
    if ((var_sym = variable_symbol_is_exported(var_name)))
      {
	result->set_symbol(var_sym);
	result->set_is_in_public_symbol_table(true);
      }
    return result;
  }

}; // end class reader.

/// Create and return a BTF reader (or front-end) which is an instance
/// of @ref btf::reader.
///
/// @param elf_path the path to the path to the elf file the reader is
/// to be used for.
///
/// @param debug_info_root_paths a vector to the paths to the
/// directories under which the debug info is to be found for @p
/// elf_path.  Pass an empty vector if th debug info is not in a split
/// file.
///
/// @param environment the environment used by the current context.
/// This environment contains resources needed by the BTF reader and
/// by the types and declarations that are to be created later.  Note
/// that ABI artifacts that are to be compared all need to be created
/// within the same environment.
///
/// Please also note that the life time of this environment object
/// must be greater than the life time of the resulting @ref
/// reader the context uses resources that are allocated in the
/// environment.
///
/// @param load_all_types if set to false only the types that are
/// reachable from publicly exported declarations (of functions and
/// variables) are read.  If set to true then all types found in the
/// debug information are loaded.
///
/// @param linux_kernel_mode if set to true, then consider the special
/// linux kernel symbol tables when determining if a symbol is
/// exported or not.
///
/// @return a smart pointer to the resulting btf::reader.
elf_based_reader_sptr
create_reader(const std::string&	elf_path,
	      const vector<char**>&	debug_info_root_paths,
	      environment&		env,
	      bool			load_all_types,
	      bool			linux_kernel_mode)
{
  reader_sptr rdr = reader::create(elf_path, debug_info_root_paths, env,
				   load_all_types, linux_kernel_mode);
  return rdr;
}

} // end namespace btf
} // end namespace abigail

#endif //WITH_BTF
