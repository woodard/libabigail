// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2015 Red Hat, Inc.
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
/// This contains the implementation of the comparison engine of
/// libabigail.

#include <ctype.h>
#include <algorithm>
#include <sstream>
#include "abg-hash.h"
#include "abg-comparison.h"
#include "abg-comp-filter.h"
#include "abg-sptr-utils.h"
#include "abg-ini.h"

namespace abigail
{

namespace comparison
{

///
///
///@defgroup DiffNode Internal Representation of the comparison engine
/// @{
///
/// @brief How changes are represented in libabigail's comparison engine.
///
///@par diff nodes
///
/// The internal representation of the comparison engine is basically
/// a graph of @ref instances of @ref diff node.  We refer to these
/// just as <em>diff nodes</em>.  A diff node represents a change
/// between two ABI artifacts represented by instances of types of the
/// abigail::ir namespace.  These two artifacts that are being
/// compared are called the <em>subjects of the diff</em>.
///
/// The types of that IR are in the abigail::comparison namespace.
///
///@par comparing diff nodes
///
/// Comparing two instances of @ref diff nodes amounts to comparing
/// the subject of the diff.  In other words, two @ref diff nodes are
/// equal if and only if their subjects are equal.  Thus, two @ref
/// diff nodes can have different memory addresses and yet be equal.
///
///@par diff reporting and context
///
/// A diff node can be serialized to an output stream to express, in
/// a human-readable textual form, the different changes that exist
/// between its two subjects.  This is done by invoking the
/// diff::report() method.  That reporting is controlled by several
/// parameters that are conceptually part of the context of the diff.
/// That context is materialized by an instance of the @ref
/// diff_context type.
///
/// Please note that the role of the instance(s) of @ref diff_context
/// is boreader than just controlling the reporting of @ref diff
/// nodes.  Basically, a @ref diff node itself is created following
/// behaviours that are controlled by a particular instance of
/// diff_context.  A diff node is created in a particular diff
/// context, so to speak.
///
/// @}
///

///
///@defgroup CanonicalDiff Canonical diff tree nodes
/// @{
///
/// @brief How equivalent diff nodes are quickly spotted.
///
/// @par Equivalence of diff nodes.
///
/// Each @ref diff node has a property named <em>Canonical Diff
/// Node</em>.  If \c D is a diff node, the canonical diff node of @c
/// D, noted @c C(D) is a particular diff node that is equal to @c D.
/// Thus, a fast way to compare two @ref diff node is to perform a
/// pointer comparison of their canonical diff nodes.
///
/// A set of equivalent @ref diff nodes is a set of diff nodes that
/// all have the same canonical node.  All the nodes of that set are
/// equal.
///
/// A canonical node is registereded for a given diff node by invoking
/// the method diff_context::initialize_canonical_diff().
///
/// Please note that the diff_context holds all the canonical diffs
/// that got registered through it.  Thus, the life time of all of
/// canonical diff objects is the same as the life time of the @ref
/// diff_context they relate to.
///
/// @}
///

// Inject types from outside in here.
using std::vector;
using std::tr1::dynamic_pointer_cast;
using std::tr1::static_pointer_cast;
using abigail::sptr_utils::noop_deleter;

/// Convenience typedef for a pair of decls.
typedef std::pair<const decl_base_sptr, const decl_base_sptr> decls_type;

/// A hashing functor for @ef decls_type.
struct decls_hash
{
  size_t
  operator()(const decls_type& d) const
  {
    size_t h1 = d.first ? d.first->get_hash() : 0;
    size_t h2 = d.second ? d.second->get_hash() : 0;
    return hashing::combine_hashes(h1, h2);
  }
};

/// An equality functor for @ref decls_type.
struct decls_equal
{
  bool
  operator()(const decls_type d1, const decls_type d2) const
  {return d1.first == d2.first && d1.second == d2.second;}
};

/// A convenience typedef for a map of @ref decls_type and diff_sptr.
typedef unordered_map<decls_type, diff_sptr, decls_hash, decls_equal>
decls_diff_map_type;

/// The overloaded or operator for @ref visiting_kind.
visiting_kind
operator|(visiting_kind l, visiting_kind r)
{return static_cast<visiting_kind>(static_cast<unsigned>(l)
				   | static_cast<unsigned>(r));}

visiting_kind
operator&(visiting_kind l, visiting_kind r)
{
  return static_cast<visiting_kind>(static_cast<unsigned>(l)
				    & static_cast<unsigned>(r));
}

visiting_kind
operator~(visiting_kind l)
{return static_cast<visiting_kind>(~static_cast<unsigned>(l));}

/// This is a subroutine of a *::report() function.
///
/// If the diff about two subjects S1 and S2 was reported earlier or
/// is being reported, emit a diagnostic message about this and return
/// from the current diff reporting function.
///
/// @param S1 the first diff subject to take in account.
///
/// @param S2 the second diff subject to take in account.
#define RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(S1, S2) \
  do {									\
    if (diff_sptr _diff_ = context()->get_canonical_diff_for(S1, S2))	\
      if (_diff_->currently_reporting() || _diff_->reported_once())	\
	{								\
	  if (_diff_->currently_reporting())				\
	    out << indent << "details are being reported\n";		\
	  else								\
	    out << indent << "details were reported earlier\n";	\
	  return ;							\
	}								\
  } while (false)

/// This is a subroutine of a *::report() function.
///
/// If a given diff was reported earlier or is being reported, emit a
/// diagnostic message about this and return from the current diff
/// reporting function.
///
/// @param S1 the first diff subject to take in account.
///
/// @param S2 the second diff subject to take in account.
///
/// @param INTRO_TEXT the introductory text that precedes the
/// diagnostic.
#define RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(D, INTRO_TEXT) \
  do {									\
    if (diff_sptr _diff_ = context()->get_canonical_diff_for(D))	\
      if (_diff_->currently_reporting() || _diff_->reported_once())	\
	{								\
	  string _name_ = _diff_->first_subject()->get_pretty_representation(); \
	  if (_diff_->currently_reporting())				\
	    out << indent << INTRO_TEXT << " '" << _name_ << "' changed; " \
	      "details are being reported\n";				\
	  else								\
	    out << indent << INTRO_TEXT << " '" << _name_ << "' changed, " \
	      "as reported earlier\n";					\
	  return ;							\
	}								\
} while (false)

/// This is a subroutine of a *::report() function.
///
/// If the diff about two subjects S1 and S2 was reported earlier or
/// is being reported, emit a diagnostic message about this and return
/// from the current diff reporting function.
///
///
/// @param INTRO_TEXT the introductory text that precedes the
/// diagnostic.
#define RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(S1, S2, INTRO_TEXT) \
    do {								\
      if (diff_sptr _diff_ = context()->get_canonical_diff_for(S1, S2)) \
	if (_diff_->currently_reporting() || _diff_->reported_once())	\
	  {								\
	    string _name_ = _diff_->first_subject()->get_pretty_representation(); \
	    if (_diff_->currently_reporting())				\
	      out << indent << INTRO_TEXT << " '" << _name_ << "' changed; " \
		"details are being reported\n";				\
	    else							\
	      out << indent << INTRO_TEXT << " '" << _name_ << "' changed, " \
		"as reported earlier\n";				\
	    return ;							\
	  } \
    } while (false)

static void
sort_string_function_decl_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted);

static void
sort_string_var_diff_sptr_map(const string_var_diff_sptr_map& map,
			      var_diff_sptrs_type& sorted);

static void
sort_unsigned_data_member_diff_sptr_map(const unsigned_var_diff_sptr_map map,
					var_diff_sptrs_type& sorted);

static void
sort_string_data_member_diff_sptr_map(const string_var_diff_sptr_map& map,
				      var_diff_sptrs_type& sorted);

static void
sort_string_virtual_member_function_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted);

static void
sort_string_diff_sptr_map(const string_diff_sptr_map& map,
			  diff_sptrs_type& sorted);

static void
sort_string_base_diff_sptr_map(const string_base_diff_sptr_map& map,
			       base_diff_sptrs_type& sorted);

static type_base_sptr
get_leaf_type(qualified_type_def_sptr t);

/// Test if a diff node is about differences between types.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual type_diff_base* that @p diff extends, iff it is
/// about differences between types.
static const type_diff_base*
is_type_diff(const diff* diff)
{return dynamic_cast<const type_diff_base*>(diff);}

/// Test if a diff node is about differences between declarations.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual decl_diff_base @p diff extends,
/// iff it is about differences between declarations.
static const decl_diff_base*
is_decl_diff(const diff* diff)
{return dynamic_cast<const decl_diff_base*>(diff);}

/// Test if a diff node is about differences between variables.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual var_diff that @p diff is a type
/// of, iff it is about differences between variables.
static const var_diff*
is_var_diff(const diff* diff)
{
  const var_diff* d = dynamic_cast<const var_diff*>(diff);
  if (d)
    assert(is_decl_diff(diff));
  return d;
}

/// Test if a diff node is about differences between functions.
///
/// @param diff the diff node to test.
///
/// @return a pointer to the actual var_diff that @p diff is a type
/// of, iff it is about differences between variables.
static const function_decl_diff*
is_function_decl_diff(const diff* diff)
{
  const function_decl_diff *d = dynamic_cast<const function_decl_diff*>(diff);
  if (d)
    assert(is_decl_diff(diff));
  return d;
}

/// The default traverse function.
///
/// @return true.
bool
diff_traversable_base::traverse(diff_node_visitor&)
{return true;}

// <suppression_base stuff>

/// The private data of @ref suppression_base.
class suppression_base::priv
{
  string label_;

public:
  priv()
  {}

  priv(const string& label)
    : label_(label)
  {}

  friend class suppression_base;
}; // end clas suppression_base::priv

/// Constructor for @ref suppression_base
///
/// @param a label for the suppression.  This represents just a
/// comment.
suppression_base::suppression_base(const string& label)
  : priv_(new priv(label))
{}

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

suppression_base::~suppression_base()
{}

static type_suppression_sptr
read_type_suppression(const ini::config::section& section);

static function_suppression_sptr
read_function_suppression(const ini::config::section& section);

static variable_suppression_sptr
read_variable_suppression(const ini::config::section& section);

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
	|| (s = read_variable_suppression(**i)))
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

  priv();

