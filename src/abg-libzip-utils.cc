// SPDX-License-Identifier: LGPL-3.0-or-later
// -*- mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

/// @file

#include "abg-internal.h"
#ifdef WITH_ZIP_ARCHIVE
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-libzip-utils.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

#include <string>
namespace abigail
{

namespace zip_utils
{

/// This is a wrapper of the zip_open function, from libzip.  Its
/// purpose is to return a zip pointer wrapped in an appropriate
/// shared_ptr and thus to free the caller from having to deal with
/// calling zip_close on it.
///
/// The arguments this wrapper have the same meaning as in zip_open.
///
/// @param path the path to the zip archive to open.
///
/// @return a non-null zip pointer if the function succeeds.
zip_sptr
open_archive(const string& path, int flags, int *errorp)
{
  zip* z = zip_open(path.c_str(), flags, errorp);
  if (!z)
    return zip_sptr();
  return zip_sptr(z, archive_deleter());
}

/// @brief Open a file from a zip archive.
///
/// Open the file that is at a given \a index in the \a archive.
///
/// @param archive the zip archive to consider
///
/// @param file_index the index of the file to open from the zip
/// \a archive.
///
/// @return a non-null zip_file* upon successful completion, a null
/// pointer otherwise.
zip_file_sptr
open_file_in_archive(zip_sptr archive,
		     int file_index)
{
  zip_file * f = zip_fopen_index(archive.get(), file_index, 0);
  if (!f)
    return zip_file_sptr();
  return zip_file_sptr(f, zip_file_deleter());
}

}// end namespace zip
}// end namespace abigail

#endif //WITH_ZIP_ARCHIVE
