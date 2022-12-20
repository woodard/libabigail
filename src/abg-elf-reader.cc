// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// Elf reader stuff

#include "abg-internal.h"

#include <fcntl.h> /* For open(3) */
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <libgen.h>
#include <fcntl.h>
#include <elfutils/libdwfl.h>


#include "abg-symtab-reader.h"
#include "abg-suppression-priv.h"
#include "abg-elf-helpers.h"

// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS
#include "abg-elf-reader.h"
#include "abg-tools-utils.h"
ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>
namespace abigail
{

using namespace elf_helpers;

namespace elf
{

/// Find the file name of the alternate debug info file.
///
/// @param elf_module the elf module to consider.
///
/// @param out parameter.  Is set to the file name of the alternate
/// debug info file, iff this function returns true.
///
/// @return true iff the location of the alternate debug info file was
/// found.
static bool
find_alt_dwarf_debug_info_link(Dwfl_Module *elf_module,
			       string &alt_file_name)
{
  GElf_Addr bias = 0;
  Dwarf *dwarf = dwfl_module_getdwarf(elf_module, &bias);
  Elf *elf = dwarf_getelf(dwarf);
  GElf_Ehdr ehmem, *elf_header;
  elf_header = gelf_getehdr(elf, &ehmem);

  Elf_Scn* section = 0;
  while ((section = elf_nextscn(elf, section)) != 0)
    {
      GElf_Shdr header_mem, *header;
      header = gelf_getshdr(section, &header_mem);
      if (header->sh_type != SHT_PROGBITS)
	continue;

      const char *section_name = elf_strptr(elf,
					    elf_header->e_shstrndx,
					    header->sh_name);

      char *alt_name = 0;
      char *buildid = 0;
      size_t buildid_len = 0;
      if (section_name != 0
	  && strcmp(section_name, ".gnu_debugaltlink") == 0)
	{
	  Elf_Data *data = elf_getdata(section, 0);
	  if (data != 0 && data->d_size != 0)
	    {
	      alt_name = (char*) data->d_buf;
	      char *end_of_alt_name =
		(char *) memchr(alt_name, '\0', data->d_size);
	      buildid_len = data->d_size - (end_of_alt_name - alt_name + 1);
	      if (buildid_len == 0)
		return false;
	      buildid = end_of_alt_name + 1;
	    }
	}
      else
	continue;

      if (buildid == 0 || alt_name == 0)
	return false;

      alt_file_name = alt_name;
      return true;
    }

  return false;
}

/// Find alternate debuginfo file of a given "link" under a set of
/// root directories.
///
/// The link is a string that is read by the function
/// find_alt_dwarf_debug_info_link().  That link is a path that is relative
/// to a given debug info file, e.g, "../../../.dwz/something.debug".
/// It designates the alternate debug info file associated to a given
/// debug info file.
///
/// This function will thus try to find the .dwz/something.debug file
/// under some given root directories.
///
/// @param root_dirs the set of root directories to look from.
///
/// @param alt_file_name a relative path to the alternate debug info
/// file to look for.
///
/// @param alt_file_path the resulting absolute path to the alternate
/// debuginfo path denoted by @p alt_file_name and found under one of
/// the directories in @p root_dirs.  This is set iff the function
/// returns true.
///
/// @return true iff the function found the alternate debuginfo file.
static bool
find_alt_dwarf_debug_info_path(const vector<char**> root_dirs,
			       const string &alt_file_name,
			       string &alt_file_path)
{
  if (alt_file_name.empty())
    return false;

  string altfile_name = tools_utils::trim_leading_string(alt_file_name, "../");

  for (vector<char**>::const_iterator i = root_dirs.begin();
       i != root_dirs.end();
       ++i)
    if (tools_utils::find_file_under_dir(**i, altfile_name, alt_file_path))
      return true;

  return false;
}

/// Return the alternate debug info associated to a given main debug
/// info file.
///
/// @param elf_module the elf module to consider.
///
/// @param debug_root_dirs a set of root debuginfo directories under
/// which too look for the alternate debuginfo file.
///
/// @param alt_file_name output parameter.  This is set to the file
/// path of the alternate debug info file associated to @p elf_module.
/// This is set iff the function returns a non-null result.
///
/// @param alt_fd the file descriptor used to access the alternate
/// debug info.  If this parameter is set by the function, then the
/// caller needs to fclose it, otherwise the file descriptor is going
/// to be leaked.  Note however that on recent versions of elfutils
/// where libdw.h contains the function dwarf_getalt(), this parameter
/// is set to 0, so it doesn't need to be fclosed.
///
/// Note that the alternate debug info file is a DWARF extension as of
/// DWARF 4 ans is decribed at
/// http://www.dwarfstd.org/ShowIssue.php?issue=120604.1.
///
/// @return the alternate debuginfo, or null.  If @p alt_fd is
/// non-zero, then the caller of this function needs to call
/// dwarf_end() on the returned alternate debuginfo pointer,
/// otherwise, it's going to be leaked.
static Dwarf*
find_alt_dwarf_debug_info(Dwfl_Module *elf_module,
			  const vector<char**> debug_root_dirs,
			  string& alt_file_name,
			  int& alt_fd)
{
  if (elf_module == 0)
    return 0;

  Dwarf* result = 0;
  find_alt_dwarf_debug_info_link(elf_module, alt_file_name);

#ifdef LIBDW_HAS_DWARF_GETALT
  // We are on recent versions of elfutils where the function
  // dwarf_getalt exists, so let's use it.
  Dwarf_Addr bias = 0;
  Dwarf* dwarf = dwfl_module_getdwarf(elf_module, &bias);
  result = dwarf_getalt(dwarf);
  alt_fd = 0;
#else
  // We are on an old version of elfutils where the function
  // dwarf_getalt doesn't exist yet, so let's open code its
  // functionality
  char *alt_name = 0;
  const char *file_name = 0;
  void **user_data = 0;
  Dwarf_Addr low_addr = 0;
  char *alt_file = 0;

  file_name = dwfl_module_info(elf_module, &user_data,
			       &low_addr, 0, 0, 0, 0, 0);

  alt_fd = dwfl_standard_find_debuginfo(elf_module, user_data,
					file_name, low_addr,
					alt_name, file_name,
					0, &alt_file);

  result = dwarf_begin(alt_fd, DWARF_C_READ);
#endif

  if (result == 0)
    {
      // So we didn't find the alternate debuginfo file from the
      // information that is in the debuginfo file associated to
      // elf_module.  Maybe the alternate debuginfo file is located
      // under one of the directories in debug_root_dirs.  So let's
      // look in there.
      string alt_file_path;
      if (!find_alt_dwarf_debug_info_path(debug_root_dirs,
					  alt_file_name,
					  alt_file_path))
	return result;

      // If we reach this point it means we have found the path to the
      // alternate debuginfo file and it's in alt_file_path.  So let's
      // open it and read it.
      alt_fd = open(alt_file_path.c_str(), O_RDONLY);
      if (alt_fd == -1)
	return result;
      result = dwarf_begin(alt_fd, DWARF_C_READ);

#ifdef LIBDW_HAS_DWARF_GETALT
      Dwarf_Addr bias = 0;
      Dwarf* dwarf = dwfl_module_getdwarf(elf_module, &bias);
      dwarf_setalt(dwarf, result);
#endif
    }

  return result;
}

/// Private data of the @ref elf::reader type.
struct reader::priv
{
  reader&				rdr;
  Elf*					elf_handle		= nullptr;
  Elf_Scn*				symtab_section		= nullptr;
  string				elf_architecture;
  vector<string>			dt_needed;
  // An abstraction of the symbol table.  This is loaded lazily, on
  // demand.
  mutable symtab_reader::symtab_sptr	symt;
  // Where split debug info is to be searched for on disk.
  vector<char**>			debug_info_root_paths;
  // Some very useful callback functions that elfutils needs to
  // perform various tasks.
  Dwfl_Callbacks			offline_callbacks;
  // A pointer to the DWARF Front End Library handle of elfutils.
  // This is useful to perform all kind of things at a higher level.
  dwfl_sptr				dwfl_handle;
  // The address range of the offline elf file we are looking at.
  Dwfl_Module*				elf_module		= nullptr;
  // A pointer to the DWARF debug info, if found by locate_dwarf_debug_info.
  Dwarf*				dwarf_handle		= nullptr;
  // A pointer to the ALT DWARF debug info, which is the debug info
  // that is constructed by the DWZ tool.  It's made of all the type
  // information that was redundant in the DWARF.  DWZ put it there
  // and make the DWARF reference it in here.
  Dwarf*				alt_dwarf_handle	= nullptr;
  string				alt_dwarf_path;
  int					alt_dwarf_fd		= 0;
  Elf_Scn*				ctf_section		= nullptr;
  Elf_Scn*				alt_ctf_section	= nullptr;

