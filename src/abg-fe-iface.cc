// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2022-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This file contains the definitions of the the fe_iface base type.

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-corpus.h"
#include "abg-fe-iface.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

/// The private data structure for the @ref fe_iface type.
struct fe_iface::priv
{
  std::string corpus_path;
  std::string dt_soname;
  fe_iface::options_type options;
  suppr::suppressions_type suppressions;
  ir::corpus_sptr corpus;
  ir::corpus_group_sptr corpus_group;

  priv(const std::string& path, environment& e)
    : corpus_path(path), options(e)
  {
    initialize();
  }

  /// This function resets the data of @ref fe_iface::priv data so
  /// that it can be re-used again.
  void
  initialize()
  {
    //TODO: initialize the options.
    corpus_path.clear();
    dt_soname.clear();
    suppressions.clear();
    corpus_group.reset();
  }
}; //end struct fe_iface::priv

/// Constructor of the type @ref fe_iface::options_type.
///
/// @param e the environment used by the Front End Interface.
fe_iface::options_type::options_type(environment& e)
  : env(e)
{
}

/// Constructor of the type @ref fe_iface.
///
/// @param corpus_path the path to the file represented by the ABI
/// corpus that is going to be built by this Front End.
///
/// @param e the environment in which the Front End operates.
fe_iface::fe_iface(const std::string& corpus_path, environment& e)
  : priv_(new priv(corpus_path, e))
{
}

/// Desctructor of the Front End Interface.
fe_iface::~fe_iface()
{delete priv_;}

/// Re-initialize the current Front End.
///
/// @param corpus_path the path to the file for which a new corpus is
/// to be created.
///
/// @param e the environment in which the Front End operates.
void
fe_iface::reset(const std::string& corpus_path,
		environment& e)
{
  delete priv_;
  priv_ = new fe_iface::priv(corpus_path, e);
}

/// Getter of the the options of the current Front End Interface.
///
/// @return the options of the current Front End Interface.
const fe_iface::options_type&
fe_iface::options() const
{return priv_->options;}

/// Getter of the the options of the current Front End Interface.
///
/// @return the options of the current Front End Interface.
fe_iface::options_type&
fe_iface::options()
{return priv_->options;}

/// Getter of the path to the file which an ABI corpus is to be
/// created for.
///
/// @return the path to the file which an ABI corpus is to be created
/// for.
const std::string&
fe_iface::corpus_path() const
{return priv_->corpus_path;}

/// Setter of the path to the file which an ABI corpus is to be
/// created for.
///
/// @param p the new path to the file which an ABI corpus is to be
/// created for.
void
fe_iface::corpus_path(const std::string& p)
{priv_->corpus_path = p;}

/// Getter for the SONAME of the analyzed binary.
///
/// @return the SONAME of the analyzed binary.
const string&
fe_iface::dt_soname() const
{return priv_->dt_soname;}

/// Getter for the SONAME of the analyzed binary.
///
/// @return the SONAME of the analyzed binary.
void
fe_iface::dt_soname(const string& soname)
{priv_->dt_soname = soname;}

/// Test if the input binary is to be considered as a Linux Kernel
/// binary.
///
/// @return true iff the input binary is to be considered as a Linux
/// Kernel binary.
bool
fe_iface::load_in_linux_kernel_mode() const
{return priv_->options.load_in_linux_kernel_mode;}

/// Getter of the vector of suppression specifications associated with
/// the current front-end.
///
/// @return the vector of suppression specifications associated with
/// the current front-end.
suppr::suppressions_type&
fe_iface::suppressions()
{return priv_->suppressions;}

/// Getter of the vector of suppression specifications associated with
/// the current front-end.
///
/// @return the vector of suppression specifications associated with
/// the current front-end.
const suppr::suppressions_type&
fe_iface::suppressions() const
{return priv_->suppressions;}

/// Setter of the vector of suppression specifications associated with
/// the current front-end.
///
/// @param supprs the new vector of suppression specifications
/// associated with the current front-end.
void
fe_iface::suppressions(suppr::suppressions_type& supprs)
{priv_->suppressions = supprs;}

/// Add suppressions specifications to the set of suppressions to be
/// used during the construction of the ABI internal representation
/// (the ABI corpus) from the input file.
///
/// During the construction of the ABI corpus, ABI artifacts that
/// match a given suppression specification are dropped on the floor;
/// that is, they are discarded and won't be part of the final ABI
/// corpus.  This is a way to reduce the amount of data held by the
/// final ABI corpus.
///
/// Note that the suppression specifications provided to this function
/// are only considered during the construction of the ABI corpus.
/// For instance, they are not taken into account during e.g
/// comparisons of two ABI corpora that might happen later.  If you
/// want to apply suppression specificatins to the comparison (or
/// reporting) of ABI corpora please refer to the documentation of the
/// @ref diff_context type to learn how to set suppressions that are
/// to be used in that context.
///
/// @param supprs the suppression specifications to be applied during
/// the construction of the ABI corpus.
void
fe_iface::add_suppressions(const suppr::suppressions_type& supprs)
{
  for (const auto& s : supprs)
    if (s->get_drops_artifact_from_ir())
      suppressions().push_back(s);
}

/// Getter for the ABI corpus being built by the current front-end.
///
/// @return the ABI corpus being built by the current front-end.
corpus_sptr
fe_iface::corpus()
{
  if (!priv_->corpus)
    {
      priv_->corpus = std::make_shared<ir::corpus>(options().env,
						   corpus_path());
    }
  return priv_->corpus;
}

