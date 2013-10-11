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

#include <string>
#include <ostream>

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

}// end namespace tools
}//end namespace abigail
