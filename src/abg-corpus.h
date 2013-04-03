// -*- mode: c++ -*-

#ifndef __ABG_CORPUS_H__
#define __ABG_CORPUS_H__

#include <tr1/memory>
#include "abg-ir.h"

using std::tr1::shared_ptr;

namespace abigail
{

/// This is the abstraction of a set of translation units (themselves
/// seen as bundles of unitary abi artefacts like types and decls)
/// bundled together as a corpus.  A corpus is thus the Application
/// binary interface of a program, a library or just a set of modules
/// put together.
class abi_corpus
{
  abi_corpus();

public:

  abi_corpus(const std::string& name);

  void
  add(const shared_ptr<translation_unit>);

  const std::list<shared_ptr<translation_unit> >&
  get_translation_units() const;

  bool
  is_empty() const;

private:
  std::string m_name;
  std::list<shared_ptr<translation_unit> > m_members;
};// end class abi_corpus

}//end namespace abigail
#endif //__ABG_CORPUS_H__
