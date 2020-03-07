// SPDX-License-Identifier: LGPL-3.0-or-later
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2020 Red Hat, Inc.

#include <sys/types.h>
#include <sys/stat.h>// The two above is for 'stat'
#include <cstring> // for 'strcmp'
#include <cstdlib> // for 'system'
#include <iostream>
#include <fstream>
#include <list>
#include "abg-corpus.h"
#include "abg-reader.h"
#include "abg-writer.h"
#include "abg-tools-utils.h"

using std::cerr;
using std::cout;
using std::ostream;
using std::string;
using std::list;

struct options
{
  bool list_content;
  string extract_dest;
  string archive;
  list<string> in_files;
  string out_dir;

  options()
    : list_content(false)
  {
  }

};//end options

/// Display a message explaining the usage of the program on stdout.
static void
display_usage(const string &prog_name, ostream& out)
{
  out << "usage: " << prog_name << " [options] [archive-file-path]\n"
       << " where options are: \n"
       << "--help|-h				display this usage message\n"
       << "--list|l <archive>			list the archive content\n"
       << "--add|-a <files-to-add> <archive>	add files to an archive\n"
       << "--extract|x [dest-dir] <archive>	extract archive content\n"
  ;
}

/// Parse the command line arguments and populate an instance of \a
/// options as a result.
///
/// @param argc the number of words on the command line, including the
/// program name.
///
/// @param argv an array of words representing the command line.  This
/// is the thing that is parsed.
///
/// @param opts the options that are set as a result of parsing the
/// command line.  This output parameter is set iff the function
/// returs true.
///
/// @return true if the function could make sense of the command line
/// and parsed it correctly.  Otherwise, return false.  If this
/// function returns false, the caller would be well inspired to call
/// display_usage() to remind the user how to properly invoke this
/// program so that its command line makes sense.
static bool
parse_args(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      char* arg = argv[i];

      if (! strcmp(arg, "--help")
	  ||! strcmp(arg, "-h"))
	return false;

      if (arg[0] != '-')
	opts.archive = arg;
      else if (! strcmp(arg, "--list")
	       ||! strcmp(arg, "-l"))
	opts.list_content = true;
      else if (! strcmp(arg, "--add")
	       || ! strcmp(arg,"-a"))
	{
	  int arg_index0, arg_index1;
	  char *f = 0;
	  for (arg_index0 = i + 1; arg_index0 < argc; ++arg_index0)
	    {
	      // --add must be followed by at N words that don't start
	      // by '-' (N > 1).  The first N-1 words are the
	      // arguments of --add (the files to add to the archive)
	      // and the last one is the name of the archive to add
	      // the files to.

	      f = argv[arg_index0];
	      if (f[0] == '-')
		break;

	      arg_index1 = arg_index0 + 1;
	      if (arg_index1 >= argc
		  || argv[arg_index1][0] == '-')
		break;

	      opts.in_files.push_back(f);
	    }
	  if (opts.in_files.empty())
	    return false;
	}
      else if (! strcmp(arg, "--extract")
			|| ! strcmp(arg, "-x"))
	{
	  int arg_index = i + 1, arch_index = arg_index + 1;
	  if (arg_index < argc
	      && argv[arg_index][0] != '-'
	      && arch_index < argc
	      && argv[arch_index][0] != '-')
	    opts.extract_dest = argv[arg_index];
	  else if (arg_index < argc
		   && argv[arg_index][0] != '-')
	    // No destination directory argument was given for the
	    // --extract option, so consider it to be the current
	    // directory
	    opts.extract_dest = ".";
	  else
	    return false;
	}
    }

  return true;
}

using abigail::corpus;
using abigail::corpus_sptr;
using abigail::translation_unit;
using abigail::translation_unit_sptr;
using abigail::translation_units;
using abigail::xml_reader::read_corpus_from_file;
using abigail::xml_writer::write_corpus_to_archive;

