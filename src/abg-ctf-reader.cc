// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2021-2023 Oracle, Inc.
//
// Author: Jose E. Marchesi

/// @file
///
/// This file contains the definitions of the entry points to
/// de-serialize an instance of @ref abigail::corpus from a file in
/// ELF format, containing CTF information.

#include "config.h"

#include <fcntl.h> /* For open(3) */
#include <sstream>
#include <iostream>
#include <memory>
#include <map>
#include <algorithm>

#include "ctf-api.h"

#include "abg-internal.h"
#include "abg-ir-priv.h"
#include "abg-symtab-reader.h"


#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-ctf-reader.h"
#include "abg-elf-based-reader.h"
#include "abg-corpus.h"
#include "abg-tools-utils.h"
#include "abg-elf-helpers.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{
namespace ctf
{
using std::dynamic_pointer_cast;
using abigail::tools_utils::dir_name;
using abigail::tools_utils::file_exists;

class reader;

static typedef_decl_sptr
process_ctf_typedef(reader *rdr,
                    ctf_dict_t *ctf_dictionary,
                    ctf_id_t ctf_type);

static type_decl_sptr
process_ctf_base_type(reader *rdr,
                      ctf_dict_t *ctf_dictionary,
                      ctf_id_t ctf_type);

static decl_base_sptr
build_ir_node_for_variadic_parameter_type(reader &rdr,
                                          translation_unit_sptr tunit);

static function_type_sptr
process_ctf_function_type(reader *rdr,
                          ctf_dict_t *ctf_dictionary,
                          ctf_id_t ctf_type);

static void
process_ctf_sou_members(reader *rdr,
                        ctf_dict_t *ctf_dictionary,
                        ctf_id_t ctf_type,
                        class_or_union_sptr sou);

static type_base_sptr
process_ctf_forward_type(reader *rdr,
                         ctf_dict_t *ctf_dictionary,
                         ctf_id_t ctf_type);

static class_decl_sptr
process_ctf_struct_type(reader *rdr,
                        ctf_dict_t *ctf_dictionary,
                        ctf_id_t ctf_type);

static union_decl_sptr
process_ctf_union_type(reader *rdr,
                       ctf_dict_t *ctf_dictionary,
                       ctf_id_t ctf_type);

static array_type_def_sptr
process_ctf_array_type(reader *rdr,
                       ctf_dict_t *ctf_dictionary,
                       ctf_id_t ctf_type);

static type_base_sptr
process_ctf_qualified_type(reader *rdr,
                           ctf_dict_t *ctf_dictionary,
                           ctf_id_t ctf_type);

static pointer_type_def_sptr
process_ctf_pointer_type(reader *rdr,
                         ctf_dict_t *ctf_dictionary,
                         ctf_id_t ctf_type);

static enum_type_decl_sptr
process_ctf_enum_type(reader *rdr,
                      ctf_dict_t *ctf_dictionary,
                      ctf_id_t ctf_type);

static void
fill_ctf_section(const Elf_Scn *elf_section, ctf_sect_t *ctf_section);

static ctf_id_t
lookup_symbol_in_ctf_archive(ctf_archive_t *ctfa, ctf_dict_t **ctf_dict,
                             const char *sym_name);

static std::string
dic_type_key(ctf_dict_t *dic, ctf_id_t ctf_type);

/// The abstraction of a CTF reader.
///
/// It groks the type information contains the CTF-specific part of
/// the ELF file and builds an ABI corpus out of it.
class reader : public elf_based_reader
{
  /// The CTF archive read from FILENAME.  If an archive couldn't
  /// be read from the file then this is NULL.
  ctf_archive_t *ctfa;

  /// A map associating CTF type ids with libabigail IR types.  This
  /// is used to reuse already generated types.
  string_type_base_sptr_map_type types_map;

  /// A set associating unknown CTF type ids
  std::set<ctf_id_t> unknown_types_set;

  /// Raw contents of several sections from the ELF file.  These are
  /// used by libctf.
  ctf_sect_t ctf_sect;
  ctf_sect_t symtab_sect;
  ctf_sect_t strtab_sect;
  translation_unit_sptr cur_tu_;

public:

  /// Getter of the exported decls builder object.
  ///
  /// @return the exported decls builder.
  corpus::exported_decls_builder*
  exported_decls_builder()
  {return corpus()->get_exported_decls_builder().get();}

  /// Associate a given CTF type ID with a given libabigail IR type.
  ///
  /// @param dic the dictionnary the type belongs to.
  ///
  /// @param ctf_type the type ID.
  ///
  /// @param type the type to associate to the ID.
  void
  add_type(ctf_dict_t *dic, ctf_id_t ctf_type, type_base_sptr type)
  {
    string key = dic_type_key(dic, ctf_type);
    types_map.insert(std::make_pair(key, type));
  }

  /// Insert a given CTF unknown type ID.
  ///
  /// @param ctf_type the unknown type ID to be added.
  void
  add_unknown_type(ctf_id_t ctf_type)
  {
    unknown_types_set.insert(ctf_type);
  }

  /// Lookup a given CTF type ID in the types map.
  ///
  /// @param dic the dictionnary the type belongs to.
  ///
  /// @param ctf_type the type ID of the type to lookup.
  type_base_sptr
  lookup_type(ctf_dict_t *dic, ctf_id_t ctf_type)
  {
    type_base_sptr result;
    std::string key = dic_type_key(dic, ctf_type);

    auto search = types_map.find(key);
    if (search != types_map.end())
      result = search->second;

    return result;
  }

  /// Lookup a given CTF unknown type ID in the unknown set.
  /// @param ctf_type the unknown type ID to lookup.
  bool
  lookup_unknown_type(ctf_id_t ctf_type)
  { return unknown_types_set.find(ctf_type) != unknown_types_set.end(); }

  /// Canonicalize all the types stored in the types map.
  void
  canonicalize_all_types(void)
  {
    canonicalize_types
      (types_map.begin(), types_map.end(),
       [](const string_type_base_sptr_map_type::const_iterator& i)
       {return i->second;});
  }