  priv(reader& reeder, const std::string& elf_path,
       const vector<char**>& debug_info_roots)
    : rdr(reeder)
  {
    rdr.corpus_path(elf_path);
    initialize(debug_info_roots);
  }

  ~priv()
  {
    clear_alt_dwarf_debug_info_data();
  }

  /// Reset the private data of @elf elf::reader.
  ///
  /// @param debug_info_roots the vector of new directories where to
  /// look for split debug info file.
  void
  initialize(const vector<char**>& debug_info_roots)
  {
    clear_alt_dwarf_debug_info_data();

    elf_handle = nullptr;
    symtab_section = nullptr;
    elf_architecture.clear();
    dt_needed.clear();
    symt.reset();
    debug_info_root_paths = debug_info_roots;
    offline_callbacks = {};
    dwfl_handle.reset();
    elf_module = nullptr;
    dwarf_handle = nullptr;
    alt_dwarf_handle = nullptr;
    alt_dwarf_path.clear();
    alt_dwarf_fd = 0;
    ctf_section = nullptr;
    alt_ctf_section = nullptr;
  }

  /// Setup the necessary plumbing to open the ELF file and find all
  /// the associated split debug info files.
  ///
  /// This function also setup the various handles on the opened ELF
  /// file and whatnot.
  void
  crack_open_elf_file()
  {
    // Initialize the callback functions used by elfutils.
    elf_helpers::initialize_dwfl_callbacks(offline_callbacks,
					   debug_info_root_paths.empty()
					   ? nullptr
					   : debug_info_root_paths.front());

    // Create a handle to the DWARF Front End Library that we'll need.
    dwfl_handle = elf_helpers::create_new_dwfl_handle(offline_callbacks);

    const string& elf_path = rdr.corpus_path();
    // Get the set of addresses that make up the ELF file we are
    // looking at.
    elf_module =
      dwfl_report_offline(dwfl_handle.get(),
			  basename(const_cast<char*>(elf_path.c_str())),
			  elf_path.c_str(), -1);
    dwfl_report_end(dwfl_handle.get(), 0, 0);
    ABG_ASSERT(elf_module);

    // Finally, get and handle at the representation of the ELF file
    // we've just cracked open.
    GElf_Addr bias = 0;
    elf_handle = dwfl_module_getelf(elf_module, &bias);
    ABG_ASSERT(elf_handle);
  }

