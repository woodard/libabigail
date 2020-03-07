// SPDX-License-Identifier: LGPL-3.0-or-later
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#ifndef __ABG_LIBZIP_UTILS_H__
#define __ABG_LIBZIP_UTILS_H__

#include <zip.h>
#include "abg-cxx-compat.h"

namespace abigail
{

namespace zip_utils
{

using abg_compat::shared_ptr;
using std::string;

/// @brief Functor passed to shared_ptr constructor during
/// instantiation with zip*
///
/// Its aim is to delete zip* managed by shared_ptr.
struct archive_deleter
{
  void
  operator()(zip* archive)
  {
    /// ??? Maybe check the return code of close and throw if the
    /// close fails?  But then callers must be prepared to handle
    /// this.
    zip_close(archive);
  }
};//end archive_deleter

/// @brief Functor passed to shared_ptr<zip_file>'s constructor.
///
/// Its aim is to close (actually delete) the zip_file* managed by the
/// shared_ptr.
struct zip_file_deleter
{
  void
  operator()(zip_file*f)
  {
    zip_fclose(f);
  }
};

typedef shared_ptr<zip> zip_sptr;
zip_sptr open_archive(const string& path, int flags, int *errorp);

typedef shared_ptr<zip_file> zip_file_sptr;
zip_file_sptr open_file_in_archive(zip_sptr archive,
				   int file_index);

}// end namespace zip
}// end namespace abigail
#endif //__ABG_LIBZIP_UTILS_H__