  /// Constructor.
  ///
  /// @param elf_path the path to the ELF file.
  ///
  /// @param debug_info_root_paths vector with the paths
  /// to directories where .debug file is located.
  ///
  /// @param env the environment used by the current context.
  /// This environment contains resources needed by the reader and by
  /// the types and declarations that are to be created later.  Note
  /// that ABI artifacts that are to be compared all need to be
  /// created within the same environment.
  reader(const string&		elf_path,
	 const vector<char**>&	debug_info_root_paths,
	 environment&		env)
    : elf_based_reader(elf_path, debug_info_root_paths, env)
  {
    initialize();
  }

  /// Initializer of the reader.
  ///
  /// This is useful to clear out the data used by the reader and get
  /// it ready to be used again.
  ///
  /// Note that the reader eeps the same environment it has been
  /// originally created with.
  ///
  /// Please also note that the life time of this environment object
  /// must be greater than the life time of the resulting @ref
  /// reader the context uses resources that are allocated in
  /// the environment.
  void
  initialize()
  {
    ctfa = nullptr;
    types_map.clear();
    cur_tu_.reset();
    corpus_group().reset();
  }

  /// Initializer of the reader.
  ///
  /// @param elf_path the new path to the new ELF file to use.
  ///
  /// @param debug_info_root_paths a vector of paths to use to look
  /// for debug info that is split out into a separate file.
  ///
  /// @param load_all_types currently not used.
  ///
  /// @param linux_kernel_mode currently not used.
  ///
  /// This is useful to clear out the data used by the reader and get
  /// it ready to be used again.
  ///
  /// Note that the reader eeps the same environment it has been
  /// originally created with.
  ///
  /// Please also note that the life time of this environment object
  /// must be greater than the life time of the resulting @ref
  /// reader the context uses resources that are allocated in
  /// the environment.
  void
  initialize(const string& elf_path,
             const vector<char**>& debug_info_root_paths,
             bool load_all_types = false,
             bool linux_kernel_mode = false)
  {
    load_all_types = load_all_types;
    linux_kernel_mode = linux_kernel_mode;
    reset(elf_path, debug_info_root_paths);
  }

  /// Setter of the current translation unit.
  ///
  /// @param tu the current translation unit being constructed.
  void
  cur_transl_unit(translation_unit_sptr tu)
  {
    if (tu)
      cur_tu_ = tu;
  }

  /// Getter of the current translation unit.
  ///
  /// @return the current translation unit being constructed.
  const translation_unit_sptr&
  cur_transl_unit() const
  {return cur_tu_;}

  /// Getter of the environment of the current CTF reader.
  ///
  /// @return the environment of the current CTF reader.
  const environment&
  env() const
  {return options().env;}

  /// Getter of the environment of the current CTF reader.
  ///
  /// @return the environment of the current CTF reader.
  environment&
  env()
  {return options().env;}

  /// Look for vmlinux.ctfa file in default directory or in
  /// directories provided by debug-info-dir command line option,
  /// it stores location path in @ref ctfa_file.
  ///
  /// @param ctfa_file file name found.
  /// @return true if file is found.
  bool
  find_ctfa_file(std::string& ctfa_file)
  {
    std::string ctfa_dirname;
    dir_name(corpus_path(), ctfa_dirname, false);

    // In corpus group we assume vmlinux as first file to
    // be processed, so default location for vmlinux.cfa
    // is vmlinux dirname.
    ctfa_file = ctfa_dirname + "/vmlinux.ctfa";
    if (file_exists(ctfa_file))
      return true;

    // If it's proccessing a module, then location directory
    // for vmlinux.ctfa should be provided with --debug-info-dir
    // option.
    for (const auto& path : debug_info_root_paths())
      if (tools_utils::find_file_under_dir(*path, "vmlinux.ctfa", ctfa_file))
        return true;

    return false;
  }

  /// Slurp certain information from the underlying ELF file, and
  /// install it the current libabigail corpus associated to the
  /// current CTF reader.
  ///
  /// @param status the resulting status flags.
  void
  slurp_elf_info(fe_iface::status& status)
  {
    // Read the ELF-specific parts of the corpus.
    elf::reader::read_corpus(status);

    corpus_sptr corp = corpus();
    if ((corp->get_origin() & corpus::LINUX_KERNEL_BINARY_ORIGIN)
	&& corpus_group())
      {
	// Not finding any debug info so far is expected if we are
	// building a kABI.
        status &= static_cast<abigail::fe_iface::status>
                    (~STATUS_DEBUG_INFO_NOT_FOUND);
	return;
      }

    if ((status & STATUS_NO_SYMBOLS_FOUND)
	|| !(status & STATUS_OK))
      // Either we couldn't find ELF symbols or something went badly
      // wrong.  There is nothing we can do with this ELF file.  Bail
      // out.
      return;

    GElf_Ehdr *ehdr, eh_mem;
    if (!(ehdr = gelf_getehdr(elf_handle(), &eh_mem)))
      return;

    // ET_{EXEC,DYN} needs .dyn{sym,str} in ctf_arc_bufopen
    const char *symtab_name = ".dynsym";
    const char *strtab_name = ".dynstr";

    if (ehdr->e_type == ET_REL)
      {
	symtab_name = ".symtab";
	strtab_name = ".strtab";
      }

    const Elf_Scn* ctf_scn = find_ctf_section();
    fill_ctf_section(ctf_scn, &ctf_sect);

    const Elf_Scn* symtab_scn =
      elf_helpers::find_section_by_name(elf_handle(), symtab_name);
    fill_ctf_section(symtab_scn, &symtab_sect);

    const Elf_Scn* strtab_scn =
      elf_helpers::find_section_by_name(elf_handle(), strtab_name);
    fill_ctf_section(strtab_scn, &strtab_sect);

    status |= fe_iface::STATUS_OK;
  }