  /// Find the alternate debuginfo file associated to a given elf file.
  ///
  /// @param elf_module represents the elf file to consider.
  ///
  /// @param alt_file_name the resulting path to the alternate
  /// debuginfo file found.  This is set iff the function returns a
  /// non-nil value.
  Dwarf*
  find_alt_dwarf_debug_info(Dwfl_Module*	elf_module,
			    string&		alt_file_name,
			    int&		alt_fd)
  {
    Dwarf *result = 0;
    result = elf::find_alt_dwarf_debug_info(elf_module,
					    debug_info_root_paths,
					    alt_file_name, alt_fd);
    return result;
  }

  /// Clear the resources related to the alternate DWARF data.
  void
  clear_alt_dwarf_debug_info_data()
  {
    if (alt_dwarf_fd)
      {
        if (alt_dwarf_handle)
          {
            dwarf_end(alt_dwarf_handle);
            alt_dwarf_handle = nullptr;
          }
        close(alt_dwarf_fd);
        alt_dwarf_fd = 0;
      }
    alt_dwarf_path.clear();
  }

  /// Locate the DWARF debug info in the ELF file.
  ///
  /// This also knows how to locate split debug info.
  void
  locate_dwarf_debug_info()
  {
    ABG_ASSERT(dwfl_handle);

    if (dwarf_handle)
      return;

    // First let's see if the ELF file that was cracked open does have
    // some DWARF debug info embedded.
    Dwarf_Addr bias = 0;
    dwarf_handle = dwfl_module_getdwarf(elf_module, &bias);

    // If no debug info was found in the binary itself, then look for
    // split debuginfo files under multiple possible debuginfo roots.
    for (vector<char**>::const_iterator i = debug_info_root_paths.begin();
	 dwarf_handle == 0 && i != debug_info_root_paths.end();
	 ++i)
      {
	offline_callbacks.debuginfo_path = *i;
	dwarf_handle = dwfl_module_getdwarf(elf_module, &bias);
      }

    alt_dwarf_handle = find_alt_dwarf_debug_info(elf_module,
						 alt_dwarf_path,
						 alt_dwarf_fd);
  }

