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
/// de-serialize an instance of @ref abigail::corpus from a file in
/// elf format, containing dwarf information.

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <assert.h>
#include <cstring>
#include <cmath>
#include <elfutils/libdwfl.h>
#include <dwarf.h>
#include <tr1/unordered_map>
#include <stack>
#include <deque>
#include <list>
#include <ostream>
#include <sstream>
#include "abg-dwarf-reader.h"

using std::string;

namespace abigail
{

/// The namespace for the DWARF reader.
namespace dwarf_reader
{

using std::tr1::dynamic_pointer_cast;
using std::tr1::unordered_map;
using std::stack;
using std::deque;
using std::list;

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

/// Convenience typedef for a map which key is the offset of a dwarf
/// die, (given by dwarf_dieoffset()) and which value is the
/// corresponding class_decl.
typedef unordered_map<Dwarf_Off, decl_base_sptr> die_class_map_type;

/// Convenience typedef for a map which key is the offset of a
/// DW_TAG_compile_unit and the key is the corresponding @ref
/// translation_unit_sptr.
typedef unordered_map<Dwarf_Off, translation_unit_sptr> die_tu_map_type;

/// Convenience typedef for a stack containing the scopes up to the
/// current point in the abigail Internal Representation (aka IR) tree
/// that is being built.
typedef stack<scope_decl*> scope_stack_type;

/// Convenience typedef for a map that contains the types that have
/// been built so far.
typedef unordered_map<shared_ptr<type_base>,
		      bool,
		      type_base::shared_ptr_hash,
		      type_shared_ptr_equal> type_ptr_map;

/// Convenience typedef for a map which key is a dwarf offset.  The
/// value is also a dwarf offset.
typedef unordered_map<Dwarf_Off, Dwarf_Off> offset_offset_map;

/// The context accumulated during the reading of dwarf debug info and
/// building of the resulting ABI Corpus as a result.
///
/// This context is to be created by the top-most function that wants
/// to read debug info and build an ABI corpus from it.  It's then
/// passed to all the routines that read specific dwarf bits as they
/// get some important data from it.
class read_context
{
  unsigned short dwarf_version_;
  dwfl_sptr handle_;
  Dwarf* dwarf_;
  const string elf_path_;
  die_decl_map_type die_decl_map_;
  die_class_map_type die_wip_classes_map_;
  die_tu_map_type die_tu_map_;
  corpus_sptr cur_corpus_;
  translation_unit_sptr cur_tu_;
  scope_stack_type scope_stack_;
  offset_offset_map die_parent_map_;
  list<var_decl_sptr> var_decls_to_add;

  read_context();

public:
  read_context(dwfl_sptr handle,
	       const string& elf_path)
    : dwarf_version_(0),
      handle_(handle),
      dwarf_(0),
      elf_path_(elf_path)
  {}

  unsigned short
  dwarf_version() const
  {return dwarf_version_;}

  void
  dwarf_version(unsigned short v)
  {dwarf_version_ = v;}

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

  /// Getter of a map that associates a die that represents a
  /// class/struct with the declaration of the class, while the class
  /// is being constructed.
  const die_class_map_type&
  die_wip_classes_map() const
  {return die_wip_classes_map_;}

  /// Getter of a map that associates a die that represents a
  /// class/struct with the declaration of the class, while the class
  /// is being constructed.
  die_class_map_type&
  die_wip_classes_map()
  {return die_wip_classes_map_;}

  const die_tu_map_type&
  die_tu_map() const
  {return die_tu_map_;}

  die_tu_map_type&
  die_tu_map()
  {return die_tu_map_;}

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

  const offset_offset_map&
  die_parent_map() const
  {return die_parent_map_;}

  offset_offset_map&
  die_parent_map()
  {return die_parent_map_;}

  const translation_unit_sptr
  current_translation_unit() const
  {return cur_tu_;}

  translation_unit_sptr
  cur_tu()
  {return cur_tu_;}

  void
  cur_tu(translation_unit_sptr tu)
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

  scope_decl*
  current_scope()
  {
    if (scope_stack().empty())
      {
	if (cur_tu())
	  scope_stack().push
	    (cur_tu()->get_global_scope().get());
      }
    return scope_stack().top();
  }

  list<var_decl_sptr>&
  var_decls_to_re_add_to_tree()
  {return var_decls_to_add;}
};// end class read_context.

static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       scope_decl*	scope,
		       bool		called_from_public_decl = false);

static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       bool		called_from_public_decl = false);

static function_decl_sptr
build_function_decl(read_context& ctxt,
		    Dwarf_Die* die,
		    function_decl_sptr fn);

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
/// @param dwfl the pointer to Dwfl to create the shared pointer for.
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
  if (!dwarf_attr_integrate(die, attr_name, &attr))
    return "";

  const char* str = dwarf_formstring(&attr);
  return str ? str : "";
}

/// Get the value of an attribute that is supposed to be an unsigned
/// constant.
///
/// @param die the DIE to read the information from.
///
/// @param attr_name the DW_AT_* name of the attribute.  Must come
/// from dwarf.h and be an enumerator representing an attribute like,
/// e.g, DW_AT_decl_line.
///
///@param cst the output parameter that is set to the value of the
/// attribute @p attr_name.  This parameter is set iff the function
/// return true.
///
/// @return true if there was an attribute of the name @p attr_name
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
  if (!dwarf_attr_integrate(die, attr_name, &attr)
      || dwarf_formudata(&attr, &result))
    return false;

  cst = result;
  return true;
}

