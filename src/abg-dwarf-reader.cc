// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions of the entry points to
/// de-serialize an instance of @ref corpus from a file in elf format,
/// containing dwarf information.

#include <libgen.h>
#include <assert.h>
#include <cstring>
#include <elfutils/libdwfl.h>
#include <dwarf.h>
#include <tr1/unordered_map>
#include <stack>
#include "abg-dwarf-reader.h"

using std::string;

namespace abigail
{

namespace dwarf_reader
{

using std::tr1::dynamic_pointer_cast;
using std::tr1::unordered_map;
using std::stack;

/// A functor used by @ref dwfl_sptr.
struct dwfl_deleter
{
  void
  operator()(Dwfl* dwfl)
  {dwfl_end(dwfl);}
};//end struct dwfl_deleter

/// A convenience typedef for a shared pointer to a Dwfl.
typedef shared_ptr<Dwfl> dwfl_sptr;

/// Convenience typedef for a map which key is the offset of a dwarf
/// die, (given by dwarf_dieoffset()) and which value is the
/// corresponding decl_base.
typedef unordered_map<Dwarf_Off, decl_base_sptr> die_decl_map_type;

/// Convenience typedef for a stack containing the scopes up to the
/// current point in the abigail Internal Representation (aka IR) tree
/// that is being built.
typedef stack<scope_decl_sptr> scope_stack_type;

/// Convenience typedef for a map that contains the types that have
/// been built so far.
typedef unordered_map<shared_ptr<type_base>,
		      bool,
		      type_base::shared_ptr_hash,
		      type_shared_ptr_equal> type_ptr_map;

/// The context accumulated during the reading of dwarf debug info and
/// building of the resulting ABI Corpus as a result.
///
/// This context is to be created by the top-most function that wants
/// to read debug info and build an ABI corpus from it.  It's then
/// passed to all the routines that read specific dwarf bits as they
/// get some important data from it.
class read_context
{
  dwfl_sptr handle_;
  Dwarf* dwarf_;
  const string elf_path_;
  die_decl_map_type die_decl_map_;
  corpus_sptr cur_corpus_;
  translation_unit_sptr cur_tu_;
  scope_stack_type scope_stack_;

  read_context();

public:
  read_context(dwfl_sptr handle,
	       const string& elf_path)
    : handle_(handle),
      dwarf_(0),
      elf_path_(elf_path)
  {}

  dwfl_sptr
  dwfl_handle() const
  {return handle_;}

  /// Load the debug info associated with an elf file that is at a
  /// given path.
  ///
  /// @return a pointer to the DWARF debug info pointer upon
  /// successful debug info loading, NULL otherwise.
  Dwarf*
  load_debug_info()
  {
    if (!dwfl_handle())
      return 0;

    Dwfl_Module* module =
      dwfl_report_offline(dwfl_handle().get(),
			  basename(const_cast<char*>(elf_path().c_str())),
			  elf_path().c_str(),
			  -1);
    dwfl_report_end(dwfl_handle().get(), 0, 0);

    Dwarf_Addr bias = 0;
    return (dwarf_ = dwfl_module_getdwarf(module, &bias));
  }

  Dwarf*
  dwarf() const
  {return dwarf_;}

  const string&
  elf_path() const
  {return elf_path_;}

  const die_decl_map_type&
  die_decl_map() const
  {return die_decl_map_;}

  die_decl_map_type&
  die_decl_map()
  {return die_decl_map_;}

  const corpus_sptr
  current_corpus() const
  {return cur_corpus_;}

  corpus_sptr
  current_corpus()
  {return cur_corpus_;}

  void
  current_corpus(corpus_sptr c)
  {
    if (c)
      cur_corpus_ = c;
  }

  void
  reset_current_corpus()
  {cur_corpus_.reset();}

  const translation_unit_sptr
  current_translation_unit() const
  {return cur_tu_;}

  translation_unit_sptr
  current_translation_unit()
  {return cur_tu_;}

  void
  current_translation_unit(translation_unit_sptr tu)
  {
    if (tu)
      cur_tu_ = tu;
  }

  const scope_stack_type&
  scope_stack() const
  {return scope_stack_;}

  scope_stack_type&
  scope_stack()
  {return scope_stack_;}