  /// Locate the CTF "alternate" debug information associated with the
  /// current ELF file ( and split out somewhere else).
  ///
  /// This is a sub-routine of @ref locate_ctf_debug_info().
  void
  locate_alt_ctf_debug_info()
  {
    if (alt_ctf_section)
      return;

    Elf_Scn *section =
      elf_helpers::find_section(elf_handle,
				".gnu_debuglink",
				SHT_PROGBITS);

    std::string name;
    Elf_Data *data;
    if (section
	&& (data = elf_getdata(section, nullptr))
	&& data->d_size != 0)
      name = (char *) data->d_buf;

    if (!name.empty())
      for (const auto& path : rdr.debug_info_root_paths())
	{
	  std::string file_path;
	  if (!tools_utils::find_file_under_dir(*path, name, file_path))
	    continue;

	  int fd;
	  if ((fd = open(file_path.c_str(), O_RDONLY)) == -1)
	    continue;

	  Elf *hdl;
	  if ((hdl = elf_begin(fd, ELF_C_READ, nullptr)) == nullptr)
	    {
	      close(fd);
	      continue;
	    }

	  // unlikely .ctf was designed to be present in stripped file
	  alt_ctf_section =
	    elf_helpers::find_section(hdl, ".ctf", SHT_PROGBITS);

	  elf_end(hdl);
	  close(fd);

	  if (alt_ctf_section)
	    break;
	}
  }