/// Get the value of an attribute that is supposed to be a signed
/// constant.
///
///@param die the DIE to read the information from.
///
/// @param attr_name the DW_AT_* name of the attribute.  Must come
/// from dwarf.h and be an enumerator representing an attribute like,
/// e.g, DW_AT_decl_line.
///
///@param cst the output parameter that is set to the value of the
/// attribute @p attr_name.  This parameter is set iff the function
/// return true.
///
/// @return true if there was an attribute of the name @p attr_name
/// and with a value that is a constant, false otherwise.
static bool
die_signed_constant_attribute(Dwarf_Die*	die,
			      unsigned		attr_name,
			      ssize_t&		cst)
{
    if (!die)
    return false;

  Dwarf_Attribute attr;
  Dwarf_Sword result = 0;
  if (!dwarf_attr_integrate(die, attr_name, &attr)
      || dwarf_formsdata(&attr, &result))
    return false;

  cst = result;
  return true;
}

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
/// @return true if the DIE has a flag attribute named @p attr_name,
/// false otherwise.
static bool
die_flag_attribute(Dwarf_Die* die, unsigned attr_name, bool& flag)
{
  Dwarf_Attribute attr;
  bool f = false;
  if (!dwarf_attr_integrate(die, attr_name, &attr)
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
/// value of the DW_AT_decl_file attribute.  If the DIE @p die
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
/// @return true if the DIE @p die contains an attribute named @p
/// attr_name that is a DIE reference, false otherwise.
static bool
die_die_attribute(Dwarf_Die* die, unsigned attr_name, Dwarf_Die& result)
{
  Dwarf_Attribute attr;
  if (!dwarf_attr_integrate(die, attr_name, &attr))
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
      translation_unit_sptr tu = ctxt.cur_tu();
      location l = tu->get_loc_mgr().create_new_location(file, line, 1);
      return l;
    }
  return location();
}

/// Return the location, the name and the mangled name of a given DIE.
///
/// @param ctxt the read context to use.
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

/// Get the size of a (type) DIE as the value for the parameter
/// DW_AT_byte_size or DW_AT_bit_size.
///
/// @param die the DIE to read the information from.
///
/// @param size the resulting size in bits.  This is set iff the
/// function return true.
///
/// @return true if the size attribute was found.
static bool
die_size_in_bits(Dwarf_Die* die, size_t& size)
{
  if (!die)
    return false;

  size_t byte_size = 0, bit_size = 0;

  if (!die_unsigned_constant_attribute(die, DW_AT_byte_size, byte_size))
    {
      if (!die_unsigned_constant_attribute(die, DW_AT_bit_size, bit_size))
	return false;
    }
  else
    bit_size = byte_size * 8;

  size = bit_size;

  return true;
}

/// Get the access specifier (from the DW_AT_accessibility attribute
/// value) of a given DIE.
///
/// @param die the DIE to consider.
///
/// @param access the resulting access.  This is set iff the function
/// returns true.
///
/// @return bool if the DIE contains the DW_AT_accessibility die.
static bool
die_access_specifier(Dwarf_Die * die, access_specifier& access)
{
  if (!die)
    return false;

  size_t a = 0;
  if (!die_unsigned_constant_attribute(die, DW_AT_accessibility, a))
    return false;

  access_specifier result = private_access;

  switch (a)
    {
    case private_access:
      result = private_access;
      break;

    case protected_access:
      result = protected_access;
      break;

    case public_access:
      result = public_access;
      break;

    default:
      break;
    }

  access = result;
  return true;
}

/// Test whether a given DIE represents a decl that is public.  That
/// is, one with the DW_AT_external attribute set.
///
/// @param die the DIE to consider for testing.
///
/// @return true if a DW_AT_external attribute is present and its
/// value is set to the true; return false otherwise.
static bool
die_is_public_decl(Dwarf_Die* die)
{
  bool is_public = false;
  die_flag_attribute(die, DW_AT_external, is_public);
  return is_public;
}

/// Test whether a given DIE represents a declaration-only DIE.
///
/// That is, if the DIE has the DW_AT_declaration flag set.
///
/// @param die the DIE to consider.
//
/// @return true if a DW_AT_declaration is present, false otherwise.
static bool
die_is_declaration_only(Dwarf_Die* die)
{
 bool is_declaration_only = false;
 die_flag_attribute(die, DW_AT_declaration, is_declaration_only);
 return is_declaration_only;
}

/// Tests whether a given DIE is artificial.
///
/// @param die the test to test for.
///
/// @return true if the DIE is artificial, false otherwise.
static bool
die_is_artificial(Dwarf_Die* die)
{
  bool is_artificial;
  return die_flag_attribute(die, DW_AT_artificial, is_artificial);
}

///@return true if a tag represents a type, false otherwise.
///
///@param tag the tag to consider.
static bool
is_type_tag(unsigned tag)
{
  bool result = false;

  switch (tag)
    {
    case DW_TAG_array_type:
    case DW_TAG_class_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_string_type:
    case DW_TAG_structure_type:
    case DW_TAG_subroutine_type:
    case DW_TAG_typedef:
    case DW_TAG_union_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_subrange_type:
    case DW_TAG_base_type:
    case DW_TAG_const_type:
    case DW_TAG_file_type:
    case DW_TAG_packed_type:
    case DW_TAG_thrown_type:
    case DW_TAG_volatile_type:
    case DW_TAG_restrict_type:
    case DW_TAG_interface_type:
    case DW_TAG_unspecified_type:
    case DW_TAG_mutable_type:
    case DW_TAG_shared_type:
    case DW_TAG_rvalue_reference_type:
      result = true;
      break;

    default:
      result = false;
      break;
    }

  return result;
}

/// Test if a DIE represents a type DIE.
///
/// @param die the DIE to consider.
///
/// @return true if @p die represents a type, false otherwise.
static bool
is_type_die(Dwarf_Die* die)
{
  if (!die)
    return false;
  return is_type_tag(dwarf_tag(die));
}

enum virtuality
{
  VIRTUALITY_NOT_VIRTUAL,
  VIRTUALITY_VIRTUAL,
  VIRTUALITY_PURE_VIRTUAL
};

/// Get the virtual-ness of a given DIE, that is, the value of the
/// DW_AT_virtuality attribute.
///
/// @param die the DIE to read from.
///
/// @param virt the resulting virtuality attribute.  This is set iff
/// the function returns true.
///
/// @return true if the virtual-ness could be determined.
static bool
die_virtuality(Dwarf_Die* die, virtuality& virt)
{
  if (!die)
    return false;

  size_t v = 0;
  die_unsigned_constant_attribute(die, DW_AT_virtuality, v);

  if (v == DW_VIRTUALITY_virtual)
    virt = VIRTUALITY_VIRTUAL;
  else if (v == DW_VIRTUALITY_pure_virtual)
    virt = VIRTUALITY_PURE_VIRTUAL;
  else
    virt = VIRTUALITY_NOT_VIRTUAL;

  return true;
}

/// Test whether the DIE represent either a virtual base or function.
///
/// @param die the DIE to consider.
///
/// @return bool if the DIE represents a virtual base or function,
/// false othersise.
static bool
die_is_virtual(Dwarf_Die* die)
{
  virtuality v;
  if (!die_virtuality(die, v))
    return false;

  return v == VIRTUALITY_PURE_VIRTUAL || v == VIRTUALITY_VIRTUAL;
}

/// Get the value of a given DIE attribute, knowing that it must be a
/// location expression.
///
/// @param die the DIE to read the attribute from.
///
/// @param attr_name the name of the attribute to read the value for.
///
/// @param expr the pointer to allocate and fill with the resulting
/// array of operators + operands forming a dwarf expression.  This is
/// set iff the function returns true.
///
/// @param expr_len the length of the resulting dwarf expression.
/// This is set iff the function returns true.
///
/// @return true if the attribute exists and has a dwarf expression as
/// value.  In that case the expr and expr_len arguments are set to
/// the resulting dwarf exprssion.
static bool
die_location_expr(Dwarf_Die* die,
		  unsigned attr_name,
		  Dwarf_Op** expr,
		  size_t* expr_len)
{
  if (!die)
    return false;

  Dwarf_Attribute attr;
  if (!dwarf_attr_integrate(die, attr_name, &attr))
    return false;
  return (dwarf_getlocation(&attr, expr, expr_len) == 0);
}

/// An abstraction of a value representing the result of the
/// evalutation of a dwarf expression.  This is abstraction represents
/// a partial view on the possible values because we are only
/// interested in extracting the latest and longuest constant
/// sub-expression of a given dwarf expression.
class expr_result
{
  bool is_const_;
  ssize_t const_value_;

public:
  expr_result()
    : is_const_(true),
      const_value_(0)
  {}

  expr_result(bool is_const)
    : is_const_(is_const),
      const_value_(0)
  {}

  explicit expr_result(ssize_t v)
    :is_const_(true),
     const_value_(v)
  {}

  /// @return true if the value is a constant.  Otherwise, return
  /// false, meaning the value represents a quantity for which we need
  /// inferior (a running program) state to determine the value.
  bool
  is_const() const
  {return is_const_;}


  /// @param f a flag saying if the value is set to a constant or not.
  void
  is_const(bool f)
  {is_const_ = f;}

  /// Get the current constant value iff this represents a
  /// constant.
  ///
  /// @param value the out parameter.  Is set to the constant value of
  /// the @ref expr_result.  This is set iff the function return true.
  ///
  ///@return true if this has a constant value, false otherwise.
  bool
  const_value(ssize_t& value)
  {
    if (is_const())
      {
	value = const_value_;
	return true;
      }
    return false;
  }

  /// Getter of the constant value of the current @ref expr_result.
  ///
  /// Note that the current @ref expr_result must be constant,
  /// otherwise the current process is aborted.
  ///
  /// @return the constant value of the current @ref expr_result.
  ssize_t
  const_value() const
  {
    assert(is_const());
    return const_value_;
  }

  operator ssize_t() const
  {return const_value();}

  expr_result&
  operator=(const ssize_t v)
  {
    const_value_ = v;
    return *this;
  }

  bool
  operator==(const expr_result& o) const
  {return const_value_ == o.const_value_ && is_const_ == o.is_const_;}

  bool
  operator>=(const expr_result& o) const
  {return const_value_ >= o.const_value_;}

  bool
  operator<=(const expr_result& o) const
  {return const_value_ <= o.const_value_;}

  bool
  operator>(const expr_result& o) const
  {return const_value_ > o.const_value_;}

  bool
  operator<(const expr_result& o) const
  {return const_value_ < o.const_value_;}

  expr_result
  operator+(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ += v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result&
  operator+=(ssize_t v)
  {
    const_value_ += v;
    return *this;
  }

  expr_result
  operator-(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ -= v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result
  operator%(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ %= v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const();
    return r;
  }

  expr_result
  operator*(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ *= v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const();
    return r;
  }

  expr_result
  operator|(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ |= v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result
  operator^(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ ^= v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result
  operator>>(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ = r.const_value_ >> v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result
  operator<<(const expr_result& v) const
  {
    expr_result r(*this);
    r.const_value_ = r.const_value_ << v.const_value_;
    r.is_const_ = r.is_const_ && v.is_const_;
    return r;
  }

  expr_result
  operator~() const
  {
    expr_result r(*this);
    r.const_value_ = ~r.const_value_;
    return r;
  }

  expr_result
  neg() const
  {
    expr_result r(*this);
    r.const_value_ = -r.const_value_;
    return r;
  }

  expr_result
  abs() const
  {
    expr_result r = *this;
    r.const_value_ = std::abs(r.const_value());
    return r;
  }

  expr_result
  operator&(const expr_result& o)
  {
    expr_result r(*this);
    r.const_value_ = *this & o;
    r.is_const_ = r.is_const_ && o.is_const_;
    return r;
  }

  expr_result
  operator/(const expr_result& o)
  {
    expr_result r(*this);
    r.is_const_ = r.is_const_ && o.is_const_;
    return r.const_value() / o.const_value();
  }
};// class end expr_result;


/// Abstraction of the evaluation context of a dwarf expression.
struct dwarf_expr_eval_context
{
  expr_result accum;
  deque<expr_result> stack;

  dwarf_expr_eval_context()
    : accum(/*is_const=*/false)
  {
    stack.push_front(expr_result(true));
  }

  expr_result
  pop()
  {
    expr_result r = stack.front();
    stack.pop_front();
    return r;
  }

  void
  push(const expr_result& v)
  {stack.push_front(v);}
};//end class dwarf_expr_eval_context

/// If the current operation in the dwarf expression represents a push
/// of a constant value onto the dwarf expr virtual machine (aka
/// DEVM), perform the operation and update the DEVM.
///
/// If the result of the operation is a constant, update the DEVM
/// accumulator with its value.  Otherwise, the DEVM accumulator is
/// left with its previous value.
///
/// @param ops the array of the dwarf expression operations to consider.
///
/// @param ops_len the lengths of @p ops array above.
///
/// @param index the index of the operation to interpret, in @p ops.
///
/// @param next_index the index of the operation to interpret at the
/// next step, after this function completed and returned.  This is
/// set an output parameter that is set iff the function returns true.
///
/// @param ctxt the DEVM evaluation context.
///
/// @return true if the current operation actually pushes a constant
/// value onto the DEVM stack, false otherwise.
static bool
op_pushes_constant_value(Dwarf_Op*			ops,
			 size_t			ops_len,
			 size_t			index,
			 size_t&			next_index,
			 dwarf_expr_eval_context&	ctxt)
{
  Dwarf_Op& op = ops[index];
  ssize_t value = 0;

  switch (op.atom)
    {
    case DW_OP_addr:
      if (index + 1 < ops_len)
	{
	  value = ops[index + 1].number;
	  next_index = index + 2;
	}
      break;

    case DW_OP_const1u:
    case DW_OP_const1s:
    case DW_OP_const2u:
    case DW_OP_const2s:
    case DW_OP_const4u:
    case DW_OP_const4s:
    case DW_OP_const8u:
    case DW_OP_const8s:
    case DW_OP_constu:
    case DW_OP_consts:
      value = ops[index].number;
      break;

    case DW_OP_lit0:
      value = 0;
      break;
    case DW_OP_lit1:
      value = 1;
      break;
    case DW_OP_lit2:
      value = 2;
      break;
    case DW_OP_lit3:
      value = 3;
      break;
    case DW_OP_lit4:
      value = 4;
      break;
    case DW_OP_lit5:
      value = 5;
      break;
    case DW_OP_lit6:
      value = 6;
      break;
    case DW_OP_lit7:
      value = 7;
      break;
    case DW_OP_lit8:
      value = 8;
      break;
    case DW_OP_lit9:
      value = 9;
      break;
    case DW_OP_lit10:
      value = 10;
      break;
    case DW_OP_lit11:
      value = 11;
      break;
    case DW_OP_lit12:
      value = 12;
      break;
    case DW_OP_lit13:
      value = 13;
      break;
    case DW_OP_lit14:
      value = 14;
      break;
    case DW_OP_lit15:
      value = 15;
      break;
    case DW_OP_lit16:
      value = 16;
      break;
    case DW_OP_lit17:
      value = 17;
      break;
    case DW_OP_lit18:
      value = 18;
      break;
    case DW_OP_lit19:
      value = 19;
      break;
    case DW_OP_lit20:
      value = 20;
      break;
    case DW_OP_lit21:
      value = 21;
      break;
    case DW_OP_lit22:
      value = 22;
      break;
    case DW_OP_lit23:
      value = 23;
      break;
    case DW_OP_lit24:
      value = 24;
      break;
    case DW_OP_lit25:
      value = 25;
      break;
    case DW_OP_lit26:
      value = 26;
      break;
    case DW_OP_lit27:
      value = 27;
      break;
    case DW_OP_lit28:
      value = 28;
      break;
    case DW_OP_lit29:
      value = 29;
      break;
    case DW_OP_lit30:
      value = 30;
      break;
    case DW_OP_lit31:
      value = 31;
      break;

    default:
      return false;
    }

  expr_result r(value);
  ctxt.stack.push_front(r);
  ctxt.accum = r;
  next_index = index + 1;

  return true;
}

/// If the current operation in the dwarf expression represents a push
/// of a non-constant value onto the dwarf expr virtual machine (aka
/// DEVM), perform the operation and update the DEVM.  A non-constant
/// is namely a quantity for which we need inferior (a running program
/// image) state to know the exact value.
///
/// Upon successful completion, as the result of the operation is a
/// non-constant the DEVM accumulator value is left to its state as of
/// before the invocation of this function.
///
/// @param ops the array of the dwarf expression operations to consider.
///
/// @param ops_len the lengths of @p ops array above.
///
/// @param index the index of the operation to interpret, in @p ops.
///
/// @param next_index the index of the operation to interpret at the
/// next step, after this function completed and returned.  This is
/// set an output parameter that is set iff the function returns true.
///
/// @param ctxt the DEVM evaluation context.
///
/// @return true if the current operation actually pushes a
/// non-constant value onto the DEVM stack, false otherwise.
static bool
op_pushes_non_constant_value(Dwarf_Op* ops,
			     size_t ops_len,
			     size_t index,
			     size_t& next_index,
			     dwarf_expr_eval_context& ctxt)
{
  assert(index < ops_len);
  Dwarf_Op& op = ops[index];

  switch (op.atom)
    {
    case DW_OP_reg0:
    case DW_OP_reg1:
    case DW_OP_reg2:
    case DW_OP_reg3:
    case DW_OP_reg4:
    case DW_OP_reg5:
    case DW_OP_reg6:
    case DW_OP_reg7:
    case DW_OP_reg8:
    case DW_OP_reg9:
    case DW_OP_reg10:
    case DW_OP_reg11:
    case DW_OP_reg12:
    case DW_OP_reg13:
    case DW_OP_reg14:
    case DW_OP_reg15:
    case DW_OP_reg16:
    case DW_OP_reg17:
    case DW_OP_reg18:
    case DW_OP_reg19:
    case DW_OP_reg20:
    case DW_OP_reg21:
    case DW_OP_reg22:
    case DW_OP_reg23:
    case DW_OP_reg24:
    case DW_OP_reg25:
    case DW_OP_reg26:
    case DW_OP_reg27:
    case DW_OP_reg28:
    case DW_OP_reg29:
    case DW_OP_reg30:
    case DW_OP_reg31:
      next_index = index + 1;
      break;

    case DW_OP_breg0:
    case DW_OP_breg1:
    case DW_OP_breg2:
    case DW_OP_breg3:
    case DW_OP_breg4:
    case DW_OP_breg5:
    case DW_OP_breg6:
    case DW_OP_breg7:
    case DW_OP_breg8:
    case DW_OP_breg9:
    case DW_OP_breg10:
    case DW_OP_breg11:
    case DW_OP_breg12:
    case DW_OP_breg13:
    case DW_OP_breg14:
    case DW_OP_breg15:
    case DW_OP_breg16:
    case DW_OP_breg17:
    case DW_OP_breg18:
    case DW_OP_breg19:
    case DW_OP_breg20:
    case DW_OP_breg21:
    case DW_OP_breg22:
    case DW_OP_breg23:
    case DW_OP_breg24:
    case DW_OP_breg25:
    case DW_OP_breg26:
    case DW_OP_breg27:
    case DW_OP_breg28:
    case DW_OP_breg29:
    case DW_OP_breg30:
    case DW_OP_breg31:
      next_index = index + 1;
      break;

    case DW_OP_regx:
      next_index = index + 2;
      break;

    case DW_OP_fbreg:
      next_index = index + 1;
      break;

    case DW_OP_bregx:
      next_index = index + 1;
      break;

    default:
      return false;
    }

  expr_result r(false);
  ctxt.stack.push_front(r);

  return true;
}

/// If the current operation in the dwarf expression represents a
/// manipulation of the stack of the DWARF Expression Virtual Machine
/// (aka DEVM), this function performs the operation and updates the
/// state of the DEVM.  If the result of the operation represents a
/// constant value, then the accumulator of the DEVM is set to that
/// result's value, Otherwise, the DEVM accumulator is left with its
/// previous value.
///
/// @param expr the array of the dwarf expression operations to consider.
///
/// @param expr_len the lengths of @p ops array above.
///
/// @param index the index of the operation to interpret, in @p ops.
///
/// @param next_index the index of the operation to interpret at the
/// next step, after this function completed and returned.  This is
/// set an output parameter that is set iff the function returns true.
///
/// @param ctxt the DEVM evaluation context.
///
/// @return true if the current operation actually manipulates the
/// DEVM stack, false otherwise.
static bool
op_manipulates_stack(Dwarf_Op* expr,
		     size_t expr_len,
		     size_t index,
		     size_t& next_index,
		     dwarf_expr_eval_context& ctxt)
{
  Dwarf_Op& op = expr[index];
  expr_result v;

  switch (op.atom)
    {
    case DW_OP_dup:
      v = ctxt.stack.front();
      ctxt.stack.push_front(v);
      break;

    case DW_OP_drop:
      v = ctxt.stack.front();
      ctxt.stack.pop_front();
      break;

    case DW_OP_over:
	assert(ctxt.stack.size() > 1);
	v = ctxt.stack[1];
	ctxt.stack.push_front(v);
      break;

    case DW_OP_pick:
	assert(index + 1 < expr_len);
	v = op.number;
	ctxt.stack.push_front(v);
      break;

    case DW_OP_swap:
	assert(ctxt.stack.size() > 1);
	v = ctxt.stack[1];
	ctxt.stack.erase(ctxt.stack.begin() + 1);
	ctxt.stack.push_front(v);
      break;

    case DW_OP_rot:
	assert(ctxt.stack.size() > 2);
	v = ctxt.stack[2];
	ctxt.stack.erase(ctxt.stack.begin() + 2);
	ctxt.stack.push_front(v);
      break;

    case DW_OP_deref:
    case DW_OP_deref_size:
      assert(ctxt.stack.size() > 0);
      ctxt.stack.pop_front();
      v.is_const(false);
      ctxt.stack.push_front(v);
      break;

    case DW_OP_xderef:
    case DW_OP_xderef_size:
      assert(ctxt.stack.size() > 1);
      ctxt.stack.pop_front();
      ctxt.stack.pop_front();
      v.is_const(false);
      ctxt.stack.push_front(v);
      break;

    case DW_OP_push_object_address:
      v.is_const(false);
      ctxt.stack.push_front(v);
      break;

    case DW_OP_form_tls_address:
      assert(ctxt.stack.size() > 0);
      ctxt.stack.pop_front();
      v.is_const(false);
      ctxt.stack.push_front(v);
      break;

    case DW_OP_call_frame_cfa:
      v.is_const(false);
      ctxt.stack.push_front(v);
      break;

    default:
      return false;
    }

  if (v.is_const())
    ctxt.accum = v;
  next_index = index + 1;

  return true;
}

/// If the current operation in the dwarf expression represents a push
/// of an arithmetic or logic operation onto the dwarf expr virtual
/// machine (aka DEVM), perform the operation and update the DEVM.
///
/// If the result of the operation is a constant, update the DEVM
/// accumulator with its value.  Otherwise, the DEVM accumulator is
/// left with its previous value.
///
/// @param expr the array of the dwarf expression operations to consider.
///
/// @param expr_len the lengths of @p expr array above.
///
/// @param index the index of the operation to interpret, in @p expr.
///
/// @param next_index the index of the operation to interpret at the
/// next step, after this function completed and returned.  This is
/// set an output parameter that is set iff the function returns true.
///
/// @param ctxt the DEVM evaluation context.
///
/// @return true if the current operation actually represent an
/// arithmetic or logic operation.
static bool
op_is_arith_logic(Dwarf_Op* expr,
		  size_t expr_len,
		  size_t index,
		  size_t& next_index,
		  dwarf_expr_eval_context& ctxt)
{
  assert(index < expr_len);

  Dwarf_Op& op = expr[index];
  expr_result val1, val2;

  switch (op.atom)
    {
    case DW_OP_abs:
      val1 = ctxt.pop();
      val1 = val1.abs();
      ctxt.push(val1);
      break;

    case DW_OP_and:
      assert(ctxt.stack.size() > 1);
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val1 & val2);
      break;

    case DW_OP_div:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      if (!val1.is_const())
	val1 = 1;
      ctxt.push(val2 / val1);
      break;

    case DW_OP_minus:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 - val1);
      break;

    case DW_OP_mod:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 % val1);
      break;

    case DW_OP_mul:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 * val1);
      break;

    case DW_OP_neg:
      val1 = ctxt.pop();
      ctxt.push(-val1);
      break;

    case DW_OP_not:
      val1 = ctxt.pop();
      ctxt.push(~val1);
      break;

    case DW_OP_or:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val1 | val2);
      break;

    case DW_OP_plus:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 + val1);
      break;

    case DW_OP_plus_uconst:
      val1 = ctxt.pop();
      val1 += op.number;
      ctxt.push(val1);
      break;

    case DW_OP_shl:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 << val1);
      break;

    case DW_OP_shr:
    case DW_OP_shra:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 >> val1);
      break;

    case DW_OP_xor:
      val1 = ctxt.pop();
      val2 = ctxt.pop();
      ctxt.push(val2 ^ val1);
      break;

    default:
      return false;
    }

  if (ctxt.stack.front().is_const())
    ctxt.accum = ctxt.stack.front();

  next_index = index + 1;
  return true;
}

