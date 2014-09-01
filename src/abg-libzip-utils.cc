// -*- mode: C++ -*-
//
// Copyright (C) 2013-2014 Red Hat, Inc.
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

/// @file

#include "config.h"
#ifdef WITH_ZIP_ARCHIVE

#include <string>
#include "abg-libzip-utils.h"

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