  scope_decl_sptr
  current_scope()
  {
    if (scope_stack().empty())
      {
	if (current_translation_unit())
	  scope_stack().push(current_translation_unit()->get_global_scope());
      }
    return scope_stack().top();
  }
};// end class read_context.

static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       bool called_from_public_decl = false);

/// Constructor for a default Dwfl handle that knows how to load debug
/// info from a library or executable elf file.
///
/// @return the constructed Dwfl handle.
static Dwfl*
create_default_dwfl()
{
  static const Dwfl_Callbacks offline_callbacks =
    {
      0,
      .find_debuginfo =  dwfl_standard_find_debuginfo,
      .section_address = dwfl_offline_section_address,
      0
    };

  return dwfl_begin(&offline_callbacks);
}


/// Create a shared pointer for a pointer to Dwfl.
///
/// @param dwlf the pointer to Dwfl to create the shared pointer for.
///
/// @return the newly created shared pointer.
static dwfl_sptr
create_dwfl_sptr(Dwfl* dwfl)
{
  dwfl_sptr result(dwfl, dwfl_deleter());
  return result;
}

/// Create a shared pointer to a default Dwfl handle.  This uses the
/// create_default_dwfl() function.
///
/// @return the created shared pointer.
static dwfl_sptr
create_default_dwfl_sptr()
{return create_dwfl_sptr(create_default_dwfl());}

/// Get the value of an attribute that is supposed to be a string, or
/// an empty string if the attribute could not be found.
///
/// @param die the DIE to get the attribute value from.
///
/// @param attr_name the attribute name.  Must come from dwarf.h and
/// be an enumerator representing an attribute like, e.g, DW_AT_name.
///
/// @return the string representing the value of the attribute, or an
/// empty string if no string attribute could be found.
static string
die_string_attribute(Dwarf_Die* die, unsigned attr_name)
{
  if (!die)
    return "";

  Dwarf_Attribute attr;
  if (!dwarf_attr(die, attr_name, &attr))
    return "";

  const char* str = dwarf_formstring(&attr);
  return str ? str : "";
}

/// Get the value of an attribute that is supposed to be an unsigned
/// constant.
///
/// @param attr_name the DW_AT_* name of the attribute.  Must come
/// from dwarf.h and be an enumerator representing an attribute like,
/// e.g, DW_AT_decl_line.
///
///@param cst the output parameter that is set to the value of the
/// attribute @ref attr_name.  This parameter is set iff the function
/// return true.
///
/// @return true if there was an attribute of the name @ref attr_name
/// and with a value that is a constant, false otherwise.
static bool
die_unsigned_constant_attribute(Dwarf_Die*	die,
				unsigned	attr_name,
				size_t&	cst)
{
  if (!die)
    return false;

  Dwarf_Attribute attr;
  Dwarf_Word result = 0;
  if (!dwarf_attr(die, attr_name, &attr)
      || dwarf_formudata(&attr, &result))
    return false;

  cst = result;
  return true;
}

#if 0
static bool
die_signed_constant_attribute(Dwarf_Die*	die,
			      unsigned		attr_name,
			      ssize_t&		cst)
{
    if (!die)
    return false;

  Dwarf_Attribute attr;
  Dwarf_Sword result = 0;
  if (!dwarf_attr(die, attr_name, &attr)
      || dwarf_formsdata(&attr, &result))
    return false;

  cst = result;
  return true;
}
#endif

/// Get the value of a DIE attribute; that value is meant to be a
/// flag.
///
/// @param die the DIE to get the attribute from.
///
/// @param attr_name the DW_AT_* name of the attribute.  Must come
/// from dwarf.h and be an enumerator representing an attribute like,
/// e.g, DW_AT_external.
///
/// @param flag the output parameter to store the flag value into.
/// This is set iff the function returns true.
///
/// @return true if the DIE has a flag attribute named @ref attr_name,
/// false otherwise.
static bool
die_flag_attribute(Dwarf_Die* die, unsigned attr_name, bool& flag)
{
  Dwarf_Attribute attr;
  bool f = false;
  if (!dwarf_attr(die, attr_name, &attr)
      || dwarf_formflag(&attr, &f))
    return false;

  flag = f;
  return true;
}

/// Get the mangled name from a given DIE.
///
/// @param die the DIE to read the mangled name from.
///
/// @return the mangled name if it's present in the DIE, or just an
/// empty string if it's not.
static string
die_mangled_name(Dwarf_Die* die)
{
  if (!die)
    return "";

  string mangled_name = die_string_attribute(die, DW_AT_linkage_name);
  if (mangled_name.empty())
    mangled_name = die_string_attribute(die, DW_AT_MIPS_linkage_name);
  return mangled_name;
}

/// Get the file path that is the value of the DW_AT_decl_file
/// attribute on a given DIE, if the DIE is a decl DIE having that
/// attribute.
///
/// @param die the DIE to consider.
///
/// @return a string containing the file path that is the logical
/// value of the DW_AT_decl_file attribute.  If the DIE @ref die
/// doesn't have a DW_AT_decl_file attribute, then the return value is
/// just an empty string.
static string
die_decl_file_attribute(Dwarf_Die* die)
{
  if (!die)
    return "";

  const char* str = dwarf_decl_file(die);

  return str ? str : "";
}

/// Get the value of an attribute which value is supposed to be a
/// reference to a DIE.
///
/// @param die the DIE to read the value from.
///
/// @param attr_name the DW_AT_* attribute name to read.
///
/// @param result the DIE resulting from reading the attribute value.
/// This is set iff the function returns true.
///
/// @return true if the DIE @ref die contains an attribute named @ref
/// attr_name that is a DIE reference, false otherwise.
static bool
die_die_attribute(Dwarf_Die* die, unsigned attr_name, Dwarf_Die& result)
{
  Dwarf_Attribute attr;
  if (!dwarf_attr(die, attr_name, &attr))
    return false;
  return dwarf_formref_die(&attr, &result);
}

/// Returns the source location associated with a decl DIE.
///
/// @param ctxt the @ref read_context to use.
///
/// @param die the DIE the read the source location from.
static location
die_location(read_context& ctxt, Dwarf_Die* die)
{
  if (!die)
    return location();

  string file = die_decl_file_attribute(die);
  size_t line = 0;
  die_unsigned_constant_attribute(die, DW_AT_decl_line, line);

  if (!file.empty() && line != 0)
    {
      translation_unit_sptr tu = ctxt.current_translation_unit();
      location l = tu->get_loc_mgr().create_new_location(file, line, 1);
      return l;
    }
  return location();
}

/// Return the location, the name and the mangled name of a given DIE.
///
/// @param cxt the read context to use.
///
/// @param die the DIE to read location and names from.
///
/// @param loc the location output parameter to set.
///
/// @param name the name output parameter to set.
///
/// @param mangled_name the mangled_name output parameter to set.
static void
die_loc_and_name(read_context&	ctxt,
		 Dwarf_Die*	die,
		 location&	loc,
		 string&	name,
		 string&	mangled_name)
{
  loc = die_location(ctxt, die);
  name = die_string_attribute(die, DW_AT_name);
  mangled_name = die_mangled_name(die);
}

/// Test whether a given DIE represents a decl that is public.  That
/// is, one with the DW_AT_external attribute set.
///
/// @param die the DIE to consider for testing.
///
/// @return true if a DW_AT_external attribute is present and its
/// value is set to the true; return false otherwise.
static bool
is_public_decl(Dwarf_Die* die)
{
  bool is_public = 0;
  die_flag_attribute(die, DW_AT_external, is_public);
  return is_public;
}

/// Given a DW_TAG_compile_unit, build and return the corresponding
/// abigail::translation_unit ir node.  Note that this function
/// recursively reads the children dies of the current DIE and
/// populates the resulting translation unit.
///
/// @param ctxt the read_context to use.
///
/// @param die the DW_TAG_compile_unit DIE to consider.
///
/// @param address_size the size of the addresses expressed in this
/// translation unit in general.
///
/// @param recurse if set to yes, this function recursively reads the
/// children dies of @ref die and populate the resulting translation
/// unit.
///
/// @return a pointer to the resulting translation_unit.
static translation_unit_sptr
build_translation_unit(read_context&	ctxt,
		       Dwarf_Die*	die,
		       char		address_size)
{
  translation_unit_sptr result;

  if (!die)
    return result;
  assert(dwarf_tag(die) == DW_TAG_compile_unit);

  string path = die_string_attribute(die, DW_AT_name);
  result.reset(new translation_unit(path, address_size));

  ctxt.current_corpus()->add(result);
  ctxt.current_translation_unit(result);

  Dwarf_Die child;
  if (dwarf_child(die, &child) != 0)
    return result;

  do
    build_ir_node_from_die(ctxt, &child);
  while (dwarf_siblingof(&child, &child) == 0);

  /// Prune types that are not referenced from any public decl, out of
  /// the translation unit.
  result->prune_unused_types();

  return result;
}

/// Build a @ref namespace_decl out of a DW_TAG_namespace or
/// DW_TAG_module (for fortran) DIE.
///
/// Note that this function connects the DW_TAG_namespace to the IR
/// being currently created, reads the children of the DIE and
/// connects them to the IR as well.
///
/// @param ctxt the read context to use.
///
/// @param die the DIE to read from.  Must be either DW_TAG_namespace
/// or DW_TAG_module.
///
/// @return the resulting @ref nampespace_decl or NULL if it couldn't
/// be created.
static namespace_decl_sptr
build_namespace_decl_and_add_to_ir(read_context&	ctxt,
				   Dwarf_Die*	die)
{
  namespace_decl_sptr result;

  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);
  if (tag != DW_TAG_namespace && tag != DW_TAG_module)
    return result;

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  result.reset(new namespace_decl(name, loc));
  add_decl_to_scope(result, ctxt.current_scope());