/// If the current operation in the dwarf expression represents a push
/// of a control flow operation onto the dwarf expr virtual machine
/// (aka DEVM), perform the operation and update the DEVM.
///
/// If the result of the operation is a constant, update the DEVM
/// accumulator with its value.  Otherwise, the DEVM accumulator is
/// left with its previous value.
///
/// @param expr the array of the dwarf expression operations to consider.
///
/// @param expr_len the lengths of @p expr array above.
///
/// @param index the index of the operation to interpret, in @p expr.
///
/// @param next_index the index of the operation to interpret at the
/// next step, after this function completed and returned.  This is
/// set an output parameter that is set iff the function returns true.
///
/// @param ctxt the DEVM evaluation context.
///
/// @return true if the current operation actually represents a
/// control flow operation, false otherwise.
static bool
op_is_control_flow(Dwarf_Op* expr,
		   size_t expr_len,
		   size_t index,
		   size_t& next_index,
		   dwarf_expr_eval_context& ctxt)
{
    assert(index < expr_len);

  Dwarf_Op& op = expr[index];
  expr_result val1, val2;

  switch (op.atom)
    {
    case DW_OP_eq:
    case DW_OP_ge:
    case DW_OP_gt:
    case DW_OP_le:
    case DW_OP_lt:
    case DW_OP_ne:
      {
	bool value = true;
	val1 = ctxt.pop();
	val2 = ctxt.pop();
      if (op.atom == DW_OP_eq)
	value = val2 == val1;
      else if (op.atom == DW_OP_ge)
	value = val2 >= val1;
      else if (op.atom == DW_OP_gt)
	value = val2 > val1;
      else if (op.atom == DW_OP_le)
	value = val2 <= val1;
      else if (op.atom == DW_OP_lt)
	value = val2 < val1;
      else if (op.atom == DW_OP_ne)
	value = val2 != val1;

      val1 = value ? 1 : 0;
      ctxt.push(val1);
      }
      break;

    case DW_OP_skip:
      if (op.number > 0)
	index += op.number - 1;
      break;

    case DW_OP_bra:
      val1 = ctxt.pop();
      if (val1 != 0)
	  index += val1.const_value() - 1;
      break;

    case DW_OP_call2:
    case DW_OP_call4:
    case DW_OP_call_ref:
    case DW_OP_nop:
      break;

    default:
      return false;
    }

  if (ctxt.stack.front().is_const())
    ctxt.accum = ctxt.stack.front();

  next_index = index + 1;
  return true;
}

