// -*- mode: C++ -*-
/// @file

#ifndef __ABG_READER_H__
#define __ABG_READER_H__

#include "abg-corpus.h"

namespace abigail
{
namespace reader
{
bool read_file(const std::string&	file_path,
	       translation_unit&	tu);

}// end namespace reader
}// end namespace abigail

#endif //__ABG_READER_H__