/// Getter for the ABI corpus being built by the current front-end.
///
/// @return the ABI corpus being built by the current front-end.
const corpus_sptr
fe_iface::corpus() const
{return const_cast<fe_iface*>(this)->corpus();}

/// Getter for the ABI corpus group being built by the current front-end.
///
/// @return the ABI corpus group being built by the current front-end.
corpus_group_sptr&
fe_iface::corpus_group()
{return priv_->corpus_group;}

/// Getter for the ABI corpus group being built by the current front-end.
///
/// @return the ABI corpus group being built by the current front-end.
const corpus_group_sptr&
fe_iface::corpus_group() const
{return const_cast<fe_iface*>(this)->corpus_group();}

/// Setter for the ABI corpus group being built by the current
/// front-end.
///
/// @param cg the new ABI corpus group being built by the current
/// front-end.
void
fe_iface::corpus_group(const ir::corpus_group_sptr& cg)
{priv_->corpus_group = cg;}

/// Test if there is a corpus group being built.
///
/// @return if there is a corpus group being built, false otherwise.
bool
fe_iface::has_corpus_group() const
{return bool(corpus_group());}

/// Return the main corpus from the current corpus group, if any.
///
/// @return the main corpus of the current corpus group, if any, nil
/// if no corpus group is being constructed.
corpus_sptr
fe_iface::main_corpus_from_current_group()
{
  if (corpus_group())
    return corpus_group()->get_main_corpus();
  return corpus_sptr();
}

/// Test if the current corpus being built is the main corpus of the
/// current corpus group.
///
/// @return return true iff the current corpus being built is the
/// main corpus of the current corpus group.
bool
fe_iface::current_corpus_is_main_corpus_from_current_group()
{
  corpus_sptr main_corpus = main_corpus_from_current_group();

  if (main_corpus.get() == corpus().get())
    return true;

  return false;
}

/// Return true if the current corpus is part of a corpus group
/// being built and if it's not the main corpus of the group.
///
/// For instance, this would return true if we are loading a linux
/// kernel *module* that is part of the current corpus group that is
/// being built.  In this case, it means we should re-use types
/// coming from the "vmlinux" binary that is the main corpus of the
/// group.
///
/// @return the corpus group the current corpus belongs to, if the
/// current corpus is part of a corpus group being built. Nil otherwise.
corpus_sptr
fe_iface::should_reuse_type_from_corpus_group()
{
  if (has_corpus_group())
    if (corpus_sptr main_corpus = main_corpus_from_current_group())
	if (!current_corpus_is_main_corpus_from_current_group())
	  return corpus_group();

  return corpus_sptr();
}

/// Try and add the representation of the ABI of a function to the set
/// of exported declarations of the current corpus.
///
/// @param fn the internal representation of the ABI of a function.
void
fe_iface::maybe_add_fn_to_exported_decls(const function_decl* fn)
{
  if (fn)
    if (corpus::exported_decls_builder* b =
	corpus()->get_exported_decls_builder().get())
      b->maybe_add_fn_to_exported_fns(const_cast<function_decl*>(fn));
}

/// Try and add the representation of the ABI of a variable to the set
/// of exported declarations of the current corpus.
///
/// @param var the internal representation of the ABI of a variable.
void
fe_iface::maybe_add_var_to_exported_decls(const var_decl* var)
{
  if (var)
    if (corpus::exported_decls_builder* b =
	corpus()->get_exported_decls_builder().get())
      b->maybe_add_var_to_exported_vars(var);
}

/// The bitwise OR operator for the @ref fe_iface::status type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the operation.
fe_iface::status
operator|(fe_iface::status l, fe_iface::status r)
{
  return static_cast<fe_iface::status>(static_cast<unsigned>(l)
				       | static_cast<unsigned>(r));
}

/// The bitwise AND operator for the @ref fe_iface::status type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the operation.
fe_iface::status
operator&(fe_iface::status l, fe_iface::status r)
{
  return static_cast<fe_iface::status>(static_cast<unsigned>(l)
				       & static_cast<unsigned>(r));
}

/// The bitwise |= operator for the @ref fe_iface::status type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the operation.
fe_iface::status&
operator|=(fe_iface::status& l, fe_iface::status r)
{
  l = l | r;
  return l;
}

/// The bitwise &= operator for the @ref fe_iface::status type.
///
/// @param l the left-hand side operand.
///
/// @param r the right-hand side operand.
///
/// @return the result of the operation.
fe_iface::status&
operator&=(fe_iface::status& l, fe_iface::status r)
{
  l = l & r;
  return l;
}

/// Return a diagnostic status with english sentences to describe the
/// problems encoded in a given abigail::elf_reader::status, if
/// there is an error.
///
/// @param status the status to diagnose
///
/// @return a string containing sentences that describe the possible
/// errors encoded in @p s.  If there is no error to encode, then the
/// empty string is returned.
std::string
status_to_diagnostic_string(fe_iface::status s)
{
  std::string str;

  if (s & fe_iface::STATUS_DEBUG_INFO_NOT_FOUND)
    str += "could not find debug info\n";

  if (s & fe_iface::STATUS_ALT_DEBUG_INFO_NOT_FOUND)
    str += "could not find alternate debug info\n";

  if (s & fe_iface::STATUS_NO_SYMBOLS_FOUND)
    str += "could not load ELF symbols\n";

  return str;
}

}// namespace abigail