/// Evaluate the value of the last sub-expression that is a constant,
/// inside a given DWARF expression.
///
/// @param expr the DWARF expression to consider.
///
/// @param expr_len the length of the expression to consider.
///
/// @param value the resulting value of the last constant
/// sub-expression of the DWARF expression.  This is set iff the
/// function returns true.
///
/// @return true if the function could find a constant sub-expression
/// to evaluate, false otherwise.
static bool
eval_last_constant_dwarf_sub_expr(Dwarf_Op* expr,
				  size_t expr_len,
				  ssize_t& value)
{
  dwarf_expr_eval_context eval_ctxt;

  size_t index = 0, next_index = 0;
  do
    {
      if (op_is_arith_logic(expr, expr_len, index,
			       next_index, eval_ctxt)
	  || op_pushes_constant_value(expr, expr_len, index,
				   next_index, eval_ctxt)
	  || op_manipulates_stack(expr, expr_len, index,
				  next_index, eval_ctxt)
	  || op_pushes_non_constant_value(expr, expr_len, index,
					  next_index, eval_ctxt)
	  || op_is_control_flow(expr, expr_len, index,
				next_index, eval_ctxt))
	;
      else
	next_index = index + 1;

      assert(next_index > index);
      index = next_index;
    } while (index < expr_len);

  if (eval_ctxt.accum.is_const())
    {
      value = eval_ctxt.accum;
      return true;
    }
  return false;
}