  Dwarf_Die child;

  if (dwarf_child(die, &child) != 0)
    return result;

  ctxt.scope_stack().push(result);
  do
    build_ir_node_from_die(ctxt, &child);
  while (dwarf_siblingof(&child, &child) == 0);
  ctxt.scope_stack().pop();

  return result;
}

/// Build a @ref type_decl out of a DW_TAG_base_type DIE.
///
/// @param ctxt the read context to use.
///
/// @param die the DW_TAG_base_type to consider.
///
/// @return the resulting decl_base_sptr.
static type_decl_sptr
build_type_decl(read_context&	ctxt,
		Dwarf_Die*	die)
{
  type_decl_sptr result;

  if (!die)
    return result;
  assert(dwarf_tag(die) == DW_TAG_base_type);

  size_t byte_size = 0, bit_size = 0;
  if (!die_unsigned_constant_attribute(die, DW_AT_byte_size, byte_size))
    if (!die_unsigned_constant_attribute(die, DW_AT_bit_size, bit_size))
      return result;

  if (byte_size == 0 && bit_size == 0)
    return result;

  if (bit_size == 0)
    bit_size = byte_size * 8;

  size_t alignment = bit_size < 8 ? 8 : bit_size;
  string type_name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, type_name, mangled_name);

  result.reset(new type_decl(type_name, bit_size,
			     alignment, loc,
			     mangled_name));
  return result;
}