  /// Process a CTF archive and create libabigail IR for the types,
  /// variables and function declarations found in the archive, iterating
  /// over public symbols.  The IR is added to the given corpus.
  void
  process_ctf_archive()
  {
    corpus_sptr corp = corpus();
    /* We only have a translation unit.  */
    translation_unit_sptr ir_translation_unit =
      std::make_shared<translation_unit>(env(), "", 64);
    ir_translation_unit->set_language(translation_unit::LANG_C);
    corp->add(ir_translation_unit);
    cur_transl_unit(ir_translation_unit);

    int ctf_err;
    ctf_dict_t *ctf_dict, *dict_tmp;
    const auto symt = symtab();
    symtab_reader::symtab_filter filter = symt->make_filter();
    filter.set_public_symbols();
    std::string dict_name;

    if ((corp->get_origin() & corpus::LINUX_KERNEL_BINARY_ORIGIN)
	&& corpus_group())
      {
	tools_utils::base_name(corpus_path(), dict_name);
	// remove .* suffix
	std::size_t pos = dict_name.find(".");
	if (pos != string::npos)
	  dict_name.erase(pos);

	std::replace(dict_name.begin(), dict_name.end(), '-', '_');
      }

    if ((ctf_dict = ctf_dict_open(ctfa,
				  dict_name.empty() ? NULL : dict_name.c_str(),
				  &ctf_err)) == NULL)
      {
	fprintf(stderr, "ERROR dictionary not found\n");
	abort();
      }

    dict_tmp = ctf_dict;

    for (const auto& symbol : symtab_reader::filtered_symtab(*symt, filter))
      {
	std::string sym_name = symbol->get_name();
	ctf_id_t ctf_sym_type;

	ctf_sym_type = lookup_symbol_in_ctf_archive(ctfa, &ctf_dict,
						    sym_name.c_str());
	if (ctf_sym_type == CTF_ERR)
          continue;

	if (ctf_type_kind(ctf_dict, ctf_sym_type) != CTF_K_FUNCTION)
	  {
	    const char *var_name = sym_name.c_str();
	    type_base_sptr var_type = build_type(ctf_dict, ctf_sym_type);
	    if (!var_type)
	      /* Ignore variable if its type can't be sorted out.  */
	      continue;

	    var_decl_sptr var_declaration;
	    var_declaration.reset(new var_decl(var_name,
					       var_type,
					       location(),
					       var_name));

	    var_declaration->set_symbol(symbol);
	    add_decl_to_scope(var_declaration,
			      ir_translation_unit->get_global_scope());
	    var_declaration->set_is_in_public_symbol_table(true);
	    maybe_add_var_to_exported_decls(var_declaration.get());
	  }
	else
	  {
	    const char *func_name = sym_name.c_str();
	    ctf_id_t ctf_sym = ctf_sym_type;
	    type_base_sptr func_type = build_type(ctf_dict, ctf_sym);
	    if (!func_type)
	      /* Ignore function if its type can't be sorted out.  */
	      continue;

	    function_decl_sptr func_declaration;
	    func_declaration.reset(new function_decl(func_name,
						     func_type,
						     0 /* is_inline */,
						     location()));
	    func_declaration->set_symbol(symbol);
	    add_decl_to_scope(func_declaration,
			      ir_translation_unit->get_global_scope());
	    func_declaration->set_is_in_public_symbol_table(true);
	    maybe_add_fn_to_exported_decls(func_declaration.get());
	  }

	ctf_dict = dict_tmp;
      }

    ctf_dict_close(ctf_dict);
    /* Canonicalize all the types generated above.  This must be
       done "a posteriori" because the processing of types may
       require other related types to not be already
       canonicalized.  */
    canonicalize_all_types();
  }

  /// Add a new type declaration to the given libabigail IR corpus CORP.
  ///
  /// @param ctf_dictionary the CTF dictionary being read.
  /// @param ctf_type the CTF type ID of the source type.
  ///
  /// Note that if @ref ctf_type can't reliably be translated to the IR
  /// then it is simply ignored.
  ///
  /// @return a shared pointer to the IR node for the type.
  type_base_sptr
  process_ctf_type(ctf_dict_t *ctf_dictionary,
		   ctf_id_t ctf_type)
  {
    corpus_sptr corp = corpus();
    translation_unit_sptr tunit = cur_transl_unit();
    int type_kind = ctf_type_kind(ctf_dictionary, ctf_type);
    type_base_sptr result;

    if (lookup_unknown_type(ctf_type))
      return nullptr;

    if ((result = lookup_type(ctf_dictionary, ctf_type)))
      return result;

    switch (type_kind)
      {
      case CTF_K_INTEGER:
      case CTF_K_FLOAT:
	{
	  type_decl_sptr type_decl
	    = process_ctf_base_type(this, ctf_dictionary, ctf_type);
	  result = is_type(type_decl);
	  break;
	}
      case CTF_K_TYPEDEF:
	{
	  typedef_decl_sptr typedef_decl
	    = process_ctf_typedef(this, ctf_dictionary, ctf_type);
	  result = is_type(typedef_decl);
	  break;
	}
      case CTF_K_POINTER:
	{
	  pointer_type_def_sptr pointer_type
	    = process_ctf_pointer_type(this, ctf_dictionary, ctf_type);
	  result = pointer_type;
	  break;
	}
      case CTF_K_CONST:
      case CTF_K_VOLATILE:
      case CTF_K_RESTRICT:
	{
	  type_base_sptr qualified_type
	    = process_ctf_qualified_type(this, ctf_dictionary, ctf_type);
	  result = qualified_type;
	  break;
	}
      case CTF_K_ARRAY:
	{
	  array_type_def_sptr array_type
	    = process_ctf_array_type(this, ctf_dictionary, ctf_type);
	  result = array_type;
	  break;
	}
      case CTF_K_ENUM:
	{
	  enum_type_decl_sptr enum_type
	    = process_ctf_enum_type(this, ctf_dictionary, ctf_type);
	  result = enum_type;
	  break;
	}
      case CTF_K_FUNCTION:
	{
	  function_type_sptr function_type
	    = process_ctf_function_type(this, ctf_dictionary, ctf_type);
	  result = function_type;
	  break;
	}
      case CTF_K_STRUCT:
	{
	  class_decl_sptr struct_decl
	    = process_ctf_struct_type(this, ctf_dictionary, ctf_type);
	  result = is_type(struct_decl);
	  break;
	}
      case CTF_K_FORWARD:
	  result = process_ctf_forward_type(this, ctf_dictionary, ctf_type);
	break;
      case CTF_K_UNION:
	{
	  union_decl_sptr union_decl
	    = process_ctf_union_type(this, ctf_dictionary, ctf_type);
	  result = is_type(union_decl);
	  break;
	}
      case CTF_K_UNKNOWN:
	/* Unknown types are simply ignored.  */
      default:
	break;
      }

    if (!result)
      {
	fprintf(stderr, "NOT PROCESSED TYPE %lu\n", ctf_type);
	add_unknown_type(ctf_type);
      }

    return result;
  }

