// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the declarations for the @ref fe_iface a.k.a
/// "Front End Interface".

#ifndef __ABG_FE_IFACE_H__
#define __ABG_FE_IFACE_H__

#include "abg-ir.h"
#include "abg-suppression.h"

namespace abigail
{

/// The base class of all libabigail front-ends: The Front End Interface.
///
/// A front end reads a given type of binary format and constructs a
/// libagbigail internal representation from it.
///
/// The middle-end then manipulates that IR.
class fe_iface
{
protected:
  struct priv;
  priv* priv_;

 public:

  /// The status of the @ref fe_iface::read_corpus call.
  enum status
  {
    /// The status is in an unknown state
    STATUS_UNKNOWN = 0,

    /// This status is for when the call went OK.
    STATUS_OK = 1,

    /// This status is for when the debug info could not be read.
    STATUS_DEBUG_INFO_NOT_FOUND = 1 << 1,

    /// This status is for when the alternate debug info could not be
    /// found.
    STATUS_ALT_DEBUG_INFO_NOT_FOUND = 1 << 2,

    /// This status is for when the symbols of the ELF binaries could
    /// not be read.
    STATUS_NO_SYMBOLS_FOUND = 1 << 3,
  };

  /// The generic options that control the behaviour of all Front-End
  /// interfaces.
  struct options_type
  {
    environment&	env;
    bool		load_in_linux_kernel_mode	= false;
    bool		load_all_types			= false;
    bool		drop_undefined_syms		= false;
    bool		show_stats			= false;
    bool		do_log				= false;
    bool		leverage_dwarf_factorization	= true;
    bool		assume_odr_for_cplusplus	= true;
    options_type(environment&);

  };// font_end_iface::options_type

  fe_iface(const std::string& corpus_path, environment& e);

  ~fe_iface();

  void
  reset(const std::string& corpus_path, environment& e);

  const options_type&
  options() const;

  options_type&
  options();

  const std::string&
  corpus_path() const;

  void
  corpus_path(const std::string&);

  const string&
  dt_soname() const;

  void
  dt_soname(const string&);

  bool
  load_in_linux_kernel_mode() const;

  suppr::suppressions_type&
  suppressions();

  const suppr::suppressions_type&
  suppressions() const;

  void
  suppressions(suppr::suppressions_type&);

  void
  add_suppressions(const suppr::suppressions_type&);

  corpus_sptr
  corpus();

  const corpus_sptr
  corpus() const;

  corpus_group_sptr&
  corpus_group();

  const corpus_group_sptr&
  corpus_group() const;

  void
  corpus_group(const ir::corpus_group_sptr& cg);

  bool
  has_corpus_group() const;

  corpus_sptr
  main_corpus_from_current_group();

  bool
  current_corpus_is_main_corpus_from_current_group();

  corpus_sptr
  should_reuse_type_from_corpus_group();

  void
  maybe_add_fn_to_exported_decls(const function_decl* fn);

  void
  maybe_add_var_to_exported_decls(const var_decl* var);

  virtual ir::corpus_sptr
  read_corpus(status& status) = 0;
}; //end class fe_iface

typedef shared_ptr<fe_iface> fe_iface_sptr;

std::string
status_to_diagnostic_string(fe_iface::status s);

fe_iface::status
operator|(fe_iface::status, fe_iface::status);

fe_iface::status
operator&(fe_iface::status, fe_iface::status);

fe_iface::status&
operator|=(fe_iface::status&, fe_iface::status);

fe_iface::status&
operator&=(fe_iface::status&, fe_iface::status);

}// end namespace abigail
#endif // __ABG_FE_IFAC_H__