/// Build a qualified type from a DW_TAG_const_type or
/// DW_TAG_volatile_type DIE.
///
/// @param ctxt the read context to consider.
///
/// @param die the input DIE to read from.
///
/// @param called_from_public_decl true if this function was called
/// from a context where either a public function or a public variable
/// is being built.
///
/// @return the resulting qualified_type_def.
static qualified_type_def_sptr
build_qualified_type(read_context&	ctxt,
		     Dwarf_Die*	die,
		     bool called_from_public_decl)
{
  qualified_type_def_sptr result;
  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);

  if (tag != DW_TAG_const_type
      && tag != DW_TAG_volatile_type)
    return result;

  Dwarf_Die underlying_type_die;
  if (!die_die_attribute(die, DW_AT_type, underlying_type_die))
    return result;

  decl_base_sptr utype_decl = build_ir_node_from_die(ctxt,
						     &underlying_type_die,
						     called_from_public_decl);
  if (!utype_decl)
    return result;

  type_base_sptr utype = is_type(utype_decl);
  assert(utype);

  if (tag == DW_TAG_const_type)
    result.reset(new qualified_type_def(utype,
					qualified_type_def::CV_CONST,
					location()));
  else if (tag == DW_TAG_volatile_type)
    result.reset(new qualified_type_def(utype,
					qualified_type_def::CV_VOLATILE,
					location()));

  return result;
}

/// Build a pointer type from a DW_TAG_pointer_type DIE.
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE to read information from.
///
/// @param called_from_public_decl true if this function was called
/// from a context where either a public function or a public variable
/// is being built.
///
/// @return the resulting pointer to pointer_type_def.
static pointer_type_def_sptr
build_pointer_type_def(read_context&	ctxt,
		       Dwarf_Die*	die,
		       bool called_from_public_decl)
{
  pointer_type_def_sptr result;

  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);
  if (tag != DW_TAG_pointer_type)
    return result;

  Dwarf_Die underlying_type_die;
  if (!die_die_attribute(die, DW_AT_type, underlying_type_die))
    return result;

  decl_base_sptr utype_decl =
    build_ir_node_from_die(ctxt, &underlying_type_die, called_from_public_decl);
  if (!utype_decl)
    return result;

  type_base_sptr utype = is_type(utype_decl);
  assert(utype);

  size_t size;
  if (!die_unsigned_constant_attribute(die, DW_AT_byte_size, size))
    return result;
  size *= 8;

  result.reset(new pointer_type_def(utype, size, size, location()));

  return result;
}

/// Build a reference type from either a DW_TAG_reference_type or
/// DW_TAG_rvalue_reference_type DIE.
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE to read from.
///
/// @param called_from_public_decl true if this function was called
/// from a context where either a public function or a public variable
/// is being built.
///
/// @return a pointer to the resulting reference_type_def.
static reference_type_def_sptr
build_reference_type(read_context& ctxt,
		     Dwarf_Die* die,
		     bool called_from_public_decl)
{
  reference_type_def_sptr result;

  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);
  if (tag != DW_TAG_reference_type
      && tag != DW_TAG_rvalue_reference_type)
    return result;

  Dwarf_Die underlying_type_die;
  if (!die_die_attribute(die, DW_AT_type, underlying_type_die))
    return result;

  decl_base_sptr utype_decl =
    build_ir_node_from_die(ctxt, &underlying_type_die, called_from_public_decl);
  if (!utype_decl)
    return result;

  type_base_sptr utype = is_type(utype_decl);
  assert(utype);

  size_t size;
  if (!die_unsigned_constant_attribute(die, DW_AT_byte_size, size))
    return result;
  size *= 8;

  bool is_lvalue = (tag == DW_TAG_reference_type) ? true : false;

  result.reset(new reference_type_def(utype, is_lvalue, size, size,
				      location()));

  return result;
}