/// Get the offset of a struct/class member as represented by the
/// value of the DW_AT_data_member_location attribute.
///
/// There is a huge gotcha in here.  The value of the
/// DW_AT_data_member_location is not a constant that one would just
/// read and be done with it.  Rather, it's a DWARF expression that
/// one has to interpret.  There are three general cases to consider:
///
///     1/ The offset in the vtable where the offset of the of a
///        virtual base can be found, aka vptr offset.  Given the
///        address of a given object O, the vptr offset for B is given
///        by the (DWARF) expression:
///
///            address(O) + *(*address(0) - VIRTUAL_OFFSET)
///
///        where VIRTUAL_OFFSET is a constant value; In this case,
///        this function returns the constant VIRTUAL_OFFSET, as this
///        is enough to detect changes in the place of a given virtual
///        base, relative to the other virtual bases.
///
///     2/ The offset of a regular data member.  Given the address of
///        a struct object, the memory location for a particular data
///        member is given by the (DWARF) expression:
///
///            address(O) + OFFSET
///
///       where OFFSET is a constant.  In this case, this function
///       returns the OFFSET constant.
///
///     3/ The offset of a virtual member function in the virtual
///     pointer.  The DWARF expression is a constant that designates
///     the offset of the function in the vtable.  In this case this
///     function returns that constant.
///
///@param die the DIE to read the information from.
///
///@param offset the resulting constant.  This argument is set iff the
///function returns true.
static bool
die_member_offset(Dwarf_Die* die,
		  ssize_t& offset)
{
  Dwarf_Op* expr = NULL;
  size_t expr_len = 0;

  if (!die_location_expr(die, DW_AT_data_member_location, &expr, &expr_len))
    return false;

  if (!eval_last_constant_dwarf_sub_expr(expr, expr_len, offset))
    return false;

  return true;
}

/// Return the index of a function in its virtual table.  That is,
/// return the value of the DW_AT_vtable_elem_location attribute.
///
/// @param die the DIE of the function to consider.
///
/// @param vindex the resulting index.  This is set iff the function
/// returns true.
///
/// @return true if the DIE has a DW_AT_vtable_elem_location
/// attribute.
static bool
die_virtual_function_index(Dwarf_Die* die,
			   size_t& vindex)
{
  if (!die)
    return false;

  Dwarf_Op* expr = NULL;
  size_t expr_len = 0;
  if (!die_location_expr(die, DW_AT_vtable_elem_location,
			 &expr, &expr_len))
    return false;

  ssize_t i = 0;
  if (!eval_last_constant_dwarf_sub_expr(expr, expr_len, i))
    return false;

  vindex = i;
  return true;
}

/// Walk the DIEs under a given die and for each child, populate the
/// read_context::die_parent_map() to record the child -> parent
/// relationship that exists between the child and the given die.
///
/// This is done recursively as for each child DIE, this function
/// walks its children as well.
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE whose children to walk recursively.
static void
build_die_parent_relations_under(read_context& ctxt, Dwarf_Die *die)
{
  if (!die)
    return;

  Dwarf_Die child;
  if (dwarf_child(die, &child) != 0)
    return;

  do
    {
      ctxt.die_parent_map()[dwarf_dieoffset(&child)] = dwarf_dieoffset(die);
      build_die_parent_relations_under(ctxt, &child);
    }
  while (dwarf_siblingof(&child, &child) == 0);
}

/// Walk all the DIEs accessible in the debug info and build a map
/// representing the relationship DIE -> parent.  That is, make it so
/// that we can get the parent for a given DIE.
///
/// @param ctxt the read context from which to get the needed
/// information.
static void
build_die_parent_map(read_context& ctxt)
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
      build_die_parent_relations_under(ctxt, &cu);
    }
}

/// Return the parent DIE for a given DIE.
///
/// Note that the function build_die_parent_map() must have been
/// called before this one can work.  This function either succeeds or
/// aborts the current process.
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE for which we want the parent.
///
/// @param parent_die the output parameter set to the parent die of
/// @p die.  Its memory must be allocated and handled by the caller.
static void
get_parent_die(read_context&	ctxt,
	       Dwarf_Die*	die,
	       Dwarf_Die*	parent_die)
{
  assert(ctxt.dwarf());

  offset_offset_map::const_iterator i =
    ctxt.die_parent_map().find(dwarf_dieoffset(die));
  assert(i != ctxt.die_parent_map().end());

  assert(dwarf_offdie(ctxt.dwarf(), i->second, parent_die) != 0);
}

