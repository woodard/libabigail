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
//
// Author: Dodji Seketeli

/// @file
///
/// This is a program aimed at checking that a binary instrumentation
/// (bi) file is well formed and valid enough.  It acts by loading an
/// input bi file and saving it back to a temporary file.  It then
/// runs a diff on the two files and expects the result of the diff to
/// be empty.

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include "abg-tools-utils.h"
#include "abg-ir.h"
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-dwarf-reader.h"
#include "abg-writer.h"

using std::string;
using std::cerr;
using std::cin;
using std::cout;
using std::ostream;
using std::ofstream;
using abigail::tools::check_file;
using abigail::tools::file_type;
using abigail::tools::guess_file_type;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::xml_reader::read_translation_unit_from_file;
using abigail::xml_reader::read_translation_unit_from_istream;
using abigail::xml_reader::read_corpus_from_file;
using abigail::xml_reader::read_corpus_from_native_xml;
using abigail::xml_reader::read_corpus_from_native_xml_file;
using abigail::dwarf_reader::read_corpus_from_elf;
using abigail::xml_writer::write_translation_unit;
using abigail::xml_writer::write_corpus_to_native_xml;
using abigail::xml_writer::write_corpus_to_archive;

struct options
{
  string file_path;
  bool read_from_stdin;
  bool read_tu;
  bool diff;
  bool bidiff;
  bool noout;

  options()
    : read_from_stdin(false),
      read_tu(false),
      diff(false),
      bidiff(false),
      noout(false)
  {}
};//end struct options;

void
display_usage(const string& prog_name, ostream& out)
{
  out << "usage: " << prog_name << "[options] [<abi-file1>\n"
      << " where options can be:\n"
      << "  --help  display this message\n"
      << "  --diff  for xml inputs, perform a text diff between "
         "the input and the memory model saved back to disk\n"
      << "  --bidiff perform an abi diff between the input "
         "and the memory model(not yet implemented)\n"
      << "  --noout  do not display anything on stdout\n"
      << "  --stdin|--  read abi-file content from stdin\n"
      << "  --tu  expect a single translation unit file\n";
}

bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    {
      opts.read_from_stdin = true;
      return true;
    }

    for (int i = 1; i < argc; ++i)
      {
	if (argv[i][0] != '-')
	  {
	    if (opts.file_path.empty())
	      opts.file_path = argv[i];
	    else
	      return false;
	  }
	else if (!strcmp(argv[i], "--help"))
	  return false;
	else if (!strcmp(argv[i], "--stdin"))
	  opts.read_from_stdin = true;
	else if (!strcmp(argv[i], "--tu"))
	  opts.read_tu = true;
	else if (!strcmp(argv[i], "--diff"))
	  opts.diff = true;
	else if (!strcmp(argv[i], "--bidiff"))
	  opts.bidiff = true;
	else if (!strcmp(argv[i], "--noout"))
	  opts.noout = true;
	else
	  return false;
      }

    if (opts.file_path.empty())
      opts.read_from_stdin = true;
    return true;
}

/// Reads a bi (binary instrumentation) file, saves it back to a
/// temporary file and run a diff on the two versions.
int
main(int argc, char* argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      display_usage(argv[0], cerr);
      return true;
    }

  if (opts.read_from_stdin)
    {
      if (!cin.good())
	return true;

      if (opts.read_tu)
	{
	  abigail::translation_unit_sptr tu =
	    read_translation_unit_from_istream(&cin);

	  if (!tu)
	    {
	      cerr << "failed to read the ABI instrumentation from stdin\n";
	      return true;
	    }

	  if (!opts.noout)
	    write_translation_unit(*tu, 0, cout);
	  return false;
	}
      else
	{
	  corpus_sptr corp = read_corpus_from_native_xml(&cin);
	  if (!opts.noout)
	    write_corpus_to_native_xml(corp, /*indent=*/0, cout);
	  return false;
	}
    }
  else if (!opts.file_path.empty())
    {
      if (!check_file(opts.file_path, cerr))
	return true;

      abigail::translation_unit_sptr tu;
      abigail::corpus_sptr corp;
      file_type type = guess_file_type(opts.file_path);

      switch (type)
	{
	case abigail::tools::FILE_TYPE_UNKNOWN:
	  cerr << "Unknown file type given in input: " << opts.file_path;
	  return true;
	case abigail::tools::FILE_TYPE_NATIVE_BI:
	  tu = read_translation_unit_from_file(opts.file_path);
	  break;
	case abigail::tools::FILE_TYPE_ELF:
	  corp = read_corpus_from_elf(opts.file_path);
	  break;
	case abigail::tools::FILE_TYPE_XML_CORPUS:
	  corp = read_corpus_from_native_xml_file(opts.file_path);
	  break;
	case abigail::tools::FILE_TYPE_ZIP_CORPUS:
	  corp = read_corpus_from_file(opts.file_path);
	  break;
	}

      if (!tu && !corp)
	{
	  cerr << "failed to read " << opts.file_path << "\n";
	  return true;
	}

      char tmpn[L_tmpnam];
      tmpnam(tmpn);

      string ofile_name = tmpn;

      ofstream of(ofile_name.c_str(), std::ios_base::trunc);
      if (!of.is_open())
	{
	  cerr << "open temporary output file " << ofile_name << "\n";
	  return true;
	}

      bool r;

      if (tu)
	r = write_translation_unit(*tu, /*indent=*/0, of);
      else
	{
	  r = true;
	  if (type == abigail::tools::FILE_TYPE_XML_CORPUS)
	    {
	      if (opts.noout)
		{
		  if (opts.diff)
		    r = write_corpus_to_native_xml(corp, /*indent=*/0, of);
		}
	      else
		r = write_corpus_to_native_xml(corp, /*indent=*/0, cout);
	    }
	  else if (type == abigail::tools::FILE_TYPE_ZIP_CORPUS)
	    {
	      if (!opts.noout)
		r = write_corpus_to_archive(*corp, ofile_name);
	      of.close();
	    }
	  else if (type == abigail::tools::FILE_TYPE_ELF)
	    {
	      if (!opts.noout)
		r = write_corpus_to_native_xml(corp, /*indent=*/0, cout);
	    }
	}

      bool is_ok = r;
      of.close();

      if (!is_ok)
	{
	  string output =
	    (type == abigail::tools::FILE_TYPE_NATIVE_BI)
	    ? "translation unit"
	    : "ABI corpus";
	  cerr << "failed to write the translation unit "
	       << opts.file_path << " back\n";
	}

      if (is_ok
	  && opts.diff
	  && ((type == abigail::tools::FILE_TYPE_XML_CORPUS)
	      || type == abigail::tools::FILE_TYPE_NATIVE_BI
	      || type == abigail::tools::FILE_TYPE_ZIP_CORPUS))
	{
	  string cmd = "diff -u " + opts.file_path + " " + ofile_name;
	  if (system(cmd.c_str()))
	    is_ok = false;
	}
      remove(ofile_name.c_str());
      return !is_ok;
    }

  return true;
}