/// Create a typedef_decl from a DW_TAG_typedef DIE.
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE to read from.
///
/// @param called_from_public_decl true if this function was called
/// from a context where either a public function or a public variable
/// is being built.
///
/// @return the newly created typedef_decl.
static typedef_decl_sptr
build_typedef_type(read_context& ctxt,
		   Dwarf_Die* die,
		   bool called_from_public_decl)
{
  typedef_decl_sptr result;

  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);
  if (tag != DW_TAG_typedef)
    return result;

  Dwarf_Die underlying_type_die;
  if (!die_die_attribute(die, DW_AT_type, underlying_type_die))
    return result;

  decl_base_sptr utype_decl =
    build_ir_node_from_die(ctxt, &underlying_type_die, called_from_public_decl);
  if (!utype_decl)
    return result;

  type_base_sptr utype = is_type(utype_decl);
  assert(utype);

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  result.reset(new typedef_decl(name, utype, loc, mangled_name));

  return result;
}

/// Build a @ref var_decl out of a DW_TAG_variable DIE.
///
/// @param ctxt the read context to use.
///
/// @param die the DIE to read from to build the @ref var_decl.
///
/// @return a pointer to the newly created var_decl.  If the var_decl
/// could not be built, this function returns NULL.
static var_decl_sptr
build_var_decl(read_context& ctxt,
	       Dwarf_Die *die)
{
  var_decl_sptr result;

  if (!die)
    return result;
  assert(dwarf_tag(die) == DW_TAG_variable);

  if (!is_public_decl(die))
    return result;

  type_base_sptr type;
  Dwarf_Die type_die;
  if (die_die_attribute(die, DW_AT_type, type_die))
    {
      decl_base_sptr ty =
	build_ir_node_from_die(ctxt, &type_die,
			       /*called_from_public_decl=*/true);
      if (!ty)
	return result;
      type = is_type(ty);
      assert(type);
    }

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  result.reset(new var_decl(name, type, loc, mangled_name));

  return result;
}

/// Build a @ref function_decl our of a DW_TAG_subprogram DIE.
///
/// @param ctxt the read context to use
///
/// @param die the DW_TAG_subprogram DIE to read from.
///
/// @param called_from_public_decl this is set to true if the function
/// was called for a public (function) decl.
static function_decl_sptr
build_function_decl(read_context& ctxt,
		    Dwarf_Die* die,
		    bool called_for_public_decl)
{
  function_decl_sptr result;
  if (!die)
    return result;
  assert(dwarf_tag(die) == DW_TAG_subprogram);

  Dwarf_Die spec_die;
  if (die_die_attribute(die, DW_AT_specification, spec_die))
    {
      // So this means that the current DW_TAG_subprogram DIE is for
      // (refers to) a function declaration that was done earlier.
      // Let's emit a function_decl representing that declaration.  We
      // typically hit this case for DIEs representing concrete
      // functions implementations; in those cases, what we want
      // really is the declaration, as that is what has the meta-data
      // we are looking after.
      decl_base_sptr r =
	build_ir_node_from_die(ctxt, &spec_die, called_for_public_decl);
      return dynamic_pointer_cast<function_decl>(r);
    }

 if (!is_public_decl(die))
    return result;

  translation_unit_sptr tu = ctxt.current_translation_unit();
  assert(tu);

  string fname, fmangled_name;
  location floc;
  die_loc_and_name(ctxt, die, floc, fname, fmangled_name);

  size_t is_inline = false;
  die_unsigned_constant_attribute(die, DW_AT_inline, is_inline);

  Dwarf_Die ret_type_die;
  die_die_attribute(die, DW_AT_type, ret_type_die);

  decl_base_sptr return_type_decl =
    build_ir_node_from_die(ctxt, &ret_type_die,
			   /*called_from_public_decl=*/true);

  Dwarf_Die child;
  function_decl::parameters function_parms;

  if (dwarf_child(die, &child) == 0)
    do
      {
	int child_tag = dwarf_tag(&child);
	if (child_tag == DW_TAG_formal_parameter)
	  {
	    string name, mangled_name;
	    location loc;
	    die_loc_and_name(ctxt, &child, loc, name, mangled_name);
	    Dwarf_Die parm_type_die;
	    die_die_attribute(&child, DW_AT_type, parm_type_die);
	    decl_base_sptr parm_type_decl =
	      build_ir_node_from_die(ctxt, &parm_type_die,
				     /*called_from_public_decl=*/true);
	    if (!parm_type_decl)
	      continue;
	    function_decl::parameter_sptr p
	      (new function_decl::parameter(is_type(parm_type_decl),
					    name, loc));
	    function_parms.push_back(p);
	  }
	else if (child_tag == DW_TAG_unspecified_parameters)
	  {
	    function_decl::parameter_sptr p
	      (new function_decl::parameter(type_base_sptr(),
					    /*name=*/"",
					    location(),
					    /*variadic_marker=*/true));
	    function_parms.push_back(p);
	  }
      }
  while (dwarf_siblingof(&child, &child) == 0);


  result.reset(new function_decl(fname, function_parms,
				 is_type(return_type_decl),
				 tu->get_address_size(),
				 tu->get_address_size(),
				 is_inline, floc,
				 fmangled_name));

  return result;
}