public:
  priv(const string&			type_name_regexp,
       const string&			type_name,
       bool				consider_type_kind,
       type_suppression::type_kind	type_kind)
    : type_name_regex_str_(type_name_regexp),
      type_name_(type_name),
      consider_type_kind_(consider_type_kind),
      type_kind_(type_kind)
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
	    sptr_utils::regex_t_sptr r(new regex_t);
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
		   /*consider_type_kings=*/false,
		   /*type_kind=*/CLASS_TYPE_KIND))
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

  type_base_sptr ft, st;
  ft = is_type(d->first_subject());
  st = is_type(d->second_subject());
  assert(ft && st);

  // If the suppression should consider type kind, then well, check
  // for that.
  if (get_consider_type_kind())
    {
      type_kind k = get_type_kind();
      switch (k)
	{
	case type_suppression::UNKNOWN_TYPE_KIND:
	case type_suppression::CLASS_TYPE_KIND:
	  if (!is_class_type(ft) && !is_class_type(st))
	    return false;
	  break;
	case type_suppression::STRUCT_TYPE_KIND:
	  {
	    class_decl_sptr fc = is_class_type(ft), sc = is_class_type(st);
	    if ((!fc && !sc)
		|| (fc && !fc->is_struct() && sc && !sc->is_struct()))
	      return false;
	  }
	  break;
	case type_suppression::UNION_TYPE_KIND:
	  // We do not support unions yet.  When we do, we should
	  // replace the abort here by a "break;" statement.
	  abort();
	case type_suppression::ENUM_TYPE_KIND:
	  if (!is_enum_type(ft) && !is_enum_type(st))
	    return false;
	  break;
	case type_suppression::ARRAY_TYPE_KIND:
	  if (!is_array_type(ft) && !is_array_type(st))
	    return false;
	  break;
	case type_suppression::TYPEDEF_TYPE_KIND:
	  if (!is_typedef(ft) && !is_typedef(st))
	    return false;
	  break;
	case type_suppression::BUILTIN_TYPE_KIND:
	  if (!is_type_decl(ft) && !is_type_decl(st))
	    return false;
	  break;
	}
    }

  string fn, sn;
  fn = get_type_declaration(ft)->get_qualified_name();
  sn = get_type_declaration(st)->get_qualified_name();

  // Check if there is an exact type name match.
  if (!get_type_name().empty())
    {
      if (get_type_name() != fn && get_type_name() != sn)
	return false;
    }
  else
    {
      // So now check if there is a regular expression match.
      //
      // If none of the qualified name of the types that are being
      // compared match the regular expression of the of the type name,
      // then this suppression doesn't apply.
      const sptr_utils::regex_t_sptr type_name_regex =
	priv_->get_type_name_regex();
      if (type_name_regex
	  && (regexec(type_name_regex.get(), fn.c_str(),
		      0, NULL, 0) != 0
	      && regexec(type_name_regex.get(), sn.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

  return true;
}

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

  ini::config::property_sptr label = section.find_property("label");
  string label_str = label && !label->second.empty() ? label->second : "";

  ini::config::property_sptr name_regex_prop =
    section.find_property("name_regexp");
  string name_regex_str =
    name_regex_prop && !name_regex_prop->second.empty()
    ? name_regex_prop->second
    : "";

  ini::config::property_sptr name_prop = section.find_property("name");
  string name_str = name_prop && !name_prop->second.empty()
    ? name_prop->second
    : "";

  bool consider_type_kind = false;
  type_suppression::type_kind type_kind = type_suppression::UNKNOWN_TYPE_KIND;
  if (ini::config::property_sptr type_kind_prop =
      section.find_property("type_kind"))
    {
      consider_type_kind = true;
      type_kind = read_type_kind_string(type_kind_prop->second);
    }

  if ((!name_regex_prop || name_regex_prop->second.empty())
      && (!name_prop || name_prop->second.empty())
      && !consider_type_kind)
    return nil;

  type_suppression_sptr suppr(new type_suppression(label_str,
						   name_regex_str,
						   name_str));
  if (consider_type_kind)
    {
      suppr->set_consider_type_kind(true);
      suppr->set_type_kind(type_kind);
    }

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
	sptr_utils::regex_t_sptr r(new regex_t);
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

  priv(const string&			name,
       const string&			name_regex_str,
       const string&			return_type_name,
       const string&			return_type_regex_str,
       const parameter_specs_type&	parm_specs,
       const string&			symbol_name,
       const string&			symbol_name_regex_str,
       const string&			symbol_version,
       const string&			symbol_version_regex_str)
    : name_(name),
      name_regex_str_(name_regex_str),
      return_type_name_(return_type_name),
      return_type_regex_str_(return_type_regex_str),
      parm_specs_(parm_specs),
      symbol_name_(symbol_name),
      symbol_name_regex_str_(symbol_name_regex_str),
      symbol_version_(symbol_version),
      symbol_version_regex_str_(symbol_version_regex_str)
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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

  string ff_name = ff->get_qualified_name(), sf_name = sf->get_qualified_name();

  // Check if the "name" property matches.
  if (!get_function_name().empty())
    {
      string n = get_function_name();
      if (n != ff->get_qualified_name()
	  && n != sf->get_qualified_name())
	return false;
    }

  // Check if the "name_regexp" property matches.
  const sptr_utils::regex_t_sptr name_regex = priv_->get_name_regex();
  if (name_regex
      && (regexec(name_regex.get(), ff_name.c_str(), 0, NULL, 0) != 0
	  && regexec(name_regex.get(), sf_name.c_str(), 0, NULL, 0) != 0))
    return false;

  // Check if the "return_type_name" or "return_type_regexp"
  // properties matches.

    string ff_return_type_name = ff->get_type()->get_return_type()
      ? (get_type_declaration(ff->get_type()->get_return_type())
	 ->get_qualified_name())
      : "";
  string sf_return_type_name = sf->get_type()->get_return_type()
    ? (get_type_declaration(sf->get_type()->get_return_type())
       ->get_qualified_name())
    : "";

  if (!get_return_type_name().empty())
    {
      if (ff_return_type_name != get_return_type_name()
	  && sf_return_type_name != get_return_type_name())
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr return_type_regex =
	priv_->get_return_type_regex();
      if (return_type_regex
	  && (regexec(return_type_regex.get(),
		      ff_return_type_name.c_str(),
		      0, NULL, 0) != 0
	      && regexec(return_type_regex.get(),
			 sf_return_type_name.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

  // Check if the "symbol_name" and "symbol_name_regexp" properties
  // match.

  string ff_sym_name, ff_sym_version, sf_sym_name, sf_sym_version;
  if (ff->get_symbol())
    {
      ff_sym_name = ff->get_symbol()->get_name();
      ff_sym_version = ff->get_symbol()->get_version().str();
    }
  if (sf->get_symbol())
    {
      sf_sym_name = sf->get_symbol()->get_name();
      sf_sym_version = sf->get_symbol()->get_version().str();
    }

  if (!get_symbol_name().empty())
    {
      if (ff_sym_name != get_symbol_name()
	  && sf_sym_name != get_symbol_name())
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr symbol_name_regex =
	priv_->get_symbol_name_regex();
      if (symbol_name_regex
	  && (regexec(symbol_name_regex.get(),
		      ff_sym_name.c_str(),
		      0, NULL, 0) != 0
	      && regexec(symbol_name_regex.get(),
			 sf_sym_name.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

// Check if the "symbol_version" and "symbol_version_regexp"
// properties match.
  if (!get_symbol_version().empty())
    {
      if (ff_sym_version != get_symbol_version()
	  && sf_sym_name != get_symbol_version())
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      ff_sym_version.c_str(),
		      0, NULL, 0) != 0
	      && regexec(symbol_version_regex.get(),
			 sf_sym_version.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

  if (!get_parameter_specs().empty())
    {
      function_type_sptr ff_type = ff->get_type();
      function_type_sptr sf_type = sf->get_type();
      type_base_sptr parm_type;

      for (parameter_specs_type::const_iterator p =
	     get_parameter_specs().begin();
	   p != get_parameter_specs().end();
	   ++p)
	{
	  size_t index = (*p)->get_index();
	  function_decl::parameter_sptr ff_parm =
	    ff_type->get_parm_at_index_from_first_non_implicit_parm(index);
	  function_decl::parameter_sptr sf_parm =
	    sf_type->get_parm_at_index_from_first_non_implicit_parm(index);
	  if (!ff_parm && ! sf_parm)
	    return false;

	  string ff_parm_type_qualified_name, sf_parm_type_qualified_name;
	  if (ff_parm)
	    {
	      parm_type = ff_parm->get_type();
	      ff_parm_type_qualified_name =
		get_type_declaration(parm_type)->get_qualified_name();
	    }

	  if (sf_parm)
	    {
	      parm_type = sf_parm->get_type();
	      sf_parm_type_qualified_name =
		get_type_declaration(parm_type)->get_qualified_name();
	    }

	  const string& tn = (*p)->get_parameter_type_name();
	  if (!tn.empty())
	    {
	      if (tn != ff_parm_type_qualified_name
		  && tn != sf_parm_type_qualified_name)
		return false;
	    }
	  else
	    {
	      const sptr_utils::regex_t_sptr parm_type_name_regex =
		(*p)->priv_->get_type_name_regex();
	      if (parm_type_name_regex)
		{
		  if ((regexec(parm_type_name_regex.get(),
			       ff_parm_type_qualified_name.c_str(),
			       0, NULL, 0) != 0
		       && regexec(parm_type_name_regex.get(),
				  sf_parm_type_qualified_name.c_str(),
				  0, NULL, 0) != 0))
		    return false;
		}
	    }
	}
    }

  return true;
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

  ini::config::property_sptr label_prop = section.find_property("label");
  string label_str = (label_prop && !label_prop->second.empty())
    ? label_prop->second
    : "";

  ini::config::property_sptr name_prop = section.find_property("name");
  string name = name_prop && !name_prop->second.empty()
    ? name_prop->second
    : "";

  ini::config::property_sptr name_regex_prop =
    section.find_property("name_regexp");
  string name_regex_str = name_regex_prop && !name_regex_prop->second.empty()
    ? name_regex_prop->second
    : "";

  ini::config::property_sptr return_type_name_prop =
    section.find_property("return_type_name");
  string return_type_name = (return_type_name_prop)
    ? return_type_name_prop->second
    : "";

  ini::config::property_sptr return_type_regex_prop =
    section.find_property("return_type_regexp");
  string return_type_regex_str =
    (return_type_regex_prop
     && !return_type_regex_prop->second.empty())
    ? return_type_regex_prop->second
    : "";

  ini::config::property_sptr sym_name_prop =
    section.find_property("symbol_name");
  string sym_name = (sym_name_prop) ? sym_name_prop->second : "";

  ini::config::property_sptr sym_name_regex_prop =
    section.find_property("symbol_name_regexp");
  string sym_name_regex_str =
    (sym_name_regex_prop && !sym_name_regex_prop->second.empty())
    ? sym_name_regex_prop->second
    : "";

  ini::config::property_sptr sym_ver_prop =
    section.find_property("symbol_version");
  string sym_version = (sym_ver_prop) ? sym_ver_prop->second : "";

  ini::config::property_sptr sym_ver_regex_prop =
    section.find_property("symbol_version_regexp");
  string sym_ver_regex_str =
    (sym_ver_regex_prop && !sym_ver_regex_prop->second.empty())
    ? sym_ver_regex_prop->second
    : "";

  function_suppression::parameter_spec_sptr parm;
  function_suppression::parameter_specs_type parms;
  for (ini::config::property_vector::const_iterator p =
	 section.get_properties().begin();
       p != section.get_properties().end();
       ++p)
    if ((*p)->first == "parameter")
      if (parm = read_parameter_spec_from_string((*p)->second))
	parms.push_back(parm);

  function_suppression_sptr result;
  if (!label_str.empty()
      || !name.empty()
      || !name_regex_str.empty()
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

  return result;
}

// </function_suppression stuff>

// <variable_suppression stuff>

/// The type of the private data of the @ref variable_suppression
/// type.
class variable_suppression::priv
{
  friend class variable_suppression;

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
    : name_(name),
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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
	sptr_utils::regex_t_sptr r(new regex_t);
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

  var_decl_sptr fv = is_var_decl(d->first_subject()),
    sv = is_var_decl(d->second_subject());

  assert(fv && sv);

  string fv_name = fv->get_name(), sv_name = sv->get_name();

  // Check for "name" property match.
  if (!get_name().empty())
    {
    if (get_name() != fv_name && get_name () != sv_name)
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
	      && (regexec(name_regex.get(), fv_name.c_str(),
			  0, NULL, 0) != 0
		  && regexec(name_regex.get(), sv_name.c_str(),
			     0, NULL, 0) != 0))
	    return false;
	}
    }

  // Check for the symbol_name property match.
  string fv_sym_name = fv->get_symbol() ? fv->get_symbol()->get_name() : "";
  string sv_sym_name = sv->get_symbol() ? sv->get_symbol()->get_name() : "";
  if (!get_symbol_name().empty())
    {
      if (get_symbol_name() != fv_sym_name && get_symbol_name() != sv_sym_name)
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr sym_name_regex =
	priv_->get_symbol_name_regex();
      if (sym_name_regex
	  && (regexec(sym_name_regex.get(), fv_sym_name.c_str(),
		      0, NULL, 0) != 0
	      && regexec(sym_name_regex.get(), sv_sym_name.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

  // Check for symbol_version and symbol_version_regexp property match
  string fv_sym_version =
    fv->get_symbol() ? fv->get_symbol()->get_version().str() : "";
  string sv_sym_version =
    sv->get_symbol() ? fv->get_symbol()->get_version().str() : "";
  if (!get_symbol_version().empty())
    {
      if (get_symbol_version() != fv_sym_version
	  && get_symbol_version() != sv_sym_version)
	return false;
    }
  else
    {
      const sptr_utils::regex_t_sptr symbol_version_regex =
	priv_->get_symbol_version_regex();
      if (symbol_version_regex
	  && (regexec(symbol_version_regex.get(),
		      fv_sym_version.c_str(),
		      0, NULL, 0) != 0
	      && regexec(symbol_version_regex.get(),
			 sv_sym_version.c_str(),
			 0, NULL, 0) != 0))
	return false;
    }

  string fv_type_name =
    get_type_declaration(fv->get_type())->get_qualified_name();
  string sv_type_name =
    get_type_declaration(sv->get_type())->get_qualified_name();

  // Check for the "type_name" and tye_name_regex properties match.
  if (!get_type_name().empty())
    {
    if (get_type_name() != fv_type_name && get_type_name() != sv_type_name)
      return false;
    }
  else
    {
      if (get_type_name().empty())
	{
	  const sptr_utils::regex_t_sptr type_name_regex =
	    priv_->get_type_name_regex();
	  if (type_name_regex
	      && (regexec(type_name_regex.get(), fv_type_name.c_str(),
			  0, NULL, 0) != 0
		  && regexec(type_name_regex.get(), sv_type_name.c_str(),
			     0, NULL, 0) != 0))
	    return false;
	}
    }

  return true;
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

  ini::config::property_sptr label_prop = section.find_property("label");
  string label_str = (label_prop && !label_prop->second.empty()
		      ? label_prop->second
		      : "");

  ini::config::property_sptr name_prop = section.find_property("name");
  string name_str = (name_prop && !name_prop->second.empty()
		     ? name_prop->second
		     : "");

  ini::config::property_sptr name_regex_prop =
    section.find_property("name_regexp");
  string name_regex_str = (name_regex_prop && !name_regex_prop->second.empty()
			   ? name_regex_prop->second
			   : "");

  ini::config::property_sptr sym_name_prop =
    section.find_property("symbol_name");
  string symbol_name = (sym_name_prop && !sym_name_prop->second.empty()
			? sym_name_prop->second
			: "");

  ini::config::property_sptr sym_name_regex_prop =
    section.find_property("symbol_name_regexp");
  string symbol_name_regex_str =
    (sym_name_regex_prop && !sym_name_regex_prop->second.empty()
     ? sym_name_regex_prop->second
     : "");

  ini::config::property_sptr sym_version_prop =
    section.find_property("symbol_version");
  string symbol_version =
    (sym_version_prop && !sym_version_prop->second.empty()
     ? sym_version_prop->second
     : "");

  ini::config::property_sptr sym_version_regex_prop =
    section.find_property("symbol_version_regexp");
  string symbol_version_regex_str =
    (sym_version_regex_prop && !sym_version_regex_prop->second.empty()
     ? sym_version_regex_prop->second
     : "");

  ini::config::property_sptr type_name_prop =
    section.find_property("type_name");
  string type_name_str = (type_name_prop && !type_name_prop->second.empty()
			  ? type_name_prop->second
			  : "");

  ini::config::property_sptr type_name_regex_prop =
    section.find_property("type_name_regexp");
  string type_name_regex_str =
    (type_name_regex_prop && !type_name_regex_prop->second.empty()
     ? type_name_regex_prop->second
     : "");

  if (label_str.empty()
      && name_str.empty()
      && name_regex_str.empty()
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
  return result;
}

// </variable_suppression stuff>

/// The private member (pimpl) for @ref diff_context.
struct diff_context::priv
{
  diff_category			allowed_category_;
  decls_diff_map_type			decls_diff_map;
  vector<diff_sptr>			canonical_diffs;
  vector<filtering::filter_base_sptr>	filters_;
  suppressions_type			suppressions_;
  pointer_map				visited_diff_nodes_;
  corpus_sptr				first_corpus_;
  corpus_sptr				second_corpus_;
  ostream*				default_output_stream_;
  ostream*				error_output_stream_;
  bool					forbid_visiting_a_node_twice_;
  bool					show_stats_only_;
  bool					show_soname_change_;
  bool					show_architecture_change_;
  bool					show_deleted_fns_;
  bool					show_changed_fns_;
  bool					show_added_fns_;
  bool					show_deleted_vars_;
  bool					show_changed_vars_;
  bool					show_added_vars_;
  bool					show_linkage_names_;
  bool					show_redundant_changes_;
  bool					show_syms_unreferenced_by_di_;
  bool					show_added_syms_unreferenced_by_di_;
  bool					dump_diff_tree_;

  priv()
    : allowed_category_(EVERYTHING_CATEGORY),
      default_output_stream_(),
      error_output_stream_(),
      forbid_visiting_a_node_twice_(true),
      show_stats_only_(false),
      show_soname_change_(true),
      show_architecture_change_(true),
      show_deleted_fns_(true),
      show_changed_fns_(true),
      show_added_fns_(true),
      show_deleted_vars_(true),
      show_changed_vars_(true),
      show_added_vars_(true),
      show_linkage_names_(false),
      show_redundant_changes_(true),
      show_syms_unreferenced_by_di_(true),
      show_added_syms_unreferenced_by_di_(true),
      dump_diff_tree_()
   {}
 };// end struct diff_context::priv

diff_context::diff_context()
  : priv_(new diff_context::priv)
{
  // Setup all the diff output filters we have.
  filtering::filter_base_sptr f;

  f.reset(new filtering::harmless_filter);
  add_diff_filter(f);

  f.reset(new filtering::harmful_filter);
  add_diff_filter(f);
}

/// Set the corpora that are being compared into the context, so that
/// some lower-level routines can have a chance to have access to
/// them.
///
/// @param corp1 the first corpus involved in the comparison.
///
/// @param corp2 the second corpus involved in the comparison.
void
diff_context::set_corpora(const corpus_sptr corp1,
			  const corpus_sptr corp2)
{
  priv_->first_corpus_ = corp1;
  priv_->second_corpus_ = corp2;
}

/// Get the first corpus of the comparison, from the current context.
///
/// @return the first corpus of the comparison.
const corpus_sptr
diff_context::get_first_corpus() const
{return priv_->first_corpus_;}

/// Get the second corpus of the comparison, from the current context.
///
/// @return the second corpus of the comparison, from the current
/// context.
const corpus_sptr
diff_context::get_second_corpus() const
{return priv_->second_corpus_;}

/// Tests if the current diff context already has a diff for two decls.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for(const decl_base_sptr first,
			   const decl_base_sptr second) const
{
  decls_diff_map_type::const_iterator i =
    priv_->decls_diff_map.find(std::make_pair(first, second));
  if (i != priv_->decls_diff_map.end())
    return i->second;
  return diff_sptr();
}

/// Tests if the current diff context already has a diff for two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @return a pointer to the diff for @p first @p second if found,
/// null otherwise.
diff_sptr
diff_context::has_diff_for_types(const type_base_sptr first,
				  const type_base_sptr second) const
{
  return has_diff_for(get_type_declaration(first),
		       get_type_declaration(second));
}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
const diff*
diff_context::has_diff_for(const diff* d) const
{return has_diff_for(d->first_subject(), d->second_subject()).get();}

/// Tests if the current diff context already has a given diff.
///
///@param d the diff to consider.
///
/// @return a pointer to the diff found for @p d
diff_sptr
diff_context::has_diff_for(const diff_sptr d) const
{return has_diff_for(d->first_subject(), d->second_subject());}

/// Getter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @return a bitmap that represents the set of categories that the
/// user wants to see reported.
diff_category
diff_context::get_allowed_category() const
{return priv_->allowed_category_;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::set_allowed_category(diff_category c)
{priv_->allowed_category_ = c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function perform a bitwise or between the new set of
/// categories and the current ones, and then sets the current
/// categories to the result of the or.
///
/// @param c a bitmap that represents the set of categories that the
/// user wants to see represented.
void
diff_context::switch_categories_on(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ | c;}

/// Setter for the bitmap that represents the set of categories that
/// the user wants to see reported
///
/// This function actually unsets bits from the current categories.
///
/// @param c a bitmap that represents the set of categories to unset
/// from the current categories.
void
diff_context::switch_categories_off(diff_category c)
{priv_->allowed_category_ = priv_->allowed_category_ & ~c;}

/// Add a diff for two decls to the cache of the current diff_context
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param the diff to add.
void
diff_context::add_diff(decl_base_sptr first,
		       decl_base_sptr second,
		       const diff_sptr d)
{priv_->decls_diff_map[std::make_pair(first, second)] = d;}

/// Add a diff tree node to the cache of the current diff_context
///
/// @param d the diff tree node to add.
void
diff_context::add_diff(const diff* d)
{
  if (d)
    {
      diff_sptr dif(const_cast<diff*>(d), noop_deleter());
      add_diff(d->first_subject(), d->second_subject(), dif);
    }
}

/// Add a diff tree node to the cache of the current diff_context
///
/// @param d the diff tree node to add.
void
diff_context::add_diff(const diff_sptr d)
{
  if (d)
      add_diff(d->first_subject(), d->second_subject(), d);
}

/// Getter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by their two subjects.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @return the canonical diff for the diff node represented by the
/// two diff subjects @p first and @p second.  If no canonical diff
/// node was registered for these subjects, then a nil node is
/// returned.
diff_sptr
diff_context::get_canonical_diff_for(const decl_base_sptr first,
				     const decl_base_sptr second) const
{return has_diff_for(first, second);}

/// Getter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by the two subjects of a given diff node.
///
/// @param d the diff node to get the canonical node for.
///
/// @return the canonical diff for the diff node represented by the
/// two diff subjects of @p d.  If no canonical diff node was
/// registered for these subjects, then a nil node is returned.
diff_sptr
diff_context::get_canonical_diff_for(const diff_sptr d) const
{return has_diff_for(d);}

/// Setter for the @ref CanonicalDiff "canonical diff node" for the
/// @ref diff represented by their two subjects.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param d the new canonical diff.
void
diff_context::set_canonical_diff_for(const decl_base_sptr first,
				     const decl_base_sptr second,
				     const diff_sptr d)
{
  assert(d);
  if (!has_diff_for(first, second))
    {
      add_diff(first, second, d);
      priv_->canonical_diffs.push_back(d);
    }
}

/// If there is is a @ref CanonicalDiff "canonical diff node"
/// registered for two diff subjects, return it.  Otherwise, register
/// a canonical diff node for these two diff subjects and return it.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param d the new canonical diff node.
///
/// @returnt the canonical diff node.
diff_sptr
diff_context::set_or_get_canonical_diff_for(const decl_base_sptr first,
					    const decl_base_sptr second,
					    const diff_sptr canonical_diff)
{
  assert(canonical_diff);

  diff_sptr canonical = get_canonical_diff_for(first, second);
  if (!canonical)
    {
      canonical = canonical_diff;
      set_canonical_diff_for(first, second, canonical);
    }
  return canonical;
}

/// Set the canonical diff node property of a given diff node
/// appropriately.
///
/// For a given diff node that has no canonical diff node, retrieve
/// the canonical diff node (by looking at its diff subjects and at
/// the current context) and set the canonical diff node property of
/// the diff node to that canonical diff node.  If no canonical diff
/// node has been registered to the diff context for the subjects of
/// the diff node then, register the canonical diff node as being the
/// diff node itself; and set its canonical diff node property as
/// such.  Otherwise, if the diff node already has a canonical diff
/// node, do nothing.
///
/// @param diff the diff node to initialize the canonical diff node
/// property for.
void
diff_context::initialize_canonical_diff(const diff_sptr diff)
{
  if (diff->get_canonical_diff() == 0)
    {
      diff_sptr canonical =
	set_or_get_canonical_diff_for(diff->first_subject(),
				      diff->second_subject(),
				      diff);
      diff->set_canonical_diff(canonical.get());
    }
}

/// Test if a diff node has been traversed.
///
/// @param d the diff node to consider.
bool
diff_context::diff_has_been_visited(const diff* d) const
{
  const diff* canonical = d->get_canonical_diff();
  assert(canonical);

  size_t ptr_value = reinterpret_cast<size_t>(canonical);
  return (priv_->visited_diff_nodes_.find(ptr_value)
	  != priv_->visited_diff_nodes_.end());
}

/// Test if a diff node has been traversed.
///
/// @param d the diff node to consider.
bool
diff_context::diff_has_been_visited(const diff_sptr d) const
{return diff_has_been_visited(d.get());}

/// Mark a diff node as traversed by a traversing algorithm.
///
/// Actually, it's the @ref CanonicalDiff "canonical diff" of this
/// node that is marked as traversed.
///
/// Subsequent invocations of diff_has_been_visited() on the diff
/// node will yield true.
void
diff_context::mark_diff_as_visited(const diff* d)
{
  const diff* canonical = d->get_canonical_diff();
  assert(canonical);

   size_t ptr_value = reinterpret_cast<size_t>(canonical);
   priv_->visited_diff_nodes_[ptr_value] = true;
}

/// Unmark all the diff nodes that were marked as being traversed.
void
diff_context::forget_visited_diffs()
{priv_->visited_diff_nodes_.clear();}

/// This sets a flag that, if it's true, then during the traversing of
/// a diff nodes tree each node is visited at most once.
///
/// @param f if true then during the traversing of a diff nodes tree
/// each node is visited at most once.
void
diff_context::forbid_visiting_a_node_twice(bool f)
{priv_->forbid_visiting_a_node_twice_ = f;}

/// Return a flag that, if true, then during the traversing of a diff
/// nodes tree each node is visited at most once.
///
/// @return the boolean flag.
bool
diff_context::visiting_a_node_twice_is_forbidden() const
{return priv_->forbid_visiting_a_node_twice_;}

/// Getter for the diff tree nodes filters to apply to diff sub-trees.
///
/// @return the vector of tree filters to apply to diff sub-trees.
const filtering::filters&
diff_context::diff_filters() const
{return priv_->filters_;}

/// Setter for the diff filters to apply to a given diff sub-tree.
///
/// @param f the new diff filter to add to the vector of diff filters
/// to apply to diff sub-trees.
void
diff_context::add_diff_filter(filtering::filter_base_sptr f)
{priv_->filters_.push_back(f);}

/// Apply the diff filters to a given diff sub-tree.
///
/// If the current context is instructed to filter out some categories
/// then this function walks the given sub-tree and categorizes its
/// nodes by using the filters held by the context.
///
/// @param diff the diff sub-tree to apply the filters to.
void
diff_context::maybe_apply_filters(diff_sptr diff)
{
  if (!diff)
    return;

  if (get_allowed_category() == EVERYTHING_CATEGORY)
    return;

  if (!diff->has_changes())
    return;

  for (filtering::filters::const_iterator i = diff_filters().begin();
       i != diff_filters().end();
       ++i)
    {
      filtering::apply_filter(*i, diff);
      propagate_categories(diff);
    }

 }

/// Apply the diff filters to the diff nodes of a @ref corpus_diff
/// instance.
///
/// If the current context is instructed to filter out some categories
/// then this function walks the diff tree and categorizes its nodes
/// by using the filters held by the context.
///
/// @param diff the corpus diff to apply the filters to.
void
diff_context::maybe_apply_filters(corpus_diff_sptr diff)
{

  if (!diff || !diff->has_changes())
    return;

  for (filtering::filters::const_iterator i = diff_filters().begin();
       i != diff_filters().end();
       ++i)
    {
      filtering::apply_filter(**i, diff);
      propagate_categories(diff);
    }
}

/// Getter for the vector of suppressions that specify which diff node
/// reports should be dropped on the floor.
///
/// @return the set of suppressions.
suppressions_type&
diff_context::suppressions() const
{return priv_->suppressions_;}

/// Add a new suppression specification that specifies which diff node
/// reports should be dropped on the floor.
///
/// @param suppr the new suppression specification to add to the
/// existing set of suppressions specifications of the diff context.
void
diff_context::add_suppression(const suppression_sptr suppr)
{priv_->suppressions_.push_back(suppr);}

/// Add new suppression specifications that specify which diff node
/// reports should be dropped on the floor.
///
/// @param supprs the new suppression specifications to add to the
/// existing set of suppression specifications of the diff context.
void
diff_context::add_suppressions(const suppressions_type& supprs)
{
  priv_->suppressions_.insert(priv_->suppressions_.end(),
			      supprs.begin(), supprs.end());
}

/// Set a flag saying if the comparison module should only show the
/// diff stats.
///
/// @param f the flag to set.
void
diff_context::show_stats_only(bool f)
{priv_->show_stats_only_ = f;}

/// Test if the comparison module should only show the diff stats.
///
/// @return true if the comparison module should only show the diff
/// stats, false otherwise.
bool
diff_context::show_stats_only() const
{return priv_->show_stats_only_;}

/// Setter for the property that says if the comparison module should
/// show the soname changes in its report.
///
/// @param f the new value of the property.
void
diff_context::show_soname_change(bool f)
{priv_->show_soname_change_ = f;}

/// Getter for the property that says if the comparison module should
/// show the soname changes in its report.
///
/// @return the value of the property.
bool
diff_context::show_soname_change() const
{return priv_->show_soname_change_;}

/// Setter for the property that says if the comparison module should
/// show the architecture changes in its report.
///
/// @param f the new value of the property.
void
diff_context::show_architecture_change(bool f)
{priv_->show_architecture_change_ = f;}

/// Getter for the property that says if the comparison module should
/// show the architecture changes in its report.
///
/// @return the value of the property.
bool
diff_context::show_architecture_change() const
{return priv_->show_architecture_change_;}

/// Set a flag saying to show the deleted functions.
///
/// @param f true to show deleted functions.
void
diff_context::show_deleted_fns(bool f)
{priv_->show_deleted_fns_ = f;}

/// @return true if we want to show the deleted functions, false
/// otherwise.
bool
diff_context::show_deleted_fns() const
{return priv_->show_deleted_fns_;}

/// Set a flag saying to show the changed functions.
///
/// @param f true to show the changed functions.
void
diff_context::show_changed_fns(bool f)
{priv_->show_changed_fns_ = f;}

/// @return true if we want to show the changed functions, false otherwise.
bool
diff_context::show_changed_fns() const
{return priv_->show_changed_fns_;}

/// Set a flag saying to show the added functions.
///
/// @param f true to show the added functions.
void
diff_context::show_added_fns(bool f)
{priv_->show_added_fns_ = f;}

/// @return true if we want to show the added functions, false
/// otherwise.
bool
diff_context::show_added_fns() const
{return priv_->show_added_fns_;}

/// Set a flag saying to show the deleted variables.
///
/// @param f true to show the deleted variables.
void
diff_context::show_deleted_vars(bool f)
{priv_->show_deleted_vars_ = f;}

/// @return true if we want to show the deleted variables, false
/// otherwise.
bool
diff_context::show_deleted_vars() const
{return priv_->show_deleted_vars_;}

/// Set a flag saying to show the changed variables.
///
/// @param f true to show the changed variables.
void
diff_context::show_changed_vars(bool f)
{priv_->show_changed_vars_ = f;}

/// @return true if we want to show the changed variables, false otherwise.
bool
diff_context::show_changed_vars() const
{return priv_->show_changed_vars_;}

/// Set a flag saying to show the added variables.
///
/// @param f true to show the added variables.
void
diff_context::show_added_vars(bool f)
{priv_->show_added_vars_ = f;}

/// @return true if we want to show the added variables, false
/// otherwise.
bool
diff_context::show_added_vars() const
{return priv_->show_added_vars_;}

bool
diff_context::show_linkage_names() const
{return priv_->show_linkage_names_;}

void
diff_context::show_linkage_names(bool f)
{priv_->show_linkage_names_= f;}

/// A getter for the flag that says if we should report about
/// functions or variables diff nodes that have *exclusively*
/// redundant diff tree children nodes.
///
/// @return the flag.
bool
diff_context::show_redundant_changes() const
{return priv_->show_redundant_changes_;}

/// A setter for the flag that says if we should report about
/// functions or variables diff nodes that have *exclusively*
/// redundant diff tree children nodes.
///
/// @param f the flag to set.
void
diff_context::show_redundant_changes(bool f)
{priv_->show_redundant_changes_ = f;}

/// Getter for the flag that indicates if symbols not referenced by
/// any debug info are to be compared and reported about.
///
/// @return the boolean flag.
bool
diff_context::show_symbols_unreferenced_by_debug_info() const
{return priv_->show_syms_unreferenced_by_di_;}

/// Setter for the flag that indicates if symbols not referenced by
/// any debug info are to be compared and reported about.
///
/// @param f the new flag to set.
void
diff_context::show_symbols_unreferenced_by_debug_info(bool f)
{priv_->show_syms_unreferenced_by_di_ = f;}

/// Getter for the flag that indicates if symbols not referenced by
/// any debug info and that got added are to be reported about.
///
/// @return true iff symbols not referenced by any debug info and that
/// got added are to be reported about.
bool
diff_context::show_added_symbols_unreferenced_by_debug_info() const
{return priv_->show_added_syms_unreferenced_by_di_;}

/// Setter for the flag that indicates if symbols not referenced by
/// any debug info and that got added are to be reported about.
///
/// @param f the new flag that says if symbols not referenced by any
/// debug info and that got added are to be reported about.
void
diff_context::show_added_symbols_unreferenced_by_debug_info(bool f)
{priv_->show_added_syms_unreferenced_by_di_ = f;}

/// Setter for the default output stream used by code of the
/// comparison engine.  By default the default output stream is a NULL
/// pointer.
///
/// @param o a pointer to the default output stream.
void
diff_context::default_output_stream(ostream* o)
{priv_->default_output_stream_ = o;}

/// Getter for the default output stream used by code of the
/// comparison engine.  By default the default output stream is a NULL
/// pointer.
///
/// @return a pointer to the default output stream.
ostream*
diff_context::default_output_stream()
{return priv_->default_output_stream_;}

/// Setter for the errror output stream used by code of the comparison
/// engine.  By default the error output stream is a NULL pointer.
///
/// @param o a pointer to the error output stream.
void
diff_context::error_output_stream(ostream* o)
{priv_->error_output_stream_ = o;}

/// Getter for the errror output stream used by code of the comparison
/// engine.  By default the error output stream is a NULL pointer.
///
/// @return a pointer to the error output stream.
ostream*
diff_context::error_output_stream() const
{return priv_->error_output_stream_;}

/// Test if the comparison engine should dump the diff tree for the
/// changed functions and variables it has.
///
/// @return true if after the comparison, the engine should dump the
/// diff tree for the changed functions and variables it has.
bool
diff_context::dump_diff_tree() const
{return priv_->dump_diff_tree_;}

/// Set if the comparison engine should dump the diff tree for the
/// changed functions and variables it has.
///
/// @param f true if after the comparison, the engine should dump the
/// diff tree for the changed functions and variables it has.
void
diff_context::dump_diff_tree(bool f)
{priv_->dump_diff_tree_ = f;}

/// Emit a textual representation of a diff tree to the error output
/// stream of the current context, for debugging purposes.
///
/// @param d the diff tree to serialize to the error output associated
/// to the current instance of @ref diff_context.
void
diff_context::do_dump_diff_tree(const diff_sptr d) const
{
  if (error_output_stream())
    print_diff_tree(d, *error_output_stream());
}

/// Emit a textual representation of a @ref corpus_diff tree to the error
/// output stream of the current context, for debugging purposes.
///
/// @param d the @ref corpus_diff tree to serialize to the error
/// output associated to the current instance of @ref diff_context.
void
diff_context::do_dump_diff_tree(const corpus_diff_sptr d) const
{
  if (error_output_stream())
    print_diff_tree(d, *error_output_stream());
}
// </diff_context stuff>

// <diff stuff>

/// Private data for the @ref diff type.
struct diff::priv
{
  bool			finished_;
  bool			traversing_;
  decl_base_sptr	first_subject_;
  decl_base_sptr	second_subject_;
  vector<diff_sptr>	children_;
  diff*		parent_;
  diff*		canonical_diff_;
  diff_context_sptr	ctxt_;
  diff_category	category_;
  mutable bool		reported_once_;
  mutable bool		currently_reporting_;
  mutable string	pretty_representation_;

  priv();

public:

  priv(decl_base_sptr first_subject,
       decl_base_sptr second_subject,
       diff_context_sptr ctxt,
       diff_category category,
       bool reported_once,
       bool currently_reporting)
    : finished_(),
      traversing_(),
      first_subject_(first_subject),
      second_subject_(second_subject),
      parent_(),
      canonical_diff_(),
      ctxt_(ctxt),
      category_(category),
      reported_once_(reported_once),
      currently_reporting_(currently_reporting)
  {}
};// end class diff::priv

/// A functor to compare two instances of @ref diff_sptr.
struct diff_less_than_functor
{
  /// An operator that takes two instances of @ref diff_sptr returns
  /// true if its first operand compares less than its second operand.
  ///
  /// @param l the first operand to consider.
  ///
  /// @param r the second operand to consider.
  ///
  /// @return true if @p l compares less than @p r.
  bool
  operator()(const diff_sptr l, const diff_sptr r) const
  {
    if (!l || !r || !l->first_subject() || !r->first_subject())
      return false;

    string l_qn = l->first_subject()->get_qualified_name();
    string r_qn = r->first_subject()->get_qualified_name();

    return l_qn < r_qn;
  }
};

/// Constructor for the @ref diff type.
///
/// This constructs a diff between two subjects that are actually
/// declarations; the first and the second one.
///
/// @param first_subject the first decl (subject) of the diff.
///
/// @param second_subject the second decl (subject) of the diff.
diff::diff(decl_base_sptr first_subject,
	   decl_base_sptr second_subject)
  : priv_(new priv(first_subject, second_subject,
		   diff_context_sptr(),
		   NO_CHANGE_CATEGORY,
		   /*reported_once=*/false,
		   /*currently_reporting=*/false))
{}

/// Constructor for the @ref diff type.
///
/// This constructs a diff between two subjects that are actually
/// declarations; the first and the second one.
///
/// @param first_subject the first decl (subject) of the diff.
///
/// @param second_subject the second decl (subject) of the diff.
///
/// @param ctxt the context of the diff.
diff::diff(decl_base_sptr	first_subject,
	   decl_base_sptr	second_subject,
	   diff_context_sptr	ctxt)
  : priv_(new priv(first_subject, second_subject,
		   ctxt, NO_CHANGE_CATEGORY,
		   /*reported_once=*/false,
		   /*currently_reporting=*/false))
{}

/// Flag a given diff node as being traversed.
///
/// For certain diff nodes like @ref class_diff, it's important to
/// avoid traversing the node again while it's already being
/// traversed; otherwise this leads to infinite loops.  So the
/// diff::begin_traversing() and diff::end_traversing() methods flag a
/// given node as being traversed (or not), so that
/// diff::is_traversing() can tell if the node is being traversed.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// The canonical node is marked as being traversed too.
///
/// These functions are called by the traversing code.
void
diff::begin_traversing()
{
  assert(!is_traversing());
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->traversing_ = true;
  priv_->traversing_ = true;
}

/// Tell if a given node is being traversed or not.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// It's the canonical node which is looked at, actually.
///
/// Please read the comments for the diff::begin_traversing() for mode
/// context.
///
/// @return true if the current instance of @diff is being traversed.
bool
diff::is_traversing() const
{
  if (priv_->canonical_diff_)
    return priv_->canonical_diff_->priv_->traversing_;
  return priv_->traversing_;
}

/// Flag a given diff node as not being traversed anymore.
///
/// Note that traversing a node means visiting it *and* visiting its
/// children nodes.
///
/// Please read the comments of the function diff::begin_traversing()
/// for mode context.
void
diff::end_traversing()
{
  assert(is_traversing());
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->traversing_ = false;
  priv_->traversing_ = false;
}

/// Finish the building of a given kind of a diff tree node.
///
/// For instance, certain kinds of diff tree node have specific
/// children nodes that are populated after the constructor of the
/// diff tree node has been called.  In that case, calling overloads
/// of this method ensures that these children nodes are properly
/// gathered and setup.
void
diff::finish_diff_type()
{
}
/// Getter of the first subject of the diff.
///
/// @return the first subject of the diff.
decl_base_sptr
diff::first_subject() const
{return priv_->first_subject_;}

/// Getter of the second subject of the diff.
///
/// @return the second subject of the diff.
decl_base_sptr
diff::second_subject() const
{return priv_->second_subject_;}

/// Getter for the children nodes of the current @ref diff node.
///
/// @return a vector of the children nodes.
const vector<diff_sptr>&
diff::children_nodes() const
{return priv_->children_;}

/// Getter for the parent node of the current @ref diff node.
///
/// @return the parent node of the current @ref diff node.
const diff*
diff::parent_node() const
{return priv_->parent_;}

/// Getter for the canonical diff of the current instance of @ref
/// diff.
///
/// Note that the canonical diff node for the current instanc eof diff
/// node must have been set by invoking
/// class_diff::initialize_canonical_diff() on the current instance of
/// diff node.
///
/// @return the canonical diff node or null if none was set.
diff*
diff::get_canonical_diff() const
{return priv_->canonical_diff_;}

/// Setter for the canonical diff of the current instance of @ref
/// diff.
///
/// @param d the new canonical node to set.
void
diff::set_canonical_diff(diff * d)
{priv_->canonical_diff_ = d;}

/// Add a new child node to the vector of children nodes for the
/// current @ref diff node.
///
/// @param d the new child node to add to the children nodes.
void
diff::append_child_node(diff_sptr d)
{
  assert(d);
  priv_->children_.push_back(d);

  diff_less_than_functor comp;
  std::sort(priv_->children_.begin(),
	    priv_->children_.end(),
	    comp);

  d->priv_->parent_ = this;
}

/// Getter of the context of the current diff.
///
/// @return the context of the current diff.
const diff_context_sptr
diff::context() const
{return priv_->ctxt_;}

/// Setter of the context of the current diff.
///
/// @param c the new context to set.
void
diff::context(diff_context_sptr c)
{priv_->ctxt_ = c;}

/// Tests if we are currently in the middle of emitting a report for
/// this diff.
///
/// @return true if we are currently emitting a report for the
/// current diff, false otherwise.
bool
diff::currently_reporting() const
{
  if (priv_->canonical_diff_)
    return priv_->canonical_diff_->priv_->currently_reporting_;
  return priv_->currently_reporting_;
}

/// Sets a flag saying if we are currently in the middle of emitting
/// a report for this diff.
///
/// @param f true if we are currently emitting a report for the
/// current diff, false otherwise.
void
diff::currently_reporting(bool f) const
{
  if (priv_->canonical_diff_)
    priv_->canonical_diff_->priv_->currently_reporting_ = f;
  priv_->currently_reporting_ = f;
}

/// Tests if a report has already been emitted for the current diff.
///
/// @return true if a report has already been emitted for the
/// current diff, false otherwise.
bool
diff::reported_once() const
{
  assert(priv_->canonical_diff_);
  return priv_->canonical_diff_->priv_->reported_once_;
}

/// The generic traversing code that walks a given diff sub-tree.
///
/// Note that there is a difference between traversing a diff node and
/// visiting it.  Basically, traversing a diff node means visiting it
/// and visiting its children nodes too.  So one can visit a node
/// without traversing it.  But traversing a node without visiting it
/// is not possible.
///
/// Note that by default this traversing code visits a given class of
/// equivalence of a diff node only once.  This behaviour can been
/// changed by calling
/// diff_context::visiting_a_node_twice_is_forbidden(), but this is
/// very risky as it might create endless loops while visiting a diff
/// tree graph that has changes that refer to themselves; that is,
/// diff tree graphs with cycles.
///
/// When a diff node is encountered, the
/// diff_node_visitor::visit_begin() method is invoked on the diff
/// node first.
///
/// If the diff node has already been visited, then
/// node_visitor::visit_end() is called on it and the node traversing
/// is done; the children of the diff node are not visited in this
/// case.
///
/// If the diff node has *NOT* been visited yet, then the
/// diff_node_visitor::visit() method is invoked with it's 'pre'
/// argument set to true.  Then if the diff_node_visitor::visit()
/// returns true, then the children nodes of the diff node are
/// visited.  Otherwise, no children nodes of the diff node is
/// visited and the diff_node_visitor::visit_end() is called.

/// After the children nodes are visited (and only if they are
/// visited) the diff_node_visitor::visit() method is invoked with
/// it's 'pre' argument set to false.  And then the
/// diff_node_visitor::visit_end() is called.
///
/// @param v the entity that visits each node of the diff sub-tree.
///
/// @return true to tell the caller that all of the sub-tree could be
/// walked.  This instructs the caller to keep walking the rest of the
/// tree.  Return false otherwise.
bool
diff::traverse(diff_node_visitor& v)
{
  finish_diff_type();

  v.visit_begin(this);

  bool already_visited = false;
  if (context()->visiting_a_node_twice_is_forbidden()
      && context()->diff_has_been_visited(this))
    already_visited = true;

  bool mark_visited_nodes_as_traversed =
    !(v.get_visiting_kind() & DO_NOT_MARK_VISITED_NODES_AS_VISITED);

  if (!already_visited && !v.visit(this, /*pre=*/true))
    {
      v.visit_end(this);
      if (mark_visited_nodes_as_traversed)
	context()->mark_diff_as_visited(this);
      return false;
    }

  if (!(v.get_visiting_kind() & SKIP_CHILDREN_VISITING_KIND)
      && !is_traversing()
      && !already_visited)
    {
      begin_traversing();
      for (vector<diff_sptr>::const_iterator i = children_nodes().begin();
	   i != children_nodes().end();
	   ++i)
	{
	  if (!(*i)->traverse(v))
	    {
	      v.visit_end(this);
	      if (mark_visited_nodes_as_traversed)
		context()->mark_diff_as_visited(this);
	      end_traversing();
	      return false;
	    }
	}
      end_traversing();
    }

  if (!v.visit(this, /*pref=*/false))
    {
      v.visit_end(this);
      if (mark_visited_nodes_as_traversed)
	context()->mark_diff_as_visited(this);
      return false;
    }

  v.visit_end(this);
  if (!already_visited && mark_visited_nodes_as_traversed)
    context()->mark_diff_as_visited(this);

  return true;
}

/// Sets a flag saying if a report has already been emitted for the
/// current diff.
///
/// @param f true if a report has already been emitted for the
/// current diff, false otherwise.
void
diff::reported_once(bool f) const
{
  assert(priv_->canonical_diff_);
  priv_->canonical_diff_->priv_->reported_once_ = f;
  priv_->reported_once_ = f;
}

/// Getter for the category of the current diff tree node.
///
/// @return the category of the current diff tree node.
diff_category
diff::get_category() const
{return priv_->category_;}

/// Adds the current diff tree node to an additional set of
/// categories.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree node belongs to.
diff_category
diff::add_to_category(diff_category c)
{
  priv_->category_ = priv_->category_ | c;
  return priv_->category_;
}

/// Remove the current diff tree node from an a existing sef of
/// categories.
///
/// @param c a bit-map representing the set of categories to add the
/// current diff tree node to.
///
/// @return the resulting bit-map representing the categories this
/// current diff tree onde belongs to.
diff_category
diff::remove_from_category(diff_category c)
{
  priv_->category_ = priv_->category_ & ~c;
  return priv_->category_;
}

/// Set the category of the current @ref diff node.
///
/// @param c the new category for the current diff node.
void
diff::set_category(diff_category c)
{priv_->category_ = c;}

/// Test if this diff tree node is to be filtered out for reporting
/// purposes.
///
/// The function tests if the categories of the diff tree node are
/// "forbidden" by the context or not.
///
/// @return true iff the current diff node should NOT be reported.
bool
diff::is_filtered_out() const
{
  if (context()->get_allowed_category() == EVERYTHING_CATEGORY)
    return false;

  /// We don't want to display nodes suppressed by a user-provided
  /// suppression specification.
  if (get_category() & SUPPRESSED_CATEGORY)
    return true;

  // We don't want to display redundant diff nodes, when the user
  // asked to avoid seeing redundant diff nodes.
  if (!context()->show_redundant_changes()
      && (get_category() & REDUNDANT_CATEGORY))
    return true;

  if (get_category() == NO_CHANGE_CATEGORY)
    return false;

  // Ignore the REDUNDANT_CATEGORY bit when comparing allowed
  // categories and the current set of categories.
  return !((get_category() & ~REDUNDANT_CATEGORY)
	   & (context()->get_allowed_category()
	      & ~REDUNDANT_CATEGORY));
}

/// Test if the current diff node has been suppressed by a
/// user-provided suppression specification.
///
/// @return true if the current diff node has been suppressed by a
/// user-provided suppression list.
bool
diff::is_suppressed() const
{
  const suppressions_type& suppressions = context()->suppressions();
    for (suppressions_type::const_iterator i = suppressions.begin();
       i != suppressions.end();
       ++i)
    if ((*i)->suppresses_diff(this))
      return true;
  return false;
}

/// Test if this diff tree node should be reported.
///
/// @return true iff the current node should be reported.
bool
diff::to_be_reported() const
{
  if (has_changes() && !is_filtered_out())
    return true;
  return false;
}

/// Get a pretty representation of the current @ref diff node.
///
/// This is suitable for e.g. emitting debugging traces for the diff
/// tree nodes.
///
/// @return the pretty representation of the diff node.
const string&
diff::get_pretty_representation() const
{
  if (priv_->pretty_representation_.empty())
    priv_->pretty_representation_ = "empty_diff";
  return priv_->pretty_representation_;
}

/// Default implementation of the hierachy chaining virtual function.
///
/// There are several types of diff nodes that have logical children
/// nodes; for instance, a typedef_diff has the diff of the underlying
/// type as a child node.  A var_diff has the diff of the types of the
/// variables as a child node, etc.
///
/// But because the @ref diff base has a generic representation for
/// children nodes of the all the types of @ref diff nodes (regardless
/// of the specific most-derived type of diff node) that one can get
/// using the method diff::children_nodes(), one need to populate that
/// vector of children node.
///
/// Populating that vector of children node is done by this function;
/// it must be overloaded by each most-derived type of diff node that
/// extends the @ref diff type.
void
diff::chain_into_hierarchy()
{}

// </diff stuff>

static bool
report_size_and_alignment_changes(decl_base_sptr	first,
				  decl_base_sptr	second,
				  diff_context_sptr	ctxt,
				  ostream&		out,
				  const string&	indent,
				  bool			nl);

// <type_diff_base stuff>
class type_diff_base::priv
{
public:
  friend class type_diff_base;
}; // end class type_diff_base

type_diff_base::type_diff_base(decl_base_sptr	first_subject,
			       decl_base_sptr	second_subject,
			       diff_context_sptr	ctxt)
  : diff(first_subject, second_subject, ctxt),
    priv_(new priv)
{}

type_diff_base::~type_diff_base()
{}
// </type_diff_base stuff>

// <decl_diff_base stuff>
class decl_diff_base::priv
{
public:
  friend class decl_diff_base;
};//end class priv

decl_diff_base::decl_diff_base(decl_base_sptr	first_subject,
			       decl_base_sptr	second_subject,
			       diff_context_sptr	ctxt)
  : diff(first_subject, second_subject, ctxt),
    priv_(new priv)
{}

decl_diff_base::~decl_diff_base()
{}

// </decl_diff_base stuff>
// <distinct_diff stuff>

/// The private data structure for @ref distinct_diff.
struct distinct_diff::priv
{
  diff_sptr compatible_child_diff;
};// end struct distinct_diff

/// @return a pretty representation for the @ref distinct_diff node.
const string&
distinct_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "distinct_diff[";
      if (first_subject())
	o << first_subject()->get_pretty_representation();
      else
	o << "null";
      o << ", ";
      if (second_subject())
	o << second_subject()->get_pretty_representation() ;
      else
	o << "null";
      o << "]" ;
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @distinct_diff.
///
/// The children nodes can then later be retrieved using
/// diff::children_nodes().
void
distinct_diff::chain_into_hierarchy()
{
  assert(entities_are_of_distinct_kinds(first(), second()));

  if (diff_sptr d = compatible_child_diff())
    append_child_node(d);
}

/// Constructor for @ref distinct_diff.
///
/// Note that the two entities considered for the diff (and passed in
/// parameter) must be of different kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
distinct_diff::distinct_diff(decl_base_sptr first,
			     decl_base_sptr second,
			     diff_context_sptr ctxt)
  : diff(first, second, ctxt),
    priv_(new priv)
{assert(entities_are_of_distinct_kinds(first, second));}

/// Finish building the current instance of @ref distinct_diff.
void
distinct_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;

  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of the diff.
///
/// @return the first subject of the diff.
const decl_base_sptr
distinct_diff::first() const
{return first_subject();}

/// Getter for the second subject of the diff.
///
/// @return the second subject of the diff.
const decl_base_sptr
distinct_diff::second() const
{return second_subject();}

/// Getter for the child diff of this distinct_diff instance.
///
/// When a distinct_diff has two subjects that are different but
/// compatible, then the distinct_diff instance has a child diff node
/// (named the compatible child diff) that is the diff between the two
/// subjects stripped from their typedefs.  Otherwise, the compatible
/// child diff is nul.
///
/// Note that two diff subjects (that compare different) are
/// considered compatible if stripping typedefs out of them makes them
/// comparing equal.
///
/// @return the compatible child diff node, if any.  Otherwise, null.
const diff_sptr
distinct_diff::compatible_child_diff() const
{
  if (!priv_->compatible_child_diff)
    {
      type_base_sptr fs = strip_typedef(is_type(first())),
	ss = strip_typedef(is_type(second()));

      if (fs && ss
	  && !entities_are_of_distinct_kinds(get_type_declaration(fs),
					     get_type_declaration(ss)))
	priv_->compatible_child_diff = compute_diff(get_type_declaration(fs),
						    get_type_declaration(ss),
						    context());
    }
  return priv_->compatible_child_diff;
}

/// Test if the two arguments are of different kind, or that are both
/// NULL.
///
/// @param first the first argument to test for similarity in kind.
///
/// @param second the second argument to test for similarity in kind.
///
/// @return true iff the two arguments are of different kind.
bool
distinct_diff::entities_are_of_distinct_kinds(decl_base_sptr first,
					      decl_base_sptr second)
{
  if (!!first != !!second)
    return true;
  if (!first && !second)
    // We do consider diffs of two empty decls as a diff of distinct
    // kinds, for now.
    return true;
  if (first == second)
    return false;

  return typeid(*first.get()) != typeid(*second.get());
}

/// @return true if the two subjects of the diff are different, false
/// otherwise.
bool
distinct_diff::has_changes() const
{
  if (!!first().get() != !!second().get())
    return true;
  else if (first().get() == second().get())
    return false;
  else if (first()->get_hash() && second()->get_hash()
	   && (first()->get_hash() != second()->get_hash()))
    return true;

  if (first() == second()
      || (first() && second()
	  && *first() == *second()))
    return false;
  return true;
}

/// @return true iff the current diff node carries local changes.
bool
distinct_diff::has_local_changes() const
{
  // The changes on a distinct_diff are all local.
  if (has_changes())
    return true;
  return false;
}

/// Emit a report about the current diff instance.
///
/// @param out the output stream to send the diff report to.
///
/// @param indent the indentation string to use in the report.
void
distinct_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  decl_base_sptr f = first(), s = second();

  string f_repr = f ? f->get_pretty_representation() : "'void'";
  string s_repr = s ? s->get_pretty_representation() : "'void'";

  diff_sptr diff = compatible_child_diff();

  if (diff)
    {
      out << indent
	  << "entity changed from '" << f_repr
	  << "' to compatible type '" << s_repr << "'\n";
    }
  else
    {
      out << indent
	  << "entity changed from '" << f_repr
	  << "' to '" << s_repr << "'\n";
    }

  type_base_sptr fs = strip_typedef(is_type(f)),
    ss = strip_typedef(is_type(s));

  if (diff_sptr diff = compatible_child_diff())
    diff->report(out, indent + "  ");
  else
    if (report_size_and_alignment_changes(f, s, context(), out, indent, true))
      out << "\n";
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
distinct_diff_sptr
compute_diff_for_distinct_kinds(const decl_base_sptr first,
				const decl_base_sptr second,
				diff_context_sptr ctxt)
{
  if (!distinct_diff::entities_are_of_distinct_kinds(first, second))
    return distinct_diff_sptr();

  distinct_diff_sptr result(new distinct_diff(first, second, ctxt));

  ctxt->initialize_canonical_diff(result);

  return result;
}

/// </distinct_diff stuff>

/// Try to compute a diff on two instances of DiffType representation.
///
/// The function template performs the diff if and only if the decl
/// representations are of a DiffType.
///
/// @tparm DiffType the type of instances to diff.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
///
///@return the diff of the two types @p first and @p second if and
///only if they represent the parametrized type DiffType.  Otherwise,
///returns a NULL pointer value.
template<typename DiffType>
diff_sptr
try_to_diff(const decl_base_sptr first,
	    const decl_base_sptr second,
	    diff_context_sptr ctxt)
{
  if (shared_ptr<DiffType> f =
      dynamic_pointer_cast<DiffType>(first))
    {
      shared_ptr<DiffType> s =
	dynamic_pointer_cast<DiffType>(second);
      if (!s)
	return diff_sptr();
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}


/// This is a specialization of @ref try_to_diff() template to diff
/// instances of @ref class_decl.
///
/// @param first the first representation of decl to consider in the
/// diff computation.
///
/// @param second the second representation of decl to consider in the
/// diff computation.
///
/// @param ctxt the diff context to use.
template<>
diff_sptr
try_to_diff<class_decl>(const decl_base_sptr first,
			const decl_base_sptr second,
			diff_context_sptr ctxt)
{
  if (class_decl_sptr f =
      dynamic_pointer_cast<class_decl>(first))
    {
      class_decl_sptr s = dynamic_pointer_cast<class_decl>(second);
      if (!s)
	return diff_sptr();

      if (f->get_is_declaration_only())
	{
	  class_decl_sptr f2 = f->get_definition_of_declaration();
	  if (f2)
	    f = f2;
	}
      if (s->get_is_declaration_only())
	{
	  class_decl_sptr s2 = s->get_definition_of_declaration();
	  if (s2)
	    s = s2;
	}
      return compute_diff(f, s, ctxt);
    }
  return diff_sptr();
}

/// Try to diff entities that are of distinct kinds.
///
/// @param first the first entity to consider for the diff.
///
/// @param second the second entity to consider for the diff.
///
/// @param ctxt the context of the diff.
///
/// @return a non-null diff if a diff object could be built, null
/// otherwise.
static diff_sptr
try_to_diff_distinct_kinds(const decl_base_sptr first,
			   const decl_base_sptr second,
			   diff_context_sptr ctxt)
{return compute_diff_for_distinct_kinds(first, second, ctxt);}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// Whenever a new kind of type decl is supported by abigail, if we
/// want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first type decl to consider for the diff
///
/// @param second the second type decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const decl_base_sptr first,
		       const decl_base_sptr second,
		       diff_context_sptr ctxt)
{
  diff_sptr d;

  const decl_base_sptr f = first;
  const decl_base_sptr s = second;

  ((d = try_to_diff<type_decl>(f, s, ctxt))
   ||(d = try_to_diff<enum_type_decl>(f, s, ctxt))
   ||(d = try_to_diff<class_decl>(f, s,ctxt))
   ||(d = try_to_diff<pointer_type_def>(f, s, ctxt))
   ||(d = try_to_diff<reference_type_def>(f, s, ctxt))
   ||(d = try_to_diff<array_type_def>(f, s, ctxt))
   ||(d = try_to_diff<qualified_type_def>(f, s, ctxt))
   ||(d = try_to_diff<typedef_decl>(f, s, ctxt))
   ||(d = try_to_diff_distinct_kinds(f, s, ctxt)));

  assert(d);

  return d;
}

diff_category
operator|(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   | static_cast<unsigned>(c2));}

diff_category&
operator|=(diff_category& c1, diff_category c2)
{
  c1 = c1 | c2;
  return c1;
}

diff_category&
operator&=(diff_category& c1, diff_category c2)
{
  c1 = c1 & c2;
  return c1;
}

diff_category
operator^(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   ^ static_cast<unsigned>(c2));}

diff_category
operator&(diff_category c1, diff_category c2)
{return static_cast<diff_category>(static_cast<unsigned>(c1)
				   & static_cast<unsigned>(c2));}

diff_category
operator~(diff_category c)
{return static_cast<diff_category>(~static_cast<unsigned>(c));}

/// Serialize an instance of @ref diff_category to an output stream.
///
/// @param o the output stream to serialize @p c to.
///
/// @param c the instance of diff_category to serialize.
///
/// @return the output stream to serialize @p c to.
ostream&
operator<<(ostream& o, diff_category c)
{
  bool emitted_a_category = false;

  if (c == NO_CHANGE_CATEGORY)
    {
      o << "NO_CHANGE_CATEGORY";
      emitted_a_category = true;
    }

  if (c & ACCESS_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "ACCESS_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & COMPATIBLE_TYPE_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "COMPATIBLE_TYPE_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_DECL_NAME_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "NON_VIRT_MEM_FUN_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & STATIC_DATA_MEMBER_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "STATIC_DATA_MEMBER_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }
  else if (c & HARMLESS_ENUM_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_ENUM_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "HARMLESS_SYMBOL_ALIAS_CHANGE_CATEORY";
      emitted_a_category |= true;
    }

  if (c & SIZE_OR_OFFSET_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "SIZE_OR_OFFSET_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & VIRTUAL_MEMBER_CHANGE_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "VIRTUAL_MEMBER_CHANGE_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & REDUNDANT_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "REDUNDANT_CATEGORY";
      emitted_a_category |= true;
    }

  if (c & SUPPRESSED_CATEGORY)
    {
      if (emitted_a_category)
	o << "|";
      o << "SUPPRESSED_CATEGORY";
      emitted_a_category |= true;
    }

  return o;
}

/// Compute the difference between two types.
///
/// The function considers every possible types known to libabigail
/// and runs the appropriate diff function on them.
///
/// @param first the first construct to consider for the diff
///
/// @param second the second construct to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.  It's a pointer to a descendent of
/// abigail::comparison::diff.
static diff_sptr
compute_diff_for_types(const type_base_sptr first,
		       const type_base_sptr second,
		       diff_context_sptr ctxt)
{
  diff_sptr d;

  decl_base_sptr f = dynamic_pointer_cast<decl_base>(first);
  decl_base_sptr s = dynamic_pointer_cast<decl_base>(second);

  d = compute_diff_for_types(f, s, ctxt);

  return d;
}

/// Compute the difference between two decls.
///
/// The function consider every possible decls known to libabigail and
/// runs the appropriate diff function on them.
///
/// Whenever a new kind of non-type decl is supported by abigail, if
/// we want to be able to diff two instances of it, we need to update
/// this function to support it.
///
/// @param first the first decl to consider for the diff
///
/// @param second the second decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff.
static diff_sptr
compute_diff_for_decls(const decl_base_sptr first,
		       const decl_base_sptr second,
		       diff_context_sptr ctxt)
{

  diff_sptr d;

  ((d = try_to_diff<function_decl>(first, second, ctxt))
   || (d = try_to_diff<var_decl>(first, second, ctxt))
   || (d = try_to_diff_distinct_kinds(first, second, ctxt)));

   assert(d);

  return d;
}

/// Compute the difference between two decls.  The decls can represent
/// either type declarations, or non-type declaration.
///
/// @param first the first decl to consider.
///
/// @param second the second decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff could not be
/// computed.
diff_sptr
compute_diff(const decl_base_sptr	first,
	     const decl_base_sptr	second,
	     diff_context_sptr		ctxt)
{
  if (!first || !second)
    return diff_sptr();

  diff_sptr d;
  if (is_type(first) && is_type(second))
    d = compute_diff_for_types(first, second, ctxt);
  else
    d = compute_diff_for_decls(first, second, ctxt);
  assert(d);
  return d;
}

/// Compute the difference between two types.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff, or NULL if the diff couldn't be
/// computed.
diff_sptr
compute_diff(const type_base_sptr	first,
	     const type_base_sptr	second,
	     diff_context_sptr		ctxt)
{
    if (!first || !second)
      return diff_sptr();

  decl_base_sptr f = get_type_declaration(first),
    s = get_type_declaration(second);
  diff_sptr d = compute_diff_for_types(f,s, ctxt);
  assert(d);
  return d;
}

/// Get a copy of the pretty representation of a diff node.
///
/// @param d the diff node to consider.
///
/// @return the pretty representation string.
string
get_pretty_representation(diff* d)
{
  if (!d)
    return "";
  string prefix= "diff of ";
  return prefix + get_pretty_representation(d->first_subject());
}
/// Return the length of the diff between two instances of @ref decl_base
///
/// @param first the first instance of @ref decl_base to consider.
///
/// @param second the second instance of @ref decl_base to consider.
///
/// @return the length of the differences between @p first and @p second.
static unsigned
diff_length_of_decl_bases(decl_base_sptr first, decl_base_sptr second)
{
  unsigned l = 0;

  if (first->get_name() != second->get_name())
    ++l;
  if (first->get_visibility() != second->get_visibility())
    ++l;
  return l;
}

/// Return the length of the diff between two instances of @ref type_base
///
/// @param first the first instance of @ref type_base to consider.
///
/// @param second the second instance of @ref type_base to consider.
///
/// @return the length of the differences between @p first and @p second.
static unsigned
diff_length_of_type_bases(type_base_sptr first, type_base_sptr second)
{
  unsigned l = 0;

  if (first->get_size_in_bits() != second->get_size_in_bits())
    ++l;
  if (first->get_alignment_in_bits() != second->get_alignment_in_bits())
    ++l;

  return l;
}

static bool
maybe_report_diff_for_member(decl_base_sptr	decl1,
			     decl_base_sptr	decl2,
			     diff_context_sptr	ctxt,
			     ostream&		out,
			     const string&	indent);

/// Stream a string representation for a member function.
///
/// @param ctxt the current diff context.
///
/// @param mem_fn the member function to stream
///
/// @param out the output stream to send the representation to
static void
represent(diff_context& ctxt,
	  class_decl::method_decl_sptr mem_fn,
	  ostream& out)
{
  if (!mem_fn || !is_member_function(mem_fn))
    return;

  class_decl::method_decl_sptr meth =
    dynamic_pointer_cast<class_decl::method_decl>(mem_fn);
  assert(meth);

  out << "'" << mem_fn->get_pretty_representation() << "'";
  if (get_member_function_is_virtual(mem_fn))
    out << ", virtual at voffset "
	<< get_member_function_vtable_offset(mem_fn)
	<< "/"
	<< meth->get_type()->get_class_type()->get_virtual_mem_fns().size();

  if (ctxt.show_linkage_names()
      && (mem_fn->get_symbol()))
    {
      out << "    {"
	  << mem_fn->get_symbol()->get_id_string()
	  << "}";
    }
  out << "\n";
}

/// Stream a string representation for a data member.
///
/// @param d the data member to stream
///
/// @param out the output stream to send the representation to
static void
represent_data_member(var_decl_sptr d, ostream& out)
{
  if (!is_data_member(d)
      || (!get_member_is_static(d) && !get_data_member_is_laid_out(d)))
    return;

  out << "'" << d->get_pretty_representation() << "'";
  if (!get_member_is_static(d))
    out << ", at offset "
	<< get_data_member_offset(d)
	<< " (in bits)\n";
}

/// Represent the changes carried by an instance of @ref var_diff that
/// represent a difference between two class data members.
///
/// @param diff diff the diff node to represent.
///
/// @param ctxt the diff context to use.
///
/// @param out the output stream to send the representation to.
///
/// @param indent the indentation string to use for the change report.
static void
represent(var_diff_sptr	diff,
	  diff_context_sptr	ctxt,
	  ostream&		out,
	  const string&	indent = "")
{
  if (!diff->to_be_reported())
    return;

  var_decl_sptr o = diff->first_var();
  var_decl_sptr n = diff->second_var();

  bool emitted = false;
  string name1 = o->get_qualified_name();
  string name2 = n->get_qualified_name();
  string pretty_representation = o->get_pretty_representation();

  if (name1 != name2)
    {
      if (filtering::has_harmless_name_change(o, n)
	  && !(ctxt->get_allowed_category()
	       & HARMLESS_DECL_NAME_CHANGE_CATEGORY))
	;
      else
	{
	  if (!emitted)
	    out << indent << "'" << pretty_representation << "' ";
	  else
	    out << ", ";
	  out << "name changed to '" << name2 << "'";
	  emitted = true;
	}
    }

  if (get_data_member_is_laid_out(o)
      != get_data_member_is_laid_out(n))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      if (get_data_member_is_laid_out(o))
	out << "is no more laid out";
      else
	out << "now becomes laid out";
      emitted = true;
    }
  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (get_data_member_offset(o)
	  != get_data_member_offset(n)))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "offset changed from "
	  << get_data_member_offset(o)
	  << " to " << get_data_member_offset(n);
      emitted = true;
    }
  if (o->get_binding() != n->get_binding())
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "elf binding changed from " << o->get_binding()
	  << " to " << n->get_binding();
      emitted = true;
    }
  if (o->get_visibility() != n->get_visibility())
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";
      out << "visibility changed from " << o->get_visibility()
	  << " to " << n->get_visibility();
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(o)
	  != get_member_access_specifier(n)))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";

      out << "access changed from '"
	  << get_member_access_specifier(o)
	  << "' to '"
	  << get_member_access_specifier(n) << "'";
      emitted = true;
    }
  if (get_member_is_static(o)
      != get_member_is_static(n))
    {
      if (!emitted)
	out << indent << "'" << pretty_representation << "' ";
      else
	out << ", ";

      if (get_member_is_static(o))
	out << "is no more static";
      else
	out << "now becomes static";
      emitted = true;
    }
  if (diff_sptr d = diff->type_diff())
    {
      if (d->to_be_reported())
	{
	  if (!emitted)
	    out << indent << "type of '" << pretty_representation << "' changed:\n";
	  else
	    out << "\n" << indent << "and its type '"
		<< get_type_declaration(o->get_type())->get_pretty_representation()
		<< "' changed:\n";
	  if (d->currently_reporting())
	    out << indent << "  details are being reported\n";
	  else if (d->reported_once())
	    out << indent << "  details were reported earlier\n";
	  else
	    d->report(out, indent + "  ");
	  emitted = false;
	}
    }
  if (emitted)
    out << "\n";
}

/// Report the size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
///
/// @param nl whether to start the first report line with a new line.
///
/// @return true iff something was reported.
static bool
report_size_and_alignment_changes(decl_base_sptr	first,
				  decl_base_sptr	second,
				  diff_context_sptr	ctxt,
				  ostream&		out,
				  const string&	indent,
				  bool			nl)
{
  type_base_sptr f = dynamic_pointer_cast<type_base>(first),
    s = dynamic_pointer_cast<type_base>(second);

  if (!s || !f)
    return false;

  bool n = false;
  unsigned fs = f->get_size_in_bits(), ss = s->get_size_in_bits(),
    fa = f->get_alignment_in_bits(), sa = s->get_alignment_in_bits();

  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (fs != ss))
    {
      if (nl)
	out << "\n";
      out << indent << "size changed from " << fs << " to " << ss << " bits";
      n = true;
    }
  if ((ctxt->get_allowed_category() & SIZE_OR_OFFSET_CHANGE_CATEGORY)
      && (fa != sa))
    {
      if (n)
	out << "\n";
      out << indent
	  << "alignment changed from " << fa << " to " << sa << " bits";
      n = true;
    }

  if (n)
    return true;
  return false;
}

/// Report the name, size and alignment changes of a type.
///
/// @param first the first type to consider.
///
/// @param second the second type to consider.
///
/// @param ctxt the content of the current diff.
///
/// @param out the output stream to report the change to.
///
/// @param indent the string to use for indentation.
///
/// @param nl whether to start the first report line with a new line.
///
/// @return true iff something was reported.
static bool
report_name_size_and_alignment_changes(decl_base_sptr		first,
				       decl_base_sptr		second,
				       diff_context_sptr	ctxt,
				       ostream&		out,
				       const string&		indent,
				       bool			nl)
{
  string fn = first->get_qualified_name(),
    sn = second->get_qualified_name();

  if (fn != sn)
    {
      if (!(ctxt->get_allowed_category() & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
	  && filtering::has_harmless_name_change(first, second))
	// This is a harmless name change.  but then
	// HARMLESS_DECL_NAME_CHANGE_CATEGORY doesn't seem allowed.
	;
      else
	{
	  if (nl)
	    out << "\n";
	  out << indent << "name changed from '"
	      << fn << "' to '" << sn << "'";
	  nl = true;
	}
    }

  nl |= report_size_and_alignment_changes(first, second, ctxt,
					  out, indent, nl);
  return nl;
}

/// Represent the kind of difference we want report_mem_header() to
/// report.
enum diff_kind
{
  del_kind,
  ins_kind,
  subtype_change_kind,
  change_kind
};

/// Output the header preceding the the report for
/// insertion/deletion/change of a part of a class.  This is a
/// subroutine of class_diff::report.
///
/// @param out the output stream to output the report to.
///
/// @param number the number of insertion/deletion to refer to in the
/// header.
///
/// @param k the kind of diff (insertion/deletion/change) we want the
/// head to introduce.
///
/// @param section_name the name of the sub-part of the class to
/// report about.
///
/// @param indent the string to use as indentation prefix in the
/// header.
static void
report_mem_header(ostream& out,
		  size_t number,
		  size_t num_filtered,
		  diff_kind k,
		  const string& section_name,
		  const string& indent)
{
  size_t net_number = number - num_filtered;
  string change;
  char colon_or_semi_colon = ':';

  switch (k)
    {
    case del_kind:
      change = (number > 1) ? "deletions" : "deletion";
      break;
    case ins_kind:
      change = (number > 1) ? "insertions" : "insertion";
      break;
    case subtype_change_kind:
    case change_kind:
      change = (number > 1) ? "changes" : "change";
      break;
    }

  if (net_number == 0)
    {
      out << indent << "no " << section_name << " " << change;
      colon_or_semi_colon = ';';
    }
  else if (net_number == 1)
    out << indent << "1 " << section_name << " " << change;
  else
    out << indent << net_number << " " << section_name
	<< " " << change;

  if (num_filtered)
    out << " (" << num_filtered << " filtered)";
  out << colon_or_semi_colon << '\n';
}

// <var_diff stuff>

/// The internal type for the impl idiom implementation of @ref
/// var_diff.
struct var_diff::priv
{
  diff_sptr type_diff_;
};//end struct var_diff

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref var_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
var_diff::chain_into_hierarchy()
{append_child_node(type_diff());}

/// @return the pretty representation for this current instance of
/// @ref var_diff.
const string&
var_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "var_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}
/// Constructor for @ref var_diff.
///
/// @param first the first instance of @ref var_decl to consider in
/// the diff.
///
/// @param second the second instance of @ref var_decl to consider in
/// the diff.
///
/// @param type_diff the diff between types of the instances of
/// var_decl.
///
/// @param ctxt the diff context to use.
var_diff::var_diff(var_decl_sptr	first,
		   var_decl_sptr	second,
		   diff_sptr		type_diff,
		   diff_context_sptr	ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv)
{priv_->type_diff_ = type_diff;}

/// Finish building the current instance of @ref var_diff.
void
var_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first @ref var_decl of the diff.
///
/// @return the first @ref var_decl of the diff.
var_decl_sptr
var_diff::first_var() const
{return dynamic_pointer_cast<var_decl>(first_subject());}

/// Getter for the second @ref var_decl of the diff.
///
/// @return the second @ref var_decl of the diff.
var_decl_sptr
var_diff::second_var() const
{return dynamic_pointer_cast<var_decl>(second_subject());}

/// Getter for the diff of the types of the instances of @ref
/// var_decl.
///
/// @return the diff of the types of the instances of @ref var_decl.
diff_sptr
var_diff::type_diff() const
{
  if (!priv_->type_diff_)
    priv_->type_diff_ = compute_diff(first_var()->get_type(),
				     second_var()->get_type(),
				     context());
  return priv_->type_diff_;
}

/// Return true iff the diff node has a change.
///
/// @return true iff the diff node has a change.
bool
var_diff::has_changes() const
{
  if (!!first_var() != !!second_var())
    return true;

  if (first_var().get() == second_var().get())
    return false;

  if (first_var()->get_hash() && second_var()->get_hash()
      && first_var()->get_hash() != second_var()->get_hash())
    return true;

  return *first_var() != *second_var();
}

/// @return true iff the current diff node carries local changes.
bool
var_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_var(), *second_var(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
var_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  decl_base_sptr first = first_var(), second = second_var();
  string n = first->get_pretty_representation();

  if (report_name_size_and_alignment_changes(first, second,
					     context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second, context(), out, indent);

  if (diff_sptr d = type_diff())
    {
      if (d->to_be_reported())
	{
	  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(d, "type");
	  out << indent << "type of variable changed:\n";
	  d->report(out, indent + " ");
	}
    }
}

/// Compute the diff between two instances of @ref var_decl.
///
/// @param first the first @ref var_decl to consider for the diff.
///
/// @param second the second @ref var_decl to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref var_decl.
var_diff_sptr
compute_diff(const var_decl_sptr	first,
	     const var_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  var_diff_sptr d(new var_diff(first, second, diff_sptr(), ctxt));
  ctxt->initialize_canonical_diff(d);
  return d;
}

// </var_diff stuff>

/// Report the differences in access specifiers and static-ness for
/// class members.
///
/// @param decl1 the first class member to consider.
///
/// @param decl2 the second class member to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the indentation string to use for the report.
///
/// @return true if something was reported, false otherwise.
static bool
maybe_report_diff_for_member(decl_base_sptr	decl1,
			     decl_base_sptr	decl2,
			     diff_context_sptr	ctxt,
			     ostream&		out,
			     const string&	indent)

{
  bool reported = false;
  if (!is_member_decl(decl1) || !is_member_decl(decl2))
    return reported;

  string decl1_repr = decl1->get_pretty_representation(),
    decl2_repr = decl2->get_pretty_representation();

  if (get_member_is_static(decl1) != get_member_is_static(decl2))
    {
      bool lost = get_member_is_static(decl1);
      out << indent << "'" << decl1_repr << "' ";
      if (lost)
	out << "became non-static";
      else
	out << "became static";
      out << "\n";
      reported = true;
    }
  if ((ctxt->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (get_member_access_specifier(decl1)
	  != get_member_access_specifier(decl2)))
    {
      out << indent << "'" << decl1_repr << "' access changed from '"
	  << get_member_access_specifier(decl1)
	  << "' to '"
	  << get_member_access_specifier(decl2)
	  << "'\n";
      reported = true;
    }
  return reported;
}

// <pointer_type_def stuff>
struct pointer_diff::priv
{
  diff_sptr underlying_type_diff_;

  priv(diff_sptr ud)
    : underlying_type_diff_(ud)
  {}
};//end struct pointer_diff::priv


/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref pointer_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
pointer_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for a pointer_diff.
///
/// @param first the first pointer to consider for the diff.
///
/// @param second the secon pointer to consider for the diff.
///
/// @param ctxt the diff context to use.
pointer_diff::pointer_diff(pointer_type_def_sptr	first,
			   pointer_type_def_sptr	second,
			   diff_sptr			underlying,
			   diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref pointer_diff.
void
pointer_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of a pointer diff
///
/// @return the first pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::first_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(first_subject());}

/// Getter for the second subject of a pointer diff
///
/// @return the second pointer considered in this pointer diff.
const pointer_type_def_sptr
pointer_diff::second_pointer() const
{return dynamic_pointer_cast<pointer_type_def>(second_subject());}

/// @return the pretty represenation for the current instance of @ref
/// pointer_diff.
const string&
pointer_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "pointer_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
pointer_diff::has_changes() const
{
  return underlying_type_diff()
    ? underlying_type_diff()->has_changes()
    : false;
}

/// @return true iff the current diff node carries local changes.
bool
pointer_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_pointer(), *second_pointer(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Getter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @return the diff between the pointed-to types.
diff_sptr
pointer_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the pointed-to types of the pointers
/// of this diff.
///
/// @param d the new diff between the pointed-to types of the pointers
/// of this diff.
void
pointer_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
pointer_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(d, "pointed to type");
      string repr = d->first_subject()
	? d->first_subject()->get_pretty_representation()
	: string("void");

      out << indent
	  << "in pointed to type '" <<  repr << "':\n";
      d->report(out, indent + "  ");
    }
}

/// Compute the diff between between two pointers.
///
/// @param first the pointer to consider for the diff.
///
/// @param second the pointer to consider for the diff.
///
/// @return the resulting diff between the two pointers.
///
/// @param ctxt the diff context to use.
pointer_diff_sptr
compute_diff(pointer_type_def_sptr	first,
	     pointer_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  pointer_diff_sptr result(new pointer_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);

  return result;
}

// </pointer_type_def>

// <array_type_def>
struct array_diff::priv
{
  /// The diff between the two array element types.
  diff_sptr element_type_diff_;

  priv(diff_sptr element_type_diff)
    : element_type_diff_(element_type_diff)
  {}
};//end struct array_diff::priv

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref array_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
array_diff::chain_into_hierarchy()
{append_child_node(element_type_diff());}

/// Constructor for array_diff
///
/// @param first the first array_type of the diff.
///
/// @param second the second array_type of the diff.
///
/// @param element_type_diff the diff between the two array element
/// types.
///
/// @param ctxt the diff context to use.
array_diff::array_diff(const array_type_def_sptr	first,
		       const array_type_def_sptr	second,
		       diff_sptr			element_type_diff,
		       diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(element_type_diff))
{}

/// Finish building the current instance of @ref array_diff.
void
array_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first array of the diff.
///
/// @return the first array of the diff.
const array_type_def_sptr
array_diff::first_array() const
{return dynamic_pointer_cast<array_type_def>(first_subject());}

/// Getter for the second array of the diff.
///
/// @return for the second array of the diff.
const array_type_def_sptr
array_diff::second_array() const
{return dynamic_pointer_cast<array_type_def>(second_subject());}

/// Getter for the diff between the two types of array elements.
///
/// @return the diff between the two types of array elements.
const diff_sptr&
array_diff::element_type_diff() const
{return priv_->element_type_diff_;}

/// Setter for the diff between the two array element types.
///
/// @param d the new diff betweend the two array element types.
void
array_diff::element_type_diff(diff_sptr d)
{priv_->element_type_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// array_diff.
const string&
array_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "array_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
array_diff::has_changes() const
{
  bool l = false;

  //  the array element types match check for differing dimensions
  //  etc...
  array_type_def_sptr
    f = dynamic_pointer_cast<array_type_def>(first_subject()),
    s = dynamic_pointer_cast<array_type_def>(second_subject());

  if (f->get_name() != s->get_name())
    l |= true;
  if (f->get_size_in_bits() != s->get_size_in_bits())
    l |= true;
  if (f->get_alignment_in_bits() != s->get_alignment_in_bits())
    l |= true;

  l |=  element_type_diff()
    ? element_type_diff()->has_changes()
    : false;

  return l;
}

/// @return true iff the current diff node carries local changes.
bool
array_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_array(), *second_array(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
array_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string name = first_array()->get_pretty_representation();
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(first_array(),
						    second_array(),
						    "array type");

  diff_sptr d = element_type_diff();
  if (d->to_be_reported())
    {
      // report array element type changes
      out << indent << "array element type of '"
	  << name << "' changed:\n";
      d->report(out, indent + "  ");
    }

  const array_type_def_sptr x = first_array(), y = second_array();

  if ((x->get_size_in_bits() != y->get_size_in_bits())
      || (x->get_dimension_count() != y->get_dimension_count()))
    {
      out << indent
	  << "in array type '"
	  << name
	  << "':\n";
      if (x->get_size_in_bits() != y->get_size_in_bits())
	{
	  out << indent + "  "
	      << "size changed from ";

	  if (x->is_infinite())
	    out << "infinity";
	  else
	    out << x->get_size_in_bits();

	  out << " to ";

	  if (y->is_infinite())
	    out << "infinity";
	  else
	    out << y->get_size_in_bits();

	  out << "\n";
	}

      size_t a, b;
      if ((a = x->get_dimension_count())
	  != (b = y->get_dimension_count()))
	{
	  out << indent + "  "
	      << "number of dimensions changed from "
	      << a
	      << " to "
	      << b
	      << "\n";
	}
      else
	{
	  array_type_def::subranges_type::const_iterator i, j;
	  for (i = x->get_subranges().begin(),
		 j = y->get_subranges().begin();
	       i != x->get_subranges().end();
	       ++i, ++j)
	    {
	      if ((*i)->get_length() != (*j)->get_length())
		{
		  out << indent + "  "
		      << "subrange "
		      << i - x->get_subranges().begin() + 1
		      << " changed length from ";

		  if ((*i)->is_infinite())
		    out << "infinity";
		  else
		    out << (*i)->get_length();

		  out << " to ";

		  if ((*j)->is_infinite())
		    out << "infinity";
		  else
		    out << (*j)->get_length();

		  out << "\n";
		}
	    }
	}
    }
}

/// Compute the diff between two arrays.
///
/// @param first the first array to consider for the diff.
///
/// @param second the second array to consider for the diff.
///
/// @param ctxt the diff context to use.
array_diff_sptr
compute_diff(array_type_def_sptr	first,
	     array_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  diff_sptr d = compute_diff_for_types(first->get_element_type(),
				       second->get_element_type(),
				       ctxt);
  array_diff_sptr result(new array_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}
// </array_type_def>

// <reference_type_def>
struct reference_diff::priv
{
  diff_sptr underlying_type_diff_;
  priv(diff_sptr underlying)
    : underlying_type_diff_(underlying)
  {}
};//end struct reference_diff::priv

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref reference_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
reference_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for reference_diff
///
/// @param first the first reference_type of the diff.
///
/// @param second the second reference_type of the diff.
///
/// @param ctxt the diff context to use.
reference_diff::reference_diff(const reference_type_def_sptr	first,
			       const reference_type_def_sptr	second,
			       diff_sptr			underlying,
			       diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
	priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref reference_diff.
void
reference_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first reference of the diff.
///
/// @return the first reference of the diff.
reference_type_def_sptr
reference_diff::first_reference() const
{return dynamic_pointer_cast<reference_type_def>(first_subject());}

/// Getter for the second reference of the diff.
///
/// @return for the second reference of the diff.
reference_type_def_sptr
reference_diff::second_reference() const
{return dynamic_pointer_cast<reference_type_def>(second_subject());}


/// Getter for the diff between the two referred-to types.
///
/// @return the diff between the two referred-to types.
const diff_sptr&
reference_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two referred-to types.
///
/// @param d the new diff betweend the two referred-to types.
diff_sptr&
reference_diff::underlying_type_diff(diff_sptr d)
{
  priv_->underlying_type_diff_ = d;
  return priv_->underlying_type_diff_;
}

/// @return the pretty representation for the current instance of @ref
/// reference_diff.
const string&
reference_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "reference_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
reference_diff::has_changes() const
{
  return underlying_type_diff()
    ? underlying_type_diff()->has_changes()
    : false;
}

/// @return true iff the current diff node carries local changes.
bool
reference_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_reference(), *second_reference(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
reference_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  if (diff_sptr d = underlying_type_diff())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(d, "referenced type");
      out << indent
	  << "in referenced type '"
	  << d->first_subject()->get_pretty_representation()
	  << "':\n";
      d->report(out, indent + "  ");
    }
}

/// Compute the diff between two references.
///
/// @param first the first reference to consider for the diff.
///
/// @param second the second reference to consider for the diff.
///
/// @param ctxt the diff context to use.
reference_diff_sptr
compute_diff(reference_type_def_sptr	first,
	     reference_type_def_sptr	second,
	     diff_context_sptr		ctxt)
{
  diff_sptr d = compute_diff_for_types(first->get_pointed_to_type(),
				       second->get_pointed_to_type(),
				       ctxt);
  reference_diff_sptr result(new reference_diff(first, second, d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}
// </reference_type_def>

// <qualified_type_diff stuff>

struct qualified_type_diff::priv
{
  diff_sptr underlying_type_diff;
  mutable diff_sptr leaf_underlying_type_diff;

  priv(diff_sptr underlying)
    : underlying_type_diff(underlying)
  {}
};// end struct qualified_type_diff::priv

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref qualified_type_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
qualified_type_diff::chain_into_hierarchy()
{append_child_node(leaf_underlying_type_diff());}

/// Constructor for qualified_type_diff.
///
/// @param first the first qualified type of the diff.
///
/// @param second the second qualified type of the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff::qualified_type_diff(qualified_type_def_sptr	first,
					 qualified_type_def_sptr	second,
					 diff_sptr			under,
					 diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(under))
{}

/// Finish building the current instance of @ref qualified_type_diff.
void
qualified_type_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first qualified type of the diff.
///
/// @return the first qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::first_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(first_subject());}

/// Getter for the second qualified type of the diff.
///
/// @return the second qualified type of the diff.
const qualified_type_def_sptr
qualified_type_diff::second_qualified_type() const
{return dynamic_pointer_cast<qualified_type_def>(second_subject());}

/// Getter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
diff_sptr
qualified_type_diff::underlying_type_diff() const
{return priv_->underlying_type_diff;}

/// Getter for the diff between the most underlying non-qualified
/// types of two qualified types.
///
/// @return the diff between the most underlying non-qualified types
/// of two qualified types.
diff_sptr
qualified_type_diff::leaf_underlying_type_diff() const
{
  if (!priv_->leaf_underlying_type_diff)
    priv_->leaf_underlying_type_diff
      = compute_diff_for_types(get_leaf_type(first_qualified_type()),
			       get_leaf_type(second_qualified_type()),
			       context());

  return priv_->leaf_underlying_type_diff;
}

/// Setter for the diff between the underlying types of the two
/// qualified types.
///
/// @return the diff between the underlying types of the two qualified
/// types.
void
qualified_type_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff = d;}

/// @return the pretty representation of the current instance of @ref
/// qualified_type_diff.
const string&
qualified_type_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "qualified_type_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
qualified_type_diff::has_changes() const
{
  bool l = false;
  char fcv = first_qualified_type()->get_cv_quals(),
    scv = second_qualified_type()->get_cv_quals();

  if (fcv != scv)
    {
      if ((fcv & qualified_type_def::CV_CONST)
	  != (scv & qualified_type_def::CV_CONST))
	l |= true;
      if ((fcv & qualified_type_def::CV_VOLATILE)
	  != (scv & qualified_type_def::CV_RESTRICT))
	l |= true;
      if ((fcv & qualified_type_def::CV_RESTRICT)
	  != (scv & qualified_type_def::CV_RESTRICT))
	l |= true;
    }

  return (underlying_type_diff()
	  ? underlying_type_diff()->has_changes() || l
	  : l);
}

/// @return true iff the current diff node carries local changes.
bool
qualified_type_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_qualified_type(), *second_qualified_type(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Return the first underlying type that is not a qualified type.
/// @param t the qualified type to consider.
///
/// @return the first underlying type that is not a qualified type, or
/// NULL if t is NULL.
static type_base_sptr
get_leaf_type(qualified_type_def_sptr t)
{
  if (!t)
    return type_base_sptr();

  type_base_sptr ut = t->get_underlying_type();
  qualified_type_def_sptr qut = dynamic_pointer_cast<qualified_type_def>(ut);

  if (!qut)
    return ut;
  return get_leaf_type(qut);
}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize to.
///
/// @param indent the string to use to indent the lines of the report.
void
qualified_type_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string fname = first_qualified_type()->get_pretty_representation(),
    sname = second_qualified_type()->get_pretty_representation();

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(first_qualified_type(),
						   second_qualified_type());

  if (fname != sname)
    {
      out << indent << "'" << fname << "' changed to '" << sname << "'\n";
      return;
    }

  diff_sptr d = leaf_underlying_type_diff();
  assert(d);
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(d,
						    "unqualified "
						    "underlying type");

  string fltname = d->first_subject()->get_pretty_representation();
  out << indent << "in unqualified underlying type '" << fltname << "':\n";
  d->report(out, indent + "  ");
}

/// Compute the diff between two qualified types.
///
/// @param first the first qualified type to consider for the diff.
///
/// @param second the second qualified type to consider for the diff.
///
/// @param ctxt the diff context to use.
qualified_type_diff_sptr
compute_diff(const qualified_type_def_sptr	first,
	     const qualified_type_def_sptr	second,
	     diff_context_sptr			ctxt)
{
  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  qualified_type_diff_sptr result(new qualified_type_diff(first, second,
							  d, ctxt));
  ctxt->initialize_canonical_diff(result);
  return result;
}

// </qualified_type_diff stuff>

// <enum_diff stuff>
struct enum_diff::priv
{
  diff_sptr underlying_type_diff_;
  edit_script enumerators_changes_;
  string_enumerator_map deleted_enumerators_;
  string_enumerator_map inserted_enumerators_;
  string_changed_enumerator_map changed_enumerators_;

  priv(diff_sptr underlying)
    : underlying_type_diff_(underlying)
  {}
};//end struct enum_diff::priv

/// Clear the lookup tables useful for reporting an enum_diff.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
enum_diff::clear_lookup_tables()
{
  priv_->deleted_enumerators_.clear();
  priv_->inserted_enumerators_.clear();
  priv_->changed_enumerators_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
enum_diff::lookup_tables_empty() const
{
  return (priv_->deleted_enumerators_.empty()
	  && priv_->inserted_enumerators_.empty()
	  && priv_->changed_enumerators_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
enum_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script e = priv_->enumerators_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	const enum_type_decl::enumerator& n =
	  first_enum()->get_enumerators()[i];
	const string& name = n.get_name();
	assert(priv_->deleted_enumerators_.find(n.get_name())
	       == priv_->deleted_enumerators_.end());
	priv_->deleted_enumerators_[name] = n;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    const enum_type_decl::enumerator& n =
	      second_enum()->get_enumerators()[i];
	    const string& name = n.get_name();
	    assert(priv_->inserted_enumerators_.find(n.get_name())
		   == priv_->inserted_enumerators_.end());
	    string_enumerator_map::const_iterator j =
	      priv_->deleted_enumerators_.find(name);
	    if (j == priv_->deleted_enumerators_.end())
	      priv_->inserted_enumerators_[name] = n;
	    else
	      {
		if (j->second != n)
		  priv_->changed_enumerators_[j->first] =
		    std::make_pair(j->second, n);
		priv_->deleted_enumerators_.erase(j);
	      }
	  }
      }
  }
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref enum_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
enum_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for enum_diff.
///
/// @param first the first enum type of the diff.
///
/// @param second the second enum type of the diff.
///
/// @param underlying_type_diff the diff of the two underlying types
/// of the two enum types.
///
/// @param ctxt the diff context to use.
enum_diff::enum_diff(const enum_type_decl_sptr	first,
		     const enum_type_decl_sptr	second,
		     const diff_sptr		underlying_type_diff,
		     const diff_context_sptr	ctxt)
  : type_diff_base(first, second,ctxt),
    priv_(new priv(underlying_type_diff))
{}

/// Finish building the current instance of @ref enum_diff.
void
enum_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the first enum of the diff.
const enum_type_decl_sptr
enum_diff::first_enum() const
{return dynamic_pointer_cast<enum_type_decl>(first_subject());}

/// @return the second enum of the diff.
const enum_type_decl_sptr
enum_diff::second_enum() const
{return dynamic_pointer_cast<enum_type_decl>(second_subject());}

/// @return the diff of the two underlying enum types.
diff_sptr
enum_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// @return a map of the enumerators that were deleted.
const string_enumerator_map&
enum_diff::deleted_enumerators() const
{return priv_->deleted_enumerators_;}

/// @return a map of the enumerators that were inserted
const string_enumerator_map&
enum_diff::inserted_enumerators() const
{return priv_->inserted_enumerators_;}

/// @return a map the enumerators that were changed
const string_changed_enumerator_map&
enum_diff::changed_enumerators() const
{return priv_->changed_enumerators_;}

/// @return the pretty representation of the current instance of @ref
/// enum_diff.
const string&
enum_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "enum_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
enum_diff::has_changes() const
{return first_enum() != second_enum();}

/// @return true iff the current diff node carries local changes.
bool
enum_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_enum(), *second_enum(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// A functor to compare two enumerators based on their value.  This
/// implements the "less than" operator.
struct enumerator_value_comp
{
  bool
  operator()(const enum_type_decl::enumerator& f,
	     const enum_type_decl::enumerator& s) const
  {return f.get_value() < s.get_value();}
};//end struct enumerator_value_comp

/// Sort a map of enumerators by their value.
///
/// @param enumerators_map the map to sort.
///
/// @param sorted the resulting vector of sorted enumerators.
static void
sort_enumerators(const string_enumerator_map& enumerators_map,
		 enum_type_decl::enumerators& sorted)
{
  for (string_enumerator_map::const_iterator i = enumerators_map.begin();
       i != enumerators_map.end();
       ++i)
    sorted.push_back(i->second);
  enumerator_value_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// A functor to compare two changed enumerators, based on their
/// initial value.
struct changed_enumerator_comp
{
  bool
  operator()(const changed_enumerator& f,
	     const changed_enumerator& s) const
  {return f.first.get_value() < s.first.get_value();}
};// end struct changed_enumerator_comp.

/// Sort a map of changed enumerators.
///
/// @param enumerators_map the map to sort.
///
///@param output parameter.  The resulting sorted enumerators.
void
sort_changed_enumerators(const string_changed_enumerator_map& enumerators_map,
			 changed_enumerators_type& sorted)
{
  for (string_changed_enumerator_map::const_iterator i =
	 enumerators_map.begin();
       i != enumerators_map.end();
       ++i)
    sorted.push_back(i->second);

  changed_enumerator_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Report the differences between the two enums.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
enum_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string name = first_enum()->get_pretty_representation();

  enum_type_decl_sptr first = first_enum(), second = second_enum();

  if (report_name_size_and_alignment_changes(first, second, context(),
					     out, indent,
					     /*start_with_num_line=*/false))
    out << "\n";
  maybe_report_diff_for_member(first, second, context(), out, indent);

  //underlying type
  underlying_type_diff()->report(out, indent);

  //report deletions/insertions/change of enumerators
  unsigned numdels = deleted_enumerators().size();
  unsigned numins = inserted_enumerators().size();
  unsigned numchanges = changed_enumerators().size();

  if (numdels)
    {
      report_mem_header(out, numdels, 0, del_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_deleted_enumerators;
      sort_enumerators(deleted_enumerators(), sorted_deleted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_deleted_enumerators.begin();
	   i != sorted_deleted_enumerators.end();
	   ++i)
	{
	  if (i != sorted_deleted_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->get_qualified_name(first)
	      << "' value '"
	      << i->get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numins)
    {
      report_mem_header(out, numins, 0, ins_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_inserted_enumerators;
      sort_enumerators(inserted_enumerators(), sorted_inserted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_inserted_enumerators.begin();
	   i != sorted_inserted_enumerators.end();
	   ++i)
	{
	  if (i != sorted_inserted_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->get_qualified_name(second)
	      << "' value '"
	      << i->get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numchanges)
    {
      report_mem_header(out, numchanges, 0, change_kind, "enumerator", indent);
      changed_enumerators_type sorted_changed_enumerators;
      sort_changed_enumerators(changed_enumerators(),
			       sorted_changed_enumerators);
      for (changed_enumerators_type::const_iterator i =
	     sorted_changed_enumerators.begin();
	   i != sorted_changed_enumerators.end();
	   ++i)
	{
	  if (i != sorted_changed_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->first.get_qualified_name(first)
	      << "' from value '"
	      << i->first.get_value() << "' to '"
	      << i->second.get_value() << "'";
	}
      out << "\n\n";
    }
}

/// Compute the set of changes between two instances of @ref
/// enum_type_decl.
///
/// @param first a pointer to the first enum_type_decl to consider.
///
/// @param second a pointer to the second enum_type_decl to consider.
///
/// @return the resulting diff of the two enums @p first and @p
/// second.
///
/// @param ctxt the diff context to use.
enum_diff_sptr
compute_diff(const enum_type_decl_sptr first,
	     const enum_type_decl_sptr second,
	     diff_context_sptr ctxt)
{
  diff_sptr ud = compute_diff_for_types(first->get_underlying_type(),
					second->get_underlying_type(),
					ctxt);
  enum_diff_sptr d(new enum_diff(first, second, ud, ctxt));

  compute_diff(first->get_enumerators().begin(),
	       first->get_enumerators().end(),
	       second->get_enumerators().begin(),
	       second->get_enumerators().end(),
	       d->priv_->enumerators_changes_);

  d->ensure_lookup_tables_populated();

  ctxt->initialize_canonical_diff(d);

  return d;
}
// </enum_diff stuff>

//<class_diff stuff>

struct class_diff::priv
{
  edit_script base_changes_;
  edit_script member_types_changes_;
  edit_script data_members_changes_;
  edit_script member_fns_changes_;
  edit_script member_fn_tmpls_changes_;
  edit_script member_class_tmpls_changes_;

  string_base_sptr_map deleted_bases_;
  string_base_sptr_map inserted_bases_;
  string_base_diff_sptr_map changed_bases_;
  base_diff_sptrs_type sorted_changed_bases_;
  string_decl_base_sptr_map deleted_member_types_;
  string_decl_base_sptr_map inserted_member_types_;
  string_diff_sptr_map changed_member_types_;
  diff_sptrs_type sorted_changed_member_types_;
  string_decl_base_sptr_map deleted_data_members_;
  unsigned_decl_base_sptr_map deleted_dm_by_offset_;
  string_decl_base_sptr_map inserted_data_members_;
  unsigned_decl_base_sptr_map inserted_dm_by_offset_;
  // This map contains the data member which sub-type changed.
  string_var_diff_sptr_map subtype_changed_dm_;
  var_diff_sptrs_type sorted_subtype_changed_dm_;
  // This one contains the list of data members changes that can be
  // represented as a data member foo that got removed from offset N,
  // and a data member bar that got inserted at offset N; IOW, this
  // can be translated as data member foo that got changed into data
  // member bar at offset N.
  unsigned_var_diff_sptr_map changed_dm_;
  var_diff_sptrs_type sorted_changed_dm_;
  string_member_function_sptr_map deleted_member_functions_;
  string_member_function_sptr_map inserted_member_functions_;
  string_function_decl_diff_sptr_map changed_member_functions_;
  function_decl_diff_sptrs_type sorted_changed_member_functions_;
  string_decl_base_sptr_map deleted_member_class_tmpls_;
  string_decl_base_sptr_map inserted_member_class_tmpls_;
  string_diff_sptr_map changed_member_class_tmpls_;
  diff_sptrs_type sorted_changed_member_class_tmpls_;

  class_decl::base_spec_sptr
  base_has_changed(class_decl::base_spec_sptr) const;

  decl_base_sptr
  member_type_has_changed(decl_base_sptr) const;

  decl_base_sptr
  subtype_changed_dm(decl_base_sptr) const;

  decl_base_sptr
  member_class_tmpl_has_changed(decl_base_sptr) const;

  size_t
  count_filtered_bases();

  size_t
  count_filtered_subtype_changed_dm();

  size_t
  count_filtered_changed_dm();

  size_t
  count_filtered_changed_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_inserted_mem_fns(const diff_context_sptr&);

  size_t
  count_filtered_deleted_mem_fns(const diff_context_sptr&);

  priv()
  {}
};//end struct class_diff::priv

/// Clear the lookup tables useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed from the class_diff::priv.
void
class_diff::clear_lookup_tables(void)
{
  priv_->deleted_bases_.clear();
  priv_->inserted_bases_.clear();
  priv_->changed_bases_.clear();
  priv_->deleted_member_types_.clear();
  priv_->inserted_member_types_.clear();
  priv_->changed_member_types_.clear();
  priv_->deleted_data_members_.clear();
  priv_->inserted_data_members_.clear();
  priv_->subtype_changed_dm_.clear();
  priv_->deleted_member_functions_.clear();
  priv_->inserted_member_functions_.clear();
  priv_->changed_member_functions_.clear();
  priv_->deleted_member_class_tmpls_.clear();
  priv_->inserted_member_class_tmpls_.clear();
  priv_->changed_member_class_tmpls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
class_diff::lookup_tables_empty(void) const
{
  return (priv_->deleted_bases_.empty()
	  && priv_->inserted_bases_.empty()
	  && priv_->changed_bases_.empty()
	  && priv_->deleted_member_types_.empty()
	  && priv_->inserted_member_types_.empty()
	  && priv_->changed_member_types_.empty()
	  && priv_->deleted_data_members_.empty()
	  && priv_->inserted_data_members_.empty()
	  && priv_->subtype_changed_dm_.empty()
	  && priv_->inserted_member_functions_.empty()
	  && priv_->deleted_member_functions_.empty()
	  && priv_->changed_member_functions_.empty()
	  && priv_->deleted_member_class_tmpls_.empty()
	  && priv_->inserted_member_class_tmpls_.empty()
	  && priv_->changed_member_class_tmpls_.empty());
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
class_diff::ensure_lookup_tables_populated(void) const
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script& e = priv_->base_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	class_decl::base_spec_sptr b =
	  first_class_decl()->get_base_specifiers()[i];
	string qname = b->get_base_class()->get_qualified_name();
	assert(priv_->deleted_bases_.find(qname)
	       == priv_->deleted_bases_.end());
	priv_->deleted_bases_[qname] = b;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    class_decl::base_spec_sptr b =
	      second_class_decl()->get_base_specifiers()[i];
	    string qname = b->get_base_class()->get_qualified_name();
	    assert(priv_->inserted_bases_.find(qname)
		   == priv_->inserted_bases_.end());
	    string_base_sptr_map::const_iterator j =
	      priv_->deleted_bases_.find(qname);
	    if (j != priv_->deleted_bases_.end())
	      {
		if (j->second != b)
		  priv_->changed_bases_[qname] =
		    compute_diff(j->second, b, context());
		priv_->deleted_bases_.erase(j);
	      }
	    else
	      priv_->inserted_bases_[qname] = b;
	  }
      }
  }

  sort_string_base_diff_sptr_map(priv_->changed_bases_,
				 priv_->sorted_changed_bases_);

  {
    edit_script& e = priv_->member_types_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  get_type_declaration(first_class_decl()->get_member_types()[i]);
	class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(d);
	if (klass_decl && klass_decl->get_is_declaration_only())
	  continue;
	string qname = d->get_qualified_name();
	priv_->deleted_member_types_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      get_type_declaration(second_class_decl()->get_member_types()[i]);
	    class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(d);
	    if (klass_decl && klass_decl->get_is_declaration_only())
	      continue;
	    string qname = d->get_qualified_name();
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_types_.find(qname);
	    if (j != priv_->deleted_member_types_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[qname] =
		    compute_diff(j->second, d, context());

		priv_->deleted_member_types_.erase(j);
	      }
	    else
	      priv_->inserted_member_types_[qname] = d;
	  }
      }
  }

  {
    edit_script& e = priv_->data_members_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d = first_class_decl()->get_data_members()[i];
	string qname = d->get_qualified_name();
	assert(priv_->deleted_data_members_.find(qname)
	       == priv_->deleted_data_members_.end());
	priv_->deleted_data_members_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d = second_class_decl()->get_data_members()[i];
	    var_decl_sptr dm = dynamic_pointer_cast<var_decl>(d);
	    string qname = dm->get_qualified_name();
	    assert(priv_->inserted_data_members_.find(qname)
		   == priv_->inserted_data_members_.end());
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_data_members_.find(qname);
	    if (j != priv_->deleted_data_members_.end())
	      {
		if (*j->second != *d)
		  {
		    var_decl_sptr old_dm =
		      dynamic_pointer_cast<var_decl>(j->second);
		    priv_->subtype_changed_dm_[qname]=
		      compute_diff(old_dm, dm, context());
		  }
		priv_->deleted_data_members_.erase(j);
	      }
	    else
	      priv_->inserted_data_members_[qname] = d;
	  }
      }

    // Now detect when a data member is deleted from offset N and
    // another one is added to offset N.  In that case, we want to be
    // able to say that the data member at offset N changed.
    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->deleted_data_members_.begin();
	 i != priv_->deleted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->deleted_dm_by_offset_[offset] = i->second;
      }

    for (string_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_data_members_.begin();
	 i != priv_->inserted_data_members_.end();
	 ++i)
      {
	unsigned offset = get_data_member_offset(i->second);
	priv_->inserted_dm_by_offset_[offset] = i->second;
      }

    for (unsigned_decl_base_sptr_map::const_iterator i =
	   priv_->inserted_dm_by_offset_.begin();
	 i != priv_->inserted_dm_by_offset_.end();
	 ++i)
      {
	unsigned_decl_base_sptr_map::const_iterator j =
	  priv_->deleted_dm_by_offset_.find(i->first);
	if (j != priv_->deleted_dm_by_offset_.end())
	  {
	    var_decl_sptr old_dm = dynamic_pointer_cast<var_decl>(j->second);
	    var_decl_sptr new_dm = dynamic_pointer_cast<var_decl>(i->second);
	    priv_->changed_dm_[i->first] =
	      compute_diff(old_dm, new_dm, context());
	  }
      }

    for (unsigned_var_diff_sptr_map::const_iterator i =
	   priv_->changed_dm_.begin();
	 i != priv_->changed_dm_.end();
	 ++i)
      {
	priv_->deleted_dm_by_offset_.erase(i->first);
	priv_->inserted_dm_by_offset_.erase(i->first);
	priv_->deleted_data_members_.erase
	  (i->second->first_var()->get_qualified_name());
	priv_->inserted_data_members_.erase
	  (i->second->second_var()->get_qualified_name());
      }
  }
  sort_string_data_member_diff_sptr_map(priv_->subtype_changed_dm_,
					priv_->sorted_subtype_changed_dm_);
  sort_unsigned_data_member_diff_sptr_map(priv_->changed_dm_,
					  priv_->sorted_changed_dm_);


  {
    edit_script& e = priv_->member_fns_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	class_decl::method_decl_sptr mem_fn =
	  first_class_decl()->get_virtual_mem_fns()[i];
	string name = mem_fn->get_linkage_name();
	if (name.empty())
	  name = mem_fn->get_pretty_representation();
	assert(!name.empty());
	if (priv_->deleted_member_functions_.find(name)
	    != priv_->deleted_member_functions_.end())
	  continue;
	priv_->deleted_member_functions_[name] = mem_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;

	    class_decl::method_decl_sptr mem_fn =
	      second_class_decl()->get_virtual_mem_fns()[i];
	    string name = mem_fn->get_linkage_name();
	    if (name.empty())
	      name = mem_fn->get_pretty_representation();
	    assert(!name.empty());
	    if (priv_->inserted_member_functions_.find(name)
		!= priv_->inserted_member_functions_.end())
	      continue;
	    string_member_function_sptr_map::const_iterator j =
	      priv_->deleted_member_functions_.find(name);
	    if (j == priv_->deleted_member_functions_.end()
		&& name == mem_fn->get_linkage_name())
	      {
		// So we didn't find a function with the same mangled
		// named that got previously deleted.  Keep in mind
		// that in some case, the mangled name can be missing
		// from DWARF (bug), so we might not find the deleted
		// function because of that.  So to work around that
		// possible bug, let's try to refer to that function
		// using its pretty representation then.
		string pretty_name = mem_fn->get_pretty_representation();
		j = priv_->deleted_member_functions_.find(pretty_name);
		if (j != priv_->deleted_member_functions_.end())
		  name = pretty_name;
	      }
	    if (j != priv_->deleted_member_functions_.end())
	      {
		if (*j->second != *mem_fn)
		  priv_->changed_member_functions_[name] =
		    compute_diff(static_pointer_cast<function_decl>(j->second),
				 static_pointer_cast<function_decl>(mem_fn),
				 context());
		priv_->deleted_member_functions_.erase(j);
	      }
	    else
	      priv_->inserted_member_functions_[name] = mem_fn;
	  }
      }

    // Now walk the allegedly deleted member functions; check if their
    // underlying symbols are deleted as well; otherwise, consider
    // that the member function in question hasn't been deleted.

    vector<string> to_delete;
    corpus_sptr f = context()->get_first_corpus(),
      s = context()->get_second_corpus();;
    if (s)
      for (string_member_function_sptr_map::const_iterator i =
	     deleted_member_fns().begin();
	   i != deleted_member_fns().end();
	   ++i)
	// We assume that all the functions we look at here have ELF
	// symbols.
	if (i->second->get_symbol()
	    && s->lookup_function_symbol(i->second->get_symbol()->get_name(),
					 i->second->get_symbol()->get_version().str()))
	  to_delete.push_back(i->first);


    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      priv_->deleted_member_functions_.erase(*i);

    // Do something similar for added functions.
    to_delete.clear();
    if (f)
      for (string_member_function_sptr_map::const_iterator i =
	     inserted_member_fns().begin();
	   i != inserted_member_fns().end();
	   ++i)
	if (i->second->get_symbol()
	    && f->lookup_function_symbol(i->second->get_symbol()->get_name(),
					 i->second->get_symbol()->get_version().str()))
	  to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      priv_->inserted_member_functions_.erase(*i);
  }

  sort_string_virtual_member_function_diff_sptr_map
    (priv_->changed_member_functions_,
     priv_->sorted_changed_member_functions_);

  {
    edit_script& e = priv_->member_class_tmpls_changes_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	decl_base_sptr d =
	  first_class_decl()->get_member_class_templates()[i]->
	  as_class_tdecl();
	string qname = d->get_qualified_name();
	assert(priv_->deleted_member_class_tmpls_.find(qname)
	       == priv_->deleted_member_class_tmpls_.end());
	priv_->deleted_member_class_tmpls_[qname] = d;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    decl_base_sptr d =
	      second_class_decl()->get_member_class_templates()[i]->
	      as_class_tdecl();
	    string qname = d->get_qualified_name();
	    assert(priv_->inserted_member_class_tmpls_.find(qname)
		   == priv_->inserted_member_class_tmpls_.end());
	    string_decl_base_sptr_map::const_iterator j =
	      priv_->deleted_member_class_tmpls_.find(qname);
	    if (j != priv_->deleted_member_class_tmpls_.end())
	      {
		if (*j->second != *d)
		  priv_->changed_member_types_[qname]=
		    compute_diff(j->second, d, context());
		priv_->deleted_member_class_tmpls_.erase(j);
	      }
	    else
	      priv_->inserted_member_class_tmpls_[qname] = d;
	  }
      }
  }
  sort_string_diff_sptr_map(priv_->changed_member_types_,
			    priv_->sorted_changed_member_types_);
}

/// Test whether a given base class has changed.  A base class has
/// changed if it's in both in deleted *and* inserted bases.
///
///@param d the declaration for the base class to consider.
///
/// @return the new base class if the given base class has changed, or
/// NULL if it hasn't.
class_decl::base_spec_sptr
class_diff::priv::base_has_changed(class_decl::base_spec_sptr d) const
{
  string qname = d->get_base_class()->get_qualified_name();
  string_base_diff_sptr_map::const_iterator it =
    changed_bases_.find(qname);

  return (it == changed_bases_.end())
    ? class_decl::base_spec_sptr()
    : it->second->second_base();

}

/// Test whether a given member type has changed.
///
/// @param d the declaration for the member type to consider.
///
/// @return the new member type if the given member type has changed,
/// or NULL if it hasn't.
decl_base_sptr
class_diff::priv::member_type_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_diff_sptr_map::const_iterator it =
    changed_member_types_.find(qname);

  return ((it == changed_member_types_.end())
	  ? decl_base_sptr()
	  : it->second->second_subject());
}

/// Test whether a given data member has changed.
///
/// @param d the declaration for the data member to consider.
///
/// @return the new data member if the given data member has changed,
/// or NULL if if hasn't.
decl_base_sptr
class_diff::priv::subtype_changed_dm(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_var_diff_sptr_map::const_iterator it =
    subtype_changed_dm_.find(qname);

  if (it == subtype_changed_dm_.end())
    return decl_base_sptr();
  return it->second->second_var();
}

/// Test whether a given member class template has changed.
///
/// @param d the declaration for the given member class template to consider.
///
/// @return the new member class template if the given one has
/// changed, or NULL if it hasn't.
decl_base_sptr
class_diff::priv::member_class_tmpl_has_changed(decl_base_sptr d) const
{
  string qname = d->get_qualified_name();
  string_diff_sptr_map::const_iterator it =
    changed_member_class_tmpls_.find(qname);

  return ((it == changed_member_class_tmpls_.end())
	  ? decl_base_sptr()
	  : dynamic_pointer_cast<decl_base>(it->second->second_subject()));
}

/// Count the number of bases classes whose changes got filtered out.
///
/// @return the number of bases classes whose changes got filtered
/// out.
size_t
class_diff::priv::count_filtered_bases()
{
  size_t num_filtered = 0;
  for (base_diff_sptrs_type::const_iterator i = sorted_changed_bases_.begin();
       i != sorted_changed_bases_.end();
       ++i)
    {
      diff_sptr diff = *i;
      if (diff && diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;
}

/// Count the number of data members whose changes got filtered out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of data members whose changes got filtered out.
size_t
class_diff::priv::count_filtered_subtype_changed_dm()
{
  size_t num_filtered= 0;
  for (var_diff_sptrs_type::const_iterator i =
	 sorted_subtype_changed_dm_.begin();
       i != sorted_subtype_changed_dm_.end();
       ++i)
    {
      if ((*i)->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;
}

/// Count the number of data member offsets that have changed.
///
/// @return the number of filtered changed data members.
size_t
class_diff::priv::count_filtered_changed_dm()
{
  size_t num_filtered= 0;

  for (unsigned_var_diff_sptr_map::const_iterator i = changed_dm_.begin();
       i != changed_dm_.end();
       ++i)
    {
      diff_sptr diff = i->second;
      if (diff->is_filtered_out())
	++num_filtered;
    }
  return num_filtered;

}

/// Skip the processing of the current member function if its
/// virtual-ness is disallowed by the user.
///
/// This is to be used in the member functions below that are used to
/// count the number of filtered inserted, deleted and changed member
/// functions.
#define SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED				\
  do {									\
    if (get_member_function_is_virtual(f)					\
	|| get_member_function_is_virtual(s))				\
      {								\
	if (!(allowed_category | VIRTUAL_MEMBER_CHANGE_CATEGORY))	\
	  continue;							\
      }								\
    else								\
      {								\
	if (!(allowed_category | NON_VIRT_MEM_FUN_CHANGE_CATEGORY))	\
	  continue;							\
      }								\
  } while (false)

/// Count the number of member functions whose changes got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of member functions whose changes got filtered
/// out.
size_t
class_diff::priv::count_filtered_changed_mem_fns(const diff_context_sptr& ctxt)
{
  size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (function_decl_diff_sptrs_type::const_iterator i =
	 sorted_changed_member_functions_.begin();
       i != sorted_changed_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f =
	dynamic_pointer_cast<class_decl::method_decl>
	((*i)->first_function_decl());
      assert(f);

      class_decl::method_decl_sptr s =
	dynamic_pointer_cast<class_decl::method_decl>
	((*i)->second_function_decl());
      assert(s);

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = *i;
      ctxt->maybe_apply_filters(diff);

      if (diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Count the number of inserted member functions whose got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of inserted member functions whose got filtered
/// out.
size_t
class_diff::priv::count_filtered_inserted_mem_fns(const diff_context_sptr& ctxt)
{
  size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (string_member_function_sptr_map::const_iterator i =
	 inserted_member_functions_.begin();
       i != inserted_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f = i->second,
	s = i->second;

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->get_category() != NO_CHANGE_CATEGORY
	  && diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Count the number of deleted member functions whose got filtered
/// out.
///
/// @param ctxt the diff context to use to get the filtering settings
/// from the user.
///
/// @return the number of deleted member functions whose got filtered
/// out.
size_t
class_diff::priv::count_filtered_deleted_mem_fns(const diff_context_sptr& ctxt)
{
  size_t count = 0;
  diff_category allowed_category = ctxt->get_allowed_category();

  for (string_member_function_sptr_map::const_iterator i =
	 deleted_member_functions_.begin();
       i != deleted_member_functions_.end();
       ++i)
    {
      class_decl::method_decl_sptr f = i->second,
	s = i->second;

      SKIP_MEM_FN_IF_VIRTUALITY_DISALLOWED;

      diff_sptr diff = compute_diff_for_decls(f, s, ctxt);
      ctxt->maybe_apply_filters(diff);

      if (diff->get_category() != NO_CHANGE_CATEGORY
	  && diff->is_filtered_out())
	++count;
    }

  return count;
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref class_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
class_diff::chain_into_hierarchy()
{
  // base class changes.
  for (base_diff_sptrs_type::const_iterator i =
	 priv_->sorted_changed_bases_.begin();
       i != priv_->sorted_changed_bases_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  // data member changes
  for (var_diff_sptrs_type::const_iterator i =
	 priv_->sorted_subtype_changed_dm_.begin();
       i != priv_->sorted_subtype_changed_dm_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  for (unsigned_var_diff_sptr_map::const_iterator i =
	 priv_->changed_dm_.begin();
       i != priv_->changed_dm_.end();
       ++i)
    if (diff_sptr d = i->second)
      append_child_node(d);

  // member types changes
  for (diff_sptrs_type::const_iterator i =
	 priv_->sorted_changed_member_types_.begin();
       i != priv_->sorted_changed_member_types_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);

  // member function changes
  for (function_decl_diff_sptrs_type::const_iterator i =
	 priv_->sorted_changed_member_functions_.begin();
       i != priv_->sorted_changed_member_functions_.end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

/// Constructor of class_diff
///
/// @param first_scope the first class of the diff.
///
/// @param second_scope the second class of the diff.
///
/// @param ctxt the diff context to use.
class_diff::class_diff(shared_ptr<class_decl>	first_scope,
		       shared_ptr<class_decl>	second_scope,
		       diff_context_sptr	ctxt)
  : type_diff_base(first_scope, second_scope, ctxt)
    //  We don't initialize the priv_ data member here.  This is an
    //  optimization to reduce memory consumption (and also execution
    //  time) for cases where there are a lot of instances of
    //  class_diff in the same equivalence class.  In compute_diff(),
    //  the priv_ is set to the priv_ of the canonical diff node.
{}

class_diff::~class_diff()
{}

/// Finish building the current instance of @ref class_diff.
void
class_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the pretty representation of the current instance of @ref
/// class_diff.
const string&
class_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "class_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
class_diff::has_changes() const
{
  if (!!first_class_decl() != !! second_class_decl())
    return true;
  if (first_class_decl().get() == second_class_decl().get())
    return false;
  if (first_class_decl()->get_hash() && second_class_decl()->get_hash()
      && first_class_decl()->get_hash() != second_class_decl()->get_hash())
    return true;

  return (*first_class_decl() != *second_class_decl());
}

/// @return true iff the current diff node carries local changes.
bool
class_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_class_decl(), *second_class_decl(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// @return the first class invoveld in the diff.
shared_ptr<class_decl>
class_diff::first_class_decl() const
{return dynamic_pointer_cast<class_decl>(first_subject());}

/// Getter of the second class involved in the diff.
///
/// @return the second class invoveld in the diff
shared_ptr<class_decl>
class_diff::second_class_decl() const
{return dynamic_pointer_cast<class_decl>(second_subject());}

/// @return the edit script of the bases of the two classes.
const edit_script&
class_diff::base_changes() const
{return priv_->base_changes_;}

/// Getter for the deleted base classes of the diff.
///
/// @return a map containing the deleted base classes, keyed with
/// their pretty representation.
const string_base_sptr_map&
class_diff::deleted_bases() const
{return priv_->deleted_bases_;}

/// Getter for the inserted base classes of the diff.
///
/// @return a map containing the inserted base classes, keyed with
/// their pretty representation.
const string_base_sptr_map&
class_diff::inserted_bases() const
{return priv_->inserted_bases_;}

/// Getter for the changed base classes of the diff.
///
/// @return a sorted vector containing the changed base classes
const base_diff_sptrs_type&
class_diff::changed_bases()
{return priv_->sorted_changed_bases_;}

/// @return the edit script of the bases of the two classes.
edit_script&
class_diff::base_changes()
{return priv_->base_changes_;}

/// @return the edit script of the member types of the two classes.
const edit_script&
class_diff::member_types_changes() const
{return priv_->member_types_changes_;}

/// @return the edit script of the member types of the two classes.
edit_script&
class_diff::member_types_changes()
{return priv_->member_types_changes_;}

/// @return the edit script of the data members of the two classes.
const edit_script&
class_diff::data_members_changes() const
{return priv_->data_members_changes_;}

/// @return the edit script of the data members of the two classes.
edit_script&
class_diff::data_members_changes()
{return priv_->data_members_changes_;}

/// Getter for the data members that got inserted.
///
/// @return a map of data members that got inserted.
const string_decl_base_sptr_map&
class_diff::inserted_data_members() const
{return priv_->inserted_data_members_;}

/// Getter for the data members that got deleted.
///
/// @return a map of data members that got deleted.
const string_decl_base_sptr_map&
class_diff::deleted_data_members() const
{return priv_->deleted_data_members_;}

/// @return the edit script of the member functions of the two
/// classes.
const edit_script&
class_diff::member_fns_changes() const
{return priv_->member_fns_changes_;}

/// Getter for the virtual members functions that have had a change in
/// a sub-type, without having a change in their symbol name.
///
/// @return a sorted vector of virtual member functions that have a
/// sub-type change.
const function_decl_diff_sptrs_type&
class_diff::changed_member_fns() const
{return priv_->sorted_changed_member_functions_;}

/// @return the edit script of the member functions of the two
/// classes.
edit_script&
class_diff::member_fns_changes()
{return priv_->member_fns_changes_;}

/// @return a map of member functions that got deleted.
const string_member_function_sptr_map&
class_diff::deleted_member_fns() const
{return priv_->deleted_member_functions_;}

/// @return a map of member functions that got inserted.
const string_member_function_sptr_map&
class_diff::inserted_member_fns() const
{return priv_->inserted_member_functions_;}

///@return the edit script of the member function templates of the two
///classes.
const edit_script&
class_diff::member_fn_tmpls_changes() const
{return priv_->member_fn_tmpls_changes_;}

/// @return the edit script of the member function templates of the
/// two classes.
edit_script&
class_diff::member_fn_tmpls_changes()
{return priv_->member_fn_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// classes.
const edit_script&
class_diff::member_class_tmpls_changes() const
{return priv_->member_class_tmpls_changes_;}

/// @return the edit script of the member class templates of the two
/// classes.
edit_script&
class_diff::member_class_tmpls_changes()
{return priv_->member_class_tmpls_changes_;}

/// A comparison function for instances of @ref base_diff.
struct base_diff_comp
{
  bool
  operator()(const base_diff& l, const base_diff& r) const
  {
    class_decl::base_spec_sptr f = l.first_base(), s = r.first_base();
    if (f->get_offset_in_bits() >= 0
	&& s->get_offset_in_bits() >= 0)
      return f->get_offset_in_bits() < s->get_offset_in_bits();
    else
      return (f->get_base_class()->get_pretty_representation()
	      < s->get_base_class()->get_pretty_representation());
  }

  bool
  operator()(const base_diff* l, const base_diff* r) const
  {return operator()(*l, *r);}

  bool
  operator()(const base_diff_sptr l, const base_diff_sptr r) const
  {return operator()(l.get(), r.get());}
}; // end struct base_diff_comp

/// Sort a map of string -> base_diff_sptr into a sorted vector of
/// base_diff_sptr.  The base_diff_sptr are sorted by increasing value
/// of their offset in their containing type.
///
/// @param map the input map to sort.
///
/// @param sorted the resulting sorted vector.
static void
sort_string_base_diff_sptr_map(const string_base_diff_sptr_map& map,
			       base_diff_sptrs_type& sorted)
{
  for (string_base_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  base_diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// A comparison functor to compare two instances of @ref var_diff
/// that represent changed data members based on the offset of their
/// initial value.
struct data_member_diff_comp
{
  /// @param f the first change to data member to take into account
  ///
  /// @param s the second change to data member to take into account.
  ///
  /// @return true iff f is before s.
  bool
  operator()(const var_diff_sptr f,
	     const var_diff_sptr s) const
  {
    var_decl_sptr first_dm = f->first_var();
    var_decl_sptr second_dm = s->first_var();

    assert(is_data_member(first_dm));
    assert(is_data_member(second_dm));

    return get_data_member_offset(first_dm) < get_data_member_offset(second_dm);
  }
}; // end struct var_diff_comp

/// Sort the values of a unsigned_var_diff_sptr_map map and store the
/// result into a vector of var_diff_sptr.
///
/// @param map the map of changed data members to sort.
///
/// @param sorted the resulting vector of sorted var_diff_sptr.
static void
sort_unsigned_data_member_diff_sptr_map(const unsigned_var_diff_sptr_map map,
					var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (unsigned_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  data_member_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort the values of a string_var_diff_sptr_map and store the result
/// in a vector of var_diff_sptr.
///
/// @param map the map of changed data members to sort.
///
/// @param sorted the resulting vector of var_diff_sptr.
static void
sort_string_data_member_diff_sptr_map(const string_var_diff_sptr_map& map,
				      var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  data_member_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// A comparison functor to compare two data members based on their
/// offset.
struct data_member_comp
{
  /// @param f the first data member to take into account.
  ///
  /// @param s the second data member to take into account.
  ///
  /// @return true iff f is before s.
  bool
  operator()(const decl_base_sptr& f,
	     const decl_base_sptr& s) const
  {
    var_decl_sptr first_dm = is_data_member(f);
    var_decl_sptr second_dm = is_data_member(s);

    assert(first_dm);
    assert(second_dm);

    return get_data_member_offset(first_dm) < get_data_member_offset(second_dm);
  }
};//end struct data_member_comp

/// Sort a map of data members by the offset of their initial value.
///
/// @param data_members the map of changed data members to sort.
///
/// @param sorted the resulting vector of sorted changed data members.
static void
sort_data_members(const string_decl_base_sptr_map &data_members,
		  vector<decl_base_sptr>& sorted)
{
  sorted.reserve(data_members.size());
  for (string_decl_base_sptr_map::const_iterator i = data_members.begin();
       i != data_members.end();
       ++i)
    sorted.push_back(i->second);

  data_member_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// A comparison functor for instances of @ref function_decl_diff that
/// represent changes between two virtual member functions.
struct virtual_member_function_diff_comp
{
  bool
  operator()(const function_decl_diff& l,
	     const function_decl_diff& r) const
  {
    assert(get_member_function_is_virtual(l.first_function_decl()));
    assert(get_member_function_is_virtual(r.first_function_decl()));

    return (get_member_function_vtable_offset(l.first_function_decl())
	    < get_member_function_vtable_offset(r.first_function_decl()));
  }

  bool
  operator()(const function_decl_diff* l,
	     const function_decl_diff* r)
  {return operator()(*l, *r);}

  bool
  operator()(const function_decl_diff_sptr l,
	     const function_decl_diff_sptr r)
  {return operator()(l.get(), r.get());}
}; // end struct virtual_member_function_diff_comp

/// Sort an map of string -> virtual member function into a vector of
/// virtual member functions.  The virtual member functions are sorted
/// by increasing order of their virtual index.
///
/// @param map the input map.
///
/// @param sorted the resulting sorted vector of virtual function
/// member.
static void
sort_string_virtual_member_function_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_function_decl_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  virtual_member_function_diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Produce a basic report about the changes between two class_decl.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
class_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  string name = first_subject()->get_pretty_representation();

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(first_subject(),
						   second_subject());

  currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = first_class_decl(),
    second = second_class_decl();

  if (report_name_size_and_alignment_changes(first, second, context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second, context(), out, indent);

  // bases classes
  if (base_changes())
    {
      // Report deletions.
      int numdels = priv_->deleted_bases_.size();
      size_t numchanges = priv_->sorted_changed_bases_.size();

      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "base class", indent);

	  for (string_base_sptr_map::const_iterator i
		 = priv_->deleted_bases_.begin();
	       i != priv_->deleted_bases_.end();
	       ++i)
	    {
	      if (i != priv_->deleted_bases_.begin())
		out << "\n";

	      class_decl::base_spec_sptr base = i->second;

	      if ( priv_->base_has_changed(base))
		continue;
	      out << indent << "  "
		  << base->get_base_class()->get_pretty_representation();
	    }
	  out << "\n";
	}

      // Report changes.
      bool emitted = false;
      size_t num_filtered = priv_->count_filtered_bases();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered, change_kind,
			    "base class", indent);
	  for (base_diff_sptrs_type::const_iterator it =
		 priv_->sorted_changed_bases_.begin();
	       it != priv_->sorted_changed_bases_.end();
	       ++it)
	    {
	      base_diff_sptr diff = *it;
	      if (!diff || !diff->to_be_reported())
		continue;

	      class_decl::base_spec_sptr o = diff->first_base();
	      out << indent << "  '"
		  << o->get_base_class()->get_pretty_representation()
		  << "' changed:\n";
	      diff->report(out, indent + "    ");
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      //Report insertions.
      int numins = priv_->inserted_bases_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "base class", indent);

	  bool emitted = false;
	  for (string_base_sptr_map::const_iterator i =
		 priv_->inserted_bases_.begin();
	       i != priv_->inserted_bases_.end();
	       ++i)
	    {
	      class_decl_sptr b = i->second->get_base_class();
	      if (emitted)
		out << "\n";
	      out << indent << "  " << b->get_pretty_representation();
	      emitted = true;
	    }
	  out << "\n";
	}
    }

  // member functions
  if (member_fns_changes())
    {
      // report deletions
      int numdels = priv_->deleted_member_functions_.size();
      size_t num_filtered = priv_->count_filtered_deleted_mem_fns(context());
      size_t net_num_dels = numdels - num_filtered;
      if (numdels)
	report_mem_header(out, numdels, num_filtered, del_kind,
			  "member function", indent);
      for (string_member_function_sptr_map::const_iterator i =
	     priv_->deleted_member_functions_.begin();
	   i != priv_->deleted_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (i != priv_->deleted_member_functions_.begin())
	    out << "\n";
	  class_decl::method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*context(), mem_fun, out);
	}
      if (net_num_dels)
	out << "\n";

      // report insertions;
      int numins = priv_->inserted_member_functions_.size();
      num_filtered = priv_->count_filtered_inserted_mem_fns(context());
      if (numins)
	report_mem_header(out, numins, num_filtered, ins_kind,
			  "member function", indent);
      bool emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     priv_->inserted_member_functions_.begin();
	   i != priv_->inserted_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (i != priv_->inserted_member_functions_.begin())
	    out << "\n";
	  class_decl::method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*context(), mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report member function with sub-types changes
      int numchanges = priv_->sorted_changed_member_functions_.size();
      num_filtered = priv_->count_filtered_changed_mem_fns(context());
      if (numchanges)
	report_mem_header(out, numchanges, num_filtered, change_kind,
			  "member function", indent);
      for (function_decl_diff_sptrs_type::const_iterator i =
	     priv_->sorted_changed_member_functions_.begin();
	   i != priv_->sorted_changed_member_functions_.end();
	   ++i)
	{
	  if (!(context()->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !(get_member_function_is_virtual
		   ((*i)->first_function_decl()))
	      && !(get_member_function_is_virtual
		   ((*i)->second_function_decl())))
	    continue;

	  diff_sptr diff = *i;
	  if (!diff || !diff->to_be_reported())
	    continue;

	  string repr =
	    (*i)->first_function_decl()->get_pretty_representation();
	  if (i != priv_->sorted_changed_member_functions_.begin())
	    out << "\n";
	  out << indent << "  '" << repr << "' has some sub-type changes:\n";
	  diff->report(out, indent + "    ");
	  emitted = true;
	}
      if (numchanges)
	out << "\n";
    }

  // data members
  if (data_members_changes())
    {
      // report deletions
      int numdels = priv_->deleted_data_members_.size();
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members(priv_->deleted_data_members_, sorted_dms);
	  bool emitted = false;
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      assert(data_mem);
	      out << indent << "  ";
	      represent_data_member(data_mem, out);
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      //report insertions
      int numins = priv_->inserted_data_members_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members(priv_->inserted_data_members_, sorted_dms);
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      assert(data_mem);
	      out << indent << "  ";
	      represent_data_member(data_mem, out);
	    }
	}

      // report change
      size_t numchanges = priv_->sorted_subtype_changed_dm_.size();
      size_t num_filtered = priv_->count_filtered_subtype_changed_dm();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    subtype_change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		priv_->sorted_subtype_changed_dm_.begin();
	       it != priv_->sorted_subtype_changed_dm_.end();
	       ++it)
	    represent(*it, context(), out, indent + " ");
	  out << "\n";
	}

      numchanges = priv_->sorted_changed_dm_.size();
      num_filtered = priv_->count_filtered_changed_dm();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		 priv_->sorted_changed_dm_.begin();
	       it != priv_->sorted_changed_dm_.end();
	       ++it)
	    represent(*it, context(), out, indent + " ");
	  out << "\n";
	}
    }

  // member types
  if (const edit_script& e = member_types_changes())
    {
      int numchanges = priv_->sorted_changed_member_types_.size();
      int numdels = priv_->deleted_member_types_.size();

      // report deletions
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "member type", indent);

	  for (string_decl_base_sptr_map::const_iterator i =
		 priv_->deleted_member_types_.begin();
	       i != priv_->deleted_member_types_.end();
	       ++i)
	    {
	      if (i != priv_->deleted_member_types_.begin())
		out << "\n";
	      decl_base_sptr mem_type = i->second;
	      out << indent << "  '"
		  << mem_type->get_pretty_representation()
		  << "'";
	    }
	  out << "\n\n";
	}
      // report changes
      if (numchanges)
	{
	  report_mem_header(out, numchanges, 0, change_kind,
			    "member type", indent);

	  for (diff_sptrs_type::const_iterator it =
		 priv_->sorted_changed_member_types_.begin();
	       it != priv_->sorted_changed_member_types_.end();
	       ++it)
	    {
	      if (!(*it)->to_be_reported())
		continue;

	      decl_base_sptr o = (*it)->first_subject();
	      decl_base_sptr n = (*it)->second_subject();
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed:\n";
	      (*it)->report(out, indent + "    ");
	    }
	  out << "\n";
	}

      // report insertions
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;

      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "member type", indent);

	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      type_base_sptr mem_type;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  if (emitted)
		    out << "\n";
		  mem_type = second->get_member_types()[*j];
		  if (!priv_->
		      member_type_has_changed(get_type_declaration(mem_type)))
		    {
		      out << indent << "  '"
			  << get_type_declaration(mem_type)->
			get_pretty_representation()
			  << "'";
		      emitted = true;
		    }
		}
	    }
	  out << "\n\n";
	}
    }

  // member function templates
  if (const edit_script& e = member_fn_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member function template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  class_decl::member_function_template_sptr mem_fn_tmpl =
	    first->get_member_function_templates()[i->index()];
	  out << indent << "  '"
	      << mem_fn_tmpl->as_function_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member function template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  class_decl::member_function_template_sptr mem_fn_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_fn_tmpl = second->get_member_function_templates()[*j];
	      out << indent << "  '"
		  << mem_fn_tmpl->as_function_tdecl()->
		get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }

  // member class templates.
  if (const edit_script& e = member_class_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member class template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  class_decl::member_class_template_sptr mem_cls_tmpl =
	    first->get_member_class_templates()[i->index()];
	  out << indent << "  '"
	      << mem_cls_tmpl->as_class_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member class template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  class_decl::member_class_template_sptr mem_cls_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_cls_tmpl = second->get_member_class_templates()[*j];
	      out << indent << "  '"
		  << mem_cls_tmpl->as_class_tdecl()
		->get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }

  currently_reporting(false);
  reported_once(true);
}

/// Compute the set of changes between two instances of class_decl.
///
/// @param first the first class_decl to consider.
///
/// @param second the second class_decl to consider.
///
/// @return changes the resulting changes.
///
/// @param ctxt the diff context to use.
class_diff_sptr
compute_diff(const class_decl_sptr	first,
	     const class_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  class_decl_sptr f = look_through_decl_only_class(first),
    s = look_through_decl_only_class(second);

  class_diff_sptr changes(new class_diff(f, s, ctxt));

  ctxt->initialize_canonical_diff(changes);
  assert(changes->get_canonical_diff());

  if (!ctxt->get_canonical_diff_for(first, second))
    {
      // Either first or second is a decl-only class; let's set the
      // canonical diff here in that case.
      diff_sptr canonical_diff = ctxt->get_canonical_diff_for(changes);
      assert(canonical_diff);
      ctxt->set_canonical_diff_for(first, second, canonical_diff);
    }

  // Ok, so this is an optimization.  Do not freak out if it looks
  // weird, because, well, it does look weird.
  //
  // We are setting the private data of the new instance of class_diff
  // (which is 'changes') to the private data of its canonical
  // instance.  That is, we are sharing the private data of 'changes'
  // with the private data of its canonical instance to consume less
  // memory in cases where the equivalence class of 'changes' is huge.
  //
  // But if changes is its own canonical instance, then we initialize
  // its private data properly.
  if (dynamic_cast<class_diff*>(changes->get_canonical_diff()) == changes.get())
    // changes is its own canonical instance, so it gets a brand new
    // private data.
    changes->priv_.reset(new class_diff::priv);
  else
    {
      // changes has a non-empty equivalence class so it's going to
      // share its private data with its canonical instance.
      changes->priv_ =
	dynamic_cast<class_diff*>(changes->get_canonical_diff())->priv_;
      assert(changes->priv_);
      return changes;
    }

  // Compare base specs
  compute_diff(f->get_base_specifiers().begin(),
	       f->get_base_specifiers().end(),
	       s->get_base_specifiers().begin(),
	       s->get_base_specifiers().end(),
	       changes->base_changes());

  // Do *not* compare member types because it generates lots of noise
  // and I doubt it's really useful.
#if 0
  compute_diff(f->get_member_types().begin(),
	       f->get_member_types().end(),
	       s->get_member_types().begin(),
	       s->get_member_types().end(),
	       changes->member_types_changes());
#endif

  // Compare data member
  compute_diff(f->get_data_members().begin(),
	       f->get_data_members().end(),
	       s->get_data_members().begin(),
	       s->get_data_members().end(),
	       changes->data_members_changes());

  // Compare virtual member functions
  compute_diff(f->get_virtual_mem_fns().begin(),
	       f->get_virtual_mem_fns().end(),
	       s->get_virtual_mem_fns().begin(),
	       s->get_virtual_mem_fns().end(),
	       changes->member_fns_changes());

  // Compare member function templates
  compute_diff(f->get_member_function_templates().begin(),
	       f->get_member_function_templates().end(),
	       s->get_member_function_templates().begin(),
	       s->get_member_function_templates().end(),
	       changes->member_fn_tmpls_changes());

  // Likewise, do not compare member class templates
#if 0
  compute_diff(f->get_member_class_templates().begin(),
	       f->get_member_class_templates().end(),
	       s->get_member_class_templates().begin(),
	       s->get_member_class_templates().end(),
	       changes->member_class_tmpls_changes());
#endif

  changes->ensure_lookup_tables_populated();

  return changes;
}

//</class_diff stuff>

// <base_diff stuff>
struct base_diff::priv
{
  class_diff_sptr underlying_class_diff_;

  priv(class_diff_sptr underlying)
    : underlying_class_diff_(underlying)
  {}
}; // end struct base_diff::priv

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref base_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
base_diff::chain_into_hierarchy()
{append_child_node(get_underlying_class_diff());}

/// @param first the first base spec to consider.
///
/// @param second the second base spec to consider.
///
/// @param ctxt the context of the diff.
base_diff::base_diff(class_decl::base_spec_sptr first,
		     class_decl::base_spec_sptr second,
		     class_diff_sptr		underlying,
		     diff_context_sptr		ctxt)
  : diff(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref base_diff.
void
base_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;

  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first base spec of the diff object.
///
/// @return the first base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::first_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(first_subject());}

/// Getter for the second base spec of the diff object.
///
/// @return the second base specifier for the diff object.
class_decl::base_spec_sptr
base_diff::second_base() const
{return dynamic_pointer_cast<class_decl::base_spec>(second_subject());}

/// Getter for the diff object for the diff of the underlying base
/// classes.
///
/// @return the diff object for the diff of the underlying base
/// classes.
const class_diff_sptr
base_diff::get_underlying_class_diff() const
{return priv_->underlying_class_diff_;}

/// Setter for the diff object for the diff of the underlyng base
/// classes.
///
/// @param d the new diff object for the diff of the underlying base
/// classes.
void
base_diff::set_underlying_class_diff(class_diff_sptr d)
{priv_->underlying_class_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// base_diff.
const string&
base_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "base_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// Return true iff the current diff node carries a change.
bool
base_diff::has_changes() const
{return first_base() != second_base();}

/// @return true iff the current diff node carries local changes.
bool
base_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_base(), *second_base(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Generates a report for the current instance of base_diff.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
base_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  class_decl::base_spec_sptr f = first_base(), s = second_base();
  string repr = f->get_base_class()->get_pretty_representation();
  bool emitted = false;

  if (f->get_is_static() != s->get_is_static())
    {
      if (f->get_is_static())
	out << indent << "is no more static";
      else
	out << indent << "now becomes static";
      emitted = true;
    }

  if ((context()->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (f->get_access_specifier() != s->get_access_specifier()))
    {
      if (emitted)
	out << ", ";

      out << "has access changed from '"
	  << f->get_access_specifier()
	  << "' to '"
	  << s->get_access_specifier()
	  << "'";

      emitted = true;
    }

  if (class_diff_sptr d = get_underlying_class_diff())
    {
      if (d->to_be_reported())
	{
	  if (emitted)
	    out << "\n";
	  d->report(out, indent);
	}
    }
}

/// Constructs the diff object representing a diff between two base
/// class specifications.
///
/// @param first the first base class specification.
///
/// @param second the second base class specification.
///
/// @param ctxt the content of the diff.
///
/// @return the resulting diff object.
base_diff_sptr
compute_diff(const class_decl::base_spec_sptr	first,
	     const class_decl::base_spec_sptr	second,
	     diff_context_sptr			ctxt)
{
  class_diff_sptr cl = compute_diff(first->get_base_class(),
				    second->get_base_class(),
				    ctxt);
  base_diff_sptr changes(new base_diff(first, second, cl, ctxt));

  ctxt->initialize_canonical_diff(changes);

  return changes;
}

// </base_diff stuff>

//<scope_diff stuff>
struct scope_diff::priv
{
  // The edit script built by the function compute_diff.
  edit_script member_changes_;

  // Below are the useful lookup tables.
  //
  // If you add a new lookup table, please update member functions
  // clear_lookup_tables, lookup_tables_empty and
  // ensure_lookup_tables_built.

  // The deleted/inserted types/decls.  These basically map what is
  // inside the member_changes_ data member.  Note that for instance,
  // a given type T might be deleted from the first scope and added to
  // the second scope again; this means that the type was *changed*.
  string_decl_base_sptr_map deleted_types_;
  string_decl_base_sptr_map deleted_decls_;
  string_decl_base_sptr_map inserted_types_;
  string_decl_base_sptr_map inserted_decls_;

  // The changed types/decls lookup tables.
  //
  // These lookup tables are populated from the lookup tables above.
  //
  // Note that the value stored in each of these tables is a pair
  // containing the old decl/type and the new one.  That way it is
  // easy to run a diff between the old decl/type and the new one.
  //
  // A changed type/decl is one that has been deleted from the first
  // scope and that has been inserted into the second scope.
  string_diff_sptr_map changed_types_;
  diff_sptrs_type sorted_changed_types_;
  string_diff_sptr_map changed_decls_;
  diff_sptrs_type sorted_changed_decls_;

  // The removed types/decls lookup tables.
  //
  // A removed type/decl is one that has been deleted from the first
  // scope and that has *NOT* been inserted into it again.
  string_decl_base_sptr_map removed_types_;
  string_decl_base_sptr_map removed_decls_;

  // The added types/decls lookup tables.
  //
  // An added type/decl is one that has been inserted to the first
  // scope but that has not been deleted from it.
  string_decl_base_sptr_map added_types_;
  string_decl_base_sptr_map added_decls_;
};//end struct scope_diff::priv

/// Clear the lookup tables that are useful for reporting.
///
/// This function must be updated each time a lookup table is added or
/// removed.
void
scope_diff::clear_lookup_tables()
{
  priv_->deleted_types_.clear();
  priv_->deleted_decls_.clear();
  priv_->inserted_types_.clear();
  priv_->inserted_decls_.clear();
  priv_->changed_types_.clear();
  priv_->changed_decls_.clear();
  priv_->removed_types_.clear();
  priv_->removed_decls_.clear();
  priv_->added_types_.clear();
  priv_->added_decls_.clear();
}

/// Tests if the lookup tables are empty.
///
/// This function must be updated each time a lookup table is added or
/// removed.
///
/// @return true iff all the lookup tables are empty.
bool
scope_diff::lookup_tables_empty() const
{
  return (priv_->deleted_types_.empty()
	  && priv_->deleted_decls_.empty()
	  && priv_->inserted_types_.empty()
	  && priv_->inserted_decls_.empty()
	  && priv_->changed_types_.empty()
	  && priv_->changed_decls_.empty()
	  && priv_->removed_types_.empty()
	  && priv_->removed_decls_.empty()
	  && priv_->added_types_.empty()
	  && priv_->added_decls_.empty());
}

/// If the lookup tables are not yet built, walk the member_changes_
/// member and fill the lookup tables.
void
scope_diff::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  edit_script& e = priv_->member_changes_;

  // Populate deleted types & decls lookup tables.
  for (vector<deletion>::const_iterator i = e.deletions().begin();
       i != e.deletions().end();
       ++i)
    {
      decl_base_sptr decl = deleted_member_at(i);
      string qname = decl->get_qualified_name();
      if (is_type(decl))
	{
	  class_decl_sptr klass_decl = dynamic_pointer_cast<class_decl>(decl);
	  if (klass_decl && klass_decl->get_is_declaration_only())
	    continue;

	  assert(priv_->deleted_types_.find(qname)
		 == priv_->deleted_types_.end());
	  priv_->deleted_types_[qname] = decl;
	}
      else
	{
	  assert(priv_->deleted_decls_.find(qname)
		 == priv_->deleted_decls_.end());
	  priv_->deleted_decls_[qname] = decl;
	}
    }

  // Populate inserted types & decls as well as chagned types & decls
  // lookup tables.
  for (vector<insertion>::const_iterator it = e.insertions().begin();
       it != e.insertions().end();
       ++it)
    {
      for (vector<unsigned>::const_iterator i = it->inserted_indexes().begin();
	   i != it->inserted_indexes().end();
	   ++i)
	{
	  decl_base_sptr decl = inserted_member_at(i);
	  string qname = decl->get_qualified_name();
	  if (is_type(decl))
	    {
	      class_decl_sptr klass_decl =
		dynamic_pointer_cast<class_decl>(decl);
	      if (klass_decl && klass_decl->get_is_declaration_only())
		continue;

	      assert(priv_->inserted_types_.find(qname)
		     == priv_->inserted_types_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_types_.find(qname);
	      if (j != priv_->deleted_types_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_types_[qname] =
		      compute_diff(j->second, decl, context());
		  priv_->deleted_types_.erase(j);
		}
	      else
		priv_->inserted_types_[qname] = decl;
	    }
	  else
	    {
	      assert(priv_->inserted_decls_.find(qname)
		     == priv_->inserted_decls_.end());
	      string_decl_base_sptr_map::const_iterator j =
		priv_->deleted_decls_.find(qname);
	      if (j != priv_->deleted_decls_.end())
		{
		  if (*j->second != *decl)
		    priv_->changed_decls_[qname] =
		      compute_diff(j->second, decl, context());
		  priv_->deleted_decls_.erase(j);
		}
	      else
		priv_->inserted_decls_[qname] = decl;
	    }
	}
    }

  sort_string_diff_sptr_map(priv_->changed_decls_,
			    priv_->sorted_changed_decls_);
  sort_string_diff_sptr_map(priv_->changed_types_,
			    priv_->sorted_changed_types_);

  // Populate removed types/decls lookup tables
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_types_.find(i->first);
      if (r == priv_->inserted_types_.end())
	priv_->removed_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->inserted_decls_.find(i->first);
      if (r == priv_->inserted_decls_.end())
	priv_->removed_decls_[i->first] = i->second;
    }

  // Populate added types/decls.
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_types_.begin();
       i != priv_->inserted_types_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_types_.find(i->first);
      if (r == priv_->deleted_types_.end())
	priv_->added_types_[i->first] = i->second;
    }
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_decls_.begin();
       i != priv_->inserted_decls_.end();
       ++i)
    {
      string_decl_base_sptr_map::const_iterator r =
	priv_->deleted_decls_.find(i->first);
      if (r == priv_->deleted_decls_.end())
	priv_->added_decls_[i->first] = i->second;
    }
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref scope_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
scope_diff::chain_into_hierarchy()
{
  for (diff_sptrs_type::const_iterator i = changed_types().begin();
       i != changed_types().end();
       ++i)
    if (*i)
      append_child_node(*i);

  for (diff_sptrs_type::const_iterator i = changed_decls().begin();
       i != changed_decls().end();
       ++i)
    if (*i)
      append_child_node(*i);
}

/// Constructor for scope_diff
///
/// @param first_scope the first scope to consider for the diff.
///
/// @param second_scope the second scope to consider for the diff.
///
/// @param ctxt the diff context to use.
scope_diff::scope_diff(scope_decl_sptr first_scope,
		       scope_decl_sptr second_scope,
		       diff_context_sptr ctxt)
  : diff(first_scope, second_scope, ctxt),
    priv_(new priv)
{}

/// Finish building the current instance of @ref scope_diff.
void
scope_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first scope of the diff.
///
/// @return the first scope of the diff.
const scope_decl_sptr
scope_diff::first_scope() const
{return dynamic_pointer_cast<scope_decl>(first_subject());}

/// Getter for the second scope of the diff.
///
/// @return the second scope of the diff.
const scope_decl_sptr
scope_diff::second_scope() const
{return dynamic_pointer_cast<scope_decl>(second_subject());}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
const edit_script&
scope_diff::member_changes() const
{return priv_->member_changes_;}

/// Accessor of the edit script of the members of a scope.
///
/// This edit script is computed using the equality operator that
/// applies to shared_ptr<decl_base>.
///
/// That has interesting consequences.  For instance, consider two
/// scopes S0 and S1.  S0 contains a class C0 and S1 contains a class
/// S0'.  C0 and C0' have the same qualified name, but have different
/// members.  The edit script will consider that C0 has been deleted
/// from S0 and that S0' has been inserted.  This is a low level
/// canonical representation of the changes; a higher level
/// representation would give us a simpler way to say "the class C0
/// has been modified into C0'".  But worry not.  We do have such
/// higher representation as well; that is what changed_types() and
/// changed_decls() is for.
///
/// @return the edit script of the changes encapsulatd in this
/// instance of scope_diff.
edit_script&
scope_diff::member_changes()
{return priv_->member_changes_;}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member that is reported
/// (in the edit script) as deleted at a given index.
///
/// @param i the index (in the edit script) of an element of the first
/// scope that has been reported as being delete.
///
/// @return the scope member that has been reported by the edit script
/// as being deleted at index i.
const decl_base_sptr
scope_diff::deleted_member_at(unsigned i) const
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(first_subject());
 return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the first scope
/// of this diff instance) that is reported (in the edit script) as
/// deleted at a given iterator.
///
/// @param i the iterator of an element of the first scope that has
/// been reported as being delete.
///
/// @return the scope member of the first scope of this diff that has
/// been reported by the edit script as being deleted at iterator i.
const decl_base_sptr
scope_diff::deleted_member_at(vector<deletion>::const_iterator i) const
{return deleted_member_at(i->index());}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given index.
///
/// @param i the index of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from index i.
const decl_base_sptr
scope_diff::inserted_member_at(unsigned i)
{
  scope_decl_sptr scope = dynamic_pointer_cast<scope_decl>(second_subject());
  return scope->get_member_decls()[i];
}

/// Accessor that eases the manipulation of the edit script associated
/// to this instance.  It returns the scope member (of the second
/// scope of this diff instance) that is reported as being inserted
/// from a given iterator.
///
/// @param i the iterator of an element of the second scope this diff
/// that has been reported by the edit script as being inserted.
///
/// @return the scope member of the second scope of this diff that has
/// been reported as being inserted from iterator i.
const decl_base_sptr
scope_diff::inserted_member_at(vector<unsigned>::const_iterator i)
{return inserted_member_at(*i);}

/// @return a sorted vector of the types which content has changed
/// from the first scope to the other.
const diff_sptrs_type&
scope_diff::changed_types() const
{return priv_->sorted_changed_types_;}

/// @return a sorted vector of the decls which content has changed
/// from the first scope to the other.
const diff_sptrs_type&
scope_diff::changed_decls() const
{return priv_->sorted_changed_decls_;}

const string_decl_base_sptr_map&
scope_diff::removed_types() const
{return priv_->removed_types_;}

const string_decl_base_sptr_map&
scope_diff::removed_decls() const
{return priv_->removed_decls_;}

const string_decl_base_sptr_map&
scope_diff::added_types() const
{return priv_->added_types_;}

const string_decl_base_sptr_map&
scope_diff::added_decls() const
{return priv_->added_decls_;}

/// @return the pretty representation for the current instance of @ref
/// scope_diff.
const string&
scope_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "scope_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// Return true iff the current diff node carries a change.
bool
scope_diff::has_changes() const
{
  // TODO: add the number of really removed/added stuff.
  return changed_types().size() + changed_decls().size();
}

/// @return true iff the current diff node carries local changes.
bool
scope_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_scope(), *second_scope(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// A comparison functor for instances of @ref diff.
struct diff_comp
{
  bool
  operator()(const diff& l, diff& r) const
  {
    return (l.first_subject()->get_qualified_name()
	    < r.first_subject()->get_qualified_name());
  }

  bool
  operator()(const diff* l, diff* r) const
  {return operator()(*l, *r);}

  bool
  operator()(const diff_sptr l, diff_sptr r) const
  {return operator()(l.get(), r.get());}
}; // end struct diff_comp;

/// Sort a map ofg string -> @ref diff_sptr into a vector of @ref
/// diff_sptr.  The diff_sptr are sorted lexicographically wrt
/// qualified names of their first subjects.
static void
sort_string_diff_sptr_map(const string_diff_sptr_map& map,
			  diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  diff_comp comp;
  sort(sorted.begin(), sorted.end(), comp);
}

/// Report the changes of one scope against another.
///
/// @param out the out stream to report the changes to.
///
/// @param indent the string to use for indentation.
void
scope_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  // Report changed types.
  unsigned num_changed_types = changed_types().size();
  if (num_changed_types == 0)
    ;
  else if (num_changed_types == 1)
    out << indent << "1 changed type:\n";
  else
    out << indent << num_changed_types << " changed types:\n";

  for (diff_sptrs_type::const_iterator d = changed_types().begin();
       d != changed_types().end();
       ++d)
    {
      if (!*d)
	continue;

      out << indent << "  '"
	  << (*d)->first_subject()->get_pretty_representation()
	  << "' changed:\n";
      (*d)->report(out, indent + "    ");
    }

  // Report changed decls
  unsigned num_changed_decls = changed_decls().size();
  if (num_changed_decls == 0)
    ;
  else if (num_changed_decls == 1)
    out << indent << "1 changed declaration:\n";
  else
    out << indent << num_changed_decls << " changed declarations:\n";

  for (diff_sptrs_type::const_iterator d= changed_decls().begin();
       d != changed_decls().end ();
       ++d)
    {
      if (!*d)
	continue;

      out << indent << "  '"
	  << (*d)->first_subject()->get_pretty_representation()
	  << "' was changed to '"
	  << (*d)->second_subject()->get_pretty_representation()
	  << "':\n";

      (*d)->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_types_.begin();
       i != priv_->deleted_types_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (priv_->deleted_types_.size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->deleted_decls_.begin();
       i != priv_->deleted_decls_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";
  if (priv_->deleted_decls_.size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_types_.begin();
       i != priv_->inserted_types_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 priv_->inserted_decls_.begin();
       i != priv_->inserted_decls_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// Compute the diff between two scopes.
///
/// @param first the first scope to consider in computing the diff.
///
/// @param second the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param d a pointer to the diff object to populate with the
/// computed diff.
///
/// @return return the populated \a d parameter passed to this
/// function.
///
/// @param ctxt the diff context to use.
scope_diff_sptr
compute_diff(const scope_decl_sptr	first,
	     const scope_decl_sptr	second,
	     scope_diff_sptr		d,
	     diff_context_sptr		ctxt)
{
  assert(d->first_scope() == first && d->second_scope() == second);

  compute_diff(first->get_member_decls().begin(),
	       first->get_member_decls().end(),
	       second->get_member_decls().begin(),
	       second->get_member_decls().end(),
	       d->member_changes());

  d->ensure_lookup_tables_populated();
  d->context(ctxt);

  return d;
}

/// Compute the diff between two scopes.
///
/// @param first_scope the first scope to consider in computing the diff.
///
/// @param second_scope the second scope to consider in the diff
/// computation.  The second scope is diffed against the first scope.
///
/// @param ctxt the diff context to use.
///
/// @return return the resulting diff
scope_diff_sptr
compute_diff(const scope_decl_sptr	first_scope,
	     const scope_decl_sptr	second_scope,
	     diff_context_sptr		ctxt)
{
  scope_diff_sptr d(new scope_diff(first_scope, second_scope, ctxt));
  d = compute_diff(first_scope, second_scope, d, ctxt);
  ctxt->initialize_canonical_diff(d);
  return d;
}

//</scope_diff stuff>

// <fn_parm_diff stuff>
struct fn_parm_diff::priv
{
  mutable diff_sptr type_diff;
}; // end struct fn_parm_diff::priv

/// Constructor for the fn_parm_diff type.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param ctxt the context of the diff.
fn_parm_diff::fn_parm_diff(const function_decl::parameter_sptr	first,
			   const function_decl::parameter_sptr	second,
			   diff_context_sptr			ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv)
{
  assert(first->get_index() == second->get_index());
  priv_->type_diff = compute_diff(first->get_type(),
				  second->get_type(),
				  ctxt);
}

/// Finish the building of the current instance of @ref fn_parm_diff.
void
fn_parm_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of this diff node.
///
/// @return the first function_decl::parameter_sptr subject of this
/// diff node.
const function_decl::parameter_sptr
fn_parm_diff::first_parameter() const
{return dynamic_pointer_cast<function_decl::parameter>(first_subject());}

/// Getter for the second subject of this diff node.
///
/// @return the second function_decl::parameter_sptr subject of this
/// diff node.
const function_decl::parameter_sptr
fn_parm_diff::second_parameter() const
{return dynamic_pointer_cast<function_decl::parameter>(second_subject());}

/// Getter for the diff representing the changes on the type of the
/// function parameter involved in the current instance of @ref
/// fn_parm_diff.
///
/// @return a diff_sptr representing the changes on the type of the
/// function parameter we are interested in.
diff_sptr
fn_parm_diff::get_type_diff() const
{return priv_->type_diff;}

/// Build and return a textual representation of the current instance
/// of @ref fn_parm_diff.
///
/// @return the string representing the current instance of
/// fn_parm_diff.
const string&
fn_parm_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "function_parameter_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
fn_parm_diff::has_changes() const
{return *first_parameter() != *second_parameter();}

/// Check if the the current diff node carries a local change.
///
/// @return true iff the current diff node carries a local change.
bool
fn_parm_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_parameter(), *second_parameter(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Emit a textual report about the current fn_parm_diff instance.
///
/// @param out the output stream to emit the textual report to.
///
/// @param indent the indentation string to use in the report.
void
fn_parm_diff::report(ostream& out, const string& indent) const
{
  function_decl::parameter_sptr f = first_parameter(), s = second_parameter();
  string f_name_id = f->get_name_id(), s_name_id = s->get_name_id();

  // either the parameter has a sub-type change (if it's name id
  // hasn't changed) or it has a compatible change (that, is a change
  // that changes his name w/o changing the signature of the
  // function).
  bool has_sub_type_change = (f_name_id == s_name_id);

  if (to_be_reported())
    {
      assert(get_type_diff()->to_be_reported());
      out << indent
	  << "parameter " << f->get_index()
	  << " of type '"
	  << f->get_type_pretty_representation();

      if (has_sub_type_change)
	out << "' has sub-type changes:\n";
      else
	out << "' changed:\n";

      get_type_diff()->report(out, indent + "  ");
    }
}

/// Populate the vector of children nodes of the @ref diff base type
/// sub-object of this instance of @ref fn_parm_diff.
///
/// The children nodes can then later be retrieved using
/// diff::children_nodes()
void
fn_parm_diff::chain_into_hierarchy()
{
  if (get_type_diff())
    append_child_node(get_type_diff());
}

/// Compute the difference between two function_decl::parameter_sptr;
/// that is, between two function parameters.  Return a resulting
/// fn_parm_diff_sptr that represents the changes.
///
/// @param first the first subject of the diff.
///
/// @param second the second subject of the diff.
///
/// @param ctxt the context of the diff.
///
/// @return fn_parm_diff_sptr the resulting diff node.
fn_parm_diff_sptr
compute_diff(const function_decl::parameter_sptr	first,
	     const function_decl::parameter_sptr	second,
	     diff_context_sptr				ctxt)
{
  if (!first || !second)
    return fn_parm_diff_sptr();

  fn_parm_diff_sptr result(new fn_parm_diff(first, second, ctxt));
  ctxt->initialize_canonical_diff(result);

  return result;
}
// </fn_parm_diff stuff>

// <function_decl_diff stuff>
struct function_decl_diff::priv
{
  enum Flags
  {
    NO_FLAG = 0,
    IS_DECLARED_INLINE_FLAG = 1,
    IS_NOT_DECLARED_INLINE_FLAG = 1 << 1,
    BINDING_NONE_FLAG = 1 << 2,
    BINDING_LOCAL_FLAG = 1 << 3,
    BINDING_GLOBAL_FLAG = 1 << 4,
    BINDING_WEAK_FLAG = 1 << 5
  };// end enum Flags

  diff_sptr	return_type_diff_;
  edit_script	parm_changes_;
  vector<char>	first_fn_flags_;
  vector<char>	second_fn_flags_;
  edit_script	fn_flags_changes_;

  // useful lookup tables.
  string_parm_map			deleted_parms_;
  string_parm_map			added_parms_;
  // This map contains parameters sub-type changes that don't change
  // the name of the type of the parameter.
  string_fn_parm_diff_sptr_map		subtype_changed_parms_;
  unsigned_parm_map			deleted_parms_by_id_;
  unsigned_parm_map			added_parms_by_id_;
  // This map contains parameter type changes that actually change the
  // name of the type of the parameter, but in a compatible way;
  // otherwise, the mangling of the function would have changed (in
  // c++ at least).
  unsigned_fn_parm_diff_sptr_map	changed_parms_by_id_;

  priv(diff_sptr return_type_diff)
    : return_type_diff_(return_type_diff)
  {}

  Flags
  fn_is_declared_inline_to_flag(function_decl_sptr f) const
  {
    return (f->is_declared_inline()
	    ? IS_DECLARED_INLINE_FLAG
	    : IS_NOT_DECLARED_INLINE_FLAG);
  }

  Flags
  fn_binding_to_flag(function_decl_sptr f) const
  {
    decl_base::binding b = f->get_binding();
    Flags result = NO_FLAG;
    switch (b)
      {
      case decl_base::BINDING_NONE:
	result = BINDING_NONE_FLAG;
	break;
      case decl_base::BINDING_LOCAL:
	  result = BINDING_LOCAL_FLAG;
	  break;
      case decl_base::BINDING_GLOBAL:
	  result = BINDING_GLOBAL_FLAG;
	  break;
      case decl_base::BINDING_WEAK:
	result = BINDING_WEAK_FLAG;
	break;
      }
    return result;
  }

};// end struct function_decl_diff::priv

/// Getter for a parameter at a given index (in the sequence of
/// parameters of the first function of the diff) marked deleted in
/// the edit script.
///
/// @param i the index of the parameter in the sequence of parameters
/// of the first function considered by the current function_decl_diff
/// object.
///
/// @return the parameter at index
const function_decl::parameter_sptr
function_decl_diff::deleted_parameter_at(int i) const
{return first_function_decl()->get_parameters()[i];}

/// Getter for a parameter at a given index (in the sequence of
/// parameters of the second function of the diff) marked inserted in
/// the edit script.
///
/// @param i the index of the parameter in the sequence of parameters
/// of the second function considered by the current
/// function_decl_diff object.
const function_decl::parameter_sptr
function_decl_diff::inserted_parameter_at(int i) const
{return second_function_decl()->get_parameters()[i];}

/// Build the lookup tables of the diff, if necessary.
void
function_decl_diff::ensure_lookup_tables_populated()
{
  string parm_name;
  function_decl::parameter_sptr parm;
  for (vector<deletion>::const_iterator i =
	 priv_->parm_changes_.deletions().begin();
       i != priv_->parm_changes_.deletions().end();
       ++i)
    {
      parm = *(first_function_decl()->get_first_non_implicit_parm()
	       + i->index());
      parm_name = parm->get_name_id();
      // If for a reason the type name is empty we want to know and
      // fix that.
      assert(!parm_name.empty());
      priv_->deleted_parms_[parm_name] = parm;
      priv_->deleted_parms_by_id_[parm->get_index()] = parm;
    }

  for (vector<insertion>::const_iterator i =
	 priv_->parm_changes_.insertions().begin();
       i != priv_->parm_changes_.insertions().end();
       ++i)
    {
      for (vector<unsigned>::const_iterator j =
	     i->inserted_indexes().begin();
	   j != i->inserted_indexes().end();
	   ++j)
	{
	  parm = *(second_function_decl()->get_first_non_implicit_parm() + *j);
	  parm_name = parm->get_name_id();
	  // If for a reason the type name is empty we want to know and
	  // fix that.
	  assert(!parm_name.empty());
	  {
	    string_parm_map::const_iterator k =
	      priv_->deleted_parms_.find(parm_name);
	    if (k != priv_->deleted_parms_.end())
	      {
		if (*k->second != *parm)
		  priv_->subtype_changed_parms_[parm_name] =
		    compute_diff(k->second, parm, context());
		priv_->deleted_parms_.erase(parm_name);
	      }
	    else
	      priv_->added_parms_[parm_name] = parm;
	  }
	  {
	    unsigned_parm_map::const_iterator k =
	      priv_->deleted_parms_by_id_.find(parm->get_index());
	    if (k != priv_->deleted_parms_by_id_.end())
	      {
		if (*k->second != *parm
		    && (k->second->get_name_id() != parm_name))
		  priv_->changed_parms_by_id_[parm->get_index()] =
		    compute_diff(k->second, parm, context());
		priv_->added_parms_.erase(parm_name);
		priv_->deleted_parms_.erase(k->second->get_name_id());
		priv_->deleted_parms_by_id_.erase(parm->get_index());
	      }
	    else
	      priv_->added_parms_by_id_[parm->get_index()] = parm;
	  }
	}
    }
}

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref function_decl_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
function_decl_diff::chain_into_hierarchy()
{
  if (diff_sptr d = return_type_diff())
    append_child_node(d);

  for (string_fn_parm_diff_sptr_map::const_iterator i =
	 subtype_changed_parms().begin();
       i != subtype_changed_parms().end();
       ++i)
    if (diff_sptr d = i->second)
      append_child_node(d);

  for (unsigned_fn_parm_diff_sptr_map::const_iterator i =
	 priv_->changed_parms_by_id_.begin();
       i != priv_->changed_parms_by_id_.end();
       ++i)
    if (diff_sptr d = i->second)
      append_child_node(d);
}

/// Constructor for function_decl_diff
///
/// @param first the first function considered by the diff.
///
/// @param second the second function considered by the diff.
function_decl_diff::function_decl_diff(const function_decl_sptr first,
				       const function_decl_sptr second,
				       const diff_sptr		ret_type_diff,
				       diff_context_sptr	ctxt)
  : decl_diff_base(first, second, ctxt),
    priv_(new priv(ret_type_diff))
{
  priv_->first_fn_flags_.push_back
    (priv_->fn_is_declared_inline_to_flag(first_function_decl()));
  priv_->first_fn_flags_.push_back
    (priv_->fn_binding_to_flag(first_function_decl()));

  priv_->second_fn_flags_.push_back
    (priv_->fn_is_declared_inline_to_flag(second_function_decl()));
  priv_->second_fn_flags_.push_back
    (priv_->fn_binding_to_flag(second_function_decl()));
}

/// Finish building the current instance of @ref function_decl_diff.
void
function_decl_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// @return the first function considered by the diff.
const function_decl_sptr
function_decl_diff::first_function_decl() const
{return dynamic_pointer_cast<function_decl>(first_subject());}

/// @return the second function considered by the diff.
const function_decl_sptr
function_decl_diff::second_function_decl() const
{return dynamic_pointer_cast<function_decl>(second_subject());}

/// Accessor for the diff of the return types of the two functions.
///
/// @return the diff of the return types.
const diff_sptr
function_decl_diff::return_type_diff() const
{return priv_->return_type_diff_;}

/// @return a map of the parameters whose type got changed.  The key
/// of the map is the name of the type.
const string_fn_parm_diff_sptr_map&
function_decl_diff::subtype_changed_parms() const
{return priv_->subtype_changed_parms_;}

/// @return a map of parameters that got removed.
const string_parm_map&
function_decl_diff::removed_parms() const
{return priv_->deleted_parms_;}

/// @return a map of parameters that got added.
const string_parm_map&
function_decl_diff::added_parms() const
{return priv_->added_parms_;}

/// @return the pretty representation for the current instance of @ref
/// function_decl_diff.
const string&
function_decl_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "function_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
function_decl_diff::has_changes() const
{return *first_function_decl() != *second_function_decl();}

/// @return true iff the current diff node carries local changes.
bool
function_decl_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_function_decl(), *second_function_decl(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}
/// A comparison functor to compare two instances of @ref fn_parm_diff
/// based on their indexes.
struct fn_parm_diff_comp
{
  /// @param f the first diff
  ///
  /// @param s the second diff
  ///
  /// @return true if the index of @p f is less than the index of @p
  /// s.
  bool
  operator()(const fn_parm_diff& f, const fn_parm_diff& s)
  {return f.first_parameter()->get_index() < s.first_parameter()->get_index();}

  bool
  operator()(const fn_parm_diff_sptr& f, const fn_parm_diff_sptr& s)
  {return operator()(*f, *s);}
}; // end struct fn_parm_diff_comp

/// Sort a map of @ref fn_parm_diff by the indexes of the function
/// parameters.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector of changed function
/// parms.
static void
sort_string_fn_parm_diff_sptr_map(const unsigned_fn_parm_diff_sptr_map& map,
				  vector<fn_parm_diff_sptr>&		sorted)
{
  sorted.reserve(map.size());
  for (unsigned_fn_parm_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  fn_parm_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Sort a map of changed function parameters by the indexes of the
/// function parameters.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector of instances of @ref
/// fn_parm_diff_sptr
static void
sort_string_fn_parm_diff_sptr_map(const string_fn_parm_diff_sptr_map&	map,
				  vector<fn_parm_diff_sptr>&		sorted)
{
  sorted.reserve(map.size());
  for (string_fn_parm_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  fn_parm_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Serialize a report of the changes encapsulated in the current
/// instance of function_decl_diff over to an output stream.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
function_decl_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  maybe_report_diff_for_member(first_function_decl(),
			       second_function_decl(),
			       context(), out, indent);

  function_decl_sptr ff = first_function_decl();
  function_decl_sptr sf = second_function_decl();

  diff_context_sptr ctxt = context();
  corpus_sptr fc = ctxt->get_first_corpus();
  corpus_sptr sc = ctxt->get_second_corpus();

  string qn1 = ff->get_qualified_name(), qn2 = sf->get_qualified_name(),
    linkage_names1, linkage_names2;
  elf_symbol_sptr s1 = ff->get_symbol(), s2 = sf->get_symbol();

  if (s1)
    linkage_names1 = s1->get_id_string();
  if (s2)
    linkage_names2 = s2->get_id_string();

  // If the symbols for ff and sf have aliases, get all the names of
  // the aliases;
  if (fc && s1)
    linkage_names1 =
      s1->get_aliases_id_string(fc->get_fun_symbol_map());
  if (sc && s2)
    linkage_names2 =
      s2->get_aliases_id_string(sc->get_fun_symbol_map());

  if (qn1 != qn2)
    {
      string frep1 = first_function_decl()->get_pretty_representation(),
	frep2 = second_function_decl()->get_pretty_representation();
      out << indent << "'" << frep1 << " {" << linkage_names1<< "}"
	  << "' now becomes '"
	  << frep2 << " {" << linkage_names2 << "}" << "'\n";
      return;
    }

  // Now report about inline-ness changes
  if (ff->is_declared_inline() != sf->is_declared_inline())
    {
      out << indent;
      if (ff->is_declared_inline())
	out << sf->get_pretty_representation()
	    << " is not declared inline anymore\n";
      else
	out << sf->get_pretty_representation()
	    << " is now declared inline\n";
    }

  // Report about the size of the function address
  if (ff->get_type()->get_size_in_bits() != sf->get_type()->get_size_in_bits())
    {
      out << indent << "address size of function changed from "
	  << ff->get_type()->get_size_in_bits()
	  << " bits to "
	  << sf->get_type()->get_size_in_bits()
	  << " bits\n";
    }

  // Report about the alignment of the function address
  if (ff->get_type()->get_alignment_in_bits()
      != sf->get_type()->get_alignment_in_bits())
    {
      out << indent << "address alignment of function changed from "
	  << ff->get_type()->get_alignment_in_bits()
	  << " bits to "
	  << sf->get_type()->get_alignment_in_bits()
	  << " bits\n";
    }

  // Report about return type differences.
  if (priv_->return_type_diff_ && priv_->return_type_diff_->to_be_reported())
    {
      out << indent << "return type changed:\n";
      priv_->return_type_diff_->report(out, indent + "  ");
    }

  // Hmmh, the above was quick.  Now report about function parameters;
  // this shouldn't be as straightforward.
  //
  // Report about the parameter types that have changed sub-types.
  vector<fn_parm_diff_sptr> sorted_fn_parms;
  sort_string_fn_parm_diff_sptr_map(priv_->subtype_changed_parms_,
				    sorted_fn_parms);
  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 sorted_fn_parms.begin();
       i != sorted_fn_parms.end();
       ++i)
    {
      diff_sptr d = *i;
      if (d && d->to_be_reported())
	d->report(out, indent);
    }
  // Report about parameters that have changed, while staying
  // compatible -- otherwise they would have changed the mangled name
  // of the function and the function would have been reported as
  // removed.
  sorted_fn_parms.clear();
  sort_string_fn_parm_diff_sptr_map(priv_->changed_parms_by_id_,
				    sorted_fn_parms);
  for (vector<fn_parm_diff_sptr>::const_iterator i = sorted_fn_parms.begin();
       i != sorted_fn_parms.end();
       ++i)
    {
      diff_sptr d = *i;
      if (d && d->to_be_reported())
	d->report(out, indent);
    }

  // Report about the parameters that got removed.
  bool emitted = false;
  for (string_parm_map::const_iterator i = priv_->deleted_parms_.begin();
       i != priv_->deleted_parms_.end();
       ++i)
    {
      out << indent << "parameter " << i->second->get_index()
	  << " of type '" << i->second->get_type_pretty_representation()
	  << "' was removed\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  // Report about the parameters that got added
  emitted = false;
  for (string_parm_map::const_iterator i = priv_->added_parms_.begin();
       i != priv_->added_parms_.end();
       ++i)
    {
      out << indent << "parameter " << i->second->get_index()
	  << " of type '" << i->second->get_type_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";
}

/// Compute the diff between two function_decl.
///
/// @param first the first function_decl to consider for the diff
///
/// @param second the second function_decl to consider for the diff
///
/// @param ctxt the diff context to use.
///
/// @return the computed diff
function_decl_diff_sptr
compute_diff(const function_decl_sptr first,
	     const function_decl_sptr second,
	     diff_context_sptr ctxt)
{
  if (!first || !second)
    {
      // TODO: implement this for either first or second being NULL.
      return function_decl_diff_sptr();
    }

  diff_sptr return_type_diff =
    compute_diff_for_types(first->get_return_type(),
			   second->get_return_type(),
			   ctxt);

  function_decl_diff_sptr result(new function_decl_diff(first, second,
							return_type_diff,
							ctxt));


  diff_utils::compute_diff(first->get_first_non_implicit_parm(),
			   first->get_parameters().end(),
			   second->get_first_non_implicit_parm(),
			   second->get_parameters().end(),
			   result->priv_->parm_changes_);

  diff_utils::compute_diff(result->priv_->first_fn_flags_.begin(),
			   result->priv_->first_fn_flags_.end(),
			   result->priv_->second_fn_flags_.begin(),
			   result->priv_->second_fn_flags_.end(),
			   result->priv_->fn_flags_changes_);

  result->ensure_lookup_tables_populated();

  ctxt->initialize_canonical_diff(result);

  return result;
}

// </function_decl_diff stuff>

// <type_decl_diff stuff>

/// Constructor for type_decl_diff.
type_decl_diff::type_decl_diff(const type_decl_sptr first,
			       const type_decl_sptr second,
			       diff_context_sptr ctxt)
  : type_diff_base(first, second, ctxt)
{}

/// Finish building the current instance of @ref type_decl_diff.
void
type_decl_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  diff::priv_->finished_ = true;
}

/// Getter for the first subject of the type_decl_diff.
///
/// @return the first type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::first_type_decl() const
{return dynamic_pointer_cast<type_decl>(first_subject());}

/// Getter for the second subject of the type_decl_diff.
///
/// @return the second type_decl involved in the diff.
const type_decl_sptr
type_decl_diff::second_type_decl() const
{return dynamic_pointer_cast<type_decl>(second_subject());}

/// @return the pretty representation for the current instance of @ref
/// type_decl_diff.
const string&
type_decl_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "type_decl_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}
/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
type_decl_diff::has_changes() const
{
  type_base_sptr f = is_type(first_type_decl()),
    s = is_type(second_type_decl());
  assert(f && s);

  return (diff_length_of_decl_bases(first_type_decl(),
				    second_type_decl())
	  || diff_length_of_type_bases(f, s));
}

/// @return true iff the current diff node carries local changes.
bool
type_decl_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_type_decl(), *second_type_decl(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}
/// Ouputs a report of the differences between of the two type_decl
/// involved in the type_decl_diff.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the string to use for indentatino indent.
void
type_decl_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  type_decl_sptr f = first_type_decl(), s = second_type_decl();

  string name = f->get_pretty_representation();

  bool n = report_name_size_and_alignment_changes(f, s, context(),
						  out, indent,
						  /*new line=*/false);

  if (f->get_visibility() != s->get_visibility())
    {
      if (n)
	out << "\n";
      out << indent
	  << "visibility changed from '"
	  << f->get_visibility() << "' to '" << s->get_visibility();
      n = true;
    }

  if (f->get_linkage_name() != s->get_linkage_name())
    {
      if (n)
	out << "\n";
      out << indent
	  << "mangled name changed from '"
	  << f->get_linkage_name() << "' to "
	  << s->get_linkage_name();
      n = true;
    }

  if (n)
    out << "\n";
}

/// Compute a diff between two type_decl.
///
/// This function doesn't actually compute a diff.  As a type_decl is
/// very simple (unlike compound constructs like function_decl or
/// class_decl) it's easy to just compare the components of the
/// type_decl to know what has changed.  Thus this function just
/// builds and return a type_decl_diff object.  The
/// type_decl_diff::report function will just compare the components
/// of the the two type_decl and display where and how they differ.
///
/// @param first a pointer to the first type_decl to
/// consider.
///
/// @param second a pointer to the second type_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the resulting type_decl_diff.
type_decl_diff_sptr
compute_diff(const type_decl_sptr	first,
	     const type_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  type_decl_diff_sptr result(new type_decl_diff(first, second, ctxt));

  // We don't need to actually compute a diff here as a type_decl
  // doesn't have complicated sub-components.  type_decl_diff::report
  // just walks the members of the type_decls and display information
  // about the ones that have changed.  On a similar note,
  // type_decl_diff::length returns 0 if the two type_decls are equal,
  // and 1 otherwise.

  ctxt->initialize_canonical_diff(result);

  return result;
}

// </type_decl_diff stuff>

// <typedef_diff stuff>

struct typedef_diff::priv
{
  diff_sptr underlying_type_diff_;

  priv(const diff_sptr underlying_type_diff)
    : underlying_type_diff_(underlying_type_diff)
  {}
};//end struct typedef_diff::priv

/// Populate the vector of children node of the @ref diff base type
/// sub-object of this instance of @ref typedef_diff.
///
/// The children node can then later be retrieved using
/// diff::children_node().
void
typedef_diff::chain_into_hierarchy()
{append_child_node(underlying_type_diff());}

/// Constructor for typedef_diff.
typedef_diff::typedef_diff(const typedef_decl_sptr	first,
			   const typedef_decl_sptr	second,
			   const diff_sptr		underlying,
			   diff_context_sptr		ctxt)
  : type_diff_base(first, second, ctxt),
    priv_(new priv(underlying))
{}

/// Finish building the current instance of @ref typedef_diff.
void
typedef_diff::finish_diff_type()
{
  if (diff::priv_->finished_)
    return;
  chain_into_hierarchy();
  diff::priv_->finished_ = true;
}

/// Getter for the firt typedef_decl involved in the diff.
///
/// @return the first subject of the diff.
const typedef_decl_sptr
typedef_diff::first_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(first_subject());}

/// Getter for the second typedef_decl involved in the diff.
///
/// @return the second subject of the diff.
const typedef_decl_sptr
typedef_diff::second_typedef_decl() const
{return dynamic_pointer_cast<typedef_decl>(second_subject());}

/// Getter for the diff between the two underlying types of the
/// typedefs.
///
/// @return the diff object reprensenting the difference between the
/// two underlying types of the typedefs.
const diff_sptr
typedef_diff::underlying_type_diff() const
{return priv_->underlying_type_diff_;}

/// Setter for the diff between the two underlying types of the
/// typedefs.
///
/// @param d the new diff object reprensenting the difference between
/// the two underlying types of the typedefs.
void
typedef_diff::underlying_type_diff(const diff_sptr d)
{priv_->underlying_type_diff_ = d;}

/// @return the pretty representation for the current instance of @ref
/// typedef_diff.
const string&
typedef_diff::get_pretty_representation() const
{
  if (diff::priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "typedef_diff["
	<< first_subject()->get_pretty_representation()
	<< ", "
	<< second_subject()->get_pretty_representation()
	<< "]";
      diff::priv_->pretty_representation_ = o.str();
    }
  return diff::priv_->pretty_representation_;
}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
typedef_diff::has_changes() const
{
  decl_base_sptr second = second_typedef_decl();
  return !(*first_typedef_decl() == *second);
}

/// @return true iff the current diff node carries local changes.
bool
typedef_diff::has_local_changes() const
{
  ir::change_kind k = ir::NO_CHANGE_KIND;
  if (!equals(*first_typedef_decl(), *second_typedef_decl(), &k))
    return k & LOCAL_CHANGE_KIND;
  return false;
}

/// Reports the difference between the two subjects of the diff in a
/// serialized form.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the indentation string to use.
void
typedef_diff::report(ostream& out, const string& indent) const
{
  if (!to_be_reported())
    return;

  bool emit_nl = false;
  typedef_decl_sptr f = first_typedef_decl(), s = second_typedef_decl();

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(f, s);

  maybe_report_diff_for_member(f, s, context(), out, indent);

  if (filtering::has_harmless_name_change(f, s)
      && context()->get_allowed_category() & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
    {
      out << indent << "typedef name changed from "
	  << f->get_qualified_name()
	  << " to "
	  << s->get_qualified_name()
	  << "\n";
      emit_nl = true;
    }

  diff_sptr d = underlying_type_diff();
  if (d && d->to_be_reported())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(d, "underlying type");
      out << indent
	  << "underlying type '"
	  << d->first_subject()->get_pretty_representation()
	  << "' changed:\n";
      d->report(out, indent + "  ");
      emit_nl = false;
    }

  if (emit_nl)
    out << "\n";
}

/// Compute a diff between two typedef_decl.
///
/// @param first a pointer to the first typedef_decl to consider.
///
/// @param second a pointer to the second typedef_decl to consider.
///
/// @param ctxt the diff context to use.
///
/// @return a pointer to the the resulting typedef_diff.
typedef_diff_sptr
compute_diff(const typedef_decl_sptr	first,
	     const typedef_decl_sptr	second,
	     diff_context_sptr		ctxt)
{
  diff_sptr d = compute_diff_for_types(first->get_underlying_type(),
				       second->get_underlying_type(),
				       ctxt);
  typedef_diff_sptr result(new typedef_diff(first, second, d, ctxt));

  ctxt->initialize_canonical_diff(result);

  return result;
}

// </typedef_diff stuff>

// <translation_unit_diff stuff>

struct translation_unit_diff::priv
{
  translation_unit_sptr first_;
  translation_unit_sptr second_;

  priv(translation_unit_sptr f, translation_unit_sptr s)
    : first_(f), second_(s)
  {}
};//end struct translation_unit_diff::priv

/// Constructor for translation_unit_diff.
///
/// @param first the first translation unit to consider for this diff.
///
/// @param second the second translation unit to consider for this diff.
translation_unit_diff::translation_unit_diff(translation_unit_sptr first,
					     translation_unit_sptr second,
					     diff_context_sptr ctxt)
  : scope_diff(first->get_global_scope(), second->get_global_scope(), ctxt),
    priv_(new priv(first, second))
{
}

/// Getter for the first translation unit of this diff.
///
/// @return the first translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::first_translation_unit() const
{return priv_->first_;}

/// Getter for the second translation unit of this diff.
///
/// @return the second translation unit of this diff.
const translation_unit_sptr
translation_unit_diff::second_translation_unit() const
{return priv_->second_;}

/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
translation_unit_diff::has_changes() const
{return scope_diff::has_changes();}

/// @return true iff the current diff node carries local changes.
bool
translation_unit_diff::has_local_changes() const
{return false;}

/// Report the diff in a serialized form.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
translation_unit_diff::report(ostream& out, const string& indent) const
{scope_diff::report(out, indent);}

/// Compute the diff between two translation_units.
///
/// @param first the first translation_unit to consider.
///
/// @param second the second translation_unit to consider.
///
/// @param ctxt the diff context to use.  If null, this function will
/// create a new context and set to the diff object returned.
///
/// @return the newly created diff object.
translation_unit_diff_sptr
compute_diff(const translation_unit_sptr	first,
	     const translation_unit_sptr	second,
	     diff_context_sptr			ctxt)
{
  if (!ctxt)
    ctxt.reset(new diff_context);

  // TODO: handle first or second having empty contents.
  translation_unit_diff_sptr tu_diff(new translation_unit_diff(first, second,
							       ctxt));
  scope_diff_sptr sc_diff = dynamic_pointer_cast<scope_diff>(tu_diff);

  compute_diff(static_pointer_cast<scope_decl>(first->get_global_scope()),
	       static_pointer_cast<scope_decl>(second->get_global_scope()),
	       sc_diff,
	       ctxt);

  ctxt->initialize_canonical_diff(tu_diff);

  return tu_diff;
}

// </translation_unit_diff stuff>

/// The type of the private data of corpus_diff::diff_stats.
class corpus_diff::diff_stats::priv
{
  friend class corpus_diff::diff_stats;

  size_t num_func_removed;
  size_t num_func_added;
  size_t num_func_changed;
  size_t num_func_filtered_out;
  size_t num_vars_removed;
  size_t num_vars_added;
  size_t num_vars_changed;
  size_t num_vars_filtered_out;
  size_t num_func_syms_removed;
  size_t num_func_syms_added;
  size_t num_var_syms_removed;
  size_t num_var_syms_added;

  priv()
    : num_func_removed(0),
      num_func_added(0),
      num_func_changed(0),
      num_func_filtered_out(0),
      num_vars_removed(0),
      num_vars_added(0),
      num_vars_changed(0),
      num_vars_filtered_out(0),
      num_func_syms_removed(0),
      num_func_syms_added(0),
      num_var_syms_removed(0),
      num_var_syms_added(0)
  {}
}; // end class corpus_diff::diff_stats::priv

/// Default constructor for the @ref diff_stat type.
corpus_diff::diff_stats::diff_stats()
  : priv_(new priv)
{}

/// Getter for the number of functions removed.
///
/// @return the number of functions removed.
size_t
corpus_diff::diff_stats::num_func_removed() const
{return priv_->num_func_removed;}

/// Setter for the number of functions removed.
///
/// @param n the new number of functions removed.
void
corpus_diff::diff_stats::num_func_removed(size_t n)
{priv_->num_func_removed = n;}

/// Getter for the number of functions added.
///
/// @return the number of functions added.
size_t
corpus_diff::diff_stats::num_func_added() const
{return priv_->num_func_added;}

/// Setter for the number of functions added.
///
/// @param n the new number of functions added.
void
corpus_diff::diff_stats::num_func_added(size_t n)
{priv_->num_func_added = n;}

/// Getter for the number of functions that have a change in one of
/// their sub-types.
///
/// @return the number of functions that have a change in one of their
/// sub-types.
size_t
corpus_diff::diff_stats::num_func_changed() const
{return priv_->num_func_changed;}

/// Setter for the number of functions that have a change in one of
/// their sub-types.
///
/// @@param n the new number of functions that have a change in one of
/// their sub-types.
void
corpus_diff::diff_stats::num_func_changed(size_t n)
{priv_->num_func_changed = n;}

/// Getter for the number of functions that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @return the number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
size_t
corpus_diff::diff_stats::num_func_filtered_out() const
{return priv_->num_func_filtered_out;}

/// Setter for the number of functions that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @param n the new number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
void
corpus_diff::diff_stats::num_func_filtered_out(size_t n)
{priv_->num_func_filtered_out = n;}

/// Getter for the number of functions that have a change in their
/// sub-types, minus the number of these functions that got filtered
/// out from the diff.
///
/// @return for the the number of functions that have a change in
/// their sub-types, minus the number of these functions that got
/// filtered out from the diff.
size_t
corpus_diff::diff_stats::net_num_func_changed() const
{return num_func_changed() - num_func_filtered_out();}

/// Getter for the number of variables removed.
///
/// @return the number of variables removed.
size_t
corpus_diff::diff_stats::num_vars_removed() const
{return priv_->num_vars_removed;}

/// Setter for the number of variables removed.
///
/// @param n the new number of variables removed.
void
corpus_diff::diff_stats::num_vars_removed(size_t n)
{priv_->num_vars_removed = n;}

/// Getter for the number of variables added.
///
/// @return the number of variables added.
size_t
corpus_diff::diff_stats::num_vars_added() const
{return priv_->num_vars_added;}

/// Setter for the number of variables added.
///
/// @param n the new number of variables added.
void
corpus_diff::diff_stats::num_vars_added(size_t n)
{priv_->num_vars_added = n;}

/// Getter for the number of variables that have a change in one of
/// their sub-types.
///
/// @return the number of variables that have a change in one of their
/// sub-types.
size_t
corpus_diff::diff_stats::num_vars_changed() const
{return priv_->num_vars_changed;}

/// Setter for the number of variables that have a change in one of
/// their sub-types.
///
/// @param n the new number of variables that have a change in one of
/// their sub-types.
void
corpus_diff::diff_stats::num_vars_changed(size_t n)
{priv_->num_vars_changed = n;}

/// Getter for the number of variables that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @return the number of functions that have a change in one of their
/// sub-types, and that have been filtered out.
size_t
corpus_diff::diff_stats::num_vars_filtered_out() const
{return priv_->num_vars_filtered_out;}

/// Setter for the number of variables that have a change in one of
/// their sub-types, and that have been filtered out.
///
/// @param n the new number of variables that have a change in one of their
/// sub-types, and that have been filtered out.
void
corpus_diff::diff_stats::num_vars_filtered_out(size_t n)
{priv_->num_vars_filtered_out = n;}

/// Getter for the number of variables that have a change in their
/// sub-types, minus the number of these variables that got filtered
/// out from the diff.
///
/// @return for the the number of variables that have a change in
/// their sub-types, minus the number of these variables that got
/// filtered out from the diff.
size_t
corpus_diff::diff_stats::net_num_vars_changed() const
{return num_vars_changed() - num_vars_filtered_out();}

/// Getter for the number of function symbols (not referenced by any
/// debug info) that got removed.
///
/// @return the number of function symbols (not referenced by any
/// debug info) that got removed.
size_t
corpus_diff::diff_stats::num_func_syms_removed() const
{return priv_->num_func_syms_removed;}

/// Setter for the number of function symbols (not referenced by any
/// debug info) that got removed.
///
/// @param n the number of function symbols (not referenced by any
/// debug info) that got removed.
void
corpus_diff::diff_stats::num_func_syms_removed(size_t n)
{priv_->num_func_syms_removed = n;}

/// Getter for the number of function symbols (not referenced by any
/// debug info) that got added.
///
/// @return the number of function symbols (not referenced by any
/// debug info) that got added.
size_t
corpus_diff::diff_stats::num_func_syms_added() const
{return priv_->num_func_syms_added;}

/// Setter for the number of function symbols (not referenced by any
/// debug info) that got added.
///
/// @param n the new number of function symbols (not referenced by any
/// debug info) that got added.
void
corpus_diff::diff_stats::num_func_syms_added(size_t n)
{priv_->num_func_syms_added = n;}

/// Getter for the number of variable symbols (not referenced by any
/// debug info) that got removed.
///
/// @return the number of variable symbols (not referenced by any
/// debug info) that got removed.
size_t
corpus_diff::diff_stats::num_var_syms_removed() const
{return priv_->num_var_syms_removed;}

/// Setter for the number of variable symbols (not referenced by any
/// debug info) that got removed.
///
/// @param n the number of variable symbols (not referenced by any
/// debug info) that got removed.
void
corpus_diff::diff_stats::num_var_syms_removed(size_t n)
{priv_->num_var_syms_removed = n;}

/// Getter for the number of variable symbols (not referenced by any
/// debug info) that got added.
///
/// @return the number of variable symbols (not referenced by any
/// debug info) that got added.
size_t
corpus_diff::diff_stats::num_var_syms_added() const
{return priv_->num_var_syms_added;}

/// Setter for the number of variable symbols (not referenced by any
/// debug info) that got added.
///
/// @param n the new number of variable symbols (not referenced by any
/// debug info) that got added.
void
corpus_diff::diff_stats::num_var_syms_added(size_t n)
{priv_->num_var_syms_added = n;}

// <corpus stuff>
struct corpus_diff::priv
{
  bool					finished_;
  string				pretty_representation_;
  vector<diff_sptr>			children_;
  diff_context_sptr			ctxt_;
  corpus_sptr				first_;
  corpus_sptr				second_;
  corpus_diff::diff_stats		diff_stats_;
  bool					filters_and_suppr_applied_;
  bool					sonames_equal_;
  bool					architectures_equal_;
  edit_script				fns_edit_script_;
  edit_script				vars_edit_script_;
  edit_script				unrefed_fn_syms_edit_script_;
  edit_script				unrefed_var_syms_edit_script_;
  string_function_ptr_map		deleted_fns_;
  string_function_ptr_map		added_fns_;
  string_function_decl_diff_sptr_map	changed_fns_map_;
  function_decl_diff_sptrs_type	changed_fns_;
  string_var_ptr_map			deleted_vars_;
  string_var_ptr_map			added_vars_;
  string_var_diff_sptr_map		changed_vars_map_;
  var_diff_sptrs_type			sorted_changed_vars_;
  string_elf_symbol_map		added_unrefed_fn_syms_;
  string_elf_symbol_map		deleted_unrefed_fn_syms_;
  string_elf_symbol_map		added_unrefed_var_syms_;
  string_elf_symbol_map		deleted_unrefed_var_syms_;

  priv()
    : finished_(false),
      filters_and_suppr_applied_(false),
      sonames_equal_(false),
      architectures_equal_(false)
  {}

  bool
  lookup_tables_empty() const;

  void
  clear_lookup_tables();

  void
  ensure_lookup_tables_populated();

  void
  apply_filters_and_compute_diff_stats(corpus_diff::diff_stats&);

  void
  emit_diff_stats(const diff_stats&	stats,
		  ostream&		out,
		  const string&	indent);

  void
  categorize_redundant_changed_sub_nodes();

  void
  clear_redundancy_categorization();

  void
  maybe_dump_diff_tree();
}; // end corpus::priv

/// Tests if the lookup tables are empty.
///
/// @return true if the lookup tables are empty, false otherwise.
bool
corpus_diff::priv::lookup_tables_empty() const
{
  return (deleted_fns_.empty()
	  && added_fns_.empty()
	  && changed_fns_map_.empty()
	  && deleted_vars_.empty()
	  && added_vars_.empty()
	  && changed_vars_map_.empty());
}

/// Clear the lookup tables useful for reporting an enum_diff.
void
corpus_diff::priv::clear_lookup_tables()
{
  deleted_fns_.clear();
  added_fns_.clear();
  changed_fns_map_.clear();
  deleted_vars_.clear();
  added_vars_.clear();
  changed_vars_map_.clear();
}

/// If the lookup tables are not yet built, walk the differences and
/// fill the lookup tables.
void
corpus_diff::priv::ensure_lookup_tables_populated()
{
  if (!lookup_tables_empty())
    return;

  {
    edit_script& e = fns_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_functions().size());

	function_decl* deleted_fn = first_->get_functions()[i];
	string n = deleted_fn->get_id();
	assert(!n.empty());
	assert(deleted_fns_.find(n) == deleted_fns_.end());
	deleted_fns_[n] = deleted_fn;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    function_decl* added_fn = second_->get_functions()[i];
	    string n = added_fn->get_id();
	    assert(!n.empty());
	    assert(added_fns_.find(n) == added_fns_.end());
	    string_function_ptr_map::const_iterator j =
	      deleted_fns_.find(n);
	    if (j != deleted_fns_.end())
	      {
		function_decl_sptr f(j->second, noop_deleter());
		function_decl_sptr s(added_fn, noop_deleter());
		function_decl_diff_sptr d = compute_diff(f, s, ctxt_);
		if (*j->second != *added_fn)
		  changed_fns_map_[j->first] = d;
		deleted_fns_.erase(j);
	      }
	    else
	      added_fns_[n] = added_fn;
	  }
      }
    sort_string_function_decl_diff_sptr_map(changed_fns_map_, changed_fns_);

    // Now walk the allegedly deleted functions; check if their
    // underlying symbols are deleted as well; otherwise, consider
    // that the function in question hasn't been deleted.

    vector<string> to_delete;
    for (string_function_ptr_map::const_iterator i = deleted_fns_.begin();
	 i != deleted_fns_.end();
	 ++i)
      if (second_->lookup_function_symbol(i->second->get_symbol()->get_name(),
					  i->second->get_symbol()->get_version().str()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      deleted_fns_.erase(*i);

    // Do something similar for added functions.

    to_delete.clear();
    for (string_function_ptr_map::const_iterator i = added_fns_.begin();
	 i != added_fns_.end();
	 ++i)
      if (first_->lookup_function_symbol(i->second->get_symbol()->get_name(),
					 i->second->get_symbol()->get_version().str()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      added_fns_.erase(*i);
  }

  {
    edit_script& e = vars_edit_script_;

    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_variables().size());

	var_decl* deleted_var = first_->get_variables()[i];
	string n = deleted_var->get_id();
	assert(!n.empty());
	assert(deleted_vars_.find(n) == deleted_vars_.end());
	deleted_vars_[n] = deleted_var;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    var_decl* added_var = second_->get_variables()[i];
	    string n = added_var->get_id();
	    assert(!n.empty());
	    {
	      string_var_ptr_map::const_iterator k = added_vars_.find(n);
	      if ( k != added_vars_.end())
		{
		  assert(is_member_decl(k->second)
			 && get_member_is_static(k->second));
		  continue;
		}
	    }
	    string_var_ptr_map::const_iterator j =
	      deleted_vars_.find(n);
	    if (j != deleted_vars_.end())
	      {
		if (*j->second != *added_var)
		  {
		    var_decl_sptr f(j->second, noop_deleter());
		    var_decl_sptr s(added_var, noop_deleter());
		    changed_vars_map_[n] = compute_diff(f, s, ctxt_);
		  }
		deleted_vars_.erase(j);
	      }
	    else
	      added_vars_[n] = added_var;
	  }
      }
    sort_string_var_diff_sptr_map(changed_vars_map_,
				  sorted_changed_vars_);

    // Now walk the allegedly deleted variables; check if their
    // underlying symbols are deleted as well; otherwise consider
    // that the variable in question hasn't been deleted.

    vector<string> to_delete;
    for (string_var_ptr_map::const_iterator i = deleted_vars_.begin();
	 i != deleted_vars_.end();
	 ++i)
      if (second_->lookup_variable_symbol(i->second->get_symbol()->get_name(),
					  i->second->get_symbol()->get_version().str()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      deleted_fns_.erase(*i);

    // Do something similar for added variables.

    to_delete.clear();
    for (string_var_ptr_map::const_iterator i = added_vars_.begin();
	 i != added_vars_.end();
	 ++i)
      if (first_->lookup_variable_symbol(i->second->get_symbol()->get_name(),
					 i->second->get_symbol()->get_version().str()))
	to_delete.push_back(i->first);

    for (vector<string>::const_iterator i = to_delete.begin();
	 i != to_delete.end();
	 ++i)
      added_vars_.erase(*i);
  }

  // Massage the edit script for added/removed function symbols that
  // were not referenced by any debug info and turn them into maps of
  // {symbol_name, symbol}.
  {
    edit_script& e = unrefed_fn_syms_edit_script_;
    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_unreferenced_function_symbols().size());
	elf_symbol_sptr deleted_sym =
	  first_->get_unreferenced_function_symbols()[i];
	if (!second_->lookup_function_symbol(deleted_sym->get_name(),
					     deleted_sym->get_version()))
	  deleted_unrefed_fn_syms_[deleted_sym->get_id_string()] = deleted_sym;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    assert(i < second_->get_unreferenced_function_symbols().size());
	    elf_symbol_sptr added_sym =
	      second_->get_unreferenced_function_symbols()[i];
	    if ((deleted_unrefed_fn_syms_.find(added_sym->get_id_string())
		 == deleted_unrefed_fn_syms_.end()))
	      {
		if (!first_->lookup_function_symbol(added_sym->get_name(),
						    added_sym->get_version()))
		  added_unrefed_fn_syms_[added_sym->get_id_string()] =
		    added_sym;
	      }
	    else
	      deleted_unrefed_fn_syms_.erase(added_sym->get_id_string());
	  }
      }
  }

  // Massage the edit script for added/removed variable symbols that
  // were not referenced by any debug info and turn them into maps of
  // {symbol_name, symbol}.
  {
    edit_script& e = unrefed_var_syms_edit_script_;
    for (vector<deletion>::const_iterator it = e.deletions().begin();
	 it != e.deletions().end();
	 ++it)
      {
	unsigned i = it->index();
	assert(i < first_->get_unreferenced_variable_symbols().size());
	elf_symbol_sptr deleted_sym =
	  first_->get_unreferenced_variable_symbols()[i];
	if (!second_->lookup_variable_symbol(deleted_sym->get_name(),
					     deleted_sym->get_version()))
	  deleted_unrefed_var_syms_[deleted_sym->get_id_string()] = deleted_sym;
      }

    for (vector<insertion>::const_iterator it = e.insertions().begin();
	 it != e.insertions().end();
	 ++it)
      {
	for (vector<unsigned>::const_iterator iit =
	       it->inserted_indexes().begin();
	     iit != it->inserted_indexes().end();
	     ++iit)
	  {
	    unsigned i = *iit;
	    assert(i < second_->get_unreferenced_variable_symbols().size());
	    elf_symbol_sptr added_sym =
	      second_->get_unreferenced_variable_symbols()[i];
	    if (deleted_unrefed_var_syms_.find(added_sym->get_id_string())
		== deleted_unrefed_var_syms_.end())
	      {
		if (!first_->lookup_variable_symbol(added_sym->get_name(),
						    added_sym->get_version()))
		  added_unrefed_var_syms_[added_sym->get_id_string()] =
		    added_sym;
	      }
	    else
	      deleted_unrefed_var_syms_.erase(added_sym->get_id_string());
	  }
      }
  }
}

/// Compute the diff stats.
///
/// To know the number of functions that got filtered out, this
/// function applies the categorizing filters to the diff sub-trees of
/// each function changes diff, prior to calculating the stats.
///
/// @param num_removed the number of removed functions.
///
/// @param num_added the number of added functions.
///
/// @param num_changed the number of changed functions.
///
/// @param num_filtered_out the number of changed functions that are
/// got filtered out from the report
void
corpus_diff::priv::apply_filters_and_compute_diff_stats(diff_stats& stat)

{
  stat.num_func_removed(deleted_fns_.size());
  stat.num_func_added(added_fns_.size());
  stat.num_func_changed(changed_fns_map_.size());

  stat.num_vars_removed(deleted_vars_.size());
  stat.num_vars_added(added_vars_.size());
  stat.num_vars_changed(changed_vars_map_.size());

  // Walk the changed function diff nodes to apply the categorization
  // filters.
  diff_sptr diff;
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i != changed_fns_.end();
       ++i)
    {
      diff_sptr diff = *i;
      ctxt_->maybe_apply_filters(diff);
    }

  // Walk the changed variable diff nodes to apply the categorization
  // filters.
  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i != sorted_changed_vars_.end();
       ++i)
    {
      diff_sptr diff = *i;
      ctxt_->maybe_apply_filters(diff);
    }

  categorize_redundant_changed_sub_nodes();

  // Walk the changed function diff nodes to count the number of
  // filtered-out functions.
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i != changed_fns_.end();
       ++i)
    if ((*i)->is_filtered_out())
      stat.num_func_filtered_out(stat.num_func_filtered_out() + 1);

  // Walk the changed variables diff nodes to count the number of
  // filtered-out variables.
  for (var_diff_sptrs_type ::const_iterator i = sorted_changed_vars_.begin();
       i != sorted_changed_vars_.end();
       ++i)
    {
      if ((*i)->is_filtered_out())
	stat.num_vars_filtered_out(stat.num_vars_filtered_out() + 1);
    }

  stat.num_func_syms_added(added_unrefed_fn_syms_.size());
  stat.num_func_syms_removed(deleted_unrefed_fn_syms_.size());
  stat.num_var_syms_added(added_unrefed_var_syms_.size());
  stat.num_var_syms_removed(deleted_unrefed_var_syms_.size());
}

/// Emit the summary of the functions & variables that got
/// removed/changed/added.
///
/// @param out the output stream to emit the stats to.
///
/// @param indent the indentation string to use in the summary.
void
corpus_diff::priv::emit_diff_stats(const diff_stats&	s,
				   ostream&		out,
				   const string&	indent)
{
  /// Report added/removed/changed functions.
  size_t total = s.num_func_removed() + s.num_func_added() +
    s.net_num_func_changed();

  if (!sonames_equal_)
    out << indent << "ELF SONAME changed\n";

  if (!architectures_equal_)
    out << indent << "ELF architecture changed\n";

  // function changes summary
  out << indent << "Functions changes summary: ";
  out << s.num_func_removed() << " Removed, ";
  out << s.num_func_changed() - s.num_func_filtered_out() << " Changed";
  if (s.num_func_filtered_out())
    out << " (" << s.num_func_filtered_out() << " filtered out)";
  out << ", ";
  out << s.num_func_added() << " Added ";
  if (total <= 1)
    out << "function\n";
  else
    out << "functions\n";

  total = s.num_vars_removed() + s.num_vars_added() +
    s.net_num_vars_changed();

  // variables changes summary
  out << indent << "Variables changes summary: ";
  out << s.num_vars_removed() << " Removed, ";
  out << s.num_vars_changed() - s.num_vars_filtered_out() << " Changed";
  if (s.num_vars_filtered_out())
    out << " (" << s.num_vars_filtered_out() << " filtered out)";
  out << ", ";
  out << s.num_vars_added() << " Added ";
  if (total <= 1)
    out << "variable\n";
  else
    out << "variables\n";

  if (ctxt_->show_symbols_unreferenced_by_debug_info()
      && (s.num_func_syms_removed()
	  || s.num_func_syms_added()
	  || s.num_var_syms_removed()
	  || s.num_var_syms_added()))
    {
      // function symbols changes summary.

      if (!ctxt_->show_added_symbols_unreferenced_by_debug_info()
	  && s.num_func_syms_removed() == 0
	  && s.num_func_syms_added() != 0)
	// If the only unreferenced function symbol change is function
	// syms that got added, but we were forbidden to show function
	// syms being added, do nothing.
	;
      else
	{
	  out << indent
	      << "Function symbols changes summary: "
	      << s.num_func_syms_removed() << " Removed, "
	      << s.num_func_syms_added() << " Added function symbol";
	  if (s.num_func_syms_added() + s.num_func_syms_removed() > 1)
	    out << "s";
	  out << " not referenced by debug info\n";
	}

      // variable symbol changes summary.

      if (!ctxt_->show_added_symbols_unreferenced_by_debug_info()
	  && s.num_var_syms_removed() == 0
	  && s.num_var_syms_added() != 0)
	// If the only unreferenced variable symbol change is variable
	// syms that got added, but we were forbidden to show variable
	// syms being added, do nothing.
	;
      else
	{
	  out << indent
	      << "Variable symbols changes summary: "
	      << s.num_var_syms_removed() << " Removed, "
	      << s.num_var_syms_added() << " Added function symbol";
	  if (s.num_var_syms_added() + s.num_var_syms_removed() > 1)
	    out << "s";
	  out << " not referenced by debug info\n";
	}
    }
}

/// Walk the changed functions and variables diff nodes to categorize
/// redundant nodes.
void
corpus_diff::priv::categorize_redundant_changed_sub_nodes()
{
  diff_sptr diff;

  ctxt_->forget_visited_diffs();
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_fns_.begin();
       i!= changed_fns_.end();
       ++i)
    {
      diff = *i;
      categorize_redundancy(diff);
    }

  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i!= sorted_changed_vars_.end();
       ++i)
    {
      diff_sptr diff = *i;
      categorize_redundancy(diff);
    }
}

/// Walk the changed functions and variables diff nodes and clear the
/// redundancy categorization they might carry.
void
corpus_diff::priv::clear_redundancy_categorization()
{
  diff_sptr diff;
  for (function_decl_diff_sptrs_type::const_iterator i = changed_fns_.begin();
       i!= changed_fns_.end();
       ++i)
    {
      diff = *i;
      abigail::comparison::clear_redundancy_categorization(diff);
    }

  for (var_diff_sptrs_type::const_iterator i = sorted_changed_vars_.begin();
       i!= sorted_changed_vars_.end();
       ++i)
    {
      diff = *i;
      abigail::comparison::clear_redundancy_categorization(diff);
    }
}

/// If the user asked to dump the diff tree node (for changed
/// variables and functions) on the error output stream, then just do
/// that.
///
/// This function is used for debugging purposes.
void
corpus_diff::priv::maybe_dump_diff_tree()
{
  if (!ctxt_->dump_diff_tree()
      || ctxt_->error_output_stream() == 0)
    return;

  if (!changed_fns_.empty())
    {
      *ctxt_->error_output_stream() << "changed functions diff tree: \n\n";
      for (function_decl_diff_sptrs_type::const_iterator i =
	     changed_fns_.begin();
	   i != changed_fns_.end();
	   ++i)
	{
	  diff_sptr d = *i;
	  print_diff_tree(d, *ctxt_->error_output_stream());
	}
    }

  if (!sorted_changed_vars_.empty())
    {
      *ctxt_->error_output_stream() << "\nchanged variables diff tree: \n\n";
      for (var_diff_sptrs_type::const_iterator i =
	     sorted_changed_vars_.begin();
	   i != sorted_changed_vars_.end();
	   ++i)
	{
	  diff_sptr d = *i;
	  print_diff_tree(d, *ctxt_->error_output_stream());
	}
    }
}

/// Populate the vector of children node of the @ref corpus_diff type.
///
/// The children node can then later be retrieved using
/// corpus_diff::children_node().
void
corpus_diff::chain_into_hierarchy()
{
  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_functions_sorted().begin();
       i != changed_functions_sorted().end();
       ++i)
    if (diff_sptr d = *i)
      append_child_node(d);
}

/// Constructor for @ref corpus_diff.
///
/// @param first the first corpus of the diff.
///
/// @param second the second corpus of the diff.
///
/// @param ctxt the diff context to use.
corpus_diff::corpus_diff(corpus_sptr first,
			 corpus_sptr second,
			 diff_context_sptr ctxt)
  : priv_(new priv)
{
  priv_->first_ = first;
  priv_->second_ = second;
  priv_->ctxt_ = ctxt;
}

/// Finish building the current instance of @ref corpus_diff.
void
corpus_diff::finish_diff_type()
{
  if (priv_->finished_)
    return;
  chain_into_hierarchy();
  priv_->finished_ = true;
}

/// @return the first corpus of the diff.
corpus_sptr
corpus_diff::first_corpus() const
{return priv_->first_;}

/// @return the second corpus of the diff.
corpus_sptr
corpus_diff::second_corpus() const
{return priv_->second_;}

/// @return the children nodes of the current instance of corpus_diff.
const vector<diff_sptr>&
corpus_diff::children_nodes() const
{return priv_->children_;}

/// Append a new child node to the vector of childre nodes for the
/// current instance of @ref corpus_diff node.
///
/// @param d the new child node.  Note that the life time of the
/// object held by @p d will thus equal the life time of the current
/// instance of @ref corpus_diff.
void
corpus_diff::append_child_node(diff_sptr d)
{
  assert(d);
  priv_->children_.push_back(d);

  diff_less_than_functor comp;
  std::sort(priv_->children_.begin(),
	    priv_->children_.end(),
	    comp);
}

/// @return the bare edit script of the functions changed as recorded
/// by the diff.
edit_script&
corpus_diff::function_changes() const
{return priv_->fns_edit_script_;}

/// @return the bare edit script of the variables changed as recorded
/// by the diff.
edit_script&
corpus_diff::variable_changes() const
{return priv_->vars_edit_script_;}

/// Test if the soname of the underlying corpus has changed.
///
/// @return true iff the soname has changed.
bool
corpus_diff::soname_changed() const
{return !priv_->sonames_equal_;}

/// Test if the architecture of the underlying corpus has changed.
///
/// @return true iff the architecture has changed.
bool
corpus_diff::architecture_changed() const
{return !priv_->architectures_equal_;}

/// Getter for the deleted functions of the diff.
///
/// @return the the deleted functions of the diff.
const string_function_ptr_map&
corpus_diff::deleted_functions() const
{return priv_->deleted_fns_;}

/// Getter for the added functions of the diff.
///
/// @return the added functions of the diff.
const string_function_ptr_map&
corpus_diff::added_functions()
{return priv_->added_fns_;}

/// Getter for the functions which signature didn't change, but which
/// do have some indirect changes in their parms.
///
/// @return a non-sorted map of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
/// The key of the map is a unique identifier for the function; it's
/// usually made of the name and version of the underlying ELF symbol
/// of the function for corpora that were built from ELF files.
const string_function_decl_diff_sptr_map&
corpus_diff::changed_functions()
{return priv_->changed_fns_map_;}

/// Getter for a sorted vector of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
///
/// @return a sorted vector of functions which signature didn't
/// change, but which do have some indirect changes in their parms.
const function_decl_diff_sptrs_type&
corpus_diff::changed_functions_sorted()
{return priv_->changed_fns_;}

/// Getter for the variables that got deleted from the first subject
/// of the diff.
///
/// @return the map of deleted variable.
const string_var_ptr_map&
corpus_diff::deleted_variables() const
{return priv_->deleted_vars_;}

/// Getter for the added variables of the diff.
///
/// @return the map of added variable.
const string_var_ptr_map&
corpus_diff::added_variables() const
{return priv_->added_vars_;}

/// Getter for the non-sorted map of variables which signature didn't
/// change but which do have some indirect changes in some sub-types.
///
/// @return the non-sorted map of changed variables.
const string_var_diff_sptr_map&
corpus_diff::changed_variables()
{return priv_->changed_vars_map_;}

/// Getter for the sorted vector of variables which signature didn't
/// change but which do have some indirect changes in some sub-types.
///
/// @return a sorted vector of changed variables.
const var_diff_sptrs_type&
corpus_diff::changed_variables_sorted()
{return priv_->sorted_changed_vars_;}

/// Getter for function symbols not referenced by any debug info and
/// that got deleted.
///
/// @return a map of elf function symbols not referenced by any debug
/// info and that got deleted.
const string_elf_symbol_map&
corpus_diff::deleted_unrefed_function_symbols() const
{return priv_->deleted_unrefed_fn_syms_;}

/// Getter for function symbols not referenced by any debug info and
/// that got added.
///
/// @return a map of elf function symbols not referenced by any debug
/// info and that got added.
const string_elf_symbol_map&
corpus_diff::added_unrefed_function_symbols() const
{return priv_->added_unrefed_fn_syms_;}

/// Getter for variable symbols not referenced by any debug info and
/// that got deleted.
///
/// @return a map of elf variable symbols not referenced by any debug
/// info and that got deleted.
const string_elf_symbol_map&
corpus_diff::deleted_unrefed_variable_symbols() const
{return priv_->deleted_unrefed_var_syms_;}

/// Getter for variable symbols not referenced by any debug info and
/// that got added.
///
/// @return a map of elf variable symbols not referenced by any debug
/// info and that got added.
const string_elf_symbol_map&
corpus_diff::added_unrefed_variable_symbols() const
{return priv_->added_unrefed_var_syms_;}

/// Getter of the diff context of this diff
///
/// @return the diff context for this diff.
const diff_context_sptr
corpus_diff::context() const
{return priv_->ctxt_;}

/// @return the pretty representation for the current instance of @ref
/// corpus_diff
const string&
corpus_diff::get_pretty_representation() const
{
  if (priv_->pretty_representation_.empty())
    {
      std::ostringstream o;
      o << "corpus_diff["
	<< first_corpus()->get_path()
	<< ", "
	<< second_corpus()->get_path()
	<< "]";
      priv_->pretty_representation_ = o.str();
    }
  return priv_->pretty_representation_;
}
/// Return true iff the current diff node carries a change.
///
/// @return true iff the current diff node carries a change.
bool
corpus_diff::has_changes() const
{
  return (soname_changed()
	  || architecture_changed()
	  || priv_->deleted_fns_.size()
	  || priv_->added_fns_.size()
	  || priv_->changed_fns_map_.size()
	  || priv_->deleted_vars_.size()
	  || priv_->added_vars_.size()
	  || priv_->changed_vars_map_.size()
	  || priv_->added_unrefed_fn_syms_.size()
	  || priv_->deleted_unrefed_fn_syms_.size()
	  || priv_->added_unrefed_var_syms_.size()
	  || priv_->deleted_unrefed_var_syms_.size());
}

/// "Less than" functor to compare instances of @ref function_decl.
struct function_comp
{
  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl& f, const function_decl& s)
  {
    string fr = f.get_pretty_representation_of_declarator(),
      sr = s.get_pretty_representation_of_declarator();

    if (fr != sr)
      return fr < sr;

    fr = f.get_pretty_representation(),
      sr = f.get_pretty_representation();

    if (fr != sr)
      return fr < sr;

    if (f.get_symbol())
      fr = f.get_symbol()->get_id_string();
    else if (!f.get_linkage_name().empty())
      fr = f.get_linkage_name();

    if (s.get_symbol())
      fr = s.get_symbol()->get_id_string();
    else if (!s.get_linkage_name().empty())
      fr = s.get_linkage_name();

    return fr < sr;
  }

  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl* f, const function_decl* s)
  {return operator()(*f, *s);}

  /// The actual "less than" operator for instances of @ref
  /// function_decl.  It returns true if the first @ref function_decl
  /// is lest than the second one.
  ///
  /// @param f the first @ref function_decl to take in account.
  ///
  /// @param s the second @ref function_decl to take in account.
  ///
  /// @return true iff @p f is less than @p s.
  bool
  operator()(const function_decl_sptr f, const function_decl_sptr s)
  {return operator()(f.get(), s.get());}
}; // end function_comp

/// Sort a an instance of @ref string_function_ptr_map map and stuff
/// a resulting sorted vector of pointers to function_decl.
///
/// @param map the map to sort.
///
/// @param sorted the resulting sorted vector.
static void
sort_string_function_ptr_map(const string_function_ptr_map& map,
			     vector<function_decl*>& sorted)
{
  sorted.reserve(map.size());
  for (string_function_ptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  function_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// A "Less Than" functor to compare instance of @ref
/// function_decl_diff.
struct function_decl_diff_comp
{
  /// The actual less than operator.
  ///
  /// It returns true if the first @ref function_decl_diff is less
  /// than the second one.
  ///
  /// param first the first @ref function_decl_diff to consider.
  ///
  /// @param second the second @ref function_decl_diff to consider.
  ///
  /// @return true iff @p first is less than @p second.
  bool
  operator()(const function_decl_diff& first,
	     const function_decl_diff& second)
  {
    function_decl_sptr f = first.first_function_decl(),
      s = second.first_function_decl();

    string fr = f->get_qualified_name(),
      sr = s->get_qualified_name();

    if (fr == sr)
      {
	if (f->get_symbol())
	  fr = f->get_symbol()->get_id_string();
	else if (!f->get_linkage_name().empty())
	  fr = f->get_linkage_name();
	else
	  fr = f->get_pretty_representation();

	if (s->get_symbol())
	  sr = s->get_symbol()->get_id_string();
	else if (!s->get_linkage_name().empty())
	  sr = s->get_linkage_name();
	else
	  sr = s->get_pretty_representation();
      }

    return (fr.compare(sr) < 0);
  }

  /// The actual less than operator.
  ///
  /// It returns true if the first @ref function_decl_diff_sptr is
  /// less than the second one.
  ///
  /// param first the first @ref function_decl_diff_sptr to consider.
  ///
  /// @param second the second @ref function_decl_diff_sptr to
  /// consider.
  ///
  /// @return true iff @p first is less than @p second.
  bool
  operator()(const function_decl_diff_sptr first,
	     const function_decl_diff_sptr second)
  {return operator()(*first, *second);}
}; // end struct function_decl_diff_comp

/// Sort the values of a @ref string_function_decl_diff_sptr_map map
/// and store the result in a vector of @ref function_decl_diff_sptr
/// objects.
///
/// @param map the map whose values to store.
///
/// @param sorted the vector of function_decl_diff_sptr to store the
/// result of the sort into.
static void
sort_string_function_decl_diff_sptr_map
(const string_function_decl_diff_sptr_map& map,
 function_decl_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_function_decl_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);
  function_decl_diff_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// Functor to sort instances of @ref var_diff_sptr
struct var_diff_sptr_comp
{
  /// Return true if the first argument is less than the second one.
  ///
  /// @param f the first argument to consider.
  ///
  /// @param s the second argument to consider.
  ///
  /// @return true if @p f is less than @p s.
  bool
  operator()(const var_diff_sptr f,
	     const var_diff_sptr s)
  {
    return (f->first_var()->get_qualified_name()
	    < s->first_var()->get_qualified_name());
  }
}; // end struct var_diff_sptr_comp

/// Sort of an instance of @ref string_var_diff_sptr_map map.
///
/// @param map the input map to sort.
///
/// @param sorted the ouptut sorted vector of @ref var_diff_sptr.
/// It's populated with the sorted content.
static void
sort_string_var_diff_sptr_map(const string_var_diff_sptr_map& map,
			      var_diff_sptrs_type& sorted)
{
  sorted.reserve(map.size());
  for (string_var_diff_sptr_map::const_iterator i = map.begin();
       i != map.end();
       ++i)
    sorted.push_back(i->second);

  var_diff_sptr_comp comp;
  std::sort(sorted.begin(), sorted.end(), comp);
}

/// For a given symbol, emit a string made of its name and version.
/// The string also contains the list of symbols that alias this one.
///
/// @param out the output string to emit the resulting string to.
///
/// @param indent the indentation string to use before emitting the
/// resulting string.
///
/// @param symbol the symbol to emit the representation string for.
///
/// @param sym_map the symbol map to consider to look for aliases of
/// @p symbol.
static void
show_linkage_name_and_aliases(ostream& out,
			      const string& indent,
			      const elf_symbol& symbol,
			      const string_elf_symbols_map_type& sym_map)
{
  out << indent << symbol.get_id_string();
  string aliases =
    symbol.get_aliases_id_string(sym_map,
				 /*include_symbol_itself=*/false);
  if (!aliases.empty())
    out << ", aliases " << aliases;
}

/// Apply the different filters that are registered to be applied to
/// the diff tree; that includes the categorization filters.  Also,
/// apply the suppression interpretation filters.
///
/// After the filters are applied, this function calculates some
/// statistics about the changes carried by the current instance of
/// @ref corpus_diff.  These statistics are represented by an instance
/// of @ref corpus_diff::diff_stats.
///
/// This member function is called by the reporting function
/// corpus_diff::report().
///
/// Note that for a given instance of corpus_diff, this function
/// applies the filters and suppressions only the first time it is
/// invoked.  Subsequent invocations just return the instance of
/// corpus_diff::diff_stats that was cached after the first
/// invocation.
///
/// @return a reference to the statistics about the changes carried by
/// the current instance of @ref corpus_diff.
const corpus_diff::diff_stats&
corpus_diff::apply_filters_and_suppressions_before_reporting()
{
  if (priv_->filters_and_suppr_applied_)
    return priv_->diff_stats_;

  apply_suppressions(this);
  priv_->apply_filters_and_compute_diff_stats(priv_->diff_stats_);

  priv_->filters_and_suppr_applied_ = true;

  return priv_->diff_stats_;
}

/// Report the diff in a serialized form.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
corpus_diff::report(ostream& out, const string& indent) const
{
  size_t total = 0, removed = 0, added = 0;
  const diff_stats &s =
    const_cast<corpus_diff*>(this)->apply_filters_and_suppressions_before_reporting();

  /// Report removed/added/changed functions.
  total = s.num_func_removed() + s.num_func_added() +
    s.num_func_changed() - s.num_func_filtered_out();
  const unsigned large_num = 100;

  priv_->emit_diff_stats(s, out, indent);
  if (context()->show_stats_only())
    return;
  out << "\n";

  if (context()->show_soname_change()
      && !priv_->sonames_equal_)
    out << indent << "SONAME changed from '"
	<< first_corpus()->get_soname() << "' to '"
	<< second_corpus()->get_soname() << "'\n\n";

  if (context()->show_architecture_change()
      && !priv_->architectures_equal_)
    out << indent << "architecture changed from '"
	<< first_corpus()->get_architecture_name() << "' to '"
	<< second_corpus()->get_architecture_name() << "'\n\n";

  if (context()->show_deleted_fns())
    {
      if (s.num_func_removed() == 1)
	out << indent << "1 Removed function:\n\n";
      else if (s.num_func_removed() > 1)
	out << indent << s.num_func_removed() << " Removed functions:\n\n";

      vector<function_decl*>sorted_deleted_fns;
      sort_string_function_ptr_map(priv_->deleted_fns_, sorted_deleted_fns);
      for (vector<function_decl*>::const_iterator i =
	     sorted_deleted_fns.begin();
	   i != sorted_deleted_fns.end();
	   ++i)
	{
	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[D] ";
	  out << "'" << (*i)->get_pretty_representation() << "'";
	  if (context()->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    first_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++removed;
	}
      if (removed)
	out << "\n";
    }

  if (context()->show_added_fns())
    {
      if (s.num_func_added() == 1)
	out << indent << "1 Added function:\n";
      else if (s.num_func_added() > 1)
	out << indent << s.num_func_added()
	    << " Added functions:\n\n";
      vector<function_decl*> sorted_added_fns;
      sort_string_function_ptr_map(priv_->added_fns_, sorted_added_fns);
      for (vector<function_decl*>::const_iterator i = sorted_added_fns.begin();
	   i != sorted_added_fns.end();
	   ++i)
	{
	  out
	    << indent
	    << "  ";
	  if (total > large_num)
	    out << "[A] ";
	  out << "'"
	      << (*i)->get_pretty_representation()
	      << "'";
	  if (context()->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases
		(out, "", *(*i)->get_symbol(),
		 second_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++added;
	}
      if (added)
	{
	  out << "\n";
	  added = false;
	}
    }

  if (context()->show_changed_fns())
    {
      size_t num_changed = s.num_func_changed() - s.num_func_filtered_out();
      if (num_changed == 1)
	out << indent << "1 function with some indirect sub-type change:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " functions with some indirect sub-type change:\n\n";

      bool emitted = false;
      vector<function_decl_diff_sptr> sorted_changed_fns;
      sort_string_function_decl_diff_sptr_map(priv_->changed_fns_map_,
					      sorted_changed_fns);
      for (vector<function_decl_diff_sptr>::const_iterator i =
	     sorted_changed_fns.begin();
	   i != sorted_changed_fns.end();
	   ++i)
	{
	  diff_sptr diff = *i;
	  if (!diff)
	    continue;

	  if (diff->to_be_reported())
	    {
	      out << indent << "  [C]'"
		  << (*i)->first_function_decl()->get_pretty_representation()
		  << "' has some indirect sub-type changes:\n";
	      diff->report(out, indent + "    ");
	      emitted |= true;
	    }
	  }
      if (emitted)
	{
	  out << "\n";
	  emitted = false;
	}
    }

 // Report added/removed/changed variables.
  total = s.num_vars_removed() + s.num_vars_added() +
    s.num_vars_changed() - s.num_vars_filtered_out();

  if (context()->show_deleted_vars())
    {
      if (s.num_vars_removed() == 1)
	out << indent << "1 Deleted variable:\n";
      else if (s.num_vars_removed() > 1)
	out << indent << s.num_vars_removed()
	    << " Deleted variables:\n\n";
      string n;
      for (string_var_ptr_map::const_iterator i =
	     priv_->deleted_vars_.begin();
	   i != priv_->deleted_vars_.end();
	   ++i)
	{
	  n = i->second->get_pretty_representation();
	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[D] ";
	  out << "'"
	      << n
	      << "'";
	  if (context()->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *i->second->get_symbol(),
					    first_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++removed;
	}
      if (removed)
	{
	  out << "\n";
	  removed = 0;
	}
    }

  if (context()->show_added_vars())
    {
      if (s.num_vars_added() == 1)
	out << indent << "1 Added variable:\n";
      else if (s.num_vars_added() > 1)
	out << indent << s.num_vars_added()
	    << " Added variables:\n";
      string n;
      for (string_var_ptr_map::const_iterator i =
	     priv_->added_vars_.begin();
	   i != priv_->added_vars_.end();
	   ++i)
	{
	  n = i->second->get_pretty_representation();
	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[A] ";
	  out << "'" << n << "'";
	  if (context()->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *i->second->get_symbol(),
					    second_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++added;
	}
      if (added)
	{
	  out << "\n";
	  added = 0;
	}
    }

  if (context()->show_changed_vars())
    {
      size_t num_changed = s.num_vars_changed() - s.num_vars_filtered_out();
      if (num_changed == 1)
	out << indent << "1 Changed variable:\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " Changed variables:\n\n";
      string n1, n2;

      for (var_diff_sptrs_type::const_iterator i =
	     priv_->sorted_changed_vars_.begin();
	   i != priv_->sorted_changed_vars_.end();
	   ++i)
	{
	  diff_sptr diff = *i;

	  if (!diff)
	    continue;

	  if (!diff->to_be_reported())
	    continue;

	  n1 = diff->first_subject()->get_pretty_representation();
	  n2 = diff->second_subject()->get_pretty_representation();

	  out << indent << "  '" << n1 << "' was changed";
	  if (n1 != n2)
	    out << " to '" << n2 << "'";
	  out << ":\n";
	  diff->report(out, indent + "    ");
	}
      if (num_changed)
	out << "\n";
    }

  // Report removed function symbols not referenced by any debug info.
  if (context()->show_symbols_unreferenced_by_debug_info()
      && priv_->deleted_unrefed_fn_syms_.size())
    {
      if (s.num_func_syms_removed() == 1)
	out << indent
	    << "1 Removed function symbol not referenced by debug info:\n\n";
      else if (s.num_func_syms_removed() > 0)
	out << indent
	    << s.num_func_syms_removed()
	    << " Removed function symbols not referenced by debug info:\n\n";

      for (string_elf_symbol_map::const_iterator i =
	     priv_->deleted_unrefed_fn_syms_.begin();
	   i != priv_->deleted_unrefed_fn_syms_.end();
	   ++i)
	{
	  out << indent << "  ";
	  if (s.num_func_syms_removed() > large_num)
	    out << "[D] ";

	  show_linkage_name_and_aliases(out, "", *i->second,
					first_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (priv_->deleted_unrefed_fn_syms_.size())
	out << '\n';
    }

  // Report added function symbols not referenced by any debug info.
  if (context()->show_symbols_unreferenced_by_debug_info()
      && priv_->added_unrefed_fn_syms_.size())
    {
      if (s.num_func_syms_added() == 1)
	out << indent
	    << "1 Added function symbol not referenced by debug info:\n\n";
      else if (s.num_func_syms_added() > 0)
	out << indent
	    << s.num_func_syms_added()
	    << " Added function symbols not referenced by debug info:\n\n";

      for (string_elf_symbol_map::const_iterator i =
	     priv_->added_unrefed_fn_syms_.begin();
	   i != priv_->added_unrefed_fn_syms_.end();
	   ++i)
	{
	  out << indent << "  ";
	  if (s.num_func_syms_added() > large_num)
	    out << "[A] ";
	  show_linkage_name_and_aliases(out, "",
					*i->second,
					second_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (priv_->added_unrefed_fn_syms_.size())
	out << '\n';
    }

  // Report removed variable symbols not referenced by any debug info.
  if (context()->show_symbols_unreferenced_by_debug_info()
      && priv_->deleted_unrefed_var_syms_.size())
    {
      if (s.num_var_syms_removed() == 1)
	out << indent
	    << "1 Removed variable symbol not referenced by debug info:\n\n";
      else if (s.num_var_syms_removed() > 0)
	out << indent
	    << s.num_var_syms_removed()
	    << " Removed variable symbols not referenced by debug info:\n\n";

      for (string_elf_symbol_map::const_iterator i =
	     priv_->deleted_unrefed_var_syms_.begin();
	   i != priv_->deleted_unrefed_var_syms_.end();
	   ++i)
	{
	  out << indent << "  ";
	  if (s.num_var_syms_removed() > large_num)
	    out << "[D] ";

	  show_linkage_name_and_aliases(out, "", *i->second,
					first_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (priv_->deleted_unrefed_var_syms_.size())
	out << '\n';
    }

  // Report added variable symbols not referenced by any debug info.
  if (context()->show_symbols_unreferenced_by_debug_info()
      && priv_->added_unrefed_var_syms_.size())
    {
      if (s.num_var_syms_added() == 1)
	out << indent
	    << "1 Added variable symbol not referenced by debug info:\n\n";
      else if (s.num_var_syms_added() > 0)
	out << indent
	    << s.num_var_syms_added()
	    << " Added variable symbols not referenced by debug info:\n\n";

      for (string_elf_symbol_map::const_iterator i =
	     priv_->added_unrefed_var_syms_.begin();
	   i != priv_->added_unrefed_var_syms_.end();
	   ++i)
	{
	  out << indent << "  ";
	  if (s.num_var_syms_added() > large_num)
	    out << "[A] ";
	  show_linkage_name_and_aliases(out, "",
					*i->second,
					second_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (priv_->added_unrefed_var_syms_.size())
	out << '\n';
    }

  priv_->maybe_dump_diff_tree();
}

/// Traverse the diff sub-tree under the current instance corpus_diff.
///
/// @param v the visitor to invoke on each diff node of the sub-tree.
///
/// @return true if the traversing has to keep going on, false otherwise.
bool
corpus_diff::traverse(diff_node_visitor& v)
{
  finish_diff_type();

  v.visit_begin(this);

  if (!v.visit(this, true))
    {
      v.visit_end(this);
      return false;
    }

  for (function_decl_diff_sptrs_type::const_iterator i =
	 changed_functions_sorted().begin();
       i != changed_functions_sorted().end();
       ++i)
    {
      if (diff_sptr d = *i)
	{
	  if (!d->traverse(v))
	    {
	      v.visit_end(this);
	      return false;
	    }
	}
    }

  for (var_diff_sptrs_type::const_iterator i =
	 changed_variables_sorted().begin();
       i != changed_variables_sorted().end();
       ++i)
    {
      if (diff_sptr d = *i)
	{
	  if (!d->traverse(v))
	    {
	      v.visit_end(this);
	      return false;
	    }
	}
    }

  v.visit_end(this);
  return true;
}

/// Compute the diff between two instances fo the @ref corpus
///
/// @param f the first @ref corpus to consider for the diff.
///
/// @param s the second @ref corpus to consider for the diff.
///
/// @param ctxt the diff context to use.
///
/// @return the resulting diff between the two @ref corpus.
corpus_diff_sptr
compute_diff(const corpus_sptr	f,
	     const corpus_sptr	s,
	     diff_context_sptr	ctxt)
{
  typedef corpus::functions::const_iterator fns_it_type;
  typedef corpus::variables::const_iterator vars_it_type;
  typedef elf_symbols::const_iterator symbols_it_type;
  typedef diff_utils::deep_ptr_eq_functor eq_type;

  if (!ctxt)
    ctxt.reset(new diff_context);

  ctxt->set_corpora(f, s);

  corpus_diff_sptr r(new corpus_diff(f, s, ctxt));

  r->priv_->sonames_equal_ = f->get_soname() == s->get_soname();

  r->priv_->architectures_equal_ =
    f->get_architecture_name() == s->get_architecture_name();

  diff_utils::compute_diff<fns_it_type, eq_type>(f->get_functions().begin(),
						 f->get_functions().end(),
						 s->get_functions().begin(),
						 s->get_functions().end(),
						 r->priv_->fns_edit_script_);

  diff_utils::compute_diff<vars_it_type, eq_type>
    (f->get_variables().begin(), f->get_variables().end(),
     s->get_variables().begin(), s->get_variables().end(),
     r->priv_->vars_edit_script_);

  diff_utils::compute_diff<symbols_it_type, eq_type>
    (f->get_unreferenced_function_symbols().begin(),
     f->get_unreferenced_function_symbols().end(),
     s->get_unreferenced_function_symbols().begin(),
     s->get_unreferenced_function_symbols().end(),
     r->priv_->unrefed_fn_syms_edit_script_);

    diff_utils::compute_diff<symbols_it_type, eq_type>
    (f->get_unreferenced_variable_symbols().begin(),
     f->get_unreferenced_variable_symbols().end(),
     s->get_unreferenced_variable_symbols().begin(),
     s->get_unreferenced_variable_symbols().end(),
     r->priv_->unrefed_var_syms_edit_script_);

  r->priv_->ensure_lookup_tables_populated();

  return r;
}

// </corpus stuff>

// <diff_node_visitor stuff>

/// This is called by the traversing code on a @ref diff node just
/// before visiting it.  That is, before visiting it and its children
/// node.
///
/// @param d the diff node to visit.
void
diff_node_visitor::visit_begin(diff* /*p*/)
{}

/// This is called by the traversing code on a @ref diff node just
/// after visiting it.  That is after visiting it and its children
/// nodes.
///
/// @param d the diff node that got visited.
void
diff_node_visitor::visit_end(diff* /*p*/)
{}

/// This is called by the traversing code on a @ref corpus_diff node
/// just before visiting it.  That is, before visiting it and its
/// children node.
///
/// @param p the corpus_diff node to visit.
///
void
diff_node_visitor::visit_begin(corpus_diff* /*p*/)
{}

/// This is called by the traversing code on a @ref corpus_diff node
/// just after visiting it.  That is after visiting it and its children
/// nodes.
///
/// @param d the diff node that got visited.
void
diff_node_visitor::visit_end(corpus_diff* /*d*/)
{}

/// Default visitor implementation
///
/// @return true
bool
diff_node_visitor::visit(diff*, bool)
{return true;}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(distinct_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(var_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(pointer_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(reference_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(qualified_type_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(enum_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(class_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(base_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(scope_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(function_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(type_decl_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(typedef_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(translation_unit_diff* dif, bool pre)
{
  diff* d = dif;
  visit(d, pre);

  return true;
}

/// Default visitor implementation.
///
/// @return true
bool
diff_node_visitor::visit(corpus_diff*, bool)
{return true;}

// </diff_node_visitor stuff>

// <redundant diff node marking>

// </redundant diff node marking>

// <diff tree category propagation>

/// A visitor to propagate the category of a node up to its parent
/// nodes.  This visitor doesn't touch the REDUNDANT_CATEGORY because
/// that one is propagated using another specific visitor.
struct category_propagation_visitor : public diff_node_visitor
{
  virtual void
  visit_end(diff* d)
  {
    // Has this diff node 'd' been already visited ?
    bool already_visited = d->context()->diff_has_been_visited(d);

    // The canonical diff node of the class of equivalence of the diff
    // node 'd'.
    diff* canonical = d->get_canonical_diff();

    // If this class of equivalence of diff node is being visited for
    // the first time, then update its canonical node's category too.
    bool update_canonical = !already_visited && canonical;

    for (vector<diff_sptr>::const_iterator i = d->children_nodes().begin();
	 i != d->children_nodes().end();
	 ++i)
      {
	// If we are visiting the class of equivalence of 'd' for the
	// first time, then let's look at the children of 'd' and
	// propagate their categories to 'd'.
	//
	// If the class of equivalence of 'd' has already been
	// visited, then let's look at the canonical diff nodes of the
	// children of 'd' and propagate their categories to 'd'.
	diff* diff = already_visited
	  ? (*i)->get_canonical_diff()
	  : (*i).get();

	assert(diff);

	diff_category c = diff->get_category();
	c &= ~(REDUNDANT_CATEGORY|SUPPRESSED_CATEGORY);
	d->add_to_category(c);
	if (!already_visited && canonical)
	  if (update_canonical)
	    canonical->add_to_category(c);
      }
  }
};// end struct category_propagation_visitor

/// Visit all the nodes of a given sub-tree.  For each node that has a
/// particular category set, propagate that category set up to its
/// parent nodes.
///
/// @param diff_tree the diff sub-tree to walk for categorization
/// purpose;
void
propagate_categories(diff* diff_tree)
{
  category_propagation_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(true);
  diff_tree->context()->forget_visited_diffs();
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Visit all the nodes of a given sub-tree.  For each node that has a
/// particular category set, propagate that category set up to its
/// parent nodes.
///
/// @param diff_tree the diff sub-tree to walk for categorization
/// purpose;
void
propagate_categories(diff_sptr diff_tree)
{propagate_categories(diff_tree.get());}

/// Visit all the nodes of a given corpus tree.  For each node that
/// has a particular category set, propagate that category set up to
/// its parent nodes.
///
/// @param diff_tree the corpus_diff tree to walk for categorization
/// purpose;
void
propagate_categories(corpus_diff* diff_tree)
{
  category_propagation_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Visit all the nodes of a given corpus tree.  For each node that
/// has a particular category set, propagate that category set up to
/// its parent nodes.
///
/// @param diff_tree the corpus_diff tree to walk for categorization
/// purpose;
void
propagate_categories(corpus_diff_sptr diff_tree)
{propagate_categories(diff_tree.get());}

/// A tree node visitor that knows how to categorizes a given in the
/// SUPPRESSED_CATEGORY category and how to propagate that
/// categorization.
struct suppression_categorization_visitor : public diff_node_visitor
{

  /// Before visiting the children of the diff node, check if the node
  /// is suppressed by a suppression specification.  If it is, mark
  /// the node as belonging to the SUPPRESSED_CATEGORY category.
  ///
  /// @param p the diff node to visit.
  virtual void
  visit_begin(diff* d)
  {
    if (d->is_suppressed())
      d->add_to_category(SUPPRESSED_CATEGORY);
  }

  /// After visiting the children nodes of a given diff node,
  /// propagate the SUPPRESSED_CATEGORY from the children nodes to the
  /// diff node, if need be.
  ///
  /// That is, if all children nodes carry a suppressed change the
  /// current node should be marked as suppressed as well.
  ///
  /// In practice, this might be too strong of a condition.  If the
  /// current node carries a local change (i.e, a change not carried
  /// by any of its children node) and if that change is not
  /// suppressed, then the current node should *NOT* be suppressed.
  ///
  /// But right now, the IR doesn't let us know about local vs
  /// children-carried changes.  So we cannot be that precise yet.
  virtual void
  visit_end(diff* d)
  {
    bool has_non_suppressed_child = false;
    bool has_non_empty_child = false;
    bool has_suppressed_child = false;

    if (!(d->get_category() & SUPPRESSED_CATEGORY)
	&& !d->has_local_changes())
      {
	for (vector<diff_sptr>::const_iterator i = d->children_nodes().begin();
	     i != d->children_nodes().end();
	     ++i)
	  {
	    diff_sptr child = *i;
	    if (child->has_changes())
	      {
		has_non_empty_child = true;
		if (child->get_category() & SUPPRESSED_CATEGORY)
		  has_suppressed_child = true;
		else
		  has_non_suppressed_child = true;
	      }
	  }

	if (has_non_empty_child
	    && has_suppressed_child
	    && !has_non_suppressed_child)
	  d->add_to_category(SUPPRESSED_CATEGORY);
      }
  }
}; //end struct suppression_categorization_visitor

/// Walk a given diff-sub tree and appply the suppressions carried by
/// the context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff-sub tree to apply the suppressions to.
void
apply_suppressions(diff* diff_tree)
{
  if (diff_tree && !diff_tree->context()->suppressions().empty())
    {
      suppression_categorization_visitor v;
      bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
      diff_tree->context()->forbid_visiting_a_node_twice(false);
      diff_tree->traverse(v);
      diff_tree->context()->forbid_visiting_a_node_twice(s);
    }
}

/// Walk a given diff-sub tree and appply the suppressions carried by
/// the context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff-sub tree to apply the suppressions to.
void
apply_suppressions(diff_sptr diff_tree)
{apply_suppressions(diff_tree.get());}

/// Walk a diff tree and appply the suppressions carried by the
/// context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff tree to apply the suppressions to.
void
apply_suppressions(const corpus_diff*  diff_tree)
{
  if (diff_tree && !diff_tree->context()->suppressions().empty())
    {
      suppression_categorization_visitor v;
      bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
      diff_tree->context()->forbid_visiting_a_node_twice(false);
      const_cast<corpus_diff*>(diff_tree)->traverse(v);
      diff_tree->context()->forbid_visiting_a_node_twice(s);
    }
}

/// Walk a diff tree and appply the suppressions carried by the
/// context.  If the suppression applies to a given node than
/// categorize the node into the SUPPRESSED_CATEGORY category and
/// propagate that categorization.
///
/// @param diff_tree the diff tree to apply the suppressions to.
void
apply_suppressions(corpus_diff_sptr  diff_tree)
{apply_suppressions(diff_tree.get());}

// </diff tree category propagation>

// <diff tree printing stuff>

/// A visitor to print (to an output stream) a pretty representation
/// of a @ref diff sub-tree or of a complete @ref corpus_diff tree.
struct diff_node_printer : public diff_node_visitor
{
  ostream& out_;
  unsigned level_;

  /// Emit a certain number of spaces to the output stream associated
  /// to this diff_node_printer.
  ///
  /// @param level half of the numver of spaces to emit.
  void
  do_indent(unsigned level)
  {
    for (unsigned i = 0; i < level; ++i)
      out_ << "  ";
  }

  diff_node_printer(ostream& out)
    : diff_node_visitor(DO_NOT_MARK_VISITED_NODES_AS_VISITED),
      out_(out),
      level_(0)
  {}

  virtual void
  visit_begin(diff*)
  {
    ++level_;
  }

  virtual void
  visit_end(diff*)
  {
    --level_;
  }

  virtual void
  visit_begin(corpus_diff*)
  {
    ++level_;
  }

  virtual void
  visit_end(corpus_diff*)
  {
    --level_;
  }

  virtual bool
  visit(diff* d, bool pre)
  {
    if (!pre)
      // We are post-visiting the diff node D.  Which means, we have
      // printed a pretty representation for it already.  So do
      // nothing now.
      return true;

    do_indent(level_);
    out_ << d->get_pretty_representation();
    out_ << "\n";
    do_indent(level_);
    out_ << "{\n";
    do_indent(level_ + 1);
    out_ << "category: "<< d->get_category() << "\n";
    do_indent(level_ + 1);
    out_ << "@: " << std::hex << d << std::dec << "\n";
    do_indent(level_ + 1);
    out_ << "@-canonical: " << std::hex
	 << d->get_canonical_diff()
	 << std::dec << "\n";
    do_indent(level_);
    out_ << "}\n";

    return true;
  }

  virtual bool
  visit(corpus_diff* d, bool pre)
  {
    if (!pre)
      // We are post-visiting the diff node D.  Which means, we have
      // printed a pretty representation for it already.  So do
      // nothing now.
      return true;

    // indent
    for (unsigned i = 0; i < level_; ++i)
      out_ << ' ';
    out_ << d->get_pretty_representation();
    out_ << '\n';
    return true;
  }
}; // end struct diff_printer_visitor

// </ diff tree printing stuff>

/// Emit a textual representation of a @ref diff sub-tree to an
/// output stream.
///
/// @param diff_tree the sub-tree to emit the textual representation
/// for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(diff* diff_tree, ostream& out)
{
  diff_node_printer p(out);
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(p);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Emit a textual representation of a @ref corpus_diff tree to an
/// output stream.
///
/// @param diff_tree the @ref corpus_diff tree to emit the textual
/// representation for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(corpus_diff* diff_tree, std::ostream& out)
{
  diff_node_printer p(out);
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(p);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Emit a textual representation of a @ref diff sub-tree to an
/// output stream.
///
/// @param diff_tree the sub-tree to emit the textual representation
/// for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(diff_sptr diff_tree,
		std::ostream& o)
{print_diff_tree(diff_tree.get(), o);}

/// Emit a textual representation of a @ref corpus_diff tree to an
/// output stream.
///
/// @param diff_tree the @ref corpus_diff tree to emit the textual
/// representation for.
///
/// @param out the output stream to emit the textual representation
/// for @p diff_tree to.
void
print_diff_tree(corpus_diff_sptr diff_tree,
		std::ostream& o)
{print_diff_tree(diff_tree.get(), o);}

// <redundancy_marking_visitor>

/// A tree visitor to categorize nodes with respect to the
/// REDUNDANT_CATEGORY.  That is, detect if a node is redundant (is
/// present on several spots of the tree) and mark such nodes
/// appropriatly.  This visitor also takes care of propagating the
/// REDUNDANT_CATEGORY of a given node to its parent nodes as
/// appropriate.
struct redundancy_marking_visitor : public diff_node_visitor
{
  bool skip_children_nodes_;

  redundancy_marking_visitor()
    : skip_children_nodes_()
  {}

  virtual void
  visit_begin(diff* d)
  {
    if (d->to_be_reported())
      {
	// A diff node that carries a change and that has been already
	// traversed elsewhere is considered redundant.  So let's mark
	// it as such and let's not traverse it; that is, let's not
	// visit its children.
	if ((d->context()->diff_has_been_visited(d)
	     || d->get_canonical_diff()->is_traversing())
	    && d->has_changes())
	  {
	    // But if two diff nodes are redundant sibbling, do not
	    // mark them as being redundant.  This is to avoid marking
	    // nodes as redundant in this case:
	    //
	    //     int foo(int a, int b);
	    // compared with:
	    //     float foo(float a, float b); (in C).
	    //
	    // In this case, we want to report all the occurences of
	    // the int->float change because logically, they are at
	    // the same level in the diff tree.

	    bool redundant_with_sibling_node = false;
	    const diff* p = d->parent_node();

	    // If this is a child node of a fn_parm_diff, look through
	    // the fn_parm_diff node.
	    if (p && dynamic_cast<const fn_parm_diff*>(p))
	      p = p->parent_node();

	    if (p)
	      for (vector<diff_sptr>::const_iterator s =
		     p->children_nodes().begin();
		   s != p->children_nodes().end();
		   ++s)
		{
		  if (s->get() == d)
		    continue;
		  diff* sib = s->get();
		  // If this is a fn_parm_diff, look through the
		  // fn_parm_diff node to get at the real type node.
		  if (fn_parm_diff_sptr f =
		      dynamic_pointer_cast<fn_parm_diff>(*s))
		    sib = f->get_type_diff().get();
		  if (sib == d)
		    continue;
		  if (sib->get_canonical_diff() == d->get_canonical_diff())
		    {
		      redundant_with_sibling_node = true;
		      break;
		    }
		}
	    if (!redundant_with_sibling_node)
	      {
		d->add_to_category(REDUNDANT_CATEGORY);
		// As we said in preamble, as this node is marked as
		// being redundant, let's not visit its children.
		// This is not an optimization; it's needed for
		// correctness.  In the case of a diff node involving
		// a class type that refers to himself, visiting the
		// children nodes might cause them to be wrongly
		// marked as redundant.
		set_visiting_kind(get_visiting_kind()
				  | SKIP_CHILDREN_VISITING_KIND);
		skip_children_nodes_ = true;
	      }
	  }
      }
    else
      {
	// If the node is not be reported, do not look at it children.
	set_visiting_kind(get_visiting_kind() | SKIP_CHILDREN_VISITING_KIND);
	skip_children_nodes_ = true;
      }
  }

  virtual void
  visit_begin(corpus_diff*)
  {
  }

  virtual void
  visit_end(diff* d)
  {
    if (skip_children_nodes_)
      // When visiting this node, we decided to skip its children
      // node.  Now that we are done visiting the node, lets stop
      // avoiding the children nodes visiting for the other tree
      // nodes.
      {
	set_visiting_kind(get_visiting_kind() & (~SKIP_CHILDREN_VISITING_KIND));
	skip_children_nodes_ = false;
      }
    else
      {
	// Propagate the redundancy categorization of the children nodes
	// to this node.  But if this node has local changes, then it
	// doesn't inherit redundancy from its children nodes.
	if (!(d->get_category() & REDUNDANT_CATEGORY)
	    && !d->has_local_changes())
	  {
	    bool has_non_redundant_child = false;
	    bool has_non_empty_child = false;
	    for (vector<diff_sptr>::const_iterator i =
		   d->children_nodes().begin();
		 i != d->children_nodes().end();
		 ++i)
	      {
		if ((*i)->has_changes())
		  {
		    has_non_empty_child = true;
		    if (((*i)->get_category() & REDUNDANT_CATEGORY) == 0)
		      has_non_redundant_child = true;
		  }
		if (has_non_redundant_child)
		  break;
	      }

	    // A diff node for which at least a child node carries a
	    // change, and for which all the children are redundant is
	    // deemed redundant too, unless it has local changes.
	    if (has_non_empty_child
		&& !has_non_redundant_child)
	      d->add_to_category(REDUNDANT_CATEGORY);
	  }
      }
  }

  virtual void
  visit_end(corpus_diff*)
  {
  }

  virtual bool
  visit(diff*, bool)
  {return true;}

  virtual bool
  visit(corpus_diff*, bool)
  {
    return true;
  }
};// end struct redundancy_marking_visitor

/// A visitor of @ref diff nodes that clears the REDUNDANT_CATEGORY
/// category out of the nodes.
struct redundancy_clearing_visitor : public diff_node_visitor
{
  bool
  visit(corpus_diff*, bool)
  {return true;}

  bool
  visit(diff* d, bool)
  {
    // clear the REDUNDANT_CATEGORY out of the current node.
    diff_category c = d->get_category();
    c &= ~REDUNDANT_CATEGORY;
    d->set_category(c);
    return true;
  }
}; // end struct redundancy_clearing_visitor

/// Walk a given @ref diff sub-tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
categorize_redundancy(diff* diff_tree)
{
  if (diff_tree->context()->show_redundant_changes())
    return;
  redundancy_marking_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a given @ref diff sub-tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
categorize_redundancy(diff_sptr diff_tree)
{categorize_redundancy(diff_tree.get());}

/// Walk a given @ref corpus_diff tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
categorize_redundancy(corpus_diff* diff_tree)
{
  redundancy_marking_visitor v;
  diff_tree->context()->forget_visited_diffs();
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
}

/// Walk a given @ref corpus_diff tree to categorize each of the nodes
/// with respect to the REDUNDANT_CATEGORY.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
categorize_redundancy(corpus_diff_sptr diff_tree)
{categorize_redundancy(diff_tree.get());}

// </redundancy_marking_visitor>

/// Walk a given @ref diff sub-tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
clear_redundancy_categorization(diff* diff_tree)
{
  redundancy_clearing_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
  diff_tree->context()->forget_visited_diffs();
}

/// Walk a given @ref diff sub-tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref diff sub-tree to walk.
void
clear_redundancy_categorization(diff_sptr diff_tree)
{clear_redundancy_categorization(diff_tree.get());}

/// Walk a given @ref corpus_diff tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
clear_redundancy_categorization(corpus_diff* diff_tree)
{
  redundancy_clearing_visitor v;
  bool s = diff_tree->context()->visiting_a_node_twice_is_forbidden();
  diff_tree->context()->forbid_visiting_a_node_twice(false);
  diff_tree->traverse(v);
  diff_tree->context()->forbid_visiting_a_node_twice(s);
  diff_tree->context()->forget_visited_diffs();
}

/// Walk a given @ref corpus_diff tree to clear the REDUNDANT_CATEGORY
/// out of the category of the nodes.
///
/// @param diff_tree the @ref corpus_diff tree to walk.
void
clear_redundancy_categorization(corpus_diff_sptr diff_tree)
{clear_redundancy_categorization(diff_tree.get());}

/// Apply the @ref diff tree filters that have been associated to the
/// context of the a given @ref corpus_diff tree.  As a result, the
/// nodes of the @diff tree are going to be categorized into one of
/// several of the categories of @ref diff_category.
///
/// @param diff_tree the @ref corpus_diff instance which @ref diff are
/// to be categorized.
void
apply_filters(corpus_diff_sptr diff_tree)
{
  diff_tree->context()->maybe_apply_filters(diff_tree);
  propagate_categories(diff_tree);
}

}// end namespace comparison
} // end namespace abigail
