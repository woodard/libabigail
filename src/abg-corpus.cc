// -*- mode: c++ -*-

#include "abg-corpus.h"

namespace abigail
{

abi_corpus::abi_corpus(const std::string& name)
  :m_name(name)
{
}

void
abi_corpus::add(const shared_ptr<decl_base> declaration)
{
  m_members.push_back(declaration);
}

const std::list<shared_ptr<decl_base> >&
abi_corpus::get_decls() const
{
  return m_members;
}

const location_manager&
abi_corpus::get_loc_mgr() const
{
  return m_loc_mgr;
}

location_manager&
abi_corpus::get_loc_mgr()
{
  return m_loc_mgr;
}

bool
abi_corpus::is_empty() const
{
  return m_members.empty();
}

}//end namespace abigail
