// -*- mode: C++ -*-

#ifndef __ABG_WRITER_H__
#define __ABG_WRITER_H__

#include <ostream>
#include "abg-corpus.h"

namespace abigail
{
namespace writer
{

bool write_to_ostream(const abi_corpus&	corpus,
		      std::ostream&	out);

}//end namespace writer

}// end namespace abigail
#endif //__ABG_WRITER_H__
