// -*- Mode: C++ -*-
//
// Copyright (C) 2016 Red Hat, Inc.
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
/// This contains the implementation of the suppression engine of
/// libabigail.

#include "abg-suppression.h"
#include "abg-ini.h"
#include "abg-sptr-utils.h"
#include "abg-tools-utils.h"

namespace abigail
{

namespace suppr
{

using std::tr1::dynamic_pointer_cast;

// <suppression_base stuff>

/// The private data of @ref suppression_base.
class suppression_base::priv
{
  string				label_;
  string				file_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	file_name_regex_;
  string				file_name_not_regex_str_;
  mutable sptr_utils::regex_t_sptr	file_name_not_regex_;
  string				soname_regex_str_;
  mutable sptr_utils::regex_t_sptr	soname_regex_;
  string				soname_not_regex_str_;
  mutable sptr_utils::regex_t_sptr	soname_not_regex_;

public:
  priv()
  {}

  priv(const string& label)
    : label_(label)
  {}

  priv(const string& label,
       const string& file_name_regex_str,
       const string& file_name_not_regex_str)
    : label_(label),
      file_name_regex_str_(file_name_regex_str),
      file_name_not_regex_str_(file_name_not_regex_str)
  {}

  friend class suppression_base;

  /// Get the regular expression object associated to the 'file_name_regex'
  /// property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'file_name_regex' property of @ref suppression_base is
  /// empty then this method returns nil.
  const sptr_utils::regex_t_sptr
  get_file_name_regex() const
  {
    if (!file_name_regex_)
      {
	if (!file_name_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			file_name_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      file_name_regex_ = r;
	  }
      }
    return file_name_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'file_name_not_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'file_name_not_regex' property of @ref suppression_base
  /// is empty then this method returns nil.
  const sptr_utils::regex_t_sptr
  get_file_name_not_regex() const
  {
    if (!file_name_not_regex_)
      {
	if (!file_name_not_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			file_name_not_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      file_name_not_regex_ = r;
	  }
      }
    return file_name_not_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'soname_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'soname_regex' property of @ref suppression_base is empty
  /// then this method returns nil.
  const sptr_utils::regex_t_sptr
  get_soname_regex() const
  {
    if (!soname_regex_)
      {
	if (!soname_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			soname_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      soname_regex_ = r;
	  }
      }
    return soname_regex_;
  }

  /// Get the regular expression object associated to the
  /// 'soname_not_regex' property of @ref suppression_base.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'soname_not_regex' property of @ref suppression_base is
  /// empty then this method returns nil.
  const sptr_utils::regex_t_sptr
  get_soname_not_regex() const
  {
    if (!soname_not_regex_)
      {
	if (!soname_not_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			soname_not_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      soname_not_regex_ = r;
	  }
      }
    return soname_not_regex_;
  }

}; // end clas suppression_base::priv

/// Constructor for @ref suppression_base
///
/// @param a label for the suppression.  This represents just a
/// comment.
suppression_base::suppression_base(const string& label)
  : priv_(new priv(label))
{}

suppression_base::suppression_base(const string& label,
				   const string& file_name_regex_str,
				   const string& file_name_not_regex_str)
  : priv_(new priv(label,
		   file_name_regex_str,
		   file_name_not_regex_str))
{
}


/// Getter for the label associated to this suppression specification.
///
/// @return the label.
const string
suppression_base::get_label() const
{return priv_->label_;}

/// Setter for the label associated to this suppression specification.
///
/// @param label the new label.
void
suppression_base::set_label(const string& label)
{priv_->label_ = label;}

/// Setter for the "file_name_regex" property of the current instance
/// of @ref suppression_base.
///
/// The "file_name_regex" property is a regular expression string that
/// designates the file name that contains the ABI artifact this
/// suppression should apply to.
///
/// @param regexp the new regular expression string.
void
suppression_base::set_file_name_regex_str(const string& regexp)
{priv_->file_name_regex_str_ = regexp;}

/// Getter for the "file_name_regex" property of the current instance
/// of @ref suppression_base.
///
/// The "file_name_regex" property is a regular expression string that
/// designates the file name that contains the ABI artifacts this
/// suppression should apply to.
///
/// @return the regular expression string.
const string&
suppression_base::get_file_name_regex_str() const
{return priv_->file_name_regex_str_;}

/// Setter for the "file_name_not_regex" property of the current
/// instance of @ref suppression_base.
///
/// The current suppression specification should apply to ABI
/// artifacts of a file which name does *NOT* match the regular
/// expression string designated by the "file_name_not_regex"
/// property.
///
/// @param regexp the new regular expression string.
void
suppression_base::set_file_name_not_regex_str(const string& regexp)
{priv_->file_name_not_regex_str_ = regexp;}

/// Getter for the "file_name_not_regex" property of the current
/// instance of @ref suppression_base.
///
/// The current suppression specification should apply to ABI
/// artifacts of a file which name does *NOT* match the regular
/// expression string designated by the "file_name_not_regex"
/// property.
///
/// @return the regular expression string.
const string&
suppression_base::get_file_name_not_regex_str() const
{return priv_->file_name_not_regex_str_;}

/// Setter of the "soname_regex_str property of the current instance
/// of @ref suppression_base.
///
/// The "soname_regex_str" is a regular expression string that
/// designates the soname of the shared library that contains the ABI
/// artifacts this suppression should apply to.
///
/// @param regexp the new regular expression string.
void
suppression_base::set_soname_regex_str(const string& regexp)
{priv_->soname_regex_str_ = regexp;}

/// Getter of the "soname_regex_str property of the current instance
/// of @ref suppression_base.
///
/// The "soname_regex_str" is a regular expression string that
/// designates the soname of the shared library that contains the ABI
/// artifacts this suppression should apply to.
///
/// @return the regular expression string.
const string&
suppression_base::get_soname_regex_str() const
{return priv_->soname_regex_str_;}

/// Setter of the "soname_not_regex_str property of the current
/// instance of @ref suppression_base.
///
/// The current suppression specification should apply to ABI
/// artifacts of a shared library which SONAME does *NOT* match the
/// regular expression string designated by the "soname_not_regex"
/// property.
///
/// @param regexp the new regular expression string.
void
suppression_base::set_soname_not_regex_str(const string& regexp)
{priv_->soname_not_regex_str_ = regexp;}

/// Getter of the "soname_not_regex_str property of the current
/// instance of @ref suppression_base.
///
/// The current suppression specification should apply to ABI
/// artifacts of a shared library which SONAME does *NOT* match the
/// regular expression string designated by the "soname_not_regex"
/// property.
///
/// @return the regular expression string.
const string&
suppression_base::get_soname_not_regex_str() const
{return priv_->soname_not_regex_str_;}

/// Check if the names of the two binaries being compared match the
/// content of the properties "file_name_regexp" and
/// "file_name_not_regexp".
///
/// @param ctxt the context of the comparison.
///
/// @return false if the regular expression contained in the property
/// file_name_regexp or in the property "file_name_not_regexp" does
/// *NOT* match at least one of the names of the two binaries being
/// compared.  Return true otherwise.
bool
suppression_base::names_of_binaries_match(const diff_context& ctxt) const
{
  // Check if the names of the binaries match
  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_file_name_regex())
    {
      string first_binary_path = ctxt.get_first_corpus()->get_path(),
	second_binary_path = ctxt.get_second_corpus()->get_path();

      string first_binary_name, second_binary_name;

      tools_utils::base_name(first_binary_path, first_binary_name);
      tools_utils::base_name(second_binary_path, second_binary_name);

      if ((regexec(regexp.get(), first_binary_name.c_str(),
		   0, NULL, 0) != 0)
	  && (regexec(regexp.get(), second_binary_name.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_file_name_not_regex())
    {
      string first_binary_path = ctxt.get_first_corpus()->get_path(),
	second_binary_path = ctxt.get_second_corpus()->get_path();

      string first_binary_name, second_binary_name;

      tools_utils::base_name(first_binary_path, first_binary_name);
      tools_utils::base_name(second_binary_path, second_binary_name);

      if ((regexec(regexp.get(), first_binary_name.c_str(),
		   0, NULL, 0) == 0)
	  || (regexec(regexp.get(), second_binary_name.c_str(),
		      0, NULL, 0) == 0))
	return false;
    }

  return true;
}

/// Check if the SONAMEs of the two binaries being compared match the
/// content of the properties "soname_regexp" and "soname_not_regexp".
///
/// @param ctxt the context of the comparison.
///
/// @return false if the regular expression contained in the property
/// soname_regexp or in the property "soname_not_regexp" does *NOT*
/// match at least one of the SONAMEs of the two binaries being
/// compared.  Return true otherwise.
bool
suppression_base::sonames_of_binaries_match(const diff_context& ctxt) const
{
  // Check if the sonames of the binaries match
  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_soname_regex())
    {
      string first_soname = ctxt.get_first_corpus()->get_soname(),
	second_soname = ctxt.get_second_corpus()->get_soname();

      if ((regexec(regexp.get(), first_soname.c_str(),
		   0, NULL, 0) != 0)
	  && (regexec(regexp.get(), second_soname.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_soname_not_regex())
    {
      string first_soname = ctxt.get_first_corpus()->get_soname(),
	second_soname = ctxt.get_second_corpus()->get_soname();

      if ((regexec(regexp.get(), first_soname.c_str(),
		   0, NULL, 0) == 0)
	  || (regexec(regexp.get(), second_soname.c_str(),
		      0, NULL, 0) == 0))
	return false;
    }

  return true;
}

suppression_base::~suppression_base()
{}

static type_suppression_sptr
read_type_suppression(const ini::config::section& section);

static function_suppression_sptr
read_function_suppression(const ini::config::section& section);

static variable_suppression_sptr
read_variable_suppression(const ini::config::section& section);

static file_suppression_sptr
read_file_suppression(const ini::config::section& section);

/// Read a vector of suppression specifications from the sections of
/// an ini::config.
///
/// Note that this function needs to be updated each time a new kind
/// of suppression specification is added.
///
/// @param config the config to read from.
///
/// @param suppressions out parameter.  The vector of suppressions to
/// append the newly read suppressions to.
static void
read_suppressions(const ini::config& config,
		  suppressions_type& suppressions)
{
  suppression_sptr s;
  for (ini::config::sections_type::const_iterator i =
	 config.get_sections().begin();
       i != config.get_sections().end();
       ++i)
    if ((s = read_type_suppression(**i))
	|| (s = read_function_suppression(**i))
	|| (s = read_variable_suppression(**i))
	|| (s = read_file_suppression(**i)))
      suppressions.push_back(s);

}

/// Read suppressions specifications from an input stream.
///
/// @param input the input stream to read from.
///
/// @param suppressions the vector of suppressions to append the newly
/// read suppressions to.
void
read_suppressions(std::istream& input,
		  suppressions_type& suppressions)
{
    if (ini::config_sptr config = ini::read_config(input))
    read_suppressions(*config, suppressions);
}

/// Read suppressions specifications from an input file on disk.
///
/// @param input the path to the input file to read from.
///
/// @param suppressions the vector of suppressions to append the newly
/// read suppressions to.
void
read_suppressions(const string& file_path,
		  suppressions_type& suppressions)
{
  if (ini::config_sptr config = ini::read_config(file_path))
    read_suppressions(*config, suppressions);
}
// </suppression_base stuff>

// <type_suppression stuff>

/// The private data for @ref type_suppression.
class type_suppression::priv
{
  string				type_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	type_name_regex_;
  string				type_name_;
  bool					consider_type_kind_;
  type_suppression::type_kind		type_kind_;
  bool					consider_reach_kind_;
  type_suppression::reach_kind		reach_kind_;
  type_suppression::insertion_ranges	insertion_ranges_;
  vector<string>			source_locations_to_keep_;
  string				source_location_to_keep_regex_str_;
  mutable sptr_utils::regex_t_sptr	source_location_to_keep_regex_;

  priv();

public:
  priv(const string&			type_name_regexp,
       const string&			type_name,
       bool				consider_type_kind,
       type_suppression::type_kind	type_kind,
       bool				consider_reach_kind,
       type_suppression::reach_kind	reach_kind)
    : type_name_regex_str_(type_name_regexp),
      type_name_(type_name),
      consider_type_kind_(consider_type_kind),
      type_kind_(type_kind),
      consider_reach_kind_(consider_reach_kind),
      reach_kind_(reach_kind)
  {}

  /// Get the regular expression object associated to the 'type_name_regex'
  /// property of @ref type_suppression.
  ///
  /// If the regular expression object is not created, this method
  /// creates it and returns it.
  ///
  /// If the 'type_name_regex' property of @ref type_suppression is
  /// empty then this method returns nil.
  const sptr_utils::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_)
      {
	if (!type_name_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			type_name_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      type_name_regex_ = r;
	  }
      }
    return type_name_regex_;
  }

  /// Setter for the type_name_regex object.
  ///
  /// @param r the new type_name_regex object.
  void
  set_type_name_regex(sptr_utils::regex_t_sptr r)
  {type_name_regex_ = r;}

  /// Getter for the source_location_to_keep_regex object.
  ///
  /// This function builds the regex if it's not yet built.
  const sptr_utils::regex_t_sptr
  get_source_location_to_keep_regex() const
  {
    if (!source_location_to_keep_regex_)
      {
	if (!source_location_to_keep_regex_str_.empty())
	  {
	    sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	    if (regcomp(r.get(),
			source_location_to_keep_regex_str_.c_str(),
			REG_EXTENDED) == 0)
	      source_location_to_keep_regex_ = r;
	  }
      }
    return source_location_to_keep_regex_;
  }

  /// Setter for the source_location_to_keep_regex object.
  ///
  /// @param r the new regex object.
  void
  set_source_location_to_keep_regex(sptr_utils::regex_t_sptr r)
  {source_location_to_keep_regex_ = r;}

  friend class type_suppression;
}; // class type_suppression::priv

/// Constructor for @ref type_suppression.
///
/// @param label the label of the suppression.  This is just a free
/// form comment explaining what the suppression is about.
///
/// @param type_name_regexp the regular expression describing the
/// types about which diff reports should be suppressed.  If it's an
/// empty string, the parameter is ignored.
///
/// @param type_name the name of the type about which diff reports
/// should be suppressed.  If it's an empty string, the parameter is
/// ignored.
///
/// Note that parameter @p type_name_regexp and @p type_name_regexp
/// should not necessarily be populated.  It usually is either one or
/// the other that the user wants.
type_suppression::type_suppression(const string& label,
				   const string& type_name_regexp,
				   const string& type_name)
  : suppression_base(label),
    priv_(new priv(type_name_regexp,
		   type_name,
		   /*consider_type_kind=*/false,
		   /*type_kind=*/CLASS_TYPE_KIND,
		   /*consider_reach_kind=*/false,
		   /*reach_kind=*/DIRECT_REACH_KIND))
{}

type_suppression::~type_suppression()
{}

/// Setter for the "type_name_regex" property of the type suppression
/// specification.
///
/// This sets a regular expression that specifies the family of types
/// about which diff reports should be suppressed.
///
/// @param name_regex_str the new regular expression to set.
void
type_suppression::set_type_name_regex_str(const string& name_regex_str)
{priv_->type_name_regex_str_ = name_regex_str;}

/// Getter for the "type_name_regex" property of the type suppression
/// specification.
///
/// This returns a regular expression that specifies the family of
/// types about which diff reports should be suppressed.
///
/// @return the regular expression.
const string&
type_suppression::get_type_name_regex_str() const
{return priv_->type_name_regex_str_;}

/// Setter for the name of the type about which diff reports should be
/// suppressed.
///
/// @param name the new type name.
void
type_suppression::set_type_name(const string& name)
{priv_->type_name_ = name;}

/// Getter for the name of the type about which diff reports should be
/// suppressed.
///
/// @param return the type name.
const string&
type_suppression::get_type_name() const
{return priv_->type_name_;}

/// Getter of the property that says whether to consider the kind of
/// type this suppression is about.
///
/// @return the boolean value of the property.
bool
type_suppression::get_consider_type_kind() const
{return priv_->consider_type_kind_;}

/// Setter of the property that says whether to consider the kind of
/// type this suppression is about.
///
/// @param f the new boolean value of the property.
void
type_suppression::set_consider_type_kind(bool f)
{priv_->consider_type_kind_ = f;}


/// Setter of the kind of type this suppression is about.
///
/// Note that this will be considered during evaluation of the
/// suppression only if type_suppression::get_consider_type_kind()
/// returns true.
///
/// @param k the new kind of type this suppression is about.
void
type_suppression::set_type_kind(type_kind k)
{priv_->type_kind_ = k;}

/// Getter of the kind of type this suppression is about.
///
/// Note that this will be considered during evaluation of the
/// suppression only if type_suppression::get_consider_type_kind()
/// returns true.
///
/// @return the kind of type this suppression is about.
type_suppression::type_kind
type_suppression::get_type_kind() const
{return priv_->type_kind_;}

/// Test if the current type suppression specification
/// suggests to consider how the matching diff node is reached.
///
/// @return true if the current type suppression specification
/// suggests to consider how the matching diff node is reached.
bool
type_suppression::get_consider_reach_kind() const
{return priv_->consider_reach_kind_;}

/// Set a flag saying if the current type suppression specification
/// suggests to consider how the matching diff node is reached.
///
/// @param f the new value of the flag.  It's true iff the current
/// type suppression specification suggests to consider how the
/// matching diff node is reached.
void
type_suppression::set_consider_reach_kind(bool f)
{priv_->consider_reach_kind_ = f;}

/// Getter of the way the diff node matching the current suppression
/// specification is to be reached.
///
/// @return the way the diff node matching the current suppression
/// specification is to be reached.
type_suppression::reach_kind
type_suppression::get_reach_kind() const
{return priv_->reach_kind_;}

/// Setter of the way the diff node matching the current suppression
/// specification is to be reached.
///
/// @param p the way the diff node matching the current suppression
/// specification is to be reached.
void
type_suppression::set_reach_kind(reach_kind k)
{priv_->reach_kind_ = k;}

/// Setter for the vector of data member insertion ranges that
/// specifies where a data member is inserted as far as this
/// suppression specification is concerned.
///
/// @param r the new insertion range vector.
void
type_suppression::set_data_member_insertion_ranges(const insertion_ranges& r)
{priv_->insertion_ranges_ = r;}

/// Getter for the vector of data member insertion range that
/// specifiers where a data member is inserted as far as this
/// suppression specification is concerned.
///
/// @return the vector of insertion ranges.
const type_suppression::insertion_ranges&
type_suppression::get_data_member_insertion_ranges() const
{return priv_->insertion_ranges_;}

/// Getter for the vector of data member insertion range that
/// specifiers where a data member is inserted as far as this
/// suppression specification is concerned.
///
/// @return the vector of insertion ranges.
type_suppression::insertion_ranges&
type_suppression::get_data_member_insertion_ranges()
{return priv_->insertion_ranges_;}

/// Getter for the array of source location paths of types that should
/// *NOT* be suppressed.
///
/// @return the array of source locations of types that should *NOT*
/// be supressed.
const vector<string>&
type_suppression::get_source_locations_to_keep() const
{return priv_->source_locations_to_keep_;}

/// Setter for the array of source location paths of types that should
/// *NOT* be suppressed.
///
/// @param l the new array.
void
type_suppression::set_source_locations_to_keep(const vector<string>& l)
{priv_->source_locations_to_keep_ = l;}

/// Getter of the regular expression string that designates the source
/// location paths of types that should not be suppressed.
///
/// @return the regular expression string.
const string&
type_suppression::get_source_location_to_keep_regex_str() const
{return priv_->source_location_to_keep_regex_str_;}

/// Setter of the regular expression string that designates the source
/// location paths of types that should not be suppressed.
///
/// @param r the new regular expression.
void
type_suppression::set_source_location_to_keep_regex_str(const string& r)
{priv_->source_location_to_keep_regex_str_ = r;}

/// Evaluate this suppression specification on a given diff node and
/// say if the diff node should be suppressed or not.
///
/// @param diff the diff node to evaluate this suppression
/// specification against.
///
/// @return true if @p diff should be suppressed.
bool
type_suppression::suppresses_diff(const diff* diff) const
{
  const type_diff_base* d = is_type_diff(diff);
  if (!d)
    return false;

  // If the suppression should consider the way the diff node has been
  // reached, then do it now.
  if (get_consider_reach_kind())
    {
      if (get_reach_kind() == POINTER_REACH_KIND)
	{
	  if (const pointer_diff* ptr_diff = is_pointer_diff(diff))
	    {
	      d = is_type_diff(ptr_diff->underlying_type_diff().get());
	      if (!d)
		// This might be of, e.g, distinct_diff type.
		return false;
	    }
	  else
	    return false;
	}
      else if (get_reach_kind() == REFERENCE_REACH_KIND)
	{
	  if (const reference_diff* ref_diff = is_reference_diff(diff))
	    {
	      d = is_type_diff(ref_diff->underlying_type_diff().get());
	      if (!d)
		// This might be of, e.g, distinct_diff type.
		return false;
	    }
	  else
	    return false;
	}
      else if (get_reach_kind() == REFERENCE_OR_POINTER_REACH_KIND)
	{
	  if (const pointer_diff* ptr_diff = is_pointer_diff(diff))
	    {
	      d = is_type_diff(ptr_diff->underlying_type_diff().get());
	      assert(d);
	    }
	  else if (const reference_diff* ref_diff = is_reference_diff(diff))
	    {
	      d = is_type_diff(ref_diff->underlying_type_diff().get());
	      assert(d);
	    }
	  else
	    return false;
	}
    }

  type_base_sptr ft, st;
  ft = is_type(d->first_subject());
  st = is_type(d->second_subject());
  assert(ft && st);

  if (!suppresses_type(ft, d->context())
      && !suppresses_type(st, d->context()))
    {
      ft = peel_typedef_type(ft);
      st = peel_typedef_type(st);

      if (!suppresses_type(ft, d->context())
	  && !suppresses_type(st, d->context()))
	return false;

      d = is_type_diff(get_typedef_diff_underlying_type_diff(d));
    }

  const class_diff* klass_diff = dynamic_cast<const class_diff*>(d);
  if (// We are looking at a class diff ...
      klass_diff
      // ... that has inserted data members ...
      && !get_data_member_insertion_ranges().empty()
      // ... that has no deleted data members ...
      && klass_diff->deleted_data_members().empty()
      // ... and in which the class size hasn't shrunk (because, e.g,
      // the base classes have changed).
      && (klass_diff->first_class_decl()->get_size_in_bits()
	  <= klass_diff->second_class_decl()->get_size_in_bits()))
    {
      for (string_decl_base_sptr_map::const_iterator m =
	     klass_diff->inserted_data_members().begin();
	   m != klass_diff->inserted_data_members().end();
	   ++m)
	{
	  decl_base_sptr member = m->second;
	  size_t dm_offset = get_data_member_offset(member);
	  size_t first_type_size =
	    klass_diff->first_class_decl()->get_size_in_bits();
	  size_t second_type_size =
	    klass_diff->second_class_decl()->get_size_in_bits();
	  bool matched = false;

	  for (insertion_ranges::const_iterator i =
		 get_data_member_insertion_ranges().begin();
	       i != get_data_member_insertion_ranges().end();
	       ++i)
	    {
	      type_suppression::insertion_range_sptr range = *i;
	      ssize_t range_begin_val = 0,range_end_val = 0;
	      if (!type_suppression::insertion_range::eval_boundary
		  (range->begin(),
		   klass_diff->first_class_decl(),
		   range_begin_val))
		break;
	      if (!type_suppression::insertion_range::eval_boundary
		  (range->end(),
		   klass_diff->first_class_decl(),
		   range_end_val))
		break;


	      unsigned range_begin =
		(range_begin_val < 0) ? first_type_size : range_begin_val;

	      unsigned range_end =
		(range_end_val < 0) ? second_type_size : range_end_val;

	      if (range_begin > range_end)
		continue;

	      if (range_begin_val < 0 || range_end_val < 0)
		{
		  if (dm_offset < range_begin)
		    continue;
		}
	      else
		if (dm_offset < range_begin || dm_offset > range_end)
		  continue;

	      matched = true;
	    }
	  if (!matched)
	    return false;
	}
    }

  return true;
}

/// Test if the current instance of @ref type_suppression suppresses a
/// change reports about a given type.
///
/// @param type the type to consider.
///
/// @param ctxt the context of comparison we are involved with.
///
/// @return true iff the suppression specification suppresses type @p
/// type.
bool
type_suppression::suppresses_type(const type_base_sptr& type,
				  const diff_context_sptr& ctxt) const
{
  if (ctxt)
    {
      // Check if the names of the binaries match
      if (!names_of_binaries_match(*ctxt))
	return false;

      // Check if the sonames of the binaries match
      if (!sonames_of_binaries_match(*ctxt))
	return false;
    }

  // If the suppression should consider type kind then, well, check
  // for that.
  if (get_consider_type_kind())
    {
      type_kind tk = get_type_kind();
      bool matches = true;
      switch (tk)
	{
	case type_suppression::UNKNOWN_TYPE_KIND:
	case type_suppression::CLASS_TYPE_KIND:
	  if (!is_class_type(type))
	    matches = false;
	  break;
	case type_suppression::STRUCT_TYPE_KIND:
	  {
	    class_decl_sptr klass = is_class_type(type);
	    if (!klass || (klass && !klass->is_struct()))
	      matches = false;
	  }
	  break;
	case type_suppression::UNION_TYPE_KIND:
	  // We do not support unions yet.  When we do, we should
	  // replace the abort here by a "break;" statement.
	  abort();
	case type_suppression::ENUM_TYPE_KIND:
	  if (!is_enum_type(type))
	    matches = false;
	  break;
	case type_suppression::ARRAY_TYPE_KIND:
	  if (!is_array_type(type))
	    matches = false;
	  break;
	case type_suppression::TYPEDEF_TYPE_KIND:
	  if (!is_typedef(type))
	    matches = false;
	  break;
	case type_suppression::BUILTIN_TYPE_KIND:
	  if (!is_type_decl(type))
	    matches = false;
	  break;
	}

      if (!matches)
	return false;
    }

  // Check if there is a source location related match.
  if (decl_base_sptr d = get_type_declaration(type))
    {
      location loc = d->get_location();
      if (loc)
	{
	  translation_unit* tu = get_translation_unit(d);
	  if (tu)
	    {
	      string loc_path, loc_path_base;
	      unsigned loc_line = 0, loc_column = 0;
	      tu->get_loc_mgr().expand_location(loc, loc_path,
						loc_line, loc_column);

	      if (sptr_utils::regex_t_sptr regexp =
		  priv_->get_source_location_to_keep_regex())
		if (regexec(regexp.get(), loc_path.c_str(), 0, NULL, 0) == 0)
		  return false;

	      tools_utils::base_name(loc_path, loc_path_base);
	      for (vector<string>::const_iterator s =
		     get_source_locations_to_keep().begin();
		   s != get_source_locations_to_keep().end();
		   ++s)
		{
		  if (tools_utils::string_ends_with(*s, loc_path)
		      || tools_utils::string_ends_with(*s, loc_path_base))
		    return false;
		}
	    }
	  else
	    {
	      if (!get_source_locations_to_keep().empty()
		  || priv_->get_source_location_to_keep_regex())
		// The user provided a "source_location_not_regexp" or
		// a "source_location_not_in" property that was not
		// triggered.  This means the current type suppression
		// doesn't suppress the type given.
		return false;
	    }
	}
      else
	{
	  if (!get_source_locations_to_keep().empty()
	      || priv_->get_source_location_to_keep_regex())
	    // The user provided a "source_location_not_regexp" or
	    // a "source_location_not_in" property that was not
	    // triggered.  This means the current type suppression
	    // doesn't suppress the type given.
	    return false;
	}
    }
  else
    {
      if (!get_source_locations_to_keep().empty()
	  || priv_->get_source_location_to_keep_regex())
	// The user provided a "source_location_not_regexp" or
	// a "source_location_not_in" property that was not
	// triggered.  This means the current type suppression
	// doesn't suppress the type given.
	return false;
    }

  if (!get_type_name().empty() || priv_->get_type_name_regex())
  {
    string name = get_name(type);

    // Check if there is an exact type name match.
    if (!get_type_name().empty())
      {
	if (get_type_name() != name)
	  return false;
      }
    else
      {
	// So now check if there is a regular expression match.
	//
	// If the qualified name of the considered type doesn't match
	// the regular expression of the type name, then this
	// suppression doesn't apply.
	const sptr_utils::regex_t_sptr& type_name_regex =
	  priv_->get_type_name_regex();
	if (type_name_regex && (regexec(type_name_regex.get(),
					name.c_str(),
					0, NULL, 0) != 0))
	  return false;
      }
  }
  return true;
}

/// The private data of type_suppression::insertion_range
struct type_suppression::insertion_range::priv
{
  boundary_sptr begin_;
  boundary_sptr end_;

  priv()
  {}

  priv(boundary_sptr begin, boundary_sptr end)
    : begin_(begin), end_(end)
  {}
}; // end struct type_suppression::insertion_range::priv

/// Default Constructor of @ref type_suppression::insertion_range.
type_suppression::insertion_range::insertion_range()
  : priv_(new priv)
{}

/// Constructor of @ref type_suppression::insertion_range.
///
/// @param begin the start of the range.  A range boundary that is an
/// instance of @ref interger_boundary with a negative value means the
/// maximum possible value.
///
/// @param end the end of the range.  A range boundary that is an
/// instance of @ref interger_boundary with a negative value means the
/// maximum possible value.
type_suppression::insertion_range::insertion_range(boundary_sptr begin,
						   boundary_sptr end)
  : priv_(new priv(begin, end))
{}

/// Getter for the beginning of the range.
///
/// @return the beginning of the range.  A range boundary that is an
/// instance of @ref interger_boundary with a negative value means the
/// maximum possible value.
type_suppression::insertion_range::boundary_sptr
type_suppression::insertion_range::begin() const
{return priv_->begin_;}

/// Getter for the end of the range.
///
/// @return the end of the range.  A range boundary that is an
/// instance of @ref interger_boundary with a negative value means the
/// maximum possible value.
type_suppression::insertion_range::boundary_sptr
type_suppression::insertion_range::end() const
{return priv_->end_;}

/// Create an integer boundary.
///
/// The return value of this function is to be used as a boundary for
/// an instance of @ref type_suppression::insertion_range.  That
/// boundary evaluates to an integer value.
///
/// @param value the value of the integer boundary.
///
/// @return the resulting integer boundary.
type_suppression::insertion_range::integer_boundary_sptr
type_suppression::insertion_range::create_integer_boundary(int value)
{return integer_boundary_sptr(new integer_boundary(value));}


/// Create a function call expression boundary.
///
/// The return value of this function is to be used as a boundary for
/// an instance of @ref type_suppression::insertion_range.  The value
/// of that boundary is actually a function call expression that
/// itself evalutates to an integer value, in the context of a @ref
/// class_decl.
///
/// @param expr the function call expression to create the boundary from.
///
/// @return the resulting function call expression boundary.
type_suppression::insertion_range::fn_call_expr_boundary_sptr
type_suppression::insertion_range::create_fn_call_expr_boundary(ini::function_call_expr_sptr expr)
{return fn_call_expr_boundary_sptr(new fn_call_expr_boundary(expr));}

/// Create a function call expression boundary.
///
/// The return value of this function is to be used as a boundary for
/// an instance of @ref type_suppression::insertion_range.  The value
/// of that boundary is actually a function call expression that
/// itself evalutates to an integer value, in the context of a @ref
/// class_decl.
///
/// @param s a string representing the expression the function call
/// expression to create the boundary from.
///
/// @return the resulting function call expression boundary.
type_suppression::insertion_range::fn_call_expr_boundary_sptr
type_suppression::insertion_range::create_fn_call_expr_boundary(const string& s)
{
  fn_call_expr_boundary_sptr result, nil;
  ini::function_call_expr_sptr expr;
  if (ini::read_function_call_expr(s, expr) && expr)
    result.reset(new fn_call_expr_boundary(expr));
  return result;
}

/// Evaluate an insertion range boundary to get a resulting integer
/// value.
///
/// @param boundary the boundary to evaluate.
///
/// @param context the context of evualuation.  It's a @ref class_decl
/// to take into account during the evaluation, if there is a need for
/// it.
///
/// @return true iff the evaluation was successful and @p value
/// contains the resulting value.
bool
type_suppression::insertion_range::eval_boundary(boundary_sptr	 boundary,
						 class_decl_sptr context,
						 ssize_t&	 value)
{
  if (integer_boundary_sptr b = is_integer_boundary(boundary))
    {
      value = b->as_integer();
      return true;
    }
  else if (fn_call_expr_boundary_sptr b = is_fn_call_expr_boundary(boundary))
    {
      ini::function_call_expr_sptr fn_call = b->as_function_call_expr();
      if ((fn_call->get_name() == "offset_of"
	   || fn_call->get_name() == "offset_after")
	  && fn_call->get_arguments().size() == 1)
	{
	  string member_name = fn_call->get_arguments()[0];
	  for (class_decl::data_members::const_iterator it =
		 context->get_data_members().begin();
	       it != context->get_data_members().end();
	       ++it)
	    {
	      if (!get_data_member_is_laid_out(**it))
		continue;
	      if ((*it)->get_name() == member_name)
		{
		  if (fn_call->get_name() == "offset_of")
		    value = get_data_member_offset(*it);
		  else if (fn_call->get_name() == "offset_after")
		    value = get_data_member_offset(*it) +
		      (*it)->get_type()->get_size_in_bits();
		  else
		    // We should not reach this point.
		    abort();
		  return true;
		}
	    }
	}
    }
  return false;
}

/// Tests if a given instance of @ref
/// type_suppression::insertion_range::boundary is actually an integer boundary.
///
/// @param b the boundary to test.
///
/// @return a pointer to the instance of
/// type_suppression::insertion_range::integer_boundary if @p b is
/// actually an integer boundary.  Otherwise, return a null pointer.
type_suppression::insertion_range::integer_boundary_sptr
is_integer_boundary(type_suppression::insertion_range::boundary_sptr b)
{return dynamic_pointer_cast<type_suppression::insertion_range::integer_boundary>(b);}

/// Tests if a given instance of @ref
/// type_suppression::insertion_range::boundary is actually an function call expression boundary.
///
/// @param b the boundary to test.
///
/// @return a pointer to the instance of
/// type_suppression::insertion_range::fn_call_expr_boundary if @p b
/// is actually an function call expression boundary.  Otherwise,
/// return a null pointer.
type_suppression::insertion_range::fn_call_expr_boundary_sptr
is_fn_call_expr_boundary(type_suppression::insertion_range::boundary_sptr b)
{return dynamic_pointer_cast<type_suppression::insertion_range::fn_call_expr_boundary>(b);}

/// The private data type of @ref
/// type_suppression::insertion_range::boundary.
struct type_suppression::insertion_range::boundary::priv
{
  priv()
  {}
}; // end struct type_suppression::insertion_range::boundary::priv

/// Default constructor of @ref
/// type_suppression::insertion_range::boundary
type_suppression::insertion_range::boundary::boundary()
  : priv_(new priv())
{}

/// Destructor of @ref type_suppression::insertion_range::boundary.
type_suppression::insertion_range::boundary::~boundary()
{}

/// Private data type for @ref
/// type_suppression::insertion_range::integer_boundary.
struct type_suppression::insertion_range::integer_boundary::priv
{
  int value_;

  priv()
    : value_()
  {}

  priv(int value)
    : value_(value)
  {}
}; // end type_suppression::insertion_range::integer_boundary::priv

/// Converting constructor of
/// type_suppression::insertion_range::integer_boundary.
///
/// @param value the integer value of the newly created integer boundary.
type_suppression::insertion_range::integer_boundary::integer_boundary(int value)
  : priv_(new priv(value))
{}

/// Return the integer value of the current inace of @ref
/// type_suppression::insertion_range::integer_boundary.
///
/// @return the integer value of the current boundary.
int
type_suppression::insertion_range::integer_boundary::as_integer() const
{return priv_->value_;}

/// Converts the current boundary into an integer value.
///
/// @return the integer value of the current boundary.
type_suppression::insertion_range::integer_boundary::operator int() const
{return as_integer();}

/// Destructor of @ref type_suppression::insertion_range::integer_boundary.
type_suppression::insertion_range::integer_boundary::~integer_boundary()
{}

/// Private data type of type @ref
/// type_suppression::insertion_range::fn_call_expr_boundary.
struct type_suppression::insertion_range::fn_call_expr_boundary::priv
{
  ini::function_call_expr_sptr expr_;

  priv()
  {}

  priv(ini::function_call_expr_sptr expr)
    : expr_(expr)
  {}
}; // end struct type_suppression::insertion_range::fn_call_expr_boundary::priv

/// Converting constructor for @ref
/// type_suppression::insertion_range::fn_call_expr_boundary.
///
/// @param expr the function call expression to build this boundary
/// from.
type_suppression::insertion_range::fn_call_expr_boundary::
fn_call_expr_boundary(ini::function_call_expr_sptr expr)
  : priv_(new priv(expr))
{}

/// Returns the function call expression value of the current boundary.
///
/// @return the function call expression value of the current boundary;
ini::function_call_expr_sptr
type_suppression::insertion_range::fn_call_expr_boundary::as_function_call_expr() const
{return priv_->expr_;}

/// Converts the current boundary to its function call expression value.
///
/// @return the function call expression value of the current boundary.
type_suppression::insertion_range::fn_call_expr_boundary::operator ini::function_call_expr_sptr () const
{return as_function_call_expr();}

/// Destructor of @ref
/// type_suppression::insertion_range::fn_call_expr_boundary.
type_suppression::insertion_range::fn_call_expr_boundary::~fn_call_expr_boundary()
{}

/// Test if an instance of @ref suppression is an instance of @ref
/// type_suppression.
///
/// @param suppr the instance of @ref suppression to test for.
///
/// @return if @p suppr is an instance of @ref type_suppression, then
/// return the sub-object of the @p suppr of type @ref
/// type_suppression, otherwise return a nil pointer.
type_suppression_sptr
is_type_suppression(suppression_sptr suppr)
{return dynamic_pointer_cast<type_suppression>(suppr);}

// </type_suppression stuff>

/// Parse the value of the "type_kind" property in the "suppress_type"
/// section.
///
/// @param input the input string representing the value of the
/// "type_kind" property.
///
/// @return the @ref type_kind enumerator parsed.
static type_suppression::type_kind
read_type_kind_string(const string& input)
{
  if (input == "class")
    return type_suppression::CLASS_TYPE_KIND;
  else if (input == "struct")
    return type_suppression::STRUCT_TYPE_KIND;
  else if (input == "union")
    return type_suppression::UNION_TYPE_KIND;
  else if (input == "enum")
    return type_suppression::ENUM_TYPE_KIND;
  else if (input == "array")
    return type_suppression::ARRAY_TYPE_KIND;
  else if (input == "typedef")
    return type_suppression::TYPEDEF_TYPE_KIND;
  else if (input == "builtin")
    return type_suppression::BUILTIN_TYPE_KIND;
  else
    return type_suppression::UNKNOWN_TYPE_KIND;
}

/// Parse the value of the "accessed_through" property in the
/// "suppress_type" section.
///
/// @param input the input string representing the value of the
/// "accessed_through" property.
///
/// @return the @ref type_suppression::reach_kind enumerator parsed.
static type_suppression::reach_kind
read_suppression_reach_kind(const string& input)
{
  if (input == "direct")
    return type_suppression::DIRECT_REACH_KIND;
  else if (input == "pointer")
    return type_suppression::POINTER_REACH_KIND;
  else if (input == "reference")
    return type_suppression::REFERENCE_REACH_KIND;
  else if (input == "reference-or-pointer")
    return type_suppression::REFERENCE_OR_POINTER_REACH_KIND;
  else
    return type_suppression::DIRECT_REACH_KIND;
}

/// Read a type suppression from an instance of ini::config::section
/// and build a @ref type_suppression as a result.
///
/// @param section the section of the ini config to read.
///
/// @return the resulting @ref type_suppression upon successful
/// parsing, or nil.
static type_suppression_sptr
read_type_suppression(const ini::config::section& section)
{
  type_suppression_sptr nil;

  if (section.get_name() != "suppress_type")
    return nil;

  ini::simple_property_sptr label =
    is_simple_property(section.find_property("label"));
  string label_str = label ? label->get_value()->as_string() : "";

  ini::simple_property_sptr file_name_regex_prop =
    is_simple_property(section.find_property("file_name_regexp"));
  string file_name_regex_str =
    file_name_regex_prop ? file_name_regex_prop->get_value()->as_string() : "";

  ini::simple_property_sptr file_name_not_regex_prop =
    is_simple_property(section.find_property("file_name_not_regexp"));
  string file_name_not_regex_str =
    file_name_not_regex_prop
    ? file_name_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr soname_regex_prop =
    is_simple_property(section.find_property("soname_regexp"));
  string soname_regex_str =
    soname_regex_prop ? soname_regex_prop->get_value()->as_string() : "";

  ini::simple_property_sptr soname_not_regex_prop =
    is_simple_property(section.find_property("soname_not_regexp"));
  string soname_not_regex_str =
    soname_not_regex_prop
    ? soname_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr name_regex_prop =
    is_simple_property(section.find_property("name_regexp"));
  string name_regex_str = name_regex_prop
    ? name_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr name_prop =
    is_simple_property(section.find_property("name"));
  string name_str = name_prop
    ? name_prop->get_value()->as_string()
    : "";

  ini::property_sptr srcloc_not_in_prop =
    section.find_property("source_location_not_in");
  vector<string> srcloc_not_in;
  if (srcloc_not_in_prop)
    {
      if (ini::simple_property_sptr p = is_simple_property(srcloc_not_in_prop))
	srcloc_not_in.push_back(p->get_value()->as_string());
      else
	{
	  ini::list_property_sptr list_property =
	    is_list_property(srcloc_not_in_prop);
	  if (list_property)
	    srcloc_not_in = list_property->get_value()->get_content();
	}
    }

  ini::simple_property_sptr srcloc_not_regexp_prop =
    is_simple_property(section.find_property("source_location_not_regexp"));
  string srcloc_not_regexp_str;
  if (srcloc_not_regexp_prop)
    srcloc_not_regexp_str = srcloc_not_regexp_prop->get_value()->as_string();

  bool consider_type_kind = false;
  type_suppression::type_kind type_kind = type_suppression::UNKNOWN_TYPE_KIND;
  if (ini::simple_property_sptr type_kind_prop =
      is_simple_property(section.find_property("type_kind")))
    {
      consider_type_kind = true;
      type_kind =
	read_type_kind_string(type_kind_prop->get_value()->as_string());
    }

  bool consider_reach_kind = false;
  type_suppression::reach_kind reach_kind = type_suppression::DIRECT_REACH_KIND;
  if (ini::simple_property_sptr reach_kind_prop =
      is_simple_property(section.find_property("accessed_through")))
    {
      consider_reach_kind = true;
      reach_kind =
	read_suppression_reach_kind(reach_kind_prop->get_value()->as_string());
    }

  // Support has_data_member_inserted_at
  vector<type_suppression::insertion_range_sptr> insert_ranges;
  bool consider_data_member_insertion = false;
  if (ini::simple_property_sptr prop =
      is_simple_property(section.find_property("has_data_member_inserted_at")))
    {
      // So this property has the form:
      //   has_data_member_inserted_at = <one-string-property-value>
      string ins_point = prop->get_value()->as_string();
      type_suppression::insertion_range::boundary_sptr begin, end;
      if (ins_point == "end")
	begin = type_suppression::insertion_range::create_integer_boundary(-1);
      else if (isdigit(ins_point[0]))
	begin = type_suppression::insertion_range::create_integer_boundary
	  (atoi(ins_point.c_str()));
      else if (type_suppression::insertion_range::fn_call_expr_boundary_sptr expr =
	       type_suppression::insertion_range::create_fn_call_expr_boundary(ini::read_function_call_expr(ins_point)))
	begin = expr;
      else
	return nil;

      end = type_suppression::insertion_range::create_integer_boundary(-1);
      type_suppression::insertion_range_sptr insert_range
	(new type_suppression::insertion_range(begin, end));
	  insert_ranges.push_back(insert_range);
	  consider_data_member_insertion = true;
    }

  // Support has_data_member_inserted_between
  if (ini::tuple_property_sptr prop =
      is_tuple_property(section.find_property
			("has_data_member_inserted_between")))
    {
      // ensures that this has the form:
      //  has_data_member_inserted_between = {0 , end};
      // and not (for instance):
      //  has_data_member_inserted_between = {{0 , end}, {1, foo}}
      //
      //  This means that the tuple_property_value contains just one
      //  value, which is a list_property that itself contains 2
      //  values.
      type_suppression::insertion_range::boundary_sptr begin, end;
      ini::tuple_property_value_sptr v = prop->get_value();
      if (v
	  && v->get_value_items().size() == 1
	  && is_list_property_value(v->get_value_items()[0])
	  && is_list_property_value(v->get_value_items()[0])->get_content().size() == 2)
	{
	  ini::list_property_value_sptr val =
	    is_list_property_value(v->get_value_items()[0]);
	  assert(val);
	  string str = val->get_content()[0];
	  if (str == "end")
	    begin =
	      type_suppression::insertion_range::create_integer_boundary(-1);
	  else if (isdigit(str[0]))
	    begin = type_suppression::insertion_range::create_integer_boundary
	      (atoi(str.c_str()));
	  else if (type_suppression::insertion_range::fn_call_expr_boundary_sptr expr =
		   type_suppression::insertion_range::create_fn_call_expr_boundary(ini::read_function_call_expr(str)))
	    begin = expr;
	  else
	    return nil;

	  str = val->get_content()[1];
	  if (str == "end")
	    end =
	      type_suppression::insertion_range::create_integer_boundary(-1);
	  else if (isdigit(str[0]))
	    end = type_suppression::insertion_range::create_integer_boundary
	      (atoi(str.c_str()));
	  else if (type_suppression::insertion_range::fn_call_expr_boundary_sptr expr =
		   type_suppression::insertion_range::create_fn_call_expr_boundary(ini::read_function_call_expr(str)))
	    end = expr;
	  else
	    return nil;

	  type_suppression::insertion_range_sptr insert_range
	    (new type_suppression::insertion_range(begin, end));
	  insert_ranges.push_back(insert_range);
	  consider_data_member_insertion = true;
	}
      else
	// the 'has_data_member_inserted_between' property has a wrong
	// value type, so let's discard the endire [suppress_type]
	// section.
	return nil;
    }

  // Support has_data_members_inserted_between
  // The syntax looks like:
  //
  //    has_data_members_inserted_between = {{8, 24}, {32, 64}, {128, end}}
  //
  // So we expect a tuple property, with potentially several pairs (as
  // part of the value); each pair designating a range.  Note that
  // each pair (range) is a list property value.
  if (ini::tuple_property_sptr prop =
      is_tuple_property(section.find_property
			("has_data_members_inserted_between")))
    {
      bool is_well_formed = true;
      for (vector<ini::property_value_sptr>::const_iterator i =
	     prop->get_value()->get_value_items().begin();
	   is_well_formed && i != prop->get_value()->get_value_items().end();
	   ++i)
	{
	  ini::tuple_property_value_sptr tuple_value =
	    is_tuple_property_value(*i);
	  if (!tuple_value
	      || tuple_value->get_value_items().size() != 1
	      || !is_list_property_value(tuple_value->get_value_items()[0]))
	    {
	      is_well_formed = false;
	      break;
	    }
	  ini::list_property_value_sptr list_value =
	    is_list_property_value(tuple_value->get_value_items()[0]);
	  if (list_value->get_content().size() != 2)
	    {
	      is_well_formed = false;
	      break;
	    }

	  type_suppression::insertion_range::boundary_sptr begin, end;
	  string str = list_value->get_content()[0];
	  if (str == "end")
	    begin =
	      type_suppression::insertion_range::create_integer_boundary(-1);
	  else if (isdigit(str[0]))
	    begin =
	      type_suppression::insertion_range::create_integer_boundary
	      (atoi(str.c_str()));
	  else if (type_suppression::insertion_range::fn_call_expr_boundary_sptr expr =
		   type_suppression::insertion_range::create_fn_call_expr_boundary(ini::read_function_call_expr(str)))
	    begin = expr;
	  else
	    return nil;

	  str = list_value->get_content()[1];
	  if (str == "end")
	    end =
	      type_suppression::insertion_range::create_integer_boundary(-1);
	  else if (isdigit(str[0]))
	    end = type_suppression::insertion_range::create_integer_boundary
	      (atoi(str.c_str()));
	  else if (type_suppression::insertion_range::fn_call_expr_boundary_sptr expr =
		   type_suppression::insertion_range::create_fn_call_expr_boundary(ini::read_function_call_expr(str)))
	    end = expr;
	  else
	    return nil;

	  type_suppression::insertion_range_sptr insert_range
	    (new type_suppression::insertion_range(begin, end));
	  insert_ranges.push_back(insert_range);
	  consider_data_member_insertion = true;
	}
      if (!is_well_formed)
	return nil;
    }

  if (file_name_regex_str.empty()
      && file_name_not_regex_str.empty()
      && soname_regex_str.empty()
      && soname_not_regex_str.empty()
      && (!name_regex_prop || name_regex_prop->get_value()->as_string().empty())
      && (!name_prop || name_prop->get_value()->as_string().empty())
      && !consider_type_kind
      && srcloc_not_regexp_str.empty()
      && srcloc_not_in.empty())
    return nil;

  type_suppression_sptr suppr(new type_suppression(label_str,
						   name_regex_str,
						   name_str));
  if (consider_type_kind)
    {
      suppr->set_consider_type_kind(true);
      suppr->set_type_kind(type_kind);
    }

  if (consider_reach_kind)
    {
      suppr->set_consider_reach_kind(true);
      suppr->set_reach_kind(reach_kind);
    }

  if (consider_data_member_insertion)
    suppr->set_data_member_insertion_ranges(insert_ranges);

  if (!file_name_regex_str.empty())
    suppr->set_file_name_regex_str(file_name_regex_str);

  if (!file_name_not_regex_str.empty())
    suppr->set_file_name_not_regex_str(file_name_not_regex_str);

  if (!soname_regex_str.empty())
    suppr->set_soname_regex_str(soname_regex_str);

  if (!soname_not_regex_str.empty())
    suppr->set_soname_not_regex_str(soname_not_regex_str);

  if (!srcloc_not_in.empty())
    suppr->set_source_locations_to_keep(srcloc_not_in);

  if (!srcloc_not_regexp_str.empty())
    suppr->set_source_location_to_keep_regex_str(srcloc_not_regexp_str);

  return suppr;
}

// <function_suppression stuff>
class function_suppression::parameter_spec::priv
{
  friend class function_suppression::parameter_spec;
  friend class function_suppression;

  size_t				index_;
  string				type_name_;
  string				type_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	type_name_regex_;

  priv()
    : index_()
  {}

  priv(size_t i, const string& tn)
    : index_(i), type_name_(tn)
  {}

  priv(size_t i, const string& tn, const string& tn_regex)
    : index_(i), type_name_(tn), type_name_regex_str_(tn_regex)
  {}

  const sptr_utils::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_ && !type_name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    type_name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  type_name_regex_ = r;
      }
    return type_name_regex_;
  }
}; // end class function_suppression::parameter_spec::priv

/// Constructor for the @ref the function_suppression::parameter_spec
/// type.
///
/// @param i the index of the parameter designated by this specification.
///
/// @param tn the type name of the parameter designated by this specification.
///
/// @param tn_regex a regular expression that defines a set of type
/// names for the parameter designated by this specification.  Note
/// that at evaluation time, this regular expression is taken in
/// account only if the parameter @p tn is empty.
function_suppression::parameter_spec::parameter_spec(size_t i,
						     const string& tn,
						     const string& tn_regex)
  : priv_(new priv(i, tn, tn_regex))
{}

/// Getter for the index of the parameter designated by this
/// specification.
///
/// @return the index of the parameter designated by this
/// specification.
size_t
function_suppression::parameter_spec::get_index() const
{return priv_->index_;}

/// Setter for the index of the parameter designated by this
/// specification.
///
/// @param i the new index to set.
void
function_suppression::parameter_spec::set_index(size_t i)
{priv_->index_ = i;}

/// Getter for the type name of the parameter designated by this specification.
///
/// @return the type name of the parameter.
const string&
function_suppression::parameter_spec::get_parameter_type_name() const
{return priv_->type_name_;}

/// Setter for the type name of the parameter designated by this
/// specification.
///
/// @param tn new parameter type name to set.
void
function_suppression::parameter_spec::set_parameter_type_name(const string& tn)
{priv_->type_name_ = tn;}

/// Getter for the regular expression that defines a set of type names
/// for the parameter designated by this specification.
///
/// Note that at evaluation time, this regular expression is taken in
/// account only if the name of the parameter as returned by
/// function_suppression::parameter_spec::get_parameter_type_name() is
/// empty.
///
/// @return the regular expression or the parameter type name.
const string&
function_suppression::parameter_spec::get_parameter_type_name_regex_str() const
{return priv_->type_name_regex_str_;}

/// Setter for the regular expression that defines a set of type names
/// for the parameter designated by this specification.
///
/// Note that at evaluation time, this regular expression is taken in
/// account only if the name of the parameter as returned by
/// function_suppression::parameter_spec::get_parameter_type_name() is
/// empty.
///
/// @param type_name_regex_str the new type name regular expression to
/// set.
void
function_suppression::parameter_spec::set_parameter_type_name_regex_str
(const string& type_name_regex_str)
{priv_->type_name_regex_str_ = type_name_regex_str;}

/// The type of the private data of the @ref function_suppression
/// type.
class function_suppression::priv
{
  friend class function_suppression;

  change_kind				change_kind_;
  string				name_;
  string				name_regex_str_;
  mutable sptr_utils::regex_t_sptr	name_regex_;
  string				return_type_name_;
  string				return_type_regex_str_;
  mutable sptr_utils::regex_t_sptr	return_type_regex_;
  parameter_specs_type			parm_specs_;
  string				symbol_name_;
  string				symbol_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	symbol_name_regex_;
  string				symbol_version_;
  string				symbol_version_regex_str_;
  mutable sptr_utils::regex_t_sptr	symbol_version_regex_;
  bool					allow_other_aliases_;

  priv(const string&			name,
       const string&			name_regex_str,
       const string&			return_type_name,
       const string&			return_type_regex_str,
       const parameter_specs_type&	parm_specs,
       const string&			symbol_name,
       const string&			symbol_name_regex_str,
       const string&			symbol_version,
       const string&			symbol_version_regex_str)
    : change_kind_(ALL_CHANGE_KIND),
      name_(name),
      name_regex_str_(name_regex_str),
      return_type_name_(return_type_name),
      return_type_regex_str_(return_type_regex_str),
      parm_specs_(parm_specs),
      symbol_name_(symbol_name),
      symbol_name_regex_str_(symbol_name_regex_str),
      symbol_version_(symbol_version),
      symbol_version_regex_str_(symbol_version_regex_str),
      allow_other_aliases_(true)
  {}


  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::name_regex_str_..
  const sptr_utils::regex_t_sptr
  get_name_regex() const
  {
    if (!name_regex_ && !name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  name_regex_ = r;
      }
    return name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::return_type_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::return_type_regex_str_.
  const sptr_utils::regex_t_sptr
  get_return_type_regex() const
  {
    if (!return_type_regex_ && !return_type_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    return_type_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  return_type_regex_ = r;
      }
    return return_type_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::symbol_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::symbol_name_regex_str_.
  const sptr_utils::regex_t_sptr
  get_symbol_name_regex() const
  {
    if (!symbol_name_regex_ && !symbol_name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    symbol_name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  symbol_name_regex_ = r;
      }
    return symbol_name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// function_suppression::priv::symbol_version_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// function_suppression::priv::symbol_version_regex_str_.
  const sptr_utils::regex_t_sptr
  get_symbol_version_regex() const
  {
    if (!symbol_version_regex_ && ! symbol_version_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    symbol_version_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  symbol_version_regex_ = r;
      }
    return symbol_version_regex_;
  }
}; // end class function_suppression::priv

/// Constructor for the @ref function_suppression type.
///
/// @param label an informative text string that the evalution code
/// might use to designate this function suppression specification in
/// error messages.  This parameter might be empty, in which case it's
/// ignored at evaluation time.
///
/// @param the name of the function the user wants the current
/// specification to designate.  This parameter might be empty, in
/// which case it's ignored at evaluation time.
///
/// @param nr if @p name is empty this parameter is a regular
/// expression for a family of names of functions the user wants the
/// current specification to designate.  If @p name is not empty, this
/// parameter is ignored at specification evaluation time.  This
/// parameter might be empty, in which case it's ignored at evaluation
/// time.
///
/// @param ret_tn the name of the return type of the function the user
/// wants this specification to designate.  This parameter might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param ret_tr if @p ret_tn is empty, then this is a regular
/// expression for a family of return type names for functions the
/// user wants the current specification to designate.  If @p ret_tn
/// is not empty, then this parameter is ignored at specification
/// evaluation time.  This parameter might be empty, in which case
/// it's ignored at evaluation time.
///
/// @param ps a vector of parameter specifications to specify
/// properties of the parameters of the functions the user wants this
/// specification to designate.  This parameter might be empty, in
/// which case it's ignored at evaluation time.
///
/// @param sym_n the name of symbol of the function the user wants
/// this specification to designate.  This parameter might be empty,
/// in which case it's ignored at evaluation time.
///
/// @param sym_nr if the parameter @p sym_n is empty, then this
/// parameter is a regular expression for a family of names of symbols
/// of functions the user wants this specification to designate.  If
/// the parameter @p sym_n is not empty, then this parameter is
/// ignored at specification evaluation time.  This parameter might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param sym_v the name of the version of the symbol of the function
/// the user wants this specification to designate.  This parameter
/// might be empty, in which case it's ignored at evaluation time.
///
/// @param sym_vr if the parameter @p sym_v is empty, then this
/// parameter is a regular expression for a family of versions of
/// symbols of functions the user wants the current specification to
/// designate.  If the parameter @p sym_v is non empty, then this
/// parameter is ignored.  This parameter might be empty, in which
/// case it's ignored at evaluation time.
function_suppression::function_suppression(const string&		label,
					   const string&		name,
					   const string&		nr,
					   const string&		ret_tn,
					   const string&		ret_tr,
					   parameter_specs_type&	ps,
					   const string&		sym_n,
					   const string&		sym_nr,
					   const string&		sym_v,
					   const string&		sym_vr)
  : suppression_base(label),
    priv_(new priv(name, nr, ret_tn, ret_tr, ps,
		   sym_n, sym_nr, sym_v, sym_vr))
{}

function_suppression::~function_suppression()
{}

/// Parses a string containing the content of the "change-kind"
/// property and returns the an instance of @ref
/// function_suppression::change_kind as a result.
///
/// @param s the string to parse.
///
/// @return the resulting @ref function_suppression::change_kind.
function_suppression::change_kind
function_suppression::parse_change_kind(const string& s)
{
  if (s == "function-subtype-change")
    return FUNCTION_SUBTYPE_CHANGE_KIND;
  else if (s == "added-function")
    return ADDED_FUNCTION_CHANGE_KIND;
  else if (s == "deleted-function")
    return DELETED_FUNCTION_CHANGE_KIND;
  else if (s == "all")
    return ALL_CHANGE_KIND;
  else
    return UNDEFINED_CHANGE_KIND;
}

/// Getter of the "change-kind" property.
///
/// @param returnthe "change-kind" property.
function_suppression::change_kind
function_suppression::get_change_kind() const
{return priv_->change_kind_;}

/// Setter of the "change-kind" property.
///
/// @param k the new value of the change_kind property.
void
function_suppression::set_change_kind(change_kind k)
{priv_->change_kind_ = k;}

/// Getter for the name of the function the user wants the current
/// specification to designate.  This might be empty, in which case
/// it's ignored at evaluation time.
///
/// @return the name of the function.
const string&
function_suppression::get_function_name() const
{return priv_->name_;}

/// Setter for the name of the function the user wants the current
/// specification to designate.  This might be empty, in which case
/// it's ignored at evaluation time.
///
/// @param n the new function name to set.
void
function_suppression::set_function_name(const string& n)
{priv_->name_ = n;}

/// Getter for a regular expression for a family of names of functions
/// the user wants the current specification to designate.
///
/// If the function name as returned by
/// function_suppression::get_function_name() is not empty, this
/// property is ignored at specification evaluation time.  This
/// property might be empty, in which case it's ignored at evaluation
/// time.
///
/// @return the regular expression for the possible names of the
/// function(s).
const string&
function_suppression::get_function_name_regex_str() const
{return priv_->name_regex_str_;}

/// Setter for a regular expression for a family of names of functions
/// the user wants the current specification to designate.
///
/// If the function name as returned by
/// function_suppression::get_function_name() is not empty, this
/// property is ignored at specification evaluation time.  This
/// property might be empty, in which case it's ignored at evaluation
/// time.
///
/// @param r the new the regular expression for the possible names of
/// the function(s).
void
function_suppression::set_function_name_regex_str(const string& r)
{priv_->name_regex_str_ = r;}

/// Getter for the name of the return type of the function the user
/// wants this specification to designate.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @return the name of the return type of the function.
const string&
function_suppression::get_return_type_name() const
{return priv_->return_type_name_;}

/// Setter for the name of the return type of the function the user
/// wants this specification to designate.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param tr the new name of the return type of the function to set.
void
function_suppression::set_return_type_name(const string& tr)
{priv_->return_type_name_ = tr;}

/// Getter for a regular expression for a family of return type names
/// for functions the user wants the current specification to
/// designate.
///
/// If the name of the return type of the function as returned by
/// function_suppression::get_return_type_name() is not empty, then
/// this property is ignored at specification evaluation time.  This
/// property might be empty, in which case it's ignored at evaluation
/// time.
///
/// @return the regular expression for the possible names of the
/// return types of the function(s).
const string&
function_suppression::get_return_type_regex_str() const
{return priv_->return_type_regex_str_;}

/// Setter for a regular expression for a family of return type names
/// for functions the user wants the current specification to
/// designate.
///
/// If the name of the return type of the function as returned by
/// function_suppression::get_return_type_name() is not empty, then
/// this property is ignored at specification evaluation time.  This
/// property might be empty, in which case it's ignored at evaluation
/// time.
///
/// @param r the new regular expression for the possible names of the
/// return types of the function(s) to set.
void
function_suppression::set_return_type_regex_str(const string& r)
{priv_->return_type_regex_str_ = r;}

/// Getter for a vector of parameter specifications to specify
/// properties of the parameters of the functions the user wants this
/// specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return the specifications of the parameters of the function(s).
const function_suppression::parameter_specs_type&
function_suppression::get_parameter_specs() const
{return priv_->parm_specs_;}

/// Setter for a vector of parameter specifications to specify
/// properties of the parameters of the functions the user wants this
/// specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @param p the new specifications of the parameters of the
/// function(s) to set.
void
function_suppression::set_parameter_specs(parameter_specs_type& p)
{priv_->parm_specs_ = p;}

/// Append a specification of a parameter of the function specification.
///
/// @param p the parameter specification to add.
void
function_suppression::append_parameter_specs(const parameter_spec_sptr p)
{priv_->parm_specs_.push_back(p);}

/// Getter for the name of symbol of the function the user wants this
/// specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return name of the symbol of the function.
const string&
function_suppression::get_symbol_name() const
{return priv_->symbol_name_;}

/// Setter for the name of symbol of the function the user wants this
/// specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return name of the symbol of the function.
void
function_suppression::set_symbol_name(const string& n)
{priv_->symbol_name_ = n;}

/// Getter for a regular expression for a family of names of symbols
/// of functions the user wants this specification to designate.
///
/// If the symbol name as returned by
/// function_suppression::get_symbol_name() is not empty, then this
/// property is ignored at specification evaluation time.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return the regular expression for a family of names of symbols of
/// functions to designate.
const string&
function_suppression::get_symbol_name_regex_str() const
{return priv_->symbol_name_regex_str_;}

/// Setter for a regular expression for a family of names of symbols
/// of functions the user wants this specification to designate.
///
/// If the symbol name as returned by
/// function_suppression::get_symbol_name() is not empty, then this
/// property is ignored at specification evaluation time.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @param r the new regular expression for a family of names of
/// symbols of functions to set.
void
function_suppression::set_symbol_name_regex_str(const string& r)
{priv_->symbol_name_regex_str_ = r;}

/// Getter for the name of the version of the symbol of the function
/// the user wants this specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return the symbol version of the function.
const string&
function_suppression::get_symbol_version() const
{return priv_->symbol_version_;}

/// Setter for the name of the version of the symbol of the function
/// the user wants this specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @param v the new symbol version of the function.
void
function_suppression::set_symbol_version(const string& v)
{priv_->symbol_version_ = v;}

/// Getter for a regular expression for a family of versions of
/// symbols of functions the user wants the current specification to
/// designate.
///
/// If the symbol version as returned by
/// function_suppression::get_symbol_version() is non empty, then this
/// property is ignored.  This property might be empty, in which case
/// it's ignored at evaluation time.
///
/// @return the regular expression for the versions of symbols of
/// functions to designate.
const string&
function_suppression::get_symbol_version_regex_str() const
{return priv_->symbol_version_regex_str_;}

/// Setter for a regular expression for a family of versions of
/// symbols of functions the user wants the current specification to
/// designate.
///
/// If the symbol version as returned by
/// function_suppression::get_symbol_version() is non empty, then this
/// property is ignored.  This property might be empty, in which case
/// it's ignored at evaluation time.
///
/// @param the new regular expression for the versions of symbols of
/// functions to designate.
void
function_suppression::set_symbol_version_regex_str(const string& r)
{priv_->symbol_version_regex_str_ = r;}

/// Getter for the "allow_other_aliases" property of the function
/// suppression specification.
///
/// @return the value of the "allow_other_aliases" property.
bool
function_suppression::get_allow_other_aliases() const
{return priv_->allow_other_aliases_;}

/// Setter for the "allow_other_aliases" property of the function
/// suppression specification.
///
/// @param f the new value of the property.
void
function_suppression::set_allow_other_aliases(bool f)
{priv_->allow_other_aliases_ = f;}

/// Evaluate this suppression specification on a given diff node and
/// say if the diff node should be suppressed or not.
///
/// @param diff the diff node to evaluate this suppression
/// specification against.
///
/// @return true if @p diff should be suppressed.
bool
function_suppression::suppresses_diff(const diff* diff) const
{
  const function_decl_diff* d = is_function_decl_diff(diff);
  if (!d)
    return false;

  function_decl_sptr ff = is_function_decl(d->first_function_decl()),
    sf = is_function_decl(d->second_function_decl());
  assert(ff && sf);

  return (suppresses_function(ff,
			      FUNCTION_SUBTYPE_CHANGE_KIND,
			      diff->context())
	  || suppresses_function(sf,
				 FUNCTION_SUBTYPE_CHANGE_KIND,
				 diff->context()));
}

/// Evaluate the current function suppression specification on a given
/// @ref function_decl and say if a report about a change involving this
/// @ref function_decl should be suppressed or not.
///
/// @param fn the @ref function_decl to evaluate this suppression
/// specification against.
///
/// @param k the kind of function change @p fn is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the function @p
/// fn should be suppressed.
bool
function_suppression::suppresses_function(const function_decl* fn,
					  change_kind k,
					  const diff_context_sptr ctxt) const
{
  if (!(get_change_kind() & k))
    return false;

  // Check if the name and soname of the binaries match
  if (ctxt)
    {
      // Check if the name of the binaries match
      if (!names_of_binaries_match(*ctxt))
	return false;

      // Check if the soname of the binaries match
      if (!sonames_of_binaries_match(*ctxt))
	return false;
    }

  string fname = fn->get_qualified_name();

  // Check if the "name" property matches.
  if (!get_function_name().empty())
    {
      if (get_function_name() != fn->get_qualified_name())
	return false;

      if (get_allow_other_aliases()
	  && fn->get_symbol()
	  && fn->get_symbol()->get_alias_from_name(fname))
	{
	  // So we are in a case of a languages in which the symbol
	  // name is the same as the function name and we want to
	  // allow the removal of change reports on an aliased
	  // function only if the suppression condition matches the
	  // names of all aliases.
	  string symbol_name;
	  elf_symbol_sptr sym = fn->get_symbol();
	  assert(sym);
	  symbol_name = sym->get_name();
	  if (sym->has_aliases() && sym->get_alias_from_name(fname))
	    {
	      for (elf_symbol_sptr a = sym->get_next_alias();
		   a && !a->is_main_symbol();
		   a = a->get_next_alias())
		if (a->get_name() != symbol_name)
		  // There is an alias which name is different from
		  // the function (symbol) name given in the
		  // suppression condition.
		  return false;
	    }
	}
    }

  // check if the "name_regexp" property matches.
  const sptr_utils::regex_t_sptr name_regex = priv_->get_name_regex();
  if (name_regex)
    {
      if (regexec(name_regex.get(),
		  fname.c_str(),
		  0, NULL, 0) != 0)
	return false;

      if (get_allow_other_aliases()
	  && fn->get_symbol()
	  && fn->get_symbol()->get_alias_from_name(fname))
	{
	  // So we are in a case of a languages in which the symbol
	  // name is the same as the function name and we want to
	  // allow the removal of change reports on an aliased
	  // function only if the suppression condition matches *all*
	  // the aliases.
	  string symbol_name;
	  elf_symbol_sptr sym = fn->get_symbol();
	  assert(sym);
	  symbol_name = sym->get_name();
	  if (sym->has_aliases())
	    {
	      for (elf_symbol_sptr a = sym->get_next_alias();
		   a && !a->is_main_symbol();
		   a = a->get_next_alias())
		if (regexec(name_regex.get(),
			    a->get_name().c_str(),
			    0, NULL, 0) != 0)
		  return false;
	    }
	}
    }

  // Check if the "return_type_name" or "return_type_regexp"
  // properties matches.

  string fn_return_type_name = fn->get_type()->get_return_type()
    ? static_cast<string>
    ((get_type_declaration(fn->get_type()->get_return_type())
      ->get_qualified_name()))
    : "";

  if (!get_return_type_name().empty())
    {
      if (fn_return_type_name != get_return_type_name())
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr return_type_regex =
	priv_->get_return_type_regex();
      if (return_type_regex
	  && (regexec(return_type_regex.get(),
		      fn_return_type_name.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  // Check if the "symbol_name" and "symbol_name_regexp" properties
  // match.
  string fn_sym_name, fn_sym_version;
  elf_symbol_sptr sym = fn->get_symbol();
  if (sym)
    {
      fn_sym_name = sym->get_name();
      fn_sym_version = sym->get_version().str();
    }

  if (sym && !get_symbol_name().empty())
    {
      if (fn_sym_name != get_symbol_name())
	return false;

      if (sym && get_allow_other_aliases())
	{
	  // In this case, we want to allow the suppression of change
	  // reports about an aliased symbol only if the suppression
	  // condition matches the name of all aliases.
	  if (sym->has_aliases())
	    {
	      for (elf_symbol_sptr a = sym->get_next_alias();
		   a && !a->is_main_symbol();
		   a = a->get_next_alias())
		if (a->get_name() != fn_sym_name)
		  return false;
	    }
	}
    }
  else if (sym)
    {
      const sptr_utils::regex_t_sptr symbol_name_regex =
	priv_->get_symbol_name_regex();
      if (symbol_name_regex
	  && (regexec(symbol_name_regex.get(),
		      fn_sym_name.c_str(),
		      0, NULL, 0) != 0))
	return false;

      if (get_allow_other_aliases())
	{
	  // In this case, we want to allow the suppression of change
	  // reports about an aliased symbol only if the suppression
	  // condition matches the name of all aliases.
	  if (sym->has_aliases())
	    {
	      for (elf_symbol_sptr a = sym->get_next_alias();
		   a && !a->is_main_symbol();
		   a = a->get_next_alias())
		if (symbol_name_regex
		    && (regexec(symbol_name_regex.get(),
				a->get_name().c_str(),
				0, NULL, 0) != 0))
		  return false;
	    }
	}
    }

  // Check if the "symbol_version" and "symbol_version_regexp"
  // properties match.
  if (sym && !get_symbol_version().empty())
    {
      if (fn_sym_version != get_symbol_version())
	return false;
    }
  else if (sym)
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      fn_sym_version.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  // Check the 'parameter' property.
  if (!get_parameter_specs().empty())
    {
      function_type_sptr fn_type = fn->get_type();
      type_base_sptr parm_type;

      for (parameter_specs_type::const_iterator p =
	     get_parameter_specs().begin();
	   p != get_parameter_specs().end();
	   ++p)
	{
	  size_t index = (*p)->get_index();
	  function_decl::parameter_sptr fn_parm =
	    fn_type->get_parm_at_index_from_first_non_implicit_parm(index);
	  if (!fn_parm)
	    return false;

	  string fn_parm_type_qualified_name;
	  if (fn_parm)
	    {
	      parm_type = fn_parm->get_type();
	      fn_parm_type_qualified_name =
		get_type_declaration(parm_type)->get_qualified_name();
	    }

	  const string& tn = (*p)->get_parameter_type_name();
	  if (!tn.empty())
	    {
	      if (tn != fn_parm_type_qualified_name)
		return false;
	    }
	  else
	    {
	      const sptr_utils::regex_t_sptr parm_type_name_regex =
		(*p)->priv_->get_type_name_regex();
	      if (parm_type_name_regex)
		{
		  if ((regexec(parm_type_name_regex.get(),
			       fn_parm_type_qualified_name.c_str(),
			       0, NULL, 0) != 0))
		    return false;
		}
	    }
	}
    }

  return true;
}

/// Evaluate the current function suppression specification on a given
/// @ref function_decl and say if a report about a change involving this
/// @ref function_decl should be suppressed or not.
///
/// @param fn the @ref function_decl to evaluate this suppression
/// specification against.
///
/// @param k the kind of function change @p fn is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the function @p
/// fn should be suppressed.
bool
function_suppression::suppresses_function(const function_decl_sptr fn,
					  change_kind k,
					  const diff_context_sptr ctxt) const
{return suppresses_function(fn.get(), k, ctxt);}

/// Evaluate the current function suppression specification on a given
/// @ref elf_symbol and say if a report about a change involving this
/// @ref elf_symbol should be suppressed or not.
///
/// @param sym the @ref elf_symbol to evaluate this suppression
/// specification against.
///
/// @param k the kind of function change @p sym is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the symbol @p
/// sym should be suppressed.
bool
function_suppression::suppresses_function_symbol(const elf_symbol* sym,
						 change_kind k,
						 const diff_context_sptr ctxt)
{
  if (!sym)
    return false;

  if (!(get_change_kind() & k))
    return false;

  if (!sym->is_function())
    return false;

  assert(k & function_suppression::ADDED_FUNCTION_CHANGE_KIND
	 || k & function_suppression::DELETED_FUNCTION_CHANGE_KIND);

  // Check if the name and soname of the binaries match
  if (ctxt)
    {
      // Check if the name of the binaries match
      if (!names_of_binaries_match(*ctxt))
	return false;

      // Check if the soname of the binaries match
      if (!sonames_of_binaries_match(*ctxt))
	return false;
    }

  string sym_name = sym->get_name(), sym_version = sym->get_version().str();
  bool no_symbol_name = false, no_symbol_version = false;

  // Consider the symbol name.
  if (!get_symbol_name().empty())
    {
      if (sym_name != get_symbol_name())
	return false;
    }
  else if (!get_symbol_name_regex_str().empty())
    {
      const sptr_utils::regex_t_sptr symbol_name_regex =
	priv_->get_symbol_name_regex();
      if (symbol_name_regex
	  && (regexec(symbol_name_regex.get(),
		      sym_name.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }
  else
    no_symbol_name = true;

  // Consider the symbol version
  if (!get_symbol_version().empty())
    {
      if (sym_version != get_symbol_version())
	return false;
    }
  else if (!get_symbol_version_regex_str().empty())
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      sym_version.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }
  else
    no_symbol_version = true;

  if (no_symbol_name && no_symbol_version)
    return false;

  return true;
}

/// Evaluate the current function suppression specification on a given
/// @ref elf_symbol and say if a report about a change involving this
/// @ref elf_symbol should be suppressed or not.
///
/// @param sym the @ref elf_symbol to evaluate this suppression
/// specification against.
///
/// @param k the kind of function change @p sym is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the symbol @p
/// sym should be suppressed.
bool
function_suppression::suppresses_function_symbol(const elf_symbol_sptr sym,
						 change_kind k,
						 const diff_context_sptr ctxt)
{return suppresses_function_symbol(sym.get(), k, ctxt);}

/// Test if an instance of @ref suppression is an instance of @ref
/// function_suppression.
///
/// @param suppr the instance of @ref suppression to test for.
///
/// @return if @p suppr is an instance of @ref function_suppression, then
/// return the sub-object of the @p suppr of type @ref
/// function_suppression, otherwise return a nil pointer.
function_suppression_sptr
is_function_suppression(const suppression_sptr suppr)
{return dynamic_pointer_cast<function_suppression>(suppr);}

/// The bitwise 'and' operator for the enum @ref
/// function_suppression::change_kind.
///
/// @param l the first operand of the 'and' operator.
///
/// @param r the second operand of the 'and' operator.
///
/// @return the result of 'and' operation on @p l and @p r.
function_suppression::change_kind
operator&(function_suppression::change_kind l,
	  function_suppression::change_kind r)
{
  return static_cast<function_suppression::change_kind>
    (static_cast<unsigned>(l) & static_cast<unsigned>(r));
}

/// The bitwise 'or' operator for the enum @ref
/// function_suppression::change_kind.
///
/// @param l the first operand of the 'or' operator.
///
/// @param r the second operand of the 'or' operator.
///
/// @return the result of 'or' operation on @p l and @p r.
function_suppression::change_kind
operator|(function_suppression::change_kind l,
	  function_suppression::change_kind r)
{
    return static_cast<function_suppression::change_kind>
      (static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

/// Parse a string containing a parameter spec, build an instance of
/// function_suppression::parameter_spec from it and return a pointer
/// to that object.
///
/// @return a shared pointer pointer to the newly built instance of
/// function_suppression::parameter_spec.  If the parameter
/// specification could not be parsed, return a nil object.
static function_suppression::parameter_spec_sptr
read_parameter_spec_from_string(const string& str)
{
  string::size_type cur = 0;
  function_suppression::parameter_spec_sptr result;

  // skip leading white spaces.
  for (; cur < str.size(); ++cur)
    if (!isspace(str[cur]))
      break;

  // look for the parameter index
  string index_str;
  if (str[cur] == '\'')
    {
      ++cur;
      for (; cur < str.size(); ++cur)
	if (!isdigit(str[cur]))
	  break;
	else
	  index_str += str[cur];
    }

  // skip white spaces.
  for (; cur < str.size(); ++cur)
    if (!isspace(str[cur]))
      break;

  bool is_regex = false;
  if (str[cur] == '/')
    {
      is_regex = true;
      ++ cur;
    }

  // look for the type name (regex)
  string type_name;
  for (; cur < str.size(); ++cur)
    if (!isspace(str[cur]))
      {
	if (is_regex && str[cur] == '/')
	  break;
	type_name += str[cur];
      }

  if (is_regex && str[cur] == '/')
    ++cur;

  if (!index_str.empty() || !type_name.empty())
    {
      function_suppression::parameter_spec* p;
      if (is_regex)
	p = new function_suppression::parameter_spec(atoi(index_str.c_str()),
						     "", type_name);
      else
	p = new function_suppression::parameter_spec(atoi(index_str.c_str()),
						     type_name, "");
      result.reset(p);
    }

  return result;
}

/// Parse function suppression specification, build a resulting @ref
/// function_suppression type and return a shared pointer to that
/// object.
///
/// @return a shared pointer to the newly built @ref
/// function_suppression.  If the function suppression specification
/// could not be parsed then a nil shared pointer is returned.
static function_suppression_sptr
read_function_suppression(const ini::config::section& section)
{
  function_suppression_sptr nil;

  if (section.get_name() != "suppress_function")
    return nil;

  ini::simple_property_sptr change_kind_prop =
    is_simple_property(section.find_property("change_kind"));
  string change_kind_str = change_kind_prop
    ? change_kind_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr label_prop =
    is_simple_property(section.find_property("label"));
  string label_str = label_prop
    ? label_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr file_name_regex_prop =
    is_simple_property(section.find_property("file_name_regexp"));
  string file_name_regex_str =
    file_name_regex_prop ? file_name_regex_prop->get_value()->as_string() : "";

  ini::simple_property_sptr file_name_not_regex_prop =
    is_simple_property(section.find_property("file_name_not_regexp"));
  string file_name_not_regex_str =
    file_name_not_regex_prop
    ? file_name_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr soname_regex_prop =
    is_simple_property(section.find_property("soname_regexp"));
  string soname_regex_str =
    soname_regex_prop ? soname_regex_prop->get_value()->as_string() : "";

  ini::simple_property_sptr soname_not_regex_prop =
    is_simple_property(section.find_property("soname_not_regexp"));
  string soname_not_regex_str =
    soname_not_regex_prop
    ? soname_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr name_prop =
    is_simple_property(section.find_property("name"));
  string name = name_prop
    ? name_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr name_regex_prop =
    is_simple_property(section.find_property("name_regexp"));
  string name_regex_str = name_regex_prop
    ? name_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr return_type_name_prop =
    is_simple_property(section.find_property("return_type_name"));
  string return_type_name = return_type_name_prop
    ? return_type_name_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr return_type_regex_prop =
    is_simple_property(section.find_property("return_type_regexp"));
  string return_type_regex_str = return_type_regex_prop
    ? return_type_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_name_prop =
    is_simple_property(section.find_property("symbol_name"));
  string sym_name = sym_name_prop
    ? sym_name_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_name_regex_prop =
    is_simple_property(section.find_property("symbol_name_regexp"));
  string sym_name_regex_str = sym_name_regex_prop
    ? sym_name_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_ver_prop =
    is_simple_property(section.find_property("symbol_version"));
  string sym_version = sym_ver_prop
    ? sym_ver_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_ver_regex_prop =
    is_simple_property(section.find_property("symbol_version_regexp"));
  string sym_ver_regex_str = sym_ver_regex_prop
    ? sym_ver_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr allow_other_aliases_prop =
    is_simple_property(section.find_property("allow_other_aliases"));
  string allow_other_aliases = allow_other_aliases_prop
    ? allow_other_aliases_prop->get_value()->as_string()
    : "";

  function_suppression::parameter_spec_sptr parm;
  function_suppression::parameter_specs_type parms;
  for (ini::config::property_vector::const_iterator p =
	 section.get_properties().begin();
       p != section.get_properties().end();
       ++p)
    if ((*p)->get_name() == "parameter")
      {
	ini::simple_property_sptr prop = is_simple_property(*p);
	assert(prop);
	if (parm = read_parameter_spec_from_string
	    (prop->get_value()->as_string()))
	  parms.push_back(parm);
      }

  function_suppression_sptr result;
  if (!label_str.empty()
      || !name.empty()
      || !name_regex_str.empty()
      || !file_name_regex_str.empty()
      || !file_name_not_regex_str.empty()
      || !soname_regex_str.empty()
      || !soname_not_regex_str.empty()
      || !return_type_name.empty()
      || !return_type_regex_str.empty()
      || !sym_name.empty()
      || !sym_name_regex_str.empty()
      || !sym_version.empty()
      || !sym_ver_regex_str.empty()
      || !parms.empty())
    result.reset(new function_suppression(label_str, name,
					  name_regex_str,
					  return_type_name,
					  return_type_regex_str,
					  parms,
					  sym_name,
					  sym_name_regex_str,
					  sym_version,
					  sym_ver_regex_str));
  if (result && !change_kind_str.empty())
    result->set_change_kind
      (function_suppression::parse_change_kind(change_kind_str));

  if (result && !allow_other_aliases.empty())
    result->set_allow_other_aliases(allow_other_aliases == "yes"
				    || allow_other_aliases == "true");

  if (!file_name_regex_str.empty())
    result->set_file_name_regex_str(file_name_regex_str);

  if (!file_name_not_regex_str.empty())
    result->set_file_name_not_regex_str(file_name_not_regex_str);

  if (!soname_regex_str.empty())
    result->set_soname_regex_str(soname_regex_str);

  if (!soname_not_regex_str.empty())
    result->set_soname_not_regex_str(soname_not_regex_str);

  return result;
}

// </function_suppression stuff>

// <variable_suppression stuff>

/// The type of the private data of the @ref variable_suppression
/// type.
class variable_suppression::priv
{
  friend class variable_suppression;

  change_kind				change_kind_;
  string				name_;
  string				name_regex_str_;
  mutable sptr_utils::regex_t_sptr	name_regex_;
  string				symbol_name_;
  string				symbol_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	symbol_name_regex_;
  string				symbol_version_;
  string				symbol_version_regex_str_;
  mutable sptr_utils::regex_t_sptr	symbol_version_regex_;
  string				type_name_;
  string				type_name_regex_str_;
  mutable sptr_utils::regex_t_sptr	type_name_regex_;

  priv(const string& name,
       const string& name_regex_str,
       const string& symbol_name,
       const string& symbol_name_regex_str,
       const string& symbol_version,
       const string& symbol_version_regex_str,
       const string& type_name,
       const string& type_name_regex_str)
    : change_kind_(ALL_CHANGE_KIND),
      name_(name),
      name_regex_str_(name_regex_str),
      symbol_name_(symbol_name),
      symbol_name_regex_str_(symbol_name_regex_str),
      symbol_version_(symbol_version),
      symbol_version_regex_str_(symbol_version_regex_str),
      type_name_(type_name),
      type_name_regex_str_(type_name_regex_str)
  {}

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::name_regex_str_.
  const sptr_utils::regex_t_sptr
  get_name_regex() const
  {
    if (!name_regex_ && !name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  name_regex_ = r;
      }
    return name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::symbol_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::symbol_name_regex_str_.
  const sptr_utils::regex_t_sptr
  get_symbol_name_regex() const
  {
    if (!symbol_name_regex_ && !symbol_name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    symbol_name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  symbol_name_regex_ = r;
      }
    return symbol_name_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::symbol_version_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::symbol_version_regex_str_.
  const sptr_utils::regex_t_sptr
  get_symbol_version_regex()  const
  {
    if (!symbol_version_regex_ && !symbol_version_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    symbol_version_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  symbol_version_regex_ = r;
      }
    return symbol_version_regex_;
  }

  /// Getter for a pointer to a regular expression object built from
  /// the regular expression string
  /// variable_suppression::priv::type_name_regex_str_.
  ///
  /// If that string is empty, then an empty regular expression object
  /// pointer is returned.
  ///
  /// @return a pointer to the regular expression object of
  /// variable_suppression::priv::type_name_regex_str_.
  const sptr_utils::regex_t_sptr
  get_type_name_regex() const
  {
    if (!type_name_regex_ && !type_name_regex_str_.empty())
      {
	sptr_utils::regex_t_sptr r = sptr_utils::build_sptr<regex_t>();
	if (regcomp(r.get(),
		    type_name_regex_str_.c_str(),
		    REG_EXTENDED) == 0)
	  type_name_regex_ = r;
      }
    return type_name_regex_;
  }
};// end class variable_supppression::priv

/// Constructor for the @ref variable_suppression type.
///
/// @param label an informative text string that the evalution code
/// might use to designate this variable suppression specification in
/// error messages.  This parameter might be empty, in which case it's
/// ignored at evaluation time.
///
/// @param name the name of the variable the user wants the current
/// specification to designate.  This parameter might be empty, in
/// which case it's ignored at evaluation time.
///
/// @param name_regex_str if @p name is empty, this parameter is a
/// regular expression for a family of names of variables the user
/// wants the current specification to designate.  If @p name is not
/// empty, then this parameter is ignored at evaluation time.  This
/// parameter might be empty, in which case it's ignored at evaluation
/// time.
///
/// @param symbol_name the name of the symbol of the variable the user
/// wants the current specification to designate.  This parameter
/// might be empty, in which case it's ignored at evaluation time.
///
/// @param symbol_name_str if @p symbol_name is empty, this parameter
/// is a regular expression for a family of names of symbols of
/// variables the user wants the current specification to designate.
/// If @p symbol_name is not empty, then this parameter is ignored at
/// evaluation time.  This parameter might be empty, in which case
/// it's ignored at evaluation time.
///
/// @param symbol_version the version of the symbol of the variable
/// the user wants the current specification to designate.  This
/// parameter might be empty, in which case it's ignored at evaluation
/// time.
///
/// @param symbol_version_regex if @p symbol_version is empty, then
/// this parameter is a regular expression for a family of versions of
/// symbol for the variables the user wants the current specification
/// to designate.  If @p symbol_version is not empty, then this
/// parameter is ignored at evaluation time.  This parameter might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param type_name the name of the type of the variable the user
/// wants the current specification to designate.  This parameter
/// might be empty, in which case it's ignored at evaluation time.
///
/// @param type_name_regex_str if @p type_name is empty, then this
/// parameter is a regular expression for a family of type names of
/// variables the user wants the current specification to designate.
/// If @p type_name is not empty, then this parameter is ignored at
/// evluation time.  This parameter might be empty, in which case it's
/// ignored at evaluation time.
variable_suppression::variable_suppression(const string& label,
					   const string& name,
					   const string& name_regex_str,
					   const string& symbol_name,
					   const string& symbol_name_regex_str,
					   const string& symbol_version,
					   const string& symbol_version_regex,
					   const string& type_name,
					   const string& type_name_regex_str)
  : suppression_base(label),
    priv_(new priv(name, name_regex_str,
		   symbol_name, symbol_name_regex_str,
		   symbol_version, symbol_version_regex,
		   type_name, type_name_regex_str))
{}

/// Virtual destructor for the @erf variable_suppression type.
/// variable_suppression type.
variable_suppression::~variable_suppression()
{}

/// Parses a string containing the content of the "change-kind"
/// property and returns the an instance of @ref
/// variable_suppression::change_kind as a result.
///
/// @param s the string to parse.
///
/// @return the resulting @ref variable_suppression::change_kind.
variable_suppression::change_kind
variable_suppression::parse_change_kind(const string& s)
{
  if (s == "variable-subtype-change")
    return VARIABLE_SUBTYPE_CHANGE_KIND;
  else if (s == "added-variable")
    return ADDED_VARIABLE_CHANGE_KIND;
  else if (s == "deleted-variable")
    return DELETED_VARIABLE_CHANGE_KIND;
  else if (s == "all")
    return ALL_CHANGE_KIND;
  else
    return UNDEFINED_CHANGE_KIND;
}

/// Getter of the "change_king" property.
///
/// @return the value of the "change_kind" property.
variable_suppression::change_kind
variable_suppression::get_change_kind() const
{return priv_->change_kind_;}

/// Setter of the "change_kind" property.
///
/// @param k the new value of of the change_kind.
void
variable_suppression::set_change_kind(change_kind k)
{priv_->change_kind_ = k;}

/// Getter for the name of the variable the user wants the current
/// specification to designate.  This property might be empty, in
/// which case it's ignored at evaluation time.
///
/// @return the name of the variable.
const string&
variable_suppression::get_name() const
{return priv_->name_;}

/// Setter for the name of the variable the user wants the current
/// specification to designate.  This property might be empty, in
/// which case it's ignored at evaluation time.
///
/// @param n the new name of the variable to set.
void
variable_suppression::set_name(const string& n)
{priv_->name_ = n;}

/// Getter for the regular expression for a family of names of
/// variables the user wants the current specification to designate.
/// If the variable name as returned by
/// variable_suppression::get_name() is not empty, then this property
/// is ignored at evaluation time.  This property might be empty, in
/// which case it's ignored at evaluation time.
///
/// @return the regular expression for the variable name.
const string&
variable_suppression::get_name_regex_str() const
{return priv_->name_regex_str_;}

/// Setter for the regular expression for a family of names of
/// variables the user wants the current specification to designate.
/// If the variable name as returned by
/// variable_suppression::get_name() is not empty, then this property
/// is ignored at evaluation time.  This property might be empty, in
/// which case it's ignored at evaluation time.
///
/// @param r the new regular expression for the variable name.
void
variable_suppression::set_name_regex_str(const string& r)
{priv_->name_regex_str_ = r;}

/// Getter for the name of the symbol of the variable the user wants
/// the current specification to designate.
///
/// This property might be empty, in which case it is ignored at
/// evaluation time.
///
/// @return the name of the symbol of the variable.
const string&
variable_suppression::get_symbol_name() const
{return priv_->symbol_name_;}

/// Setter for the name of the symbol of the variable the user wants
/// the current specification to designate.
///
/// This property might be empty, in which case it is ignored at
/// evaluation time.
///
/// @param n the new name of the symbol of the variable.
void
variable_suppression::set_symbol_name(const string& n)
{priv_->symbol_name_ = n;}

/// Getter of the regular expression for a family of symbol names of
/// the variables this specification is about to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.  Otherwise, it is taken in account iff the
/// property returned by variable_suppression::get_symbol_name() is
/// empty.
///
/// @return the regular expression for a symbol name of the variable.
const string&
variable_suppression::get_symbol_name_regex_str() const
{return priv_->symbol_name_regex_str_;}

/// Setter of the regular expression for a family of symbol names of
/// the variables this specification is about to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.  Otherwise, it is taken in account iff the
/// property returned by variable_suppression::get_symbol_name() is
/// empty.
///
/// @param r the regular expression for a symbol name of the variable.
void
variable_suppression::set_symbol_name_regex_str(const string& r)
{priv_->symbol_name_regex_str_ = r;}

/// Getter for the version of the symbol of the variable the user
/// wants the current specification to designate.  This property might
/// be empty, in which case it's ignored at evaluation time.
///
/// @return the symbol version of the variable.
const string&
variable_suppression::get_symbol_version() const
{return priv_->symbol_version_;}

/// Setter for the version of the symbol of the variable the user
/// wants the current specification to designate.  This property might
/// be empty, in which case it's ignored at evaluation time.
///
/// @return the new symbol version of the variable.
void
variable_suppression::set_symbol_version(const string& v)
{priv_->symbol_version_ = v;}

/// Getter of the regular expression for a family of versions of
/// symbol for the variables the user wants the current specification
/// to designate.  If @p symbol_version is not empty, then this
/// property is ignored at evaluation time.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @return the regular expression of the symbol version of the
/// variable.
const string&
variable_suppression::get_symbol_version_regex_str() const
{return priv_->symbol_version_regex_str_;}

/// Setter of the regular expression for a family of versions of
/// symbol for the variables the user wants the current specification
/// to designate.  If @p symbol_version is not empty, then this
/// property is ignored at evaluation time.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param v the new regular expression of the symbol version of the
/// variable.
void
variable_suppression::set_symbol_version_regex_str(const string& r)
{priv_->symbol_version_regex_str_ = r;}

/// Getter for the name of the type of the variable the user wants the
/// current specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @return the name of the variable type.
const string&
variable_suppression::get_type_name() const
{return priv_->type_name_;}

/// Setter for the name of the type of the variable the user wants the
/// current specification to designate.
///
/// This property might be empty, in which case it's ignored at
/// evaluation time.
///
/// @param n the new name of the variable type.
void
variable_suppression::set_type_name(const string& n)
{priv_->type_name_ = n;}

/// Getter for the regular expression for a family of type names of
/// variables the user wants the current specification to designate.
///
/// If the type name as returned by
/// variable_suppression::get_type_name() is not empty, then this
/// property is ignored at evaluation time.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @return the regular expression of the variable type name.
const string&
variable_suppression::get_type_name_regex_str() const
{return priv_->type_name_regex_str_;}

/// Setter for the regular expression for a family of type names of
/// variables the user wants the current specification to designate.
///
/// If the type name as returned by
/// variable_suppression::get_type_name() is not empty, then this
/// property is ignored at evaluation time.  This property might be
/// empty, in which case it's ignored at evaluation time.
///
/// @param r the regular expression of the variable type name.
void
variable_suppression::set_type_name_regex_str(const string& r)
{priv_->type_name_regex_str_ = r;}

/// Evaluate this suppression specification on a given diff node and
/// say if the diff node should be suppressed or not.
///
/// @param diff the diff node to evaluate this suppression
/// specification against.
///
/// @return true if @p diff should be suppressed.
bool
variable_suppression::suppresses_diff(const diff* diff) const
{
  const var_diff* d = is_var_diff(diff);
  if (!d)
    return false;

  var_decl_sptr fv = is_var_decl(is_decl(d->first_subject())),
    sv = is_var_decl(is_decl(d->second_subject()));

  assert(fv && sv);

  return (suppresses_variable(fv,
			      VARIABLE_SUBTYPE_CHANGE_KIND,
			      diff->context())
	  || suppresses_variable(sv,
				 VARIABLE_SUBTYPE_CHANGE_KIND,
				 diff->context()));
}

/// Evaluate the current variable suppression specification on a given
/// @ref var_decl and say if a report about a change involving this
/// @ref var_decl should be suppressed or not.
///
/// @param var the @ref var_decl to evaluate this suppression
/// specification against.
///
/// @param k the kind of variable change @p var is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the variable @p
/// var should be suppressed.
bool
variable_suppression::suppresses_variable(const var_decl* var,
					  change_kind k,
					  const diff_context_sptr ctxt) const
{
  if (!(get_change_kind() & k))
    return false;

  // Check if the name and soname of the binaries match
  if (ctxt)
    {
      // Check if the name of the binaries match
      if (!names_of_binaries_match(*ctxt))
	return false;

      // Check if the soname of the binaries match
      if (!sonames_of_binaries_match(*ctxt))
	return false;
    }

  string var_name = var->get_qualified_name();

    // Check for "name" property match.
  if (!get_name().empty())
    {
      if (get_name() != var_name)
	return false;
    }
  else
    {
      // If the "name" property is empty, then consider checking for the
      // "name_regex" property match
      if (get_name().empty())
	{
	  const sptr_utils::regex_t_sptr name_regex = priv_->get_name_regex();
	  if (name_regex
	      && (regexec(name_regex.get(), var_name.c_str(),
			  0, NULL, 0) != 0))
	    return false;
	}
    }

  // Check for the symbol_name property match.
  string var_sym_name = var->get_symbol() ? var->get_symbol()->get_name() : "";
  if (!get_symbol_name().empty())
    {
      if (get_symbol_name() != var_sym_name)
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr sym_name_regex =
	priv_->get_symbol_name_regex();
      if (sym_name_regex
	  && (regexec(sym_name_regex.get(), var_sym_name.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  // Check for symbol_version and symbol_version_regexp property match
  string var_sym_version =
    var->get_symbol() ? var->get_symbol()->get_version().str() : "";
  if (!get_symbol_version().empty())
    {
      if (get_symbol_version() != var_sym_version)
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      var_sym_version.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }

  // Check for the "type_name" and tye_name_regex properties match.
  string var_type_name =
    get_type_declaration(var->get_type())->get_qualified_name();

  if (!get_type_name().empty())
    {
      if (get_type_name() != var_type_name)
	return false;
    }
  else
    {
      if (get_type_name().empty())
	{
	  const sptr_utils::regex_t_sptr type_name_regex =
	    priv_->get_type_name_regex();
	  if (type_name_regex
	      && (regexec(type_name_regex.get(), var_type_name.c_str(),
			  0, NULL, 0) != 0))
	    return false;
	}
    }

  return true;
}

/// Evaluate the current variable suppression specification on a given
/// @ref var_decl and say if a report about a change involving this
/// @ref var_decl should be suppressed or not.
///
/// @param var the @ref var_decl to evaluate this suppression
/// specification against.
///
/// @param k the kind of variable change @p var is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the variable @p
/// var should be suppressed.
bool
variable_suppression::suppresses_variable(const var_decl_sptr var,
					  change_kind k,
					  const diff_context_sptr ctxt) const
{return suppresses_variable(var.get(), k, ctxt);}

/// Evaluate the current variable suppression specification on a given
/// @ref elf_symbol and say if a report about a change involving this
/// @ref elf_symbol should be suppressed or not.
///
/// @param sym the @ref elf_symbol to evaluate this suppression
/// specification against.
///
/// @param k the kind of variable change @p sym is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the symbol @p
/// sym should be suppressed.
bool
variable_suppression::suppresses_variable_symbol(const elf_symbol* sym,
						 change_kind k,
						 const diff_context_sptr ctxt) const
{
  if (!sym)
    return false;

  if (!(get_change_kind() & k))
    return false;

  if (!sym->is_variable())
    return false;

  assert(k & ADDED_VARIABLE_CHANGE_KIND
	 || k & DELETED_VARIABLE_CHANGE_KIND);

  // Check if the name and soname of the binaries match
  if (ctxt)
    {
      // Check if the name of the binaries match
      if (!names_of_binaries_match(*ctxt))
	return false;

      // Check if the soname of the binaries match
      if (!sonames_of_binaries_match(*ctxt))
	return false;
    }

  string sym_name = sym->get_name(), sym_version = sym->get_version().str();

  bool no_symbol_name = false, no_symbol_version = false;

  // Consider the symbol name
  if (!get_name().empty())
    {
      if (get_name() != sym_name)
	return false;
    }
  else if (!get_symbol_name().empty())
    {
      if (get_symbol_name() != sym_name)
	return false;
    }
  else if (!get_symbol_name_regex_str().empty())
    {
      const sptr_utils::regex_t_sptr sym_name_regex =
	priv_->get_symbol_name_regex();
      if (sym_name_regex
	  && (regexec(sym_name_regex.get(), sym_name.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }
  else
    no_symbol_name = true;


  // Consider the symbol version.
  if (!get_symbol_version().empty())
    {
      if (get_symbol_version() != sym_version)
	return false;
    }
  else if (!get_symbol_version_regex_str().empty())
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      sym_version.c_str(),
		      0, NULL, 0) != 0))
	return false;
    }
  else
    no_symbol_version = true;

  if (no_symbol_name && no_symbol_version)
    return false;

  return true;
}

/// Evaluate the current variable suppression specification on a given
/// @ref elf_symbol and say if a report about a change involving this
/// @ref elf_symbol should be suppressed or not.
///
/// @param sym the @ref elf_symbol to evaluate this suppression
/// specification against.
///
/// @param k the kind of variable change @p sym is supposed to have.
///
/// @param ctxt the context of the current diff.
///
/// @return true iff a report about a change involving the symbol @p
/// sym should be suppressed.
bool
variable_suppression::suppresses_variable_symbol(const elf_symbol_sptr sym,
						 change_kind k,
						 const diff_context_sptr ctxt) const
{return suppresses_variable_symbol(sym.get(), k, ctxt);}

/// Test if an instance of @ref suppression is an instance of @ref
/// variable_suppression.
///
/// @param suppr the instance of @ref suppression to test for.
///
/// @return if @p suppr is an instance of @ref variable_suppression, then
/// return the sub-object of the @p suppr of type @ref
/// variable_suppression, otherwise return a nil pointer.
variable_suppression_sptr
is_variable_suppression(const suppression_sptr s)
{return dynamic_pointer_cast<variable_suppression>(s);}

/// The bitwise 'and' operator for the enum @ref
/// variable_suppression::change_kind.
///
/// @param l the first operand of the 'and' operator.
///
/// @param r the second operand of the 'and' operator.
///
/// @return the result of 'and' operation on @p l and @p r.
variable_suppression::change_kind
operator&(variable_suppression::change_kind l,
	  variable_suppression::change_kind r)
{
  return static_cast<variable_suppression::change_kind>
    (static_cast<unsigned>(l) & static_cast<unsigned>(r));
}

/// The bitwise 'or' operator for the enum @ref
/// variable_suppression::change_kind.
///
/// @param l the first operand of the 'or' operator.
///
/// @param r the second operand of the 'or' operator.
///
/// @return the result of 'or' operation on @p l and @p r.
variable_suppression::change_kind
operator|(variable_suppression::change_kind l,
	  variable_suppression::change_kind r)
{
    return static_cast<variable_suppression::change_kind>
    (static_cast<unsigned>(l) | static_cast<unsigned>(r));
}

/// Parse variable suppression specification, build a resulting @ref
/// variable_suppression type and return a shared pointer to that
/// object.
///
/// @return a shared pointer to the newly built @ref
/// variable_suppression.  If the variable suppression specification
/// could not be parsed then a nil shared pointer is returned.
static variable_suppression_sptr
read_variable_suppression(const ini::config::section& section)
{
  variable_suppression_sptr result;

  if (section.get_name() != "suppress_variable")
    return result;

  ini::simple_property_sptr change_kind_prop =
    is_simple_property(section.find_property("change_kind"));
  string change_kind_str = change_kind_prop
    ? change_kind_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr label_prop =
    is_simple_property(section.find_property("label"));
  string label_str = (label_prop
		      ? label_prop->get_value()->as_string()
		      : "");

  ini::simple_property_sptr file_name_regex_prop =
    is_simple_property(section.find_property("file_name_regexp"));
  string file_name_regex_str =
    file_name_regex_prop ? file_name_regex_prop->get_value()->as_string() : "";

 ini::simple_property_sptr file_name_not_regex_prop =
    is_simple_property(section.find_property("file_name_not_regexp"));
  string file_name_not_regex_str =
    file_name_not_regex_prop
    ? file_name_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr soname_regex_prop =
    is_simple_property(section.find_property("soname_regexp"));
  string soname_regex_str =
    soname_regex_prop ? soname_regex_prop->get_value()->as_string() : "";

  ini::simple_property_sptr soname_not_regex_prop =
    is_simple_property(section.find_property("soname_not_regexp"));
  string soname_not_regex_str =
    soname_not_regex_prop
    ? soname_not_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr name_prop =
    is_simple_property(section.find_property("name"));
  string name_str = (name_prop
		     ? name_prop->get_value()->as_string()
		     : "");

  ini::simple_property_sptr name_regex_prop =
    is_simple_property(section.find_property("name_regexp"));
  string name_regex_str = (name_regex_prop
			   ? name_regex_prop->get_value()->as_string()
			   : "");

  ini::simple_property_sptr sym_name_prop =
    is_simple_property(section.find_property("symbol_name"));
  string symbol_name = (sym_name_prop
			? sym_name_prop->get_value()->as_string()
			: "");

  ini::simple_property_sptr sym_name_regex_prop =
    is_simple_property(section.find_property("symbol_name_regexp"));
  string symbol_name_regex_str = sym_name_regex_prop
    ? sym_name_regex_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_version_prop =
    is_simple_property(section.find_property("symbol_version"));
  string symbol_version = sym_version_prop
    ? sym_version_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr sym_version_regex_prop =
    is_simple_property(section.find_property("symbol_version_regexp"));
  string symbol_version_regex_str = sym_version_regex_prop
    ? sym_version_regex_prop->get_value()->as_string()
     : "";

  ini::simple_property_sptr type_name_prop =
    is_simple_property(section.find_property("type_name"));
  string type_name_str = type_name_prop
    ? type_name_prop->get_value()->as_string()
    : "";

  ini::simple_property_sptr type_name_regex_prop =
    is_simple_property(section.find_property("type_name_regexp"));
  string type_name_regex_str = type_name_regex_prop
    ? type_name_regex_prop->get_value()->as_string()
     : "";

  if (label_str.empty()
      && name_str.empty()
      && name_regex_str.empty()
      && file_name_regex_str.empty()
      && file_name_not_regex_str.empty()
      && soname_regex_str.empty()
      && soname_not_regex_str.empty()
      && symbol_name.empty()
      && symbol_name_regex_str.empty()
      && symbol_version.empty()
      && symbol_version_regex_str.empty()
      && type_name_str.empty()
      && type_name_regex_str.empty())
    return result;

  result.reset(new variable_suppression(label_str, name_str, name_regex_str,
					symbol_name, symbol_name_regex_str,
					symbol_version, symbol_version_regex_str,
					type_name_str, type_name_regex_str));

  if (result && !change_kind_str.empty())
    result->set_change_kind
      (variable_suppression::parse_change_kind(change_kind_str));

  if (!file_name_regex_str.empty())
    result->set_file_name_regex_str(file_name_regex_str);

  if (!file_name_not_regex_str.empty())
    result->set_file_name_not_regex_str(file_name_not_regex_str);

  if (!soname_regex_str.empty())
    result->set_soname_regex_str(soname_regex_str);

  if (!soname_not_regex_str.empty())
    result->set_soname_not_regex_str(soname_not_regex_str);

  return result;
}

// </variable_suppression stuff>

// <file_suppression stuff>

/// Constructor for the the @ref file_suppression type.
///
/// @param label the label of the suppression directive.
///
/// @param fname_regex_str the regular expression string that
/// designates the file name that instances of @ref file_suppression
/// should match.
///
/// @param fname_not_regex_str the regular expression string that
/// designates the file name that instances of @ref file_suppression
/// shoult *NOT* match.  In other words, this file_suppression should
/// be activated if its file name does not match the regular
/// expression @p fname_not_regex_str.
file_suppression::file_suppression(const string& label,
				   const string& fname_regex_str,
				   const string& fname_not_regex_str)
  : suppression_base(label,
		     fname_regex_str,
		     fname_not_regex_str)
{}

/// Test if instances of this @ref file_suppression suppresses a
/// certain instance of @ref diff.
///
/// This function always returns false because, obviously, a
/// file_suppression is meants to prevents Abigail tools from loading
/// some files.  It is not meant to act on instance of @ref diff.
/// @return false.
bool
file_suppression::suppresses_diff(const diff*) const
{return false;}

/// Test if a instances of this @ref file_suppression suppresses a
/// given file.
///
/// @param file_path the file path to test against.
///
/// @return true iff this file_suppression matches the file path @p
/// file_path.
bool
file_suppression::suppresses_file(const string& file_path)
{
  if (file_path.empty())
    return false;

  string fname;
  tools_utils::base_name(file_path, fname);

  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_file_name_regex())
    if (regexec(regexp.get(), fname.c_str(), 0, NULL, 0) != 0)
      return false;

  if (sptr_utils::regex_t_sptr regexp =
      suppression_base::priv_->get_file_name_not_regex())
    if (regexec(regexp.get(), fname.c_str(), 0, NULL, 0) == 0)
      return false;

  return true;
}

/// Destructor of @ref file_suppression.
file_suppression::~file_suppression()
{
}

/// Read a file suppression from an instance of ini::config::section
/// and build a @ref type_suppression as a result.
///
/// @param section the section (from an ini file) to read the file
/// suppression from.
///
/// @return file_suppression_sptr.
static file_suppression_sptr
read_file_suppression(const ini::config::section& section)
{
  file_suppression_sptr result;

  if (section.get_name() != "suppress_file")
    return result;

  ini::simple_property_sptr label_prop =
    is_simple_property(section.find_property("label"));
  string label_str = (label_prop
		      ? label_prop->get_value()->as_string()
		      : "");

  ini::simple_property_sptr file_name_regex_prop =
    is_simple_property(section.find_property("file_name_regexp"));
  string file_name_regex_str =
    file_name_regex_prop ? file_name_regex_prop->get_value()->as_string() : "";

 ini::simple_property_sptr file_name_not_regex_prop =
    is_simple_property(section.find_property("file_name_not_regexp"));
  string file_name_not_regex_str =
    file_name_not_regex_prop
    ? file_name_not_regex_prop->get_value()->as_string()
    : "";

  if (file_name_regex_str.empty()
      && file_name_not_regex_str.empty())
    return result;

  result.reset(new file_suppression(label_str,
				    file_name_regex_str,
				    file_name_not_regex_str));

  return result;
}

/// Test if a given suppression specification is a file suppression
/// specification.
///
/// @param s the instance of @ref suppression_base to test.
///
/// @return the instance of @ref file_suppression that @p s points to,
/// iff s is an instance of @ref file_suppression.  Otherwise, returns
/// nil.
file_suppression_sptr
is_file_suppression(const suppression_sptr s)
{return dynamic_pointer_cast<file_suppression>(s);}


/// Test if a given file path is "suppressed" by at least one file
/// suppression specification among a vector of suppression
/// specifications.
///
/// @param file_path the file path to test.
///
/// @param sprs the vector of suppressions to use to test if one of
/// them at lease matches the file path @p file_path.
///
/// @return a pointer to the first instance of @ref file_suppression
/// that matches @p file_path, or nil if no file suppression matches.
file_suppression_sptr
file_is_suppressed(const string& file_path,
		   const suppressions_type& sprs)
{
  for (suppressions_type::const_iterator i = sprs.begin(); i != sprs.end(); ++i)
    if (file_suppression_sptr s = is_file_suppression(*i))
      if (s->suppresses_file(file_path))
	return s;

  return file_suppression_sptr();
}
// </file_suppression stuff>
}// end namespace suppr
} // end namespace abigail