  /// Locate the CTF debug information associated with the current ELF
  /// file.  It also locates the CTF debug information that is split
  /// out in a separate file.
  void
  locate_ctf_debug_info()
  {
    ABG_ASSERT(elf_handle);

    ctf_section = elf_helpers::find_section_by_name(elf_handle, ".ctf");
    if (ctf_section == nullptr)
      {
	locate_alt_ctf_debug_info();
	ctf_section = alt_ctf_section;
      }
  }
}; //end reader::priv

/// The constructor of the @ref elf::reader type.
///
/// @param elf_path the path to the ELF file to read from.
///
/// @param debug_info_root a vector of directory paths to look into
/// for split debug information files.
///
/// @param env the environment which the reader operates in.
reader::reader(const string&		elf_path,
	       const vector<char**>&	debug_info_roots,
	       ir::environment&	env)
  : fe_iface(elf_path, env),
    priv_(new priv(*this, elf_path, debug_info_roots))
{
  priv_->crack_open_elf_file();
  priv_->locate_dwarf_debug_info();
  priv_->locate_ctf_debug_info();
}

/// The destructor of the @ref elf::reader type.
reader::~reader()
{delete priv_;}

/// Resets (erase) the resources used by the current @ref
/// elf::reader type.
///
/// This lets the reader in a state where it's ready to read from
/// another ELF file.
///
/// @param elf_path the new ELF path to read from.
///
/// @param debug_info_roots a vector of directory paths to look into
/// for split debug information files.
void
reader::reset(const std::string&	elf_path,
	      const vector<char**>&	debug_info_roots)
{
  fe_iface::options_type opts = options();
  fe_iface::reset(elf_path, opts.env);
  corpus_path(elf_path);
  priv_->initialize(debug_info_roots);
  priv_->crack_open_elf_file();
  priv_->locate_dwarf_debug_info();
  priv_->locate_ctf_debug_info();
}

/// Getter of the vector of directory paths to look into for split
/// debug information files.
///
/// @return the vector of directory paths to look into for split
/// debug information files.
const vector<char**>&
reader::debug_info_root_paths() const
{return priv_->debug_info_root_paths;}

/// Getter of the functions used by the DWARF Front End library of
/// elfutils to locate DWARF debug information.
///
/// @return the functions used by the DWARF Front End library of
const Dwfl_Callbacks&
reader::dwfl_offline_callbacks() const
{return priv_->offline_callbacks;}

/// Getter of the functions used by the DWARF Front End library of
/// elfutils to locate DWARF debug information.
///
/// @return the functions used by the DWARF Front End library of
Dwfl_Callbacks&
reader::dwfl_offline_callbacks()
{return priv_->offline_callbacks;}

/// Getter of the handle used to access ELF information from the
/// current ELF file.
///
/// @return the handle used to access ELF information from the current
/// ELF file.
Elf*
reader::elf_handle() const
{return priv_->elf_handle;}

/// Getter of the handle used to access DWARF information from the
/// current ELF file.
///
/// @return the handle used to access DWARF information from the
/// current ELF file.
const Dwarf*
reader::dwarf_debug_info() const
{return priv_->dwarf_handle;}

/// Test if the binary has DWARF debug info.
///
/// @return true iff the binary has DWARF debug info.
bool
reader::has_dwarf_debug_info() const
{return ((priv_->dwarf_handle != nullptr)
	  || (priv_->alt_dwarf_handle != nullptr));}

/// Test if the binary has CTF debug info.
///
/// @return true iff the binary has CTF debug info.
bool
reader::has_ctf_debug_info() const
{return (priv_->ctf_section != nullptr);}

/// Getter of the handle use to access DWARF information from the
/// alternate split DWARF information.
///
/// In other words, this accesses the factorized DWARF information
/// that has been constructed by the DWZ tool to de-duplicate DWARF
/// information on disk.
///
/// @return the handle use to access DWARF information from the
/// alternate split DWARF information.
const Dwarf*
reader::alternate_dwarf_debug_info() const
{return priv_->alt_dwarf_handle;}


/// Getter of the path to the alternate split DWARF information file,
/// on disk.  In othe words, this returns the path to the factorized
/// DWARF information used by the current ELF file, created by the
/// 'DWZ' tool.
///
/// @return the path to the alternate split DWARF information file,
/// on disk.
const string&
reader::alternate_dwarf_debug_info_path() const
{return priv_->alt_dwarf_path;}

/// Check if the underlying elf file refers to an alternate debug info
/// file associated to it.
///
/// Note that "alternate debug info sections" is a GNU extension as
/// of DWARF4 and is described at
/// http://www.dwarfstd.org/ShowIssue.php?issue=120604.1.
///
/// @param alt_di the path to the alternate debug info file.  This is
/// set iff the function returns true.
///
/// @return true if the ELF file refers to an alternate debug info
/// file.
bool
reader::refers_to_alt_debug_info(string& alt_di_path) const
{
  if (!alternate_dwarf_debug_info_path().empty())
    {
      alt_di_path = alternate_dwarf_debug_info_path();
      return true;
    }
  return false;
}

/// Find and return a pointer to the ELF symbol table
/// section.
///
/// @return a pointer to the ELF symbol table section.
const Elf_Scn*
reader::find_symbol_table_section() const
{
  if (!priv_->symtab_section)
      priv_->symtab_section =
	elf_helpers::find_symbol_table_section(elf_handle());
    return priv_->symtab_section;
}

/// Clear the pointer to the ELF symbol table section.
void
reader::reset_symbol_table_section()
{priv_->symtab_section = nullptr;}

/// Find and return a pointer to the the CTF section.
///
/// @return a pointer to the the CTF section.
const Elf_Scn*
reader::find_ctf_section() const
{
  if (priv_->ctf_section == nullptr)
    priv_->locate_ctf_debug_info();

  if (priv_->ctf_section)
    return priv_->ctf_section;

  return priv_->alt_ctf_section;
}

/// Find and return a pointer to the alternate CTF section of the
/// current ELF file.
///
/// @return a pointer to the alternate CTF section of the current ELF
/// file.
const Elf_Scn*
reader::find_alternate_ctf_section() const
{
  if (priv_->alt_ctf_section == nullptr)
    priv_->locate_alt_ctf_debug_info();

  return priv_->alt_ctf_section;
}

/// Get the value of the DT_NEEDED property of the current ELF file.
///
/// @return the value of the DT_NEEDED property.
const vector<string>&
reader::dt_needed()const
{return priv_->dt_needed;}


/// Get the value of the 'ARCHITECTURE' property of the current ELF file.
///
/// @return the value of the 'ARCHITECTURE' property of the current
/// ELF file.
const string&
reader::elf_architecture() const
{return priv_->elf_architecture;}

/// Getter of an abstract representation of the symbol table of the
/// underlying ELF file.
///
/// Note that the symbol table is loaded lazily, upon the first
/// invocation of this member function.
///
/// @returnt the symbol table.
symtab_reader::symtab_sptr&
reader::symtab() const
{
  ABG_ASSERT(elf_handle());

  if (!priv_->symt)
    priv_->symt = symtab_reader::symtab::load
      (elf_handle(), options().env,
       [&](const elf_symbol_sptr& symbol)
       {return suppr::is_elf_symbol_suppressed(*this, symbol);});

  if (!priv_->symt)
    std::cerr << "Symbol table of '" << corpus_path()
	      << "' could not be loaded\n";
  return priv_->symt;
}

/// Test if a given function symbol has been exported.
///
/// @param symbol_address the address of the symbol we are looking
/// for.  Note that this address must be a relative offset from the
/// beginning of the .text section, just like the kind of addresses
/// that are present in the .symtab section.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
reader::function_symbol_is_exported(GElf_Addr symbol_address) const
{
  elf_symbol_sptr symbol = symtab()->lookup_symbol(symbol_address);
  if (!symbol)
    return symbol;

  if (!symbol->is_function() || !symbol->is_public())
    return elf_symbol_sptr();

  address_set_sptr set;
  bool looking_at_linux_kernel_binary =
    load_in_linux_kernel_mode() && elf_helpers::is_linux_kernel(elf_handle());

  if (looking_at_linux_kernel_binary)
    {
	if (symbol->is_in_ksymtab())
	  return symbol;
	return elf_symbol_sptr();
    }

  return symbol;
}

/// Test if a given variable symbol has been exported.
///
/// @param symbol_address the address of the symbol we are looking
/// for.  Note that this address must be a relative offset from the
/// beginning of the .text section, just like the kind of addresses
/// that are present in the .symtab section.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
reader::variable_symbol_is_exported(GElf_Addr symbol_address) const
{
  elf_symbol_sptr symbol = symtab()->lookup_symbol(symbol_address);
  if (!symbol)
    return symbol;

  if (!symbol->is_variable() || !symbol->is_public())
    return elf_symbol_sptr();

  address_set_sptr set;
  bool looking_at_linux_kernel_binary =
    load_in_linux_kernel_mode() && elf_helpers::is_linux_kernel(elf_handle());

  if (looking_at_linux_kernel_binary)
    {
	if (symbol->is_in_ksymtab())
	  return symbol;
	return elf_symbol_sptr();
    }

  return symbol;
}

/// Test if a given function symbol has been exported.
///
/// @param name the name of the symbol we are looking for.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
reader::function_symbol_is_exported(const string& name) const
{
  const elf_symbols& syms = symtab()->lookup_symbol(name);
  for (auto s : syms)
    {
      if (s->is_function() && s->is_public())
	{
	  bool looking_at_linux_kernel_binary =
	    (load_in_linux_kernel_mode()
	     && elf_helpers::is_linux_kernel(elf_handle()));

	  if (looking_at_linux_kernel_binary)
	    {
	      if (s->is_in_ksymtab())
		return s;
	    }
	  else
	    return s;
	}
    }
  return elf_symbol_sptr();
}

/// Test if a given variable symbol has been exported.
///
/// @param name the name of the symbol we are looking
/// for.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
reader::variable_symbol_is_exported(const string& name) const
{
  const elf_symbols& syms = symtab()->lookup_symbol(name);
  for (auto s : syms)
    {
      if (s->is_variable() && s->is_public())
	{
	  bool looking_at_linux_kernel_binary =
	    (load_in_linux_kernel_mode()
	     && elf_helpers::is_linux_kernel(elf_handle()));

	  if (looking_at_linux_kernel_binary)
	    {
	      if (s->is_in_ksymtab())
		return s;
	    }
	  else
	    return s;
	}
    }
  return elf_symbol_sptr();
}
/// Load the DT_NEEDED and DT_SONAME elf TAGS.
void
reader::load_dt_soname_and_needed()
{
  elf_helpers::lookup_data_tag_from_dynamic_segment(elf_handle(),
						    DT_NEEDED,
						    priv_->dt_needed);

  vector<string> dt_tag_data;
  elf_helpers::lookup_data_tag_from_dynamic_segment(elf_handle(),
						    DT_SONAME,
						    dt_tag_data);
  if (!dt_tag_data.empty())
    dt_soname(dt_tag_data[0]);
}

/// Read the string representing the architecture of the current ELF
/// file.
void
reader::load_elf_architecture()
{
  if (!elf_handle())
    return;

  GElf_Ehdr eh_mem;
  GElf_Ehdr* elf_header = gelf_getehdr(elf_handle(), &eh_mem);

  priv_->elf_architecture =
    elf_helpers::e_machine_to_string(elf_header->e_machine);
}

/// Load various ELF data.
///
/// This function loads ELF data that are not symbol maps or debug
/// info.  That is, things like various tags, elf architecture and
/// so on.
void
reader::load_elf_properties()
{
  // Note that we don't load the symbol table as it's loaded lazily,
  // on demand.

  load_dt_soname_and_needed();
  load_elf_architecture();
}

/// Read the ELF information associated to the current ELF file and
/// construct an ABI representation from it.
///
/// Note that this reader doesn't know how to interpret any debug
/// information so the resulting ABI corpus won't have any type
/// information.  Rather, it will only have ELF symbol representation.
///
/// To have type information, consider using readers that know how to
/// interpret the symbolic type information comprised in DWARF, CTF or
/// other symbolic debug information format, like the @ref or
/// abigail::dwarf_reader::reader, @ref abigail::ctf_reader::reader
/// readers.
///
/// @return the resulting ABI corpus.
ir::corpus_sptr
reader::read_corpus(status& status)
{
  status = STATUS_UNKNOWN;

  corpus::origin origin = corpus()->get_origin();
  origin |= corpus::ELF_ORIGIN;
  if (is_linux_kernel(elf_handle()))
    origin |= corpus::LINUX_KERNEL_BINARY_ORIGIN;
  corpus()->set_origin(origin);

  load_elf_properties(); // DT_SONAME, DT_NEEDED, architecture
  corpus()->set_soname(dt_soname());
  corpus()->set_needed(dt_needed());
  corpus()->set_architecture_name(elf_architecture());

  // See if we could find symbol tables.
  if (!symtab() || !symtab()->has_symbols())
    {
      status |= STATUS_NO_SYMBOLS_FOUND;
      // We found no ELF symbol, so we can't handle the binary.
      return corpus_sptr();
    }

  // Set symbols information to the corpus.
  corpus()->set_symtab(symtab());

  // If we couldn't load debug info from the elf path, then say it.
  if ((origin & abigail::ir::corpus::DWARF_ORIGIN)
        && !has_dwarf_debug_info())
    status |= STATUS_DEBUG_INFO_NOT_FOUND;
  else if ((origin & abigail::ir::corpus::CTF_ORIGIN)
             && !has_ctf_debug_info())
    status |= STATUS_DEBUG_INFO_NOT_FOUND;

  status |= STATUS_OK;
  return corpus();
}

/// Get the SONAME property of a designated ELF file.
///
/// @param path the path to the ELF file to consider.
///
/// @param soname output parameter.  This is set to the SONAME of the
/// file located at @p path, iff this function return true.
///
/// @return true iff the SONAME property was found in the ELF file
/// located at @p path and set into the argument of the parameter @p
/// soname.
bool
get_soname_of_elf_file(const string& path, string &soname)
{return elf_helpers::get_soname_of_elf_file(path, soname);}

/// Convert the type of ELF file into @ref elf_type.
///
/// @param elf the elf handle to use for the query.
///
/// @return the @ref elf_type for a given elf type.
static elf::elf_type
elf_file_type(Elf* elf)
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *header = gelf_getehdr (elf, &ehdr_mem);
  vector<string> dt_debug_data;

  switch (header->e_type)
    {
    case ET_DYN:
      if (lookup_data_tag_from_dynamic_segment(elf, DT_DEBUG, dt_debug_data))
	return elf::ELF_TYPE_PI_EXEC;
      else
	return elf::ELF_TYPE_DSO;
    case ET_EXEC:
      return elf::ELF_TYPE_EXEC;
    case ET_REL:
      return elf::ELF_TYPE_RELOCATABLE;
    default:
      return elf::ELF_TYPE_UNKNOWN;
    }
}

/// Get the type of a given elf type.
///
/// @param path the absolute path to the ELF file to analyzed.
///
/// @param type the kind of the ELF file designated by @p path.
///
/// @param out parameter.  Is set to the type of ELF file of @p path.
/// This parameter is set iff the function returns true.
///
/// @return true iff the file could be opened and analyzed.
bool
get_type_of_elf_file(const string& path, elf::elf_type& type)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1)
    return false;

  elf_version (EV_CURRENT);
  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  type = elf_file_type(elf);
  elf_end(elf);
  close(fd);

  return true;
}

}// end namespace elf
} // end namespace abigail