  /// Given a CTF type id, build the corresponding libabigail IR type.
  /// If the IR type has been generated it returns the corresponding
  /// type.
  ///
  /// @param ctf_dictionary the CTF dictionary being read.
  /// @param ctf_type the CTF type ID of the looked type.
  ///
  /// Note that if @ref ctf_type can't reliably be translated to the IR
  /// then a NULL shared pointer is returned.
  ///
  /// @return a shared pointer to the IR node for the type.
  type_base_sptr
  build_type(ctf_dict_t *ctf_dictionary, ctf_id_t ctf_type)
  {
    type_base_sptr result = lookup_type(ctf_dictionary, ctf_type);

    if (!result)
      result = process_ctf_type(ctf_dictionary, ctf_type);
    return result;
  }

  /// Read the CTF information in the binary and construct an ABI
  /// corpus from it.
  ///
  /// @param status output parameter.  Contains the status of the ABI
  /// corpus construction.
  ///
  /// @return the corpus created as a result of processing the debug
  /// information.
  corpus_sptr
  read_corpus(fe_iface::status &status)
  {
    corpus_sptr corp = corpus();
    status = fe_iface::STATUS_UNKNOWN;

    corpus::origin origin = corpus()->get_origin();
    origin |= corpus::CTF_ORIGIN;
    corp->set_origin(origin);

    slurp_elf_info(status);
    if (status & fe_iface::STATUS_NO_SYMBOLS_FOUND)
       return corpus_sptr();

    if (!(origin & corpus::LINUX_KERNEL_BINARY_ORIGIN)
          && (status & fe_iface::STATUS_DEBUG_INFO_NOT_FOUND))
      return corp;

    int errp;
    if ((corp->get_origin() & corpus::LINUX_KERNEL_BINARY_ORIGIN)
	&& corpus_group())
      {
	if (ctfa == NULL)
	  {
	    std::string ctfa_filename;
	    if (find_ctfa_file(ctfa_filename))
	      ctfa = ctf_arc_open(ctfa_filename.c_str(), &errp);
	  }
      }
    else
      /* Build the ctfa from the contents of the relevant ELF sections,
	 and process the CTF archive in the read context, if any.
	 Information about the types, variables, functions, etc contained
	 in the archive are added to the given corpus.  */
      ctfa = ctf_arc_bufopen(&ctf_sect, &symtab_sect,
			     &strtab_sect, &errp);

    env().canonicalization_is_done(false);
    if (ctfa == NULL)
      status |= fe_iface::STATUS_DEBUG_INFO_NOT_FOUND;
    else
      {
	process_ctf_archive();
	corpus()->sort_functions();
	corpus()->sort_variables();
      }

    env().canonicalization_is_done(true);

    return corp;
  }