/// Read all @ref translation_unit possible from the debug info
/// accessible through a DWARF Front End Library handle, and stuff
/// them into a libabigail ABI Corpus.
///
/// @param ctxt the read context.
///
/// @return a pointer to the resulting corpus, or NULL if the corpus
/// could not be constructed.
static corpus_sptr
build_corpus(read_context& ctxt)
{
  uint8_t address_size = 0;
  size_t header_size = 0;

  for (Dwarf_Off offset = 0, next_offset = 0;
       (dwarf_nextcu(ctxt.dwarf(), offset, &next_offset, &header_size,
		     NULL, &address_size, NULL) == 0);
       offset = next_offset)
    {
      Dwarf_Off die_offset = offset + header_size;
      Dwarf_Die cu;
      if (!dwarf_offdie(ctxt.dwarf(), die_offset, &cu))
	continue;

      if (!ctxt.current_corpus())
	{
	  corpus_sptr corp (new corpus(ctxt.elf_path()));
	  ctxt.current_corpus(corp);
	}

      address_size *= 8;

      // Build a translation_unit IR node from cu; note that cu must
      // be a DW_TAG_compile_unit die.
      translation_unit_sptr ir_node =
	build_translation_unit(ctxt, &cu, address_size);
      assert(ir_node);
    }
  return ctxt.current_corpus();
}

/// Canonicalize a type and add it to the current IR being built, if
/// necessary. The canonicalized type is appended to the children IR
/// nodes of a given scope.
///
/// @param type_declaration the declaration of the type to
/// canonicalize.
///
/// @param type_scope the scope into which the canonicalized type is
/// to be added.
///
/// @return the resulting canonicalized type.
static decl_base_sptr
canonicalize_and_add_type_to_ir(decl_base_sptr type_declaration,
				scope_decl* type_scope)
{
  if (!type_declaration)
    return type_declaration;

  translation_unit* tu = get_translation_unit(type_scope);
  assert(tu);

  /// TODO: maybe change the interfance of
  /// translation_unit::canonicalize_type to include the final
  /// qualified name of the type (i.e, one that includes the qualified
  /// name of type_scope), to handle two user defined types that might
  /// be same, but at different scopes.  In that case, the two types
  /// should be considered different by
  /// translation_unit::canonicalize_type.
  decl_base_sptr result = tu->canonicalize_type(type_declaration);
  assert(result);

  if (result->get_scope())
    // This type is the same as a type that was already added to the
    // IR tree.  Do not add a new one.  Just re-use the previous one.
    ;
  else
    add_decl_to_scope(result, type_scope);

  return result;
}

/// Canonicalize a type and add it to the current IR being built, if
/// necessary.
///
/// @param type_declaration the declaration of the type to
/// canonicalize.
///
/// @param type_scope the scope into which the canonicalized type
/// needs to be added.
///
/// @return the resulting canonicalized type.
decl_base_sptr
canonicalize_and_add_type_to_ir(decl_base_sptr type_declaration,
				scope_decl_sptr type_scope)
{return canonicalize_and_add_type_to_ir(type_declaration, type_scope.get());}

/// Canonicalize a given type and insert it into the children of a
/// given scope right before a given child.
///
/// @param type_declaration the declaration of the type to canonicalize.
///
/// @param before an iterator pointing to an IR node that is a child
/// of the scope under wich the canonicalized type is to be inserted.
/// The canonicalized type is to be inserted right before that
/// iterator.
static decl_base_sptr
canonicalize_and_insert_type_into_ir(decl_base_sptr type_declaration,
				     scope_decl::declarations::iterator before,
				     scope_decl* type_scope)
{
  if (!type_declaration)
    return type_declaration;

  translation_unit* tu = get_translation_unit(type_scope);
  assert(tu);

  /// TODO: maybe change the interfance of
  /// translation_unit::canonicalize_type to include the final
  /// qualified name of the type (i.e, one that includes the qualified
  /// name of type_scope), to handle two user defined types that might
  /// be same, but at different scopes.  In that case, the two types
  /// should be considered different by
  /// translation_unit::canonicalize_type.
  decl_base_sptr result = tu->canonicalize_type(type_declaration);
  assert(result);

  if (result->get_scope())
    // This type is the same as a type that was already added to the
    // IR tree.  Do not add a new one.  Just re-use the previous one.
    ;
  else
    insert_decl_into_scope(result, before, type_scope);

  return result;
}

