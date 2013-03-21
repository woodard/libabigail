// -*- mode: c++ -*-

#ifndef __ABG_CORPUS_H__
#define __ABG_CORPUS_H__

#include <tr1/memory>
#include "abg-ir.h"

using std::tr1::shared_ptr;

namespace abigail
{
/// This is the abstraction of the set of relevant artefacts (types,
/// variable declarations, functions, templates, etc) bundled together
/// to represent an Application Binary Interface.
class abi_corpus
{
  abi_corpus();

public:
  typedef std::list<shared_ptr<decl_base> > decls_type;

  abi_corpus(const std::string& name);

  void
  add(const shared_ptr<decl_base> declaration);

  const std::list<shared_ptr<decl_base> >&
  get_decls() const;

  location_manager&
  get_loc_mgr();

  const location_manager&
  get_loc_mgr() const;

  bool
  is_empty() const;

private:
  std::string m_name;
  location_manager m_loc_mgr;
  std::list<shared_ptr<decl_base> > m_members;
};

}//end namespace abigail
#endif //__ABG_CORPUS_H__