  /// Destructor of the CTF reader.
  ~reader()
  {
    ctf_close(ctfa);
  }
}; // end class reader.

typedef shared_ptr<reader> reader_sptr;

/// Build and return a typedef libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the typedef.

static typedef_decl_sptr
process_ctf_typedef(reader *rdr,
                    ctf_dict_t *ctf_dictionary,
                    ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  typedef_decl_sptr result;

  ctf_id_t ctf_utype = ctf_type_reference(ctf_dictionary, ctf_type);
  if (ctf_utype == CTF_ERR)
    return result;

  const char *typedef_name = ctf_type_name_raw(ctf_dictionary, ctf_type);
  if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
    if ((result = lookup_typedef_type(typedef_name, *corp)))
      return result;

  type_base_sptr utype = rdr->build_type(ctf_dictionary, ctf_utype);

  if (!utype)
    return result;

  result = dynamic_pointer_cast<typedef_decl>
             (rdr->lookup_type(ctf_dictionary, ctf_type));
  if (result)
    return result;

  result.reset(new typedef_decl(typedef_name, utype, location(),
                                typedef_name /* mangled_name */));

  /* If this typedef "names" an anonymous type, reflect this fact in
     the underlying type.  In C enum, struct and union types can be
     anonymous.  */
  if (is_anonymous_type(utype)
      && (is_enum_type(utype) || is_class_or_union_type(utype)))
    {
      decl_base_sptr decl = is_decl(utype);
      ABG_ASSERT(decl);
      decl->set_naming_typedef(result);
    }

  if (result)
    {
      add_decl_to_scope(result, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Build and return an integer or float type declaration libabigail
/// IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the type.

static type_decl_sptr
process_ctf_base_type(reader *rdr,
                      ctf_dict_t *ctf_dictionary,
                      ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  type_decl_sptr result;

  ctf_id_t ctf_ref = ctf_type_reference(ctf_dictionary, ctf_type);
  const char *type_name = ctf_type_name_raw(ctf_dictionary,
                                            (ctf_ref != CTF_ERR) ? ctf_ref : ctf_type);

  /* Get the type encoding and extract some useful properties of
     the type from it.  In case of any error, just ignore the
     type.  */
  ctf_encoding_t type_encoding;
  if (ctf_type_encoding(ctf_dictionary,
                        (ctf_ref != CTF_ERR) ? ctf_ref : ctf_type,
                        &type_encoding))
    return result;

  /* Create the IR type corresponding to the CTF type.  */
  if (type_encoding.cte_bits == 0
      && type_encoding.cte_format == CTF_INT_SIGNED)
    {
      /* This is the `void' type.  */
      type_base_sptr void_type = rdr->env().get_void_type();
      decl_base_sptr type_declaration = get_type_declaration(void_type);
      result = is_type_decl(type_declaration);
      canonicalize(result);
    }
  else
    {
      if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
        {
          string normalized_type_name = type_name;
          integral_type int_type;
          if (parse_integral_type(type_name, int_type))
            normalized_type_name = int_type.to_string();
          if ((result = lookup_basic_type(normalized_type_name, *corp)))
            return result;
        }

      result = lookup_basic_type(type_name, *corp);
      if (!result)
        result.reset(new type_decl(rdr->env(),
                                   type_name,
                                   type_encoding.cte_bits,
                                   /*alignment=*/0,
                                   location(),
                                   type_name /* mangled_name */));

    }

  if (result)
    {
      add_decl_to_scope(result, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Build the IR node for a variadic parameter type.
///
/// @param rdr the read context to use.
///
/// @return the variadic parameter type.
static decl_base_sptr
build_ir_node_for_variadic_parameter_type(reader &rdr,
                                          translation_unit_sptr tunit)
{

  const ir::environment& env = rdr.env();
  type_base_sptr t = env.get_variadic_parameter_type();
  decl_base_sptr type_declaration = get_type_declaration(t);
  if (!has_scope(type_declaration))
    add_decl_to_scope(type_declaration, tunit->get_global_scope());
  canonicalize(t);
  return type_declaration;
}

/// Build and return a function type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the function type.

static function_type_sptr
process_ctf_function_type(reader *rdr,
                          ctf_dict_t *ctf_dictionary,
                          ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  function_type_sptr result;

  /* Fetch the function type info from the CTF type.  */
  ctf_funcinfo_t funcinfo;
  ctf_func_type_info(ctf_dictionary, ctf_type, &funcinfo);
  int vararg_p = funcinfo.ctc_flags & CTF_FUNC_VARARG;

  /* Take care first of the result type.  */
  ctf_id_t ctf_ret_type = funcinfo.ctc_return;
  type_base_sptr ret_type = rdr->build_type(ctf_dictionary, ctf_ret_type);
  if (!ret_type)
    return result;

  /* Now process the argument types.  */
  int argc = funcinfo.ctc_argc;
  std::vector<ctf_id_t> argv(argc);
  if (static_cast<ctf_id_t>(ctf_func_type_args(ctf_dictionary, ctf_type,
					       argc, argv.data())) == CTF_ERR)
    return result;

  function_decl::parameters function_parms;
  for (int i = 0; i < argc; i++)
    {
      ctf_id_t ctf_arg_type = argv[i];
      type_base_sptr arg_type = rdr->build_type(ctf_dictionary, ctf_arg_type);
      if (!arg_type)
        return result;

      function_decl::parameter_sptr parm
        (new function_decl::parameter(arg_type, "",
                                      location(),
                                      false,
                                      false /* is_artificial */));
      function_parms.push_back(parm);
    }

  if (vararg_p)
    {
      type_base_sptr arg_type =
       is_type(build_ir_node_for_variadic_parameter_type(*rdr, tunit));

      function_decl::parameter_sptr parm
       (new function_decl::parameter(arg_type, "",
                                     location(),
                                     true,
                                     false /* is_artificial */));
      function_parms.push_back(parm);
    }

  result = dynamic_pointer_cast<function_type>
             (rdr->lookup_type(ctf_dictionary, ctf_type));
  if (result)
    return result;

  /* Ok now the function type itself.  */
  result.reset(new function_type(ret_type,
                                 function_parms,
                                 tunit->get_address_size(),
                                 /*alignment=*/0));

  if (result)
    {
      tunit->bind_function_type_life_time(result);
      result->set_is_artificial(true);
      decl_base_sptr function_type_decl = get_type_declaration(result);
      add_decl_to_scope(function_type_decl, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Add member information to a IR struct or union type.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
/// @param sou the IR struct or union type to which add the members.

static void
process_ctf_sou_members(reader *rdr,
                        ctf_dict_t *ctf_dictionary,
                        ctf_id_t ctf_type,
                        class_or_union_sptr sou)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  ssize_t member_size;
  ctf_next_t *member_next = NULL;
  const char *member_name = NULL;
  ctf_id_t member_ctf_type;

  while ((member_size = ctf_member_next(ctf_dictionary, ctf_type,
                                        &member_next, &member_name,
                                        &member_ctf_type,
                                        0 /* flags */)) >= 0)
    {
      ctf_membinfo_t membinfo;

      if (static_cast<ctf_id_t>(ctf_member_info(ctf_dictionary,
						ctf_type,
						member_name,
						&membinfo)) == CTF_ERR)
        return;

      /* Build the IR for the member's type.  */
      type_base_sptr member_type = rdr->build_type(ctf_dictionary,
                                                   member_ctf_type);
      if (!member_type)
        /* Ignore this member.  */
        continue;

      /* Create a declaration IR node for the member and add it to the
         struct type.  */
      var_decl_sptr data_member_decl(new var_decl(member_name,
                                                  member_type,
                                                  location(),
                                                  member_name));
      sou->add_data_member(data_member_decl,
                           public_access,
                           true /* is_laid_out */,
                           false /* is_static */,
                           membinfo.ctm_offset);
    }
  if (ctf_errno(ctf_dictionary) != ECTF_NEXT_END)
    fprintf(stderr, "ERROR from ctf_member_next\n");
}

/// Create a declaration-only union or struct type and add it to the
/// IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
/// @return the resulting IR node created.

static type_base_sptr
process_ctf_forward_type(reader *rdr,
                         ctf_dict_t *ctf_dictionary,
                         ctf_id_t ctf_type)
{
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  decl_base_sptr result;
  std::string type_name = ctf_type_name_raw(ctf_dictionary,
                                            ctf_type);
  bool type_is_anonymous = (type_name == "");
  uint32_t kind = ctf_type_kind_forwarded (ctf_dictionary, ctf_type);

  if (kind == CTF_K_UNION)
    {
      union_decl_sptr
	union_fwd(new union_decl(rdr->env(),
				 type_name,
				 /*alignment=*/0,
				 location(),
				 decl_base::VISIBILITY_DEFAULT,
				 type_is_anonymous));
      union_fwd->set_is_declaration_only(true);
      result = union_fwd;
    }
  else
    {
      if (!type_is_anonymous)
        if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
          if ((result = lookup_class_type(type_name, *corp)))
            return is_type(result);

      class_decl_sptr
	struct_fwd(new class_decl(rdr->env(), type_name,
                                 /*alignment=*/0, /*size=*/0,
                                 true /* is_struct */,
                                 location(),
                                 decl_base::VISIBILITY_DEFAULT,
                                 type_is_anonymous));
      struct_fwd->set_is_declaration_only(true);
      result = struct_fwd;
    }

  if (!result)
    return is_type(result);

  add_decl_to_scope(result, tunit->get_global_scope());
  rdr->add_type(ctf_dictionary, ctf_type, is_type(result));

  return is_type(result);
}

/// Build and return a struct type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the struct type.

static class_decl_sptr
process_ctf_struct_type(reader *rdr,
                        ctf_dict_t *ctf_dictionary,
                        ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  class_decl_sptr result;
  std::string struct_type_name = ctf_type_name_raw(ctf_dictionary,
                                                   ctf_type);
  bool struct_type_is_anonymous = (struct_type_name == "");

  if (!struct_type_is_anonymous)
    if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
      if ((result = lookup_class_type(struct_type_name, *corp)))
        return result;

  /* The libabigail IR encodes C struct types in `class' IR nodes.  */
  result.reset(new class_decl(rdr->env(),
                              struct_type_name,
                              ctf_type_size(ctf_dictionary, ctf_type) * 8,
                              /*alignment=*/0,
                              true /* is_struct */,
                              location(),
                              decl_base::VISIBILITY_DEFAULT,
                              struct_type_is_anonymous));
  if (!result)
    return result;

  /* The C type system indirectly supports loops by the mean of
     pointers to structs or unions.  Since some contained type can
     refer to this struct, we have to make it available in the cache
     at this point even if the members haven't been added to the IR
     node yet.  */
  add_decl_to_scope(result, tunit->get_global_scope());
  rdr->add_type(ctf_dictionary, ctf_type, result);

  /* Now add the struct members as specified in the CTF type description.
     This is C, so named types can only be defined in the global
     scope.  */
  process_ctf_sou_members(rdr, ctf_dictionary, ctf_type, result);

  return result;
}

/// Build and return an union type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the union type.

static union_decl_sptr
process_ctf_union_type(reader *rdr,
                       ctf_dict_t *ctf_dictionary,
                       ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  union_decl_sptr result;
  std::string union_type_name = ctf_type_name_raw(ctf_dictionary,
                                                   ctf_type);
  bool union_type_is_anonymous = (union_type_name == "");

  if (!union_type_is_anonymous)
    if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
      if ((result = lookup_union_type(union_type_name, *corp)))
        return result;

  /* Create the corresponding libabigail union IR node.  */
  result.reset(new union_decl(rdr->env(),
                                union_type_name,
                                ctf_type_size(ctf_dictionary, ctf_type) * 8,
                                location(),
                                decl_base::VISIBILITY_DEFAULT,
                                union_type_is_anonymous));
  if (!result)
    return result;

  /* The C type system indirectly supports loops by the mean of
     pointers to structs or unions.  Since some contained type can
     refer to this union, we have to make it available in the cache
     at this point even if the members haven't been added to the IR
     node yet.  */
  add_decl_to_scope(result, tunit->get_global_scope());
  rdr->add_type(ctf_dictionary, ctf_type, result);

  /* Now add the union members as specified in the CTF type description.
     This is C, so named types can only be defined in the global
     scope.  */
  process_ctf_sou_members(rdr, ctf_dictionary, ctf_type, result);

  return result;
}

/// Build and return an array subrange.
///
/// @param rdr the read context.
///
/// @param ctf_dictionary the CTF dictionary where @ref index
/// will be found.
///
/// @param index the CTF type ID for the array index.
///
/// @param nelems the elements number of the array.
///
/// @return a shared pointer to subrange built.
static array_type_def::subrange_sptr
build_array_ctf_range(reader *rdr, ctf_dict_t *dic,
                      ctf_id_t index, uint64_t nelems)
{
  bool is_infinite = false;
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  array_type_def::subrange_sptr subrange;
  array_type_def::subrange_type::bound_value lower_bound;
  array_type_def::subrange_type::bound_value upper_bound;

  type_base_sptr index_type = rdr->build_type(dic, index);
  if (!index_type)
    return nullptr;

  lower_bound.set_unsigned(0); /* CTF supports C only.  */
  upper_bound.set_unsigned(nelems > 0 ? nelems - 1 : 0U);

  /* for VLAs number of array elements is 0 */
  if (upper_bound.get_unsigned_value() == 0 && nelems == 0)
    is_infinite = true;

  subrange.reset(new array_type_def::subrange_type(rdr->env(),
                                                   "",
                                                   lower_bound,
                                                   upper_bound,
                                                   index_type,
                                                   location(),
                                                   translation_unit::LANG_C));
  if (!subrange)
    return nullptr;

  subrange->is_infinite(is_infinite);
  add_decl_to_scope(subrange, tunit->get_global_scope());
  canonicalize(subrange);

  return subrange;
}

/// Build and return an array type libabigail IR.
///
/// @param rdr the read context.
///
/// @param ctf_dictionary the CTF dictionary being read.
///
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the array type.
static array_type_def_sptr
process_ctf_array_type(reader *rdr,
                       ctf_dict_t *ctf_dictionary,
                       ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  array_type_def_sptr result;
  ctf_arinfo_t ctf_ainfo;

  /* First, get the information about the CTF array.  */
  if (static_cast<ctf_id_t>(ctf_array_info(ctf_dictionary,
					   ctf_type,
					   &ctf_ainfo)) == CTF_ERR)
    return result;

  ctf_id_t ctf_element_type = ctf_ainfo.ctr_contents;
  ctf_id_t ctf_index_type = ctf_ainfo.ctr_index;
  uint64_t nelems = ctf_ainfo.ctr_nelems;
  array_type_def::subrange_sptr subrange;
  array_type_def::subranges_type subranges;

  int type_array_kind = ctf_type_kind(ctf_dictionary, ctf_element_type);
  while (type_array_kind == CTF_K_ARRAY)
    {
      if (static_cast<ctf_id_t>(ctf_array_info(ctf_dictionary,
                                               ctf_element_type,
                                               &ctf_ainfo)) == CTF_ERR)
        return result;

      subrange = build_array_ctf_range(rdr, ctf_dictionary,
                                       ctf_ainfo.ctr_index,
                                       ctf_ainfo.ctr_nelems);
      subranges.push_back(subrange);
      ctf_element_type = ctf_ainfo.ctr_contents;
      type_array_kind = ctf_type_kind(ctf_dictionary, ctf_element_type);
    }

  std::reverse(subranges.begin(), subranges.end());

  /* Make sure the element type is generated.  */
  type_base_sptr element_type = rdr->build_type(ctf_dictionary,
                                                ctf_element_type);
  if (!element_type)
    return result;

  /* Ditto for the index type.  */
  type_base_sptr index_type = rdr->build_type(ctf_dictionary,
                                              ctf_index_type);
  if (!index_type)
    return result;

  result = dynamic_pointer_cast<array_type_def>
             (rdr->lookup_type(ctf_dictionary, ctf_type));
  if (result)
    return result;

  subrange = build_array_ctf_range(rdr, ctf_dictionary,
                                   ctf_index_type, nelems);
  subranges.push_back(subrange);

  /* Finally build the IR for the array type and return it.  */
  result.reset(new array_type_def(element_type, subranges, location()));
  if (result)
    {
      decl_base_sptr array_type_decl = get_type_declaration(result);
      add_decl_to_scope(array_type_decl, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Strip qualification from a qualified type, when it makes sense.
///
/// The C language specification says in [6.7.3]/8:
///
///     [If the specification of an array type includes any type
///      qualifiers, the element type is so- qualified, not the
///      array type.]
///
/// In more mundane words, a const array of int is the same as an
/// array of const int.
///
/// This function thus removes the qualifiers of the array and applies
/// them to the array element.  The function then pretends that the
/// array itself it not qualified.
///
/// It might contain code to strip other cases like this in the
/// future.
///
/// @param t the type to strip const qualification from.
///
/// @return the stripped type or just return @p t.
static decl_base_sptr
maybe_strip_qualification(const qualified_type_def_sptr t)
{
  if (!t)
    return t;

  decl_base_sptr result = t;
  type_base_sptr u = t->get_underlying_type();

  if (is_array_type(u))
    {
      // Let's apply the qualifiers of the array to the array element
      // and pretend that the array itself is not qualified, as per
      // section [6.7.3]/8 of the C specification.

      array_type_def_sptr array = is_array_type(u);
      ABG_ASSERT(array);
      // We should not be editing types that are already canonicalized.
      ABG_ASSERT(!array->get_canonical_type());
      type_base_sptr element_type = array->get_element_type();

      if (qualified_type_def_sptr qualified = is_qualified_type(element_type))
        {
          qualified_type_def::CV quals = qualified->get_cv_quals();
          quals |= t->get_cv_quals();
	  // So we apply the qualifiers of the array to the array
	  // element.
          qualified->set_cv_quals(quals);
	  // Let's pretend that the array is no more qualified.
          result = is_decl(u);
        }
    }

  return result;
}

/// Build and return a qualified type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.

static type_base_sptr
process_ctf_qualified_type(reader *rdr,
                           ctf_dict_t *ctf_dictionary,
                           ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  type_base_sptr result;
  int type_kind = ctf_type_kind(ctf_dictionary, ctf_type);
  ctf_id_t ctf_utype = ctf_type_reference(ctf_dictionary, ctf_type);
  type_base_sptr utype = rdr->build_type(ctf_dictionary, ctf_utype);
  if (!utype)
    return result;

  result = dynamic_pointer_cast<type_base>
             (rdr->lookup_type(ctf_dictionary, ctf_type));
  if (result)
    return result;

  qualified_type_def::CV qualifiers = qualified_type_def::CV_NONE;
  if (type_kind == CTF_K_CONST)
    qualifiers |= qualified_type_def::CV_CONST;
  else if (type_kind == CTF_K_VOLATILE)
    qualifiers |= qualified_type_def::CV_VOLATILE;
  else if (type_kind == CTF_K_RESTRICT)
    qualifiers |= qualified_type_def::CV_RESTRICT;
  else
    ABG_ASSERT_NOT_REACHED;

  // qualifiers are not be use in functions
  if (is_function_type(utype))
    return result;

  result.reset(new qualified_type_def(utype, qualifiers, location()));
  if (result)
    {
      // Strip some potentially redundant type qualifiers from
      // the qualified type we just built.
      decl_base_sptr d = maybe_strip_qualification(is_qualified_type(result));
      if (!d)
        d = get_type_declaration(result);
      ABG_ASSERT(d);

      add_decl_to_scope(d, tunit->get_global_scope());
      result = is_type(d);
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Build and return a pointer type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the pointer type.

static pointer_type_def_sptr
process_ctf_pointer_type(reader *rdr,
                         ctf_dict_t *ctf_dictionary,
                         ctf_id_t ctf_type)
{
  corpus_sptr corp = rdr->corpus();
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  pointer_type_def_sptr result;
  ctf_id_t ctf_target_type = ctf_type_reference(ctf_dictionary, ctf_type);
  if (ctf_target_type == CTF_ERR)
    return result;

  type_base_sptr target_type = rdr->build_type(ctf_dictionary,
                                               ctf_target_type);
  if (!target_type)
    return result;

  result = dynamic_pointer_cast<pointer_type_def>
             (rdr->lookup_type(ctf_dictionary, ctf_type));
  if (result)
    return result;

  result.reset(new pointer_type_def(target_type,
                                      ctf_type_size(ctf_dictionary, ctf_type) * 8,
                                      ctf_type_align(ctf_dictionary, ctf_type) * 8,
                                      location()));
  if (result)
    {
      add_decl_to_scope(result, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Build and return an enum type libabigail IR.
///
/// @param rdr the read context.
/// @param ctf_dictionary the CTF dictionary being read.
/// @param ctf_type the CTF type ID of the source type.
///
/// @return a shared pointer to the IR node for the enum type.

static enum_type_decl_sptr
process_ctf_enum_type(reader *rdr,
                      ctf_dict_t *ctf_dictionary,
                      ctf_id_t ctf_type)
{
  translation_unit_sptr tunit = rdr->cur_transl_unit();
  enum_type_decl_sptr result;
  ctf_id_t ctf_ref = ctf_type_reference(ctf_dictionary, ctf_type);
  std::string enum_name = ctf_type_name_raw(ctf_dictionary,
                                            (ctf_ref != CTF_ERR)
                                              ? ctf_ref : ctf_type);

  if (!enum_name.empty())
    if (corpus_sptr corp = rdr->should_reuse_type_from_corpus_group())
      if ((result = lookup_enum_type(enum_name, *corp)))
        return result;

  /* Build a signed integral type for the type of the enumerators, aka
     the underlying type.  The size of the enumerators in bytes is
     specified in the CTF enumeration type.  */
  size_t utype_size_in_bits = ctf_type_size(ctf_dictionary,
                                            (ctf_ref != CTF_ERR)
                                              ? ctf_ref : ctf_type) * 8;
  string underlying_type_name =
        build_internal_underlying_enum_type_name(enum_name, true,
                                                 utype_size_in_bits);

  type_decl_sptr utype;
  utype.reset(new type_decl(rdr->env(),
                            underlying_type_name,
                            utype_size_in_bits,
                            utype_size_in_bits,
                            location()));
  utype->set_is_anonymous(true);
  utype->set_is_artificial(true);
  if (!utype)
    return result;

  add_decl_to_scope(utype, tunit->get_global_scope());
  canonicalize(utype);

  /* Iterate over the enum entries.  */
  enum_type_decl::enumerators enms;
  ctf_next_t *enum_next = NULL;
  const char *ename;
  int evalue;

  while ((ename = ctf_enum_next(ctf_dictionary, ctf_type, &enum_next, &evalue)))
    enms.push_back(enum_type_decl::enumerator(ename, evalue));

  if (ctf_errno(ctf_dictionary) != ECTF_NEXT_END)
    {
      fprintf(stderr, "ERROR from ctf_enum_next\n");
      return result;
    }

  result.reset(new enum_type_decl(enum_name.c_str(), location(),
                                  utype, enms, enum_name.c_str()));
  if (result)
    {
      add_decl_to_scope(result, tunit->get_global_scope());
      rdr->add_type(ctf_dictionary, ctf_type, result);
    }

  return result;
}

/// Given a symbol name, lookup the corresponding CTF information in
/// the default dictionary (CTF archive member provided by the caller)
/// If the search is not success, the  looks for the symbol name
/// in _all_ archive members.
///
/// @param ctfa the CTF archive.
/// @param dict the default dictionary to looks for.
/// @param sym_name the symbol name.
/// @param corp the IR corpus.
///
/// Note that if @ref sym_name is found in other than its default dictionary
/// @ref ctf_dict will be updated and it must be explicitly closed by its
/// caller.
///
/// @return a valid CTF type id, if @ref sym_name was found, CTF_ERR otherwise.

static ctf_id_t
lookup_symbol_in_ctf_archive(ctf_archive_t *ctfa, ctf_dict_t **ctf_dict,
                             const char *sym_name)
{
  int ctf_err;
  ctf_dict_t *dict = *ctf_dict;
  ctf_id_t ctf_type = ctf_lookup_by_symbol_name(dict, sym_name);

  if (ctf_type != CTF_ERR)
    return ctf_type;

  /* Probably --ctf-variables option was used by ld, so symbol type
     definition must be found in the CTF Variable section. */
  ctf_type = ctf_lookup_variable(dict, sym_name);

  /* Not lucky, then, search in whole archive */
  if (ctf_type == CTF_ERR)
    {
      ctf_dict_t *fp;
      ctf_next_t *i = NULL;
      const char *arcname;

      while ((fp = ctf_archive_next(ctfa, &i, &arcname, 1, &ctf_err)) != NULL)
        {
          if ((ctf_type = ctf_lookup_by_symbol_name (fp, sym_name)) == CTF_ERR)
            ctf_type = ctf_lookup_variable(fp, sym_name);

          if (ctf_type != CTF_ERR)
            {
              *ctf_dict = fp;
              break;
            }
          ctf_dict_close(fp);
        }
    }

  return ctf_type;
}

/// Fill a CTF section description with the information in a given ELF
/// section.
///
/// @param elf_section the ELF section from which to get.
/// @param ctf_section the CTF section to fill with the raw data.

static void
fill_ctf_section(const Elf_Scn *elf_section, ctf_sect_t *ctf_section)
{
  GElf_Shdr section_header_mem, *section_header;
  Elf_Data *section_data;

  section_header = gelf_getshdr(const_cast<Elf_Scn*>(elf_section),
				&section_header_mem);
  section_data = elf_getdata(const_cast<Elf_Scn*>(elf_section), 0);

  ABG_ASSERT (section_header != NULL);
  ABG_ASSERT (section_data != NULL);

  ctf_section->cts_name = ""; /* This is not actually used by libctf.  */
  ctf_section->cts_data = (char *) section_data->d_buf;
  ctf_section->cts_size = section_data->d_size;
  ctf_section->cts_entsize = section_header->sh_entsize;
}

/// Create and return a new read context to process CTF information
/// from a given ELF file.
///
/// @param elf_path the patch of some ELF file.
/// @param env a libabigail IR environment.

elf_based_reader_sptr
create_reader(const std::string& elf_path,
	      const vector<char**>& debug_info_root_paths,
	      environment& env)
{
  reader_sptr result(new reader(elf_path,
				debug_info_root_paths,
				env));
  return result;
}

/// Re-initialize a reader so that it can re-used to read
/// another binary.
///
/// @param rdr the context to re-initialize.
///
/// @param elf_path the path to the elf file the context is to be used
/// for.
///
/// @param environment the environment used by the current context.
/// This environment contains resources needed by the reader and by
/// the types and declarations that are to be created later.  Note
/// that ABI artifacts that are to be compared all need to be created
/// within the same environment.
///
/// Please also note that the life time of this environment object
/// must be greater than the life time of the resulting @ref
/// reader the context uses resources that are allocated in the
/// environment.
void
reset_reader(elf_based_reader&		rdr,
	     const std::string&	elf_path,
	     const vector<char**>&	debug_info_root_path)
{
  ctf::reader& r = dynamic_cast<reader&>(rdr);
  r.initialize(elf_path, debug_info_root_path);
}

/// Returns a key to be use in types_map dict conformed by
/// dictionary id and the CTF type id for a given type.
///
/// CTF id types are unique by child dictionary, but CTF id
/// types in parent dictionary are unique across the all
/// dictionaries in the CTF archive, to differentiate
/// one each other this member function relies in
/// ctf_type_isparent function.
///
/// @param dic the pointer to CTF dictionary where the @p type
/// was found.
///
/// @param type the id for given CTF type.
static std::string
dic_type_key(ctf_dict_t *dic, ctf_id_t ctf_type)
{
  std::stringstream key;

  if (ctf_type_isparent (dic, ctf_type))
    key << std::hex << ctf_type;
  else
    key << std::hex << ctf_type << '-' << ctf_cuname(dic);
  return key.str();
}

} // End of namespace ctf
} // End of namespace abigail