/// List the content of a given archive.  The names of the files of
/// the archive are then displayed on stdout.
///
/// @param archive_path a path to the file containing the archive file
/// to list the content of.
///
/// @return true upon successful completion, false otherwise.
static bool
list_content(const string& archive_path)
{
  if (archive_path.empty())
    {
      cerr << "Empty archive path\n";
      return false;
    }

  corpus_sptr archive = read_corpus_from_file(archive_path);
  if (!archive)
    {
      cerr << "Could not read archive at '" << archive_path << "'\n";
      return false;
    }

  for (translation_units::const_iterator i =
	 archive->get_translation_units().begin();
       i != archive->get_translation_units().end();
       ++i)
    cout << (*i)->get_path() << "\n";

  return true;
}

/// Add a translation unit to an archive -- or create one for that
/// matter.
///
/// @param tu_paths a list of paths to add to the archive.
///
/// @param archive_path the path of the archive to either open or
/// create.  The specified in \tu_paths are then added to this
/// archive.  Note that this function creates the entire directory
/// tree needed up to \a archive_path, if needed.
///
/// @return true upon successful completion, false otherwise.
static bool
add_tus_to_archive(const list<string>& tu_paths,
		   const string& archive_path)
{
  translation_units tus;
  corpus corp(archive_path);

  bool added_some_tus = false;
  for (list<string>::const_iterator i = tu_paths.begin();
       i != tu_paths.end();
       ++i)
    {
      translation_unit_sptr tu =
	abigail::xml_reader::read_translation_unit_from_file(*i);
      if (!tu)
	{
	  cerr << "could not read binary instrumentation file '"
	       << *i
	       << "'\n";
	  continue;
	}
      corp.add(tu);
      added_some_tus = true;
    }

  if (added_some_tus)
    {
      if (!write_corpus_to_archive(corp))
	{
	  cerr << "could not write archive file '"
	       << corp.get_path()
	       << "'\n";
	  return false;
	}
    }

  return true;
}

/// Extract translation units from a given archive.
///
/// @param dest_path the path of the destination directory which the
/// elements of the archive are to be extracted under.
///
/// @param archive_path the path to the archive to extract.  The
/// archive must exist and be accessible with read privileges.
///
/// @return true upon successful completion, false otherwise.
static bool
extract_tus_from_archive(const string& dest_path,
			 const string& archive_path)
{
  if (dest_path.empty())
    {
      cerr << "empty file directory\n";
      return false;
    }

  corpus_sptr archive(new corpus(archive_path));

  if (read_corpus_from_file(archive) < 1)
    {
      cerr << "could not read archive at '"
	   << archive_path
	   << "'\n;";
      return false;
    }

  string cmd = "mkdir -p " + dest_path;
  if (system(cmd.c_str()))
    {
      cerr << "could not create file directory '"
	   << dest_path
	   << "'\n";
      return false;
    }

  for (translation_units::const_iterator i =
	 archive->get_translation_units().begin();
       i != archive->get_translation_units().end();
       ++i)
    {
      string dest = dest_path + "/" + (*i)->get_path();
      if (!abigail::tools_utils::ensure_parent_dir_created(dest))
	{
	  cerr << "could not create parent director for '" << dest << "'\n";
	  return false;
	}

      if (!abigail::xml_writer::write_translation_unit(**i, /*indent=*/0, dest))
	{
	  cerr << "could not write binary instrumentation file to '"
	       << dest
	       << "'\n";
	  return false;
	}
    }

  return true;
}

/// Parse the command line and perform the archive-related operations
/// asked by the user, if the command line makes sense; otherwise,
/// display a usage help message and bail out.
int
main(int argc, char* argv[])
{
  options opts;

  if (!parse_args(argc, argv, opts)
      || opts.archive.empty())
    {
      display_usage(argv[0], cout);
      return -1;
    }

  if (opts.list_content)
    return !list_content(opts.archive);
  else if (!opts.in_files.empty())
    return !add_tus_to_archive(opts.in_files, opts.archive);
  else if (!opts.extract_dest.empty())
    return !extract_tus_from_archive(opts.extract_dest, opts.archive);
  else
    {
      display_usage(argv[0], cout);
      return -1;
    }

  return 0;
}
