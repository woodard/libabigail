// -*- mode: C++ -*-
#ifndef __ABG_READER_H__
#define __ABG_READER_H__

#include "abg-corpus.h"

namespace abigail
{
namespace reader
{
bool read_file(const std::string& file_path,
	       abi_corpus&  abi_corpus);

}// end namespace reader
}// end namespace abigail

#endif //__ABG_READER_H__
