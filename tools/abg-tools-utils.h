// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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

///@file

#include <tr1/memory>
#include <string>
#include <ostream>
#include <istream>

namespace abigail
{

namespace tools
{

bool file_exists(const std::string&);
bool is_regular_file(const std::string&);
bool is_dir(const std::string&);
bool dirname(std::string const& path,
	     std::string& dir_name);
bool base_name(std::string const& path,
	       std::string& file_name);
bool ensure_dir_path_created(const std::string&);
bool ensure_parent_dir_created(const std::string&);
bool check_file(const std::string& path, std::ostream& out);

/// The different types of files understood the bi* suite of tools.
enum file_type
{
  /// A file type we don't know about.
  FILE_TYPE_UNKNOWN,
  /// The native xml file format representing a translation unit.
  FILE_TYPE_NATIVE_BI,
  /// An elf file.  Read this kind of file should yield an
  /// abigail::corpus type.
  FILE_TYPE_ELF,
  /// An archive (AR) file.
  FILE_TYPE_AR,
  // A native xml file format representing a corpus of one or several
  // translation units.
  FILE_TYPE_XML_CORPUS,
  // A zip file, possibly containing a corpus of one of several
  // translation units.
  FILE_TYPE_ZIP_CORPUS,
};

file_type guess_file_type(std::istream& in);
file_type guess_file_type(const std::string& file_path);

std::tr1::shared_ptr<char>
make_path_absolute(const char*p);

}// end namespace tools
}//end namespace abigail