/// Canonicalize a type and insert it into the current IR.  The
/// canonicalized type is to be inserted before the current scope C
/// and under a given scope S.  If C and S are equal the the
/// canonicalized type is just appended to the current scope.
///
/// @param ctxt the read context to consider.
///
/// @param type_decl the declaration of the type to canonicalize.
///
/// @param scope the scope under which the canonicalized type is to be
/// added.  This must be a scope that is higher or equal to the
/// current scope.
///
/// @return the declaration of the canonicalized type.
static decl_base_sptr
canonicalize_and_insert_type_into_ir_under_scope(read_context& ctxt,
						 decl_base_sptr type_decl,
						 scope_decl* scope)
{
  decl_base_sptr result;

  const scope_decl* ns_under_scope =
    get_top_most_scope_under(ctxt.current_scope(), scope);

  if (ns_under_scope == scope)
    result =
      canonicalize_and_add_type_to_ir(type_decl, scope);
  else
    {
      scope_decl::declarations::iterator it;
      assert(scope->find_iterator_for_member(ns_under_scope,
					     it));
      result =
	canonicalize_and_insert_type_into_ir(type_decl, it, scope);
    }

  return result;
}

/// Canonicalize a type and insert it into the current IR.  The
/// canonicalized type is to be inserted before the current scope C
/// and under a given scope S.  If C and S are equal the the
/// canonicalized type is just appended to the current scope.
///
/// @param ctxt the read context to consider.
///
/// @param type_decl the declaration of the type to canonicalize.
///
/// @param scope the scope under which the canonicalized type is to be
/// added.  This must be a scope that is higher or equal to the
/// current scope.
///
/// @return the declaration of the canonicalized type.
static decl_base_sptr
canonicalize_and_insert_type_into_ir_under_scope(read_context& ctxt,
						 decl_base_sptr type_decl,
						 scope_decl_sptr scope)
{return canonicalize_and_insert_type_into_ir_under_scope(ctxt, type_decl,
							 scope.get());}