/// Return the abigail IR node representing the scope of a given DIE.
/// If that
static scope_decl_sptr
get_scope_for_die(read_context& ctxt,
		  Dwarf_Die* die,
		  bool called_for_public_decl = true)
{
  Dwarf_Die parent_die;
  get_parent_die(ctxt, die, &parent_die);

  if (dwarf_tag(&parent_die) == DW_TAG_compile_unit)
    {
      // For top level DIEs like DW_TAG_compile_unit, we just want to
      // return the global scope for the corresponding translation
      // unit.  This must have been set by
      // build_translation_unit_and_add_to_ir.
      die_tu_map_type::const_iterator i =
	ctxt.die_tu_map().find(dwarf_dieoffset(&parent_die));
      assert(i != ctxt.die_tu_map().end());
      return i->second->get_global_scope();
    }

  scope_decl_sptr s;
  decl_base_sptr d;
  if (dwarf_tag(&parent_die) == DW_TAG_subprogram)
    // this is an entity defined in a scope that is a function.
    // Normally, I would say that this should be dropped.  But I have
    // seen a case where a typedef DIE needed by a function parameter
    // was defined right before the parameter, under the scope of the
    // function.  Yeah, weird.  So if I drop the typedef DIE, I'd drop
    // the function parm too.  So for that case, let's say that the
    // scope is the scope of the function itself.
    return get_scope_for_die(ctxt, &parent_die, called_for_public_decl);
  else
    d = build_ir_node_from_die(ctxt, &parent_die,
			       called_for_public_decl);
  s =  dynamic_pointer_cast<scope_decl>(d);
  if (!s)
    // this is an entity defined in someting that is not a scope.
    // Let's drop it.
    return scope_decl_sptr();

  class_decl_sptr cl = dynamic_pointer_cast<class_decl>(d);
  if (cl && cl->get_is_declaration_only())
    {
      scope_decl_sptr scop (cl->get_definition_of_declaration());
      if (scop)
	s = scop;
      else
	s = cl;
    }
  return s;
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
/// @return a pointer to the resulting translation_unit.
static translation_unit_sptr
build_translation_unit_and_add_to_ir(read_context&	ctxt,
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
  ctxt.cur_tu(result);
  ctxt.die_tu_map()[dwarf_dieoffset(die)] = result;

  Dwarf_Die child;
  if (dwarf_child(die, &child) != 0)
    return result;

  do
    build_ir_node_from_die(ctxt, &child);
  while (dwarf_siblingof(&child, &child) == 0);

  if (!ctxt.var_decls_to_re_add_to_tree().empty())
    for (list<var_decl_sptr>::const_iterator v =
	   ctxt.var_decls_to_re_add_to_tree().begin();
	 v != ctxt.var_decls_to_re_add_to_tree().end();
	 ++v)
      {
	  if (is_member_decl(*v))
	  continue;

	assert((*v)->get_scope());
	string demangled_name =
	  demangle_cplus_mangled_name((*v)->get_mangled_name());
	if (!demangled_name.empty())
	  {
	    std::list<string> fqn_comps;
	    fqn_to_components(demangled_name, fqn_comps);
	    string mem_name = fqn_comps.back();
	    fqn_comps.pop_back();
	    decl_base_sptr ty_decl;
	    if (!fqn_comps.empty())
	      ty_decl = lookup_type_in_translation_unit(fqn_comps,
							*ctxt.cur_tu());
	    if (class_decl_sptr cl = dynamic_pointer_cast<class_decl>(ty_decl))
	      {
		// So we are seeing a member variable for which there
		// is a global variable definition DIE not having a
		// reference attribute pointing back to the member
		// variable declaration DIE.  Thus remove the global
		// variable definition from its current non-class
		// scope ...
		remove_decl_from_scope(*v);
		decl_base_sptr d;
		if (d = lookup_var_decl_in_scope(mem_name,cl))
		  // This is the data member with the same name in cl.
		  // We need to flag it as static.
		  ;
		else
		  // In this case there is no data member with the
		  // same name in cl already.  Let's add it there then
		  // ...
		  d = add_decl_to_scope(*v, cl);

		assert(dynamic_pointer_cast<var_decl>(d));
		// Let's flag the data member as static.
		set_member_is_static(d, true);
	      }
	    if (ty_decl)
	      assert(ty_decl->get_scope());
	  }
      }
  ctxt.var_decls_to_re_add_to_tree().clear();
  return result;
}

/// Build a abigail::namespace_decl out of a DW_TAG_namespace or
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
/// @return the resulting @ref abigail::namespace_decl or NULL if it
/// couldn't be created.
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

  scope_decl_sptr scope = get_scope_for_die(ctxt, die);

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  result.reset(new namespace_decl(name, loc));
  add_decl_to_scope(result, scope.get());
  ctxt.die_decl_map()[dwarf_dieoffset(die)] = result;

  Dwarf_Die child;
  if (dwarf_child(die, &child) != 0)
    return result;

  ctxt.scope_stack().push(result.get());
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

/// Build an enum_type_decl from a DW_TAG_enumeration_type DIE.
///
/// @param ctxt the read context to use.
///
/// @param die the DIE to read from.
///
/// @return the built enum_type_decl or NULL if it could not be built.
static enum_type_decl_sptr
build_enum_type(read_context& ctxt, Dwarf_Die* die)
{
  enum_type_decl_sptr result;
  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);
  if (tag != DW_TAG_enumeration_type)
    return result;

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  size_t size = 0;
  if (die_unsigned_constant_attribute(die, DW_AT_byte_size, size))
    size *= 8;

  string underlying_type_name;
  if (name.empty())
    underlying_type_name = "unnamed-enum";
  else
    underlying_type_name = string("enum-") + name;
  underlying_type_name += "-underlying-type";

  enum_type_decl::enumerators enms;
  Dwarf_Die child;
  if (dwarf_child(die, &child) == 0)
    {
      do
	{
	  if (dwarf_tag(&child) != DW_TAG_enumerator)
	    continue;

	  string n, m;
	  location l;
	  die_loc_and_name(ctxt, &child, loc, n, m);
	  ssize_t val = 0;
	  die_signed_constant_attribute(&child, DW_AT_const_value, val);
	  enms.push_back(enum_type_decl::enumerator(n, val));
	}
      while (dwarf_siblingof(&child, &child) == 0);
    }

  // DWARF up to version 4 (at least) doesn't seem to carry the
  // underlying type, so let's create an artificial one here, which
  // sole purpose is to be passed to the constructor of the
  // enum_type_decl type.
  type_decl_sptr t(new type_decl(underlying_type_name,
				 size, size, location()));
  translation_unit_sptr tu = ctxt.cur_tu();
  decl_base_sptr d =
    add_decl_to_scope(t, tu->get_global_scope().get());

  t = dynamic_pointer_cast<type_decl>(d);
  assert(t);
  result.reset(new enum_type_decl(name, loc, t, enms, mangled_name));

  return result;
}

/// Build a an IR node for class type from a DW_TAG_structure_type or
/// DW_TAG_class_type and
///
/// @param ctxt the read context to consider.
///
/// @param die the DIE to read information from.  Must be either a
/// DW_TAG_structure_type or a DW_TAG_class_type.
///
/// @param is_struct wheter the class was declared as a struct.
///
/// @param scope a pointer to the scope_decl* under which this class
/// is to be added to.
///
/// @param klass if non-null, this is a klass to append the members
/// too.  Otherwise, this function just builds the class from scratch.
///
/// @param called_from_public_decl set to true if this class is being
/// called from a "Pubblic declaration like vars or public symbols".
///
/// @return the resulting class_type.
static decl_base_sptr
build_class_type_and_add_to_ir(read_context&	ctxt,
			       Dwarf_Die*	die,
			       scope_decl*	scope,
			       bool		is_struct,
			       class_decl_sptr  klass,
			       bool		called_from_public_decl)
{
  class_decl_sptr result;
  if (!die)
    return result;

  unsigned tag = dwarf_tag(die);

  if (tag != DW_TAG_class_type && tag != DW_TAG_structure_type)
    return result;

  {
    die_class_map_type::const_iterator i =
      ctxt.die_wip_classes_map().find(dwarf_dieoffset(die));
    if (i != ctxt.die_wip_classes_map().end())
      return i->second;
  }

  string name, mangled_name;
  location loc;
  die_loc_and_name(ctxt, die, loc, name, mangled_name);

  size_t size = 0;
  die_size_in_bits(die, size);

  Dwarf_Die child;
  bool has_child = (dwarf_child(die, &child) == 0);
  bool is_declaration_only = die_is_declaration_only(die);

  decl_base_sptr res;
  if (klass)
    {
      res = result = klass;
      result->set_size_in_bits(size);
      result->set_location(loc);
    }
  else
    {
      result.reset(new class_decl(name, size, 0, is_struct, loc,
				  decl_base::VISIBILITY_DEFAULT));

      if (is_declaration_only)
	result->set_is_declaration_only(true);

      res = add_decl_to_scope(result, scope);
      result = dynamic_pointer_cast<class_decl>(res);
      assert(result);
    }

  if (!has_child)
    // TODO: set the access specifier for the declaration-only class
    // here.
    return res;

  ctxt.die_wip_classes_map()[dwarf_dieoffset(die)] = res;

  scope_decl_sptr scop =
    dynamic_pointer_cast<scope_decl>(res);
  assert(scop);
  ctxt.scope_stack().push(scop.get());

  if (has_child)
    {
      do
	{
	  tag = dwarf_tag(&child);

	  // Handle base classes.
	  if (tag == DW_TAG_inheritance)
	    {
	      result->set_is_declaration_only(false);

	      Dwarf_Die type_die;
	      if (!die_die_attribute(&child, DW_AT_type, type_die))
		continue;

	      decl_base_sptr base_type =
		build_ir_node_from_die(ctxt, &type_die,
				       called_from_public_decl);
	      class_decl_sptr b = dynamic_pointer_cast<class_decl>(base_type);
	      if (!b)
		continue;
	      if (lookup_type_in_scope(base_type->get_name(), result))
		continue;

	      access_specifier access =
		is_struct
		? public_access
		: private_access;

	      die_access_specifier(&child, access);

	      bool is_virt= die_is_virtual(&child);
	      ssize_t offset = 0;
	      bool is_offset_present =
		die_member_offset(&child, offset);

	      class_decl::base_spec_sptr base(new class_decl::base_spec
					      (b, access,
					       is_offset_present
					       ? offset
					       : -1,
					       is_virt));
	      result->add_base_specifier(base);
	    }
	  // Handle data members.
	  else if (tag == DW_TAG_member
		   || tag == DW_TAG_variable)
	    {
	      result->set_is_declaration_only(false);

	      Dwarf_Die type_die;
	      if (!die_die_attribute(&child, DW_AT_type, type_die))
		continue;

	      decl_base_sptr ty =
		build_ir_node_from_die(ctxt, &type_die,
				       called_from_public_decl);
	      type_base_sptr t = is_type(ty);
	      if (!t)
		continue;

	      string n, m;
	      location loc;
	      die_loc_and_name(ctxt, &child, loc, n, m);
	      if (lookup_var_decl_in_scope(n, result))
		continue;

	      ssize_t offset_in_bits = 0;
	      bool is_laid_out = false;
	      is_laid_out = die_member_offset(&child, offset_in_bits);
	      offset_in_bits *= 8;

	      access_specifier access =
		is_struct
		? public_access
		: private_access;

	      die_access_specifier(&child, access);

	      var_decl_sptr dm(new var_decl(n, t, loc, m));
	      result->add_data_member(dm, access, is_laid_out,
				      // For now, is_static ==
				      // !is_laid_out.  When we have
				      // templates, we'll try to be
				      // more specific.  For now, this
				      // approximation should do OK.
				      /*is_static=*/!is_laid_out,
				      offset_in_bits);
	      assert(has_scope(dm));
	      ctxt.die_decl_map()[dwarf_dieoffset(&child)] = dm;
	    }
	  // Handle member functions;
	  else if (tag == DW_TAG_subprogram)
	    {
	      if (die_is_artificial(&child))
		// For now, let's not consider artificial functions.
		// To consider them, we'd need to make the IR now
		// about artificial functions and the
		// (de)serialization and comparison machineries to
		// know how to cope with these.
		continue;

	      function_decl_sptr f =
		build_function_decl(ctxt, &child,
				    function_decl_sptr());
	      if (!f)
		continue;
	      class_decl::method_decl_sptr m =
		dynamic_pointer_cast<class_decl::method_decl>(f);
	      assert(m);

	      bool is_ctor = (f->get_name() == result->get_name());
	      bool is_dtor = (f->get_name() == "~" + result->get_name());
	      bool is_virtual = die_is_virtual(&child);
	      size_t vindex = 0;
	      if (is_virtual)
		die_virtual_function_index(&child, vindex);
	      access_specifier access =
		is_struct
		? public_access
		: private_access;
	      die_access_specifier(&child, access);
	      bool is_static = false;
	      {
		Dwarf_Die this_ptr_type;
		if (ctxt.dwarf_version() > 2
		    && !die_die_attribute(&child,
					  DW_AT_object_pointer,
					  this_ptr_type))
		  is_static = true;
		else if (ctxt.dwarf_version() < 3)
		  {
		    is_static = true;
		    // For dwarf < 3, let's see if the first parameter
		    // has class type and has a DW_AT_artificial
		    // attribute flag set.
		    function_decl::parameter_sptr first_parm;
		    if (!f->get_parameters().empty())
		      first_parm = f->get_parameters()[0];

		    bool is_artificial =
		      first_parm && first_parm->get_artificial();;
		    pointer_type_def_sptr this_type;
		    if (is_artificial)
		      this_type =
			dynamic_pointer_cast<pointer_type_def>
			(first_parm->get_type());
		    if (this_type)
		      is_static = false;
		  }
	      }
	      result->add_member_function(m, access, is_virtual,
					  vindex, is_static, is_ctor,
					  is_dtor, /*is_const*/false);
	      assert(is_member_function(m));
	      ctxt.die_decl_map()[dwarf_dieoffset(&child)] = m;
	    }
	  // Handle member types
	  else if (is_type_die(&child))
	    {
	      decl_base_sptr td =
		build_ir_node_from_die(ctxt, &child, result.get(),
				       called_from_public_decl);
	      if (td)
		{
		  access_specifier access =
		    is_struct
		    ? public_access
		    : private_access;
		  die_access_specifier(&child, access);

		  set_member_access_specifier(td, access);
		  ctxt.die_decl_map()[dwarf_dieoffset(&child)] = td;
		}
	    }
	} while (dwarf_siblingof(&child, &child) == 0);
    }

  ctxt.scope_stack().pop();

  {
    die_class_map_type::const_iterator i =
      ctxt.die_wip_classes_map().find(dwarf_dieoffset(die));
    if (i != ctxt.die_wip_classes_map().end())
      {
	if (is_member_type(i->second))
	  set_member_access_specifier(res,
				      get_member_access_specifier(i->second));
	ctxt.die_wip_classes_map().erase(i);
      }
  }

  return res;
}

/// build a qualified type from a DW_TAG_const_type or
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
		     bool		called_from_public_decl)
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

  if (!die_is_public_decl(die))
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
/// @param called_for_public_decl this is set to true if the function
/// was called for a public (function) decl.
static function_decl_sptr
build_function_decl(read_context& ctxt,
		    Dwarf_Die* die,
		    function_decl_sptr fn)
{
  function_decl_sptr result = fn;
  if (!die)
    return result;
  assert(dwarf_tag(die) == DW_TAG_subprogram);

 if (!die_is_public_decl(die))
   return result;

  translation_unit_sptr tu = ctxt.cur_tu();
  assert(tu);

  string fname, fmangled_name;
  location floc;
  die_loc_and_name(ctxt, die, floc, fname, fmangled_name);

  size_t is_inline = false;
  die_unsigned_constant_attribute(die, DW_AT_inline, is_inline);

  decl_base_sptr return_type_decl;
  Dwarf_Die ret_type_die;
  if (die_die_attribute(die, DW_AT_type, ret_type_die))
    return_type_decl =
      build_ir_node_from_die(ctxt, &ret_type_die,
			     /*called_from_public_decl=*/true);

  class_decl_sptr is_method =
    dynamic_pointer_cast<class_decl>(get_scope_for_die(ctxt, die));

  Dwarf_Die child;
  function_decl::parameters function_parms;

  if (!result && dwarf_child(die, &child) == 0)
    do
      {
	int child_tag = dwarf_tag(&child);
	if (child_tag == DW_TAG_formal_parameter)
	  {
	    string name, mangled_name;
	    location loc;
	    die_loc_and_name(ctxt, &child, loc, name, mangled_name);
	    bool is_artificial = die_is_artificial(&child);
	    decl_base_sptr parm_type_decl;
	    Dwarf_Die parm_type_die;
	    if (die_die_attribute(&child, DW_AT_type, parm_type_die))
	      parm_type_decl =
		build_ir_node_from_die(ctxt, &parm_type_die,
				       /*called_from_public_decl=*/true);
	    if (!parm_type_decl)
	      continue;
	    function_decl::parameter_sptr p
	      (new function_decl::parameter(is_type(parm_type_decl), name, loc,
					    /*variadic_marker=*/false,
					    is_artificial));
	    function_parms.push_back(p);
	  }
	else if (child_tag == DW_TAG_unspecified_parameters)
	  {
	    bool is_artificial = die_is_artificial(&child);
	    function_decl::parameter_sptr p
	      (new function_decl::parameter(type_base_sptr(),
					    /*name=*/"",
					    location(),
					    /*variadic_marker=*/true,
					    is_artificial));
	    function_parms.push_back(p);
	  }
      }
  while (!result && dwarf_siblingof(&child, &child) == 0);

  if (result)
    {
      // Add the properties that might have been missing from the
      // first declaration of the function.  For now, it usually is
      // the mangled name that goes missing in the first declarations.
      if (!fmangled_name.empty())
	result->set_mangled_name(fmangled_name);
    }
  else
    {
      function_type_sptr fn_type(is_method
				 ? new method_type(is_type(return_type_decl),
						   is_method,
						   function_parms,
						   tu->get_address_size(),
						   tu->get_address_size())
				 : new function_type(is_type(return_type_decl),
						     function_parms,
						     tu->get_address_size(),
						     tu->get_address_size()));

      result.reset(is_method
		   ? new class_decl::method_decl(fname, fn_type,
						 is_inline, floc,
						 fmangled_name)
		   : new function_decl(fname, fn_type,
				       is_inline, floc,
				       fmangled_name));
    }
  return result;
}

/// Read all @ref abigail::translation_unit possible from the debug info
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

  // Walk all the DIEs of the debug info to build a DIE -> parent map
  // useful for get_die_parent() to work.
  build_die_parent_map(ctxt);

  // And now walk all the DIEs again to build the libabigail IR.
  Dwarf_Half dwarf_version = 0;
  for (Dwarf_Off offset = 0, next_offset = 0;
       (dwarf_next_unit(ctxt.dwarf(), offset, &next_offset, &header_size,
			&dwarf_version, NULL, &address_size, NULL,
			NULL, NULL) == 0);
       offset = next_offset)
    {
      Dwarf_Off die_offset = offset + header_size;
      Dwarf_Die cu;
      if (!dwarf_offdie(ctxt.dwarf(), die_offset, &cu))
	continue;

      ctxt.dwarf_version(dwarf_version);

      if (!ctxt.current_corpus())
	{
	  corpus_sptr corp (new corpus(ctxt.elf_path()));
	  ctxt.current_corpus(corp);
	}

      address_size *= 8;

      // Build a translation_unit IR node from cu; note that cu must
      // be a DW_TAG_compile_unit die.
      translation_unit_sptr ir_node =
	build_translation_unit_and_add_to_ir(ctxt, &cu, address_size);
      assert(ir_node);
    }
  return ctxt.current_corpus();
}