/// Build an IR node from a given DIE and add the node to the current
/// IR being build and held in the read_context.  Doing that is called
/// "emitting an IR node for the DIE".
///
/// @param ctxt the read context.
///
/// @parm die the DIE to consider.
///
/// @param called_from_public_decl if yes flag the types that are
/// possibly going to be created by the invocation to this function as
/// being used by a public decl.  This is later going to be useful to
/// prune all the types that are *not* used by any public public decl.
///
/// @return the resulting IR node.
static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       bool called_from_public_decl)
{
  decl_base_sptr result;

  if (!die)
    return result;

  die_decl_map_type::const_iterator it =
    ctxt.die_decl_map().find(dwarf_dieoffset(die));
  if (it != ctxt.die_decl_map().end())
    {
      result = it->second;
      if (called_from_public_decl)
	{
	  type_base_sptr t = is_type(result);
	  if (t)
	    ctxt.current_translation_unit()->mark_type_as_used(t);
	}
      return result;
    }

  int tag = dwarf_tag(die);

  switch (tag)
    {
      // Type DIEs we intent to support someday, maybe.
    case DW_TAG_base_type:
      if((result = build_type_decl(ctxt, die)))
	{
	  translation_unit_sptr tu = ctxt.current_translation_unit();
	  global_scope_sptr gscope = tu->get_global_scope();
	  result = canonicalize_and_insert_type_into_ir_under_scope(ctxt,
								    result,
								    gscope);
	}
      break;

    case DW_TAG_typedef:
      {
	typedef_decl_sptr t = build_typedef_type(ctxt, die,
						 called_from_public_decl);
	result = canonicalize_and_add_type_to_ir(t, ctxt.current_scope());
      }
      break;

    case DW_TAG_pointer_type:
      {
	pointer_type_def_sptr p =
	  build_pointer_type_def(ctxt, die, called_from_public_decl);
	decl_base_sptr utype =
	  get_type_declaration(p->get_pointed_to_type());
	result =
	  canonicalize_and_insert_type_into_ir_under_scope(ctxt, p,
							   utype->get_scope());
      }
      break;

    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
      {
	reference_type_def_sptr r =
	  build_reference_type(ctxt, die, called_from_public_decl);
	decl_base_sptr utype =
	  get_type_declaration(r->get_pointed_to_type());
	result =
	  canonicalize_and_insert_type_into_ir_under_scope(ctxt, r,
							   utype->get_scope());
      }
      break;

    case DW_TAG_const_type:
    case DW_TAG_volatile_type:
      {
	qualified_type_def_sptr q =
	  build_qualified_type(ctxt, die, called_from_public_decl);
	if (q)
	  {
	    decl_base_sptr t =
	      get_type_declaration(q->get_underlying_type());
	    result =
	      canonicalize_and_insert_type_into_ir_under_scope(ctxt, q,
							       t->get_scope());
	  }
      }
      break;

    case DW_TAG_enumeration_type:
      break;
    case DW_TAG_class_type:
      break;
    case DW_TAG_string_type:
      break;
    case DW_TAG_structure_type:
      break;
    case DW_TAG_subroutine_type:
      break;
    case DW_TAG_union_type:
      break;
    case DW_TAG_array_type:
      break;
    case DW_TAG_packed_type:
      break;
    case DW_TAG_set_type:
      break;
    case DW_TAG_file_type:
      break;
    case DW_TAG_ptr_to_member_type:
      break;
    case DW_TAG_subrange_type:
      break;
    case DW_TAG_thrown_type:
      break;
    case DW_TAG_restrict_type:
      break;
    case DW_TAG_interface_type:
      break;
    case DW_TAG_unspecified_type:
      break;
    case DW_TAG_mutable_type:
      break;
    case DW_TAG_shared_type:
      break;

      // Other declarations we intend to support someday, maybe.

    case DW_TAG_compile_unit:
      // We shouldn't reach this point b/c this should be handled by
      // build_translation_unit.
      abort();
      break;

    case DW_TAG_namespace:
    case DW_TAG_module:
      result = build_namespace_decl_and_add_to_ir(ctxt, die);
      break;

    case DW_TAG_variable:
      if ((result = build_var_decl(ctxt, die)))
	add_decl_to_scope(result, ctxt.current_scope());
      break;

    case DW_TAG_subprogram:
      if ((result = build_function_decl(ctxt, die, called_from_public_decl)))
	  add_decl_to_scope(result, ctxt.current_scope());
      break;

    case DW_TAG_formal_parameter:
      // We should not read this case as it should have been dealt
      // with by build_function_decl above.
      abort();
      break;

    case DW_TAG_constant:
      break;
    case DW_TAG_enumerator:
      break;

      // Other declaration we don't really intend to support yet.
    case DW_TAG_dwarf_procedure:
    case DW_TAG_imported_declaration:
    case DW_TAG_entry_point:
    case DW_TAG_label:
    case DW_TAG_lexical_block:
    case DW_TAG_member:
    case DW_TAG_unspecified_parameters:
    case DW_TAG_variant:
    case DW_TAG_common_block:
    case DW_TAG_common_inclusion:
    case DW_TAG_inheritance:
    case DW_TAG_inlined_subroutine:
    case DW_TAG_with_stmt:
    case DW_TAG_access_declaration:
    case DW_TAG_catch_block:
    case DW_TAG_friend:
    case DW_TAG_namelist:
    case DW_TAG_namelist_item:
    case DW_TAG_template_type_parameter:
    case DW_TAG_template_value_parameter:
    case DW_TAG_try_block:
    case DW_TAG_variant_part:
    case DW_TAG_imported_module:
    case DW_TAG_partial_unit:
    case DW_TAG_imported_unit:
    case DW_TAG_condition:
    case DW_TAG_type_unit:
    case DW_TAG_template_alias:
    case DW_TAG_lo_user:
    case DW_TAG_MIPS_loop:
    case DW_TAG_format_label:
    case DW_TAG_function_template:
    case DW_TAG_class_template:
    case DW_TAG_GNU_BINCL:
    case DW_TAG_GNU_EINCL:
    case DW_TAG_GNU_template_template_param:
    case DW_TAG_GNU_template_parameter_pack:
    case DW_TAG_GNU_formal_parameter_pack:
    case DW_TAG_GNU_call_site:
    case DW_TAG_GNU_call_site_parameter:
    case DW_TAG_hi_user:
    default:
      break;
    }

  if (result)
    {
      if (called_from_public_decl)
	{
	  type_base_sptr t = is_type(result);
	  if (t)
	    ctxt.current_translation_unit()->mark_type_as_used(t);
	}
      ctxt.die_decl_map()[dwarf_dieoffset(die)] = result;
    }

  return result;
}

/// Read all @ref translation_unit possible from the debug info
/// accessible from an elf file, stuff them into a libabigail ABI
/// Corpus and return it.
///
/// @param elf_path the path to the elf file.
///
/// @return a pointer to the resulting @ref corpus.
corpus_sptr
read_corpus_from_elf(const std::string& elf_path)
{
  // Create a DWARF Front End Library handle to be used by functions
  // of that library.
  dwfl_sptr handle = create_default_dwfl_sptr();

  read_context ctxt(handle, elf_path);

  // Load debug info from the elf path.
  if (!ctxt.load_debug_info())
    return corpus_sptr();

  // Now, read an ABI corpus proper from the debug info we have
  // through the dwfl handle.
  corpus_sptr corp = build_corpus(ctxt);

  return corp;
}

}// end namespace dwarf_reader

}// end namespace abigail