/// Build an IR node from a given DIE and add the node to the current
/// IR being build and held in the read_context.  Doing that is called
/// "emitting an IR node for the DIE".
///
/// @param ctxt the read context.
///
/// @param die the DIE to consider.
///
/// @param scope the scope under which the resulting IR node has to be
/// added.
///
/// @param called_from_public_decl set to yes if this function is
/// called from the functions used to build a public decl (functions
/// and variables).  In that case, this function accepts building IR
/// nodes representing types.  Otherwise, this function only creates
/// IR nodes representing public decls (functions and variables).
/// This is done to avoid emitting IR nodes for types that are not
/// referenced by public functions or variables.
///
/// @return the resulting IR node.
static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       scope_decl*	scope,
		       bool		called_from_public_decl)
{
  decl_base_sptr result;

  if (!die || !scope)
    return result;

  int tag = dwarf_tag(die);

  if (!called_from_public_decl)
    {
      if (tag != DW_TAG_subprogram
	  && tag != DW_TAG_variable
	  && tag != DW_TAG_namespace)
	return result;
    }

  die_decl_map_type::const_iterator it =
    ctxt.die_decl_map().find(dwarf_dieoffset(die));
  if (it != ctxt.die_decl_map().end())
    {
      result = it->second;
      return result;
    }

  switch (tag)
    {
      // Type DIEs we intent to support someday, maybe.
    case DW_TAG_base_type:
      if((result = build_type_decl(ctxt, die)))
	result =
	  add_decl_to_scope(result, scope);
      break;

    case DW_TAG_typedef:
      {
	typedef_decl_sptr t = build_typedef_type(ctxt, die,
						 called_from_public_decl);
	result = add_decl_to_scope(t, scope);
      }
      break;

    case DW_TAG_pointer_type:
      {
	pointer_type_def_sptr p =
	  build_pointer_type_def(ctxt, die, called_from_public_decl);
	if(p)
	  result = add_decl_to_scope(p, scope);
      }
      break;

    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
      {
	reference_type_def_sptr r =
	  build_reference_type(ctxt, die, called_from_public_decl);
	if (r)
	    result = add_decl_to_scope(r, scope);
      }
      break;

    case DW_TAG_const_type:
    case DW_TAG_volatile_type:
      {
	qualified_type_def_sptr q =
	  build_qualified_type(ctxt, die, called_from_public_decl);
	if (q)
	  result = add_decl_to_scope(q, scope);
      }
      break;

    case DW_TAG_enumeration_type:
      {
	enum_type_decl_sptr e = build_enum_type(ctxt, die);
	if (e)
	  result = add_decl_to_scope(e, scope);
      }
      break;

    case DW_TAG_class_type:
    case DW_TAG_structure_type:
      {
	Dwarf_Die spec_die;
	scope_decl_sptr scop;
	if (die_die_attribute(die, DW_AT_specification, spec_die))
	  {
	    scope_decl_sptr skope = get_scope_for_die(ctxt, &spec_die);
	    assert(skope);
	    decl_base_sptr cl = build_ir_node_from_die(ctxt, &spec_die,
						       skope.get(),
						       called_from_public_decl);
	    assert(cl);
	    class_decl_sptr klass = dynamic_pointer_cast<class_decl>(cl);
	    assert(klass);

	    result =
	      build_class_type_and_add_to_ir(ctxt, die,
					     skope.get(),
					     tag == DW_TAG_structure_type,
					     klass,
					     called_from_public_decl);
	  }
	else
	  result =
	    build_class_type_and_add_to_ir(ctxt, die,
					   scope,
					   tag == DW_TAG_structure_type,
					   class_decl_sptr(),
					   called_from_public_decl);
      }
      break;
    case DW_TAG_string_type:
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
      {
	Dwarf_Die spec_die;
	if (die_die_attribute(die, DW_AT_specification, spec_die))
	  {
	    scope_decl_sptr scop = get_scope_for_die(ctxt, &spec_die);
	    if (scop)
	      {
		decl_base_sptr d =
		  build_ir_node_from_die(ctxt, &spec_die, scop.get(),
					 called_from_public_decl);
		if (d)
		  {
		    var_decl_sptr m =
		      dynamic_pointer_cast<var_decl>(d);
		    if (is_data_member(m))
		      {
			set_member_is_static(m, true);
			ctxt.die_decl_map()[dwarf_dieoffset(die)] = d;
		      }
		    else
		      {
			result = add_decl_to_scope(m, scope);
			assert(has_scope(m));
			ctxt.var_decls_to_re_add_to_tree().push_back(m);
		      }
		    assert(d->get_scope());
		    return d;
		  }
	      }
	  }
	else if (var_decl_sptr v = build_var_decl(ctxt, die))
	  {
	    result = add_decl_to_scope(v, scope);
	    assert(result->get_scope());
	    v = dynamic_pointer_cast<var_decl>(result);
	    assert(v);
	    assert(v->get_scope());
	    ctxt.var_decls_to_re_add_to_tree().push_back(v);
	  }
      }
      break;

    case DW_TAG_subprogram:
      {
	  Dwarf_Die spec_die;
	  scope_decl_sptr scop;
	  if (!die_is_public_decl(die)
	      || die_is_artificial(die))
	    break;

	  function_decl_sptr fn;
	  if (die_die_attribute(die, DW_AT_specification, spec_die)
	      || die_die_attribute(die, DW_AT_abstract_origin, spec_die))
	    {
	      scop = get_scope_for_die(ctxt, &spec_die);
	      if (scop)
		{
		  decl_base_sptr d =
		    build_ir_node_from_die(ctxt,
					   &spec_die,
					   scop.get(),
					   called_from_public_decl);
		  if (d)
		    {
		      fn = dynamic_pointer_cast<function_decl>(d);
		      ctxt.die_decl_map()[dwarf_dieoffset(die)] = d;
		    }
		}
	    }
	  {
	    const class_decl* cl = dynamic_cast<class_decl*>(scope);
	    // we shouldn't be in this class b/c, if this DIE is for a
	    // member function, get_scope_for_die on it (prior to
	    // calling this function) should have built the member
	    // function for this DIE, and thus, this function should
	    // have found the DIE for the member function in its cache.
	    assert(!cl);
	  }
	ctxt.scope_stack().push(scope);

	if ((result = build_function_decl(ctxt, die, fn)))
	  result = add_decl_to_scope(result, scope);

	ctxt.scope_stack().pop();
      }
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
    ctxt.die_decl_map()[dwarf_dieoffset(die)] = result;

  return result;
}

/// Build an IR node from a given DIE and add the node to the current
/// IR being build and held in the read_context.  Doing that is called
/// "emitting an IR node for the DIE".
///
/// @param ctxt the read context.
///
/// @param die the DIE to consider.
///
/// @param called_from_public_decl set to yes if this function is
/// called from the functions used to build a public decl (functions
/// and variables).  In that case, this function accepts building IR
/// nodes representing types.  Otherwise, this function only creates
/// IR nodes representing public decls (functions and variables).
/// This is done to avoid emitting IR nodes for types that are not
/// referenced by public functions or variables.
///
/// @return the resulting IR node.
static decl_base_sptr
build_ir_node_from_die(read_context&	ctxt,
		       Dwarf_Die*	die,
		       bool		called_from_public_decl)
{
  if (!die)
    return decl_base_sptr();

  scope_decl_sptr scope = get_scope_for_die(ctxt, die);
  return build_ir_node_from_die(ctxt, die, scope.get(),
				called_from_public_decl);
}

/// Read all @ref abigail::translation_unit possible from the debug info
/// accessible from an elf file, stuff them into a libabigail ABI
/// Corpus and return it.
///
/// @param elf_path the path to the elf file.
///
/// @return a pointer to the resulting @ref abigail::corpus.
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
  corp->set_path(elf_path);
  corp->set_origin(corpus::DWARF_ORIGIN);

  return corp;
}

}// end namespace dwarf_reader

}// end namespace abigail
