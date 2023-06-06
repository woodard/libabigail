// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2023 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// This program reads an elf file, try to load its debug info (in
/// DWARF format) and emit it back in a set of "text sections" in native
/// libabigail XML format.

#include "config.h"
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "abg-config.h"
#include "abg-tools-utils.h"
#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#ifdef WITH_CTF
#include "abg-ctf-reader.h"
#endif
#ifdef WITH_BTF
#include "abg-btf-reader.h"
#endif
#include "abg-writer.h"
#include "abg-reader.h"
#include "abg-comparison.h"
#include "abg-suppression.h"

using std::string;
using std::cerr;
using std::cout;
using std::ostream;
using std::ofstream;
using std::vector;
using std::shared_ptr;
using abg_compat::optional;
using abigail::tools_utils::emit_prefix;
using abigail::tools_utils::temp_file;
using abigail::tools_utils::temp_file_sptr;
using abigail::tools_utils::check_file;
using abigail::tools_utils::build_corpus_group_from_kernel_dist_under;
using abigail::tools_utils::timer;
using abigail::tools_utils::best_elf_based_reader_opts;
using abigail::tools_utils::create_best_elf_based_reader;
using abigail::tools_utils::options_base;
// Whern we switch to C++20 which has P1099R5 this can be uncommented and
// all the namespace qualifiers for the enum abidiff_status can be dropped.
// using abigail::tools_utils::abidiff_status;
using abigail::ir::environment_sptr;
using abigail::ir::environment;
using abigail::corpus;
using abigail::corpus_sptr;
using abigail::translation_units;
using abigail::suppr::suppression_sptr;
using abigail::suppr::suppressions_type;
using abigail::suppr::read_suppressions;
using abigail::comparison::corpus_diff;
using abigail::comparison::corpus_diff_sptr;
using abigail::comparison::compute_diff;
using abigail::comparison::diff_context_sptr;
using abigail::comparison::diff_context;
using abigail::xml_writer::SEQUENCE_TYPE_ID_STYLE;
using abigail::xml_writer::HASH_TYPE_ID_STYLE;
using abigail::xml_writer::create_write_context;
using abigail::xml_writer::type_id_style_kind;
using abigail::xml_writer::write_context_sptr;
using abigail::xml_writer::write_corpus;
using abigail::abixml::read_corpus_from_abixml_file;

using namespace abigail;

struct options: public options_base
{
  string		out_file_path;
  vector<string>	headers_dirs;
  vector<string>	header_files;
  string		vmlinux;
  suppressions_type	kabi_whitelist_supprs;
  bool			check_alt_debug_info_path;
  bool			show_base_name_alt_debug_info_path;
  bool			write_architecture;
  bool			write_corpus_path;
  bool			write_comp_dir;
  bool			write_elf_needed;
  bool			write_parameter_names;
  bool			short_locs;
  bool			default_sizes;
  bool			corpus_group_for_linux;
  bool			show_stats;
  bool			noout;
  bool			show_locs;
  bool			abidiff;
  bool			annotate;
  bool			drop_private_types;
  bool			drop_undefined_syms;
  bool			assume_odr_for_cplusplus;
  bool			leverage_dwarf_factorization;
  optional<bool>	exported_interfaces_only;
  type_id_style_kind	type_id_style;
#ifdef WITH_DEBUG_SELF_COMPARISON
  string		type_id_file_path;
#endif

  options()
    : check_alt_debug_info_path(),
      show_base_name_alt_debug_info_path(),
      write_architecture(true),
      write_corpus_path(true),
      write_comp_dir(true),
      write_elf_needed(true),
      write_parameter_names(true),
      short_locs(false),
      default_sizes(true),
      corpus_group_for_linux(false),
       noout(),
      show_locs(true),
      abidiff(),
      annotate(),
      drop_private_types(false),
      drop_undefined_syms(false),
      assume_odr_for_cplusplus(true),
      leverage_dwarf_factorization(true),
      type_id_style(SEQUENCE_TYPE_ID_STYLE)
  {}

  /// Do the final preparation of the reader_opts structure before handing it
  /// off to create_best_elf_based_reader()
  ///
  /// @return reader_opts
  best_elf_based_reader_opts &get_reader_opts()
  {
    return reader_opts;
  }

  /// Check that the header files supplied are present.
  /// If not, emit an error on stderr.
  ///
  /// @return true if all header files are present, false otherwise.
  bool
  maybe_check_header_files()
  {
    for (vector<string>::const_iterator file = header_files.begin();
	 file != header_files.end();
	 ++file)
      if (!check_file(*file, cerr, "abidw"))
	return false;

    return true;
  }
};

static const char usage[]= " [options] [<path-to-elf-file>]\n"
  " where options can be: \n"
  "  --help|-h  display this message\n"
  "  --version|-v  display program version information and exit\n"
  "  --abixml-version  display the version of the ABIXML ABI format\n"
  "  --debug-info-dir|-d <dir-path>  look for debug info under 'dir-path'\n"
  "  --headers-dir|--hd <path> the path to headers of the elf file\n"
  "  --header-file|--hf <path> the path one header of the elf file\n"
  "  --out-file <file-path>  write the output to 'file-path'\n"
  "  --noout  do not emit anything after reading the binary\n"
  "  --suppressions|--suppr <path> specify a suppression file\n"
  "  --no-architecture  do not emit architecture info in the output\n"
  "  --no-corpus-path  do not take the path to the corpora into account\n"
  "  --no-show-locs  do not show location information\n"
  "  --short-locs  only print filenames rather than paths\n"
  "  --drop-private-types  drop private types from representation\n"
  "  --drop-undefined-syms  drop undefined symbols from representation\n"
  "  --exported-interfaces-only  analyze exported interfaces only\n"
  "  --allow-non-exported-interfaces  analyze interfaces that "
  "might not be exported\n"
  "  --no-comp-dir-path  do not show compilation path information\n"
  "  --no-elf-needed  do not show the DT_NEEDED information\n"
  "  --no-write-default-sizes  do not emit pointer size when it equals"
  " the default address size of the translation unit\n"
  "  --no-parameter-names  do not show names of function parameters\n"
  "  --type-id-style <sequence|hash>  type id style (sequence(default): "
  "\"type-id-\" + number; hash: hex-digits)\n"
  "  --check-alternate-debug-info <elf-path>  check alternate debug info "
  "of <elf-path>\n"
  "  --check-alternate-debug-info-base-name <elf-path>  check alternate "
  "debug info of <elf-path>, and show its base name\n"
  "  --load-all-types  read all types including those not reachable from "
  "exported declarations\n"
  "  --no-linux-kernel-mode  don't consider the input binary as "
  "a Linux Kernel binary\n"
  "  --kmi-whitelist|-w  path to a linux kernel "
  "abi whitelist\n"
  "  --linux-tree|--lt  emit the ABI for the union of a "
  "vmlinux and its modules\n"
  "  --vmlinux <path>  the path to the vmlinux binary to consider to emit "
  "the ABI of the union of vmlinux and its modules\n"
  "  --abidiff  compare the loaded ABI against itself\n"
#ifdef WITH_DEBUG_SELF_COMPARISON
  "  --debug-abidiff  debug the process of comparing the loaded ABI against itself\n"
#endif
#ifdef WITH_DEBUG_TYPE_CANONICALIZATION
  "  --debug-tc  debug the type canonicalization process\n"
  "  --debug-dc  debug the DIE canonicalization process\n"
#endif
#ifdef WITH_CTF
  "  --ctf use CTF instead of DWARF in ELF files\n"
#endif
  "  --no-leverage-dwarf-factorization  do not use DWZ optimisations to "
  "speed-up the analysis of the binary\n"
  "  --no-assume-odr-for-cplusplus  do not assume the ODR to speed-up the "
  "analysis of the binary\n"
#ifdef WITH_BTF
  "  --btf use BTF instead of DWARF in ELF files\n"
#endif
  "  --annotate  annotate the ABI artifacts emitted in the output\n"
  "  --stats  show statistics about various internal stuff\n"
  "  --verbose show verbose messages about internal stuff\n";

void
display_usage(const string& prog_name, ostream& out)
{
  emit_prefix(prog_name, out)
    << "usage: " << prog_name << usage;
}

static bool
parse_command_line(int argc, char* argv[], options& opts)
{
  if (argc < 2)
    return false;

  for (int i = 1; i < argc; ++i)
    {
      if (argv[i][0] != '-')
	{
	  if (opts.reader_opts.elf_file_path.empty())
	    opts.reader_opts.elf_file_path = argv[i];
	  else
	    return false;
	}
      else if (opts.common_options(argc, argv, i, usage))
	continue; // we handled this option
      else if( opts.missing_operand)
	return false;
      else if (!strcmp(argv[i], "--debug-info-dir")
	       || !strcmp(argv[i], "-d"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-')
	    return false;
	  // elfutils wants the root path to the debug info to be
	  // absolute.
	  opts.di_root_paths.push_back
	    (abigail::tools_utils::make_path_absolute_to_be_freed(argv[i + 1]));
	  ++i;
	}
      else if (!strcmp(argv[i], "--headers-dir")
	       || !strcmp(argv[i], "--hd"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.headers_dirs.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--header-file")
	       || !strcmp(argv[i], "--hf"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.header_files.push_back(argv[j]);
	  ++i;
	}
      else if (!strcmp(argv[i], "--out-file"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.out_file_path.empty())
	    return false;

	  opts.out_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--linux-tree")
	       || !strcmp(argv[i], "--lt"))
	opts.corpus_group_for_linux = true;
      else if (!strcmp(argv[i], "--vmlinux"))
	{
	  int j = i + 1;
	  if (j >= argc)
	    return false;
	  opts.vmlinux = argv[j];
	  ++i;
	}
      else if (!strcmp(argv[i], "--noout"))
	opts.noout = true;
      else if (!strcmp(argv[i], "--no-architecture"))
	opts.write_architecture = false;
      else if (!strcmp(argv[i], "--no-corpus-path"))
	opts.write_corpus_path = false;
      else if (!strcmp(argv[i], "--no-show-locs"))
	opts.show_locs = false;
      else if (!strcmp(argv[i], "--short-locs"))
	opts.short_locs = true;
      else if (!strcmp(argv[i], "--no-comp-dir-path"))
	opts.write_comp_dir = false;
      else if (!strcmp(argv[i], "--no-elf-needed"))
	opts.write_elf_needed = false;
      else if (!strcmp(argv[i], "--no-write-default-sizes"))
	opts.default_sizes = false;
      else if (!strcmp(argv[i], "--no-parameter-names"))
	opts.write_parameter_names = false;
      else if (!strcmp(argv[i], "--type-id-style"))
        {
          ++i;
          if (i >= argc)
            return false;
          if (!strcmp(argv[i], "sequence"))
            opts.type_id_style = SEQUENCE_TYPE_ID_STYLE;
          else if (!strcmp(argv[i], "hash"))
            opts.type_id_style = HASH_TYPE_ID_STYLE;
          else
            return false;
        }
      else if (!strcmp(argv[i], "--check-alternate-debug-info")
	       || !strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	{
	  if (argc <= i + 1
	      || argv[i + 1][0] == '-'
	      || !opts.reader_opts.elf_file_path.empty())
	    return false;
	  if (!strcmp(argv[i], "--check-alternate-debug-info-base-name"))
	    opts.show_base_name_alt_debug_info_path = true;
	  opts.check_alt_debug_info_path = true;
	  opts.reader_opts.elf_file_path = argv[i + 1];
	  ++i;
	}
      else if (!strcmp(argv[i], "--load-all-types"))
	opts.reader_opts.show_all_types = true;
      else if (!strcmp(argv[i], "--drop-private-types"))
	opts.drop_private_types = true;
      else if (!strcmp(argv[i], "--drop-undefined-syms"))
	opts.drop_undefined_syms = true;
      else if (!strcmp(argv[i], "--exported-interfaces-only"))
	opts.exported_interfaces_only = true;
      else if (!strcmp(argv[i], "--allow-non-exported-interfaces"))
	opts.exported_interfaces_only = false;
      else if (!strcmp(argv[i], "--no-linux-kernel-mode"))
	opts.reader_opts.linux_kernel_mode = false;
      else if (!strcmp(argv[i], "--abidiff"))
	opts.abidiff = true;
#ifdef WITH_DEBUG_SELF_COMPARISON
      // Can't be factored with abidiff option name is different and behavior
      // is different
      else if (!strcmp(argv[i], "--debug-abidiff"))
	{
	  opts.abidiff = true;
	  opts.debug_abidiff = true;
	}
#endif
      else if (!strcmp (argv[i], "--no-assume-odr-for-cplusplus"))
	opts.assume_odr_for_cplusplus = false;
      else if (!strcmp (argv[i], "--no-leverage-dwarf-factorization"))
	opts.leverage_dwarf_factorization = false;
      else if (!strcmp(argv[i], "--annotate"))
	opts.annotate = true;
      else
	{
	  if (strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] == '-')
	    opts.wrong_option = argv[i];
	  return false;
	}
      if(opts.missing_operand)
	return true;
    }

  // final checks
  if (!opts.complete_parse( argv[0]))
    return false;

  // these only exists in abidw
  if (!opts.maybe_check_header_files())
    return false;

  if (opts.corpus_group_for_linux)
    {
      if (!abigail::tools_utils::check_dir(opts.reader_opts.elf_file_path,
					   cerr, argv[0]))
	return false;
    }
  else
    if (!abigail::tools_utils::check_file(opts.reader_opts.elf_file_path,
					  cerr, argv[0]))
      return false;

  return true;
}

/// Initialize the context use for driving ABI comparison.
///
/// @param ctxt the context to initialize.
static void
set_diff_context(diff_context_sptr& ctxt)
{
  ctxt->default_output_stream(&cerr);
  ctxt->error_output_stream(&cerr);
  // Filter out changes that are not meaningful from an ABI
  // standpoint, from the diff output.
  ctxt->switch_categories_off
    (abigail::comparison::ACCESS_CHANGE_CATEGORY
     | abigail::comparison::COMPATIBLE_TYPE_CHANGE_CATEGORY
     | abigail::comparison::HARMLESS_DECL_NAME_CHANGE_CATEGORY);
}

/// Set suppression specifications to the @p read_context used to load
/// the ABI corpus from the ELF/DWARF file.
///
/// These suppression specifications are going to be applied to drop
/// some ABI artifacts on the floor (while reading the ELF/DWARF file)
/// and thus minimize the size of the resulting ABI corpus.
///
/// @param read_ctxt the read context to apply the suppression
/// specifications to.
///
/// @param opts the options where to get the suppression
/// specifications from.
static void
set_suppressions(abigail::elf_based_reader& rdr, options& opts)
{
  suppressions_type supprs;
  for (vector<string>::const_iterator i = opts.suppression_paths.begin();
       i != opts.suppression_paths.end();
       ++i)
    read_suppressions(*i, supprs);

  suppression_sptr suppr =
    abigail::tools_utils::gen_suppr_spec_from_headers(opts.headers_dirs,
						      opts.header_files);
  if (suppr)
    {
      if (opts.drop_private_types)
	suppr->set_drops_artifact_from_ir(true);
      supprs.push_back(suppr);
    }

  using abigail::tools_utils::gen_suppr_spec_from_kernel_abi_whitelists;
  const suppressions_type& wl_suppr =
      gen_suppr_spec_from_kernel_abi_whitelists(opts.kabi_whitelist_paths);

  opts.kabi_whitelist_supprs.insert(opts.kabi_whitelist_supprs.end(),
				    wl_suppr.begin(), wl_suppr.end());

  rdr.add_suppressions(supprs);
  rdr.add_suppressions(opts.kabi_whitelist_supprs);
}

/// Set a bunch of tunable buttons on the ELF-based reader from the
/// command-line options.
///
/// @param rdr the reader to tune.
///
/// @param opts the command line options.
static void
set_generic_options(abigail::elf_based_reader& rdr, options& opts)
{
  rdr.options().drop_undefined_syms = opts.drop_undefined_syms;
  rdr.options().show_stats = opts.show_stats;
  rdr.options().do_log = opts.do_log;
  rdr.options().leverage_dwarf_factorization =
    opts.leverage_dwarf_factorization;
  rdr.options().assume_odr_for_cplusplus =
    opts.assume_odr_for_cplusplus;
}

/// Load an ABI @ref corpus (the internal representation of the ABI of
/// a binary) and write it out as an abixml.
///
/// @param argv the arguments the program was called with.
///
/// @param opts the options of the program.
///
/// @return the exit code: 0 if everything went fine, non-zero
/// otherwise.
static  abigail::tools_utils::abidiff_status
load_corpus_and_write_abixml(char* argv[],
			     options& opts)
{
  auto exit_code = abigail::tools_utils::ABITOOL_OK;
  timer t;
  fe_iface::status s = fe_iface::STATUS_UNKNOWN;
  corpus_sptr corp;

  // First of all, create a reader to read the ABI from the file
  // specfied in opts ...
  abigail::elf_based_reader_sptr reader =
    create_best_elf_based_reader(opts.get_reader_opts());
  ABG_ASSERT(reader);

  // ... then tune a bunch of "buttons" on the newly created reader
  // ...
  set_generic_options(*reader, opts);
  set_suppressions(*reader, opts);

  // If the user asked us to check if we found the "alternate debug
  // info file" associated to the input binary, then proceed to do so
  // ...
  if (opts.check_alt_debug_info_path)
    {
      string alt_di_path = reader->alternate_dwarf_debug_info_path();
      if (!alt_di_path.empty())
	{
	  cout << "found the alternate debug info file";
	  if (opts.show_base_name_alt_debug_info_path)
	    {
	      tools_utils::base_name(alt_di_path, alt_di_path);
	      cout << " '" << alt_di_path << "'";
	    }
	  cout << "\n";
	  return abigail::tools_utils::ABITOOL_OK;
	}
      else
	{
	  emit_prefix(argv[0], cerr)
	    << "could not find alternate debug info file\n";
	  return abigail::tools_utils::ABITOOL_ERROR;
	}
    }

  // ... ff we are asked to only analyze exported interfaces (to stay
  // concise), then take that into account ...
  if (opts.exported_interfaces_only.has_value())
    opts.env.analyze_exported_interfaces_only(*opts.exported_interfaces_only);

  // And now, really read/analyze the ABI of the input file.
  t.start();
  corp = reader->read_corpus(s);
  t.stop();
  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "read corpus from elf file in: " << t << "\n";

  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "reset reader ELF in: " << t << "\n";

  // If we couldn't create a corpus, emit some (hopefully) useful
  // diagnostics and return and error.
  if (!corp)
    {
      if (s == fe_iface::STATUS_DEBUG_INFO_NOT_FOUND)
	{
	  if (opts.di_root_paths.empty())
	    {
	      emit_prefix(argv[0], cerr)
		<< "Could not read debug info from "
		<< opts.reader_opts.elf_file_path << "\n";

	      emit_prefix(argv[0], cerr)
		<< "You might want to supply the root directory where "
		"to search debug info from, using the "
		"--debug-info-dir option "
		"(e.g --debug-info-dir /usr/lib/debug)\n";
	    }
	  else
	    {
	      emit_prefix(argv[0], cerr)
		<< "Could not read debug info for '"
		<< opts.reader_opts.elf_file_path
		<< "' from debug info root directory '";
	      for (vector<char*>::const_iterator i =
		     opts.di_root_paths.begin();
		   i != opts.di_root_paths.end();
		   ++i)
		{
		  if (i != opts.di_root_paths.begin())
		    cerr << ", ";
		  cerr << *i;
		}
	    }
	}
      else if (s == fe_iface::STATUS_NO_SYMBOLS_FOUND)
	emit_prefix(argv[0], cerr)
	  << "Could not read ELF symbol information from "
	  << opts.reader_opts.elf_file_path << "\n";
      else if (s & fe_iface::STATUS_ALT_DEBUG_INFO_NOT_FOUND)
	{
	  emit_prefix(argv[0], cerr)
	    << "Could not read alternate debug info file";
	  if (!reader->alternate_dwarf_debug_info_path().empty())
	    cerr << " '" << reader->alternate_dwarf_debug_info_path() << "'";
	  cerr << " for '"
	    << opts.reader_opts.elf_file_path << "'.\n";
	  emit_prefix(argv[0], cerr)
	    << "You might have forgotten to install some "
	    "additional needed debug info\n";
	}

      return  abigail::tools_utils::ABITOOL_ERROR;
    }

  // Clear some resources to gain back some space.
  t.start();
  reader.reset();
  t.stop();

  // Now create a write context and write out an ABI XML description
  // of the read corpus.
  t.start();
  const write_context_sptr& write_ctxt = create_write_context(opts.env, cout);
  set_common_options(*write_ctxt, opts);
  t.stop();

  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "created & initialized write context in: "
      << t << "\n";

  if (opts.abidiff)
    {
      // Save the abi in abixml format in a temporary file, read
      // it back, and compare the ABI of what we've read back
      // against the ABI of the input ELF file.
      temp_file_sptr tmp_file = temp_file::create();
      set_ostream(*write_ctxt, tmp_file->get_stream());
      write_corpus(*write_ctxt, corp, 0);
      tmp_file->get_stream().flush();

#ifdef WITH_DEBUG_SELF_COMPARISON
      if (opts.debug_abidiff)
        {
          opts.type_id_file_path = tmp_file->get_path() + string(".typeid");
          write_canonical_type_ids(*write_ctxt, opts.type_id_file_path);
        }
#endif
      fe_iface_sptr rdr = abixml::create_reader(tmp_file->get_path(),
						opts.env);

#ifdef WITH_DEBUG_SELF_COMPARISON
      if (opts.debug_abidiff
          && !opts.type_id_file_path.empty())
        load_canonical_type_ids(*rdr, opts.type_id_file_path);
#endif
      t.start();
      fe_iface::status sts;
      corpus_sptr corp2 = rdr->read_corpus(sts);
      t.stop();
      if (opts.do_log)
        emit_prefix(argv[0], cerr)
          << "Read corpus in: " << t << "\n";

      if (!corp2)
        {
          emit_prefix(argv[0], cerr)
            << "Could not read temporary XML representation of "
            "elf file back\n";
          return  abigail::tools_utils::ABITOOL_ERROR;
        }

      diff_context_sptr ctxt(new diff_context);
      set_diff_context(ctxt);
      ctxt->show_locs(opts.show_locs);
      t.start();
      corpus_diff_sptr diff = compute_diff(corp, corp2, ctxt);
      t.stop();
      if (opts.do_log)
        emit_prefix(argv[0], cerr)
          << "computed diff in: " << t << "\n";

      bool has_error = diff->has_changes();
      if (has_error)
        {
          t.start();
          diff->report(cerr);
          t.stop();
          if (opts.do_log)
            emit_prefix(argv[0], cerr)
              << "emitted report in: " << t << "\n";
          return  abigail::tools_utils::ABITOOL_ERROR;
        }
      return abigail::tools_utils::ABITOOL_OK;
    }

#ifdef WITH_DEBUG_SELF_COMPARISON
  if (opts.debug_abidiff
      && !opts.type_id_file_path.empty())
    remove(opts.type_id_file_path.c_str());
#endif

  if (opts.noout)
    return abigail::tools_utils::ABITOOL_OK;

  if (!opts.out_file_path.empty())
    {
      ofstream of(opts.out_file_path.c_str(), std::ios_base::trunc);
      if (!of.is_open())
        {
          emit_prefix(argv[0], cerr)
            << "could not open output file '"
            << opts.out_file_path << "'\n";
          return abigail::tools_utils::ABITOOL_ERROR;
        }
      set_ostream(*write_ctxt, of);
      t.start();
      write_corpus(*write_ctxt, corp, 0);
      t.stop();
      if (opts.do_log)
        emit_prefix(argv[0], cerr)
          << "emitted abixml output in: " << t << "\n";
      of.close();
      return abigail::tools_utils::ABITOOL_OK;
    }
  else
    {
      t.start();
      exit_code = write_corpus(*write_ctxt, corp, 0)
	? abigail::tools_utils::ABITOOL_OK:abigail::tools_utils::ABITOOL_ERROR;
      t.stop();
      if (opts.do_log)
        emit_prefix(argv[0], cerr)
          << "emitted abixml out in: " << t << "\n";
    }

  return exit_code;
}

/// Load a corpus group representing the union of a Linux Kernel
/// vmlinux binary and its modules, and emit an abixml representation
/// for it.
///
/// @param argv the arguments this program was called with.
///
/// @param opts the options this program was created with.
///
/// @return the exit code.  Zero if everything went well, non-zero
/// otherwise.
static abigail::tools_utils::abidiff_status
load_kernel_corpus_group_and_write_abixml(char* argv[],
					  options& opts)
{
  if (!(tools_utils::is_dir(opts.reader_opts.elf_file_path)
	&& opts.corpus_group_for_linux))
    return abigail::tools_utils::ABITOOL_ERROR;

  auto exit_code =  abigail::tools_utils::ABITOOL_OK;

  if (!opts.vmlinux.empty())
    if (!abigail::tools_utils::check_file(opts.vmlinux, cerr, argv[0]))
      return abigail::tools_utils::ABITOOL_ERROR;

  timer t, global_timer;
  suppressions_type supprs;

  if (opts.exported_interfaces_only.has_value())
    opts.env.analyze_exported_interfaces_only(*opts.exported_interfaces_only);

  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "going to build ABI representation of the Linux Kernel ...\n";

  global_timer.start();
  t.start();
  corpus::origin requested_fe_kind =
#ifdef WITH_CTF
    opts.use_ctf ? corpus::CTF_ORIGIN :
#endif
    corpus::DWARF_ORIGIN;
  corpus_group_sptr group =
    build_corpus_group_from_kernel_dist_under(opts.reader_opts.elf_file_path,
					      /*debug_info_root=*/"",
					      opts.vmlinux,
					      opts.suppression_paths,
					      opts.kabi_whitelist_paths,
					      supprs, opts.do_log,
					      opts.env,
					      requested_fe_kind);
  t.stop();

  if (opts.do_log)
    {
      emit_prefix(argv[0], cerr)
	<< "built ABI representation of the Linux Kernel in: "
	<< t << "\n";
    }

  if (!group)
    return abigail::tools_utils::ABITOOL_ERROR;

  if (!opts.noout)
    {
      const xml_writer::write_context_sptr& ctxt
	  = xml_writer::create_write_context(opts.env, cout);
      set_common_options(*ctxt, opts);

      if (!opts.out_file_path.empty())
	{
	  ofstream of(opts.out_file_path.c_str(), std::ios_base::trunc);
	  if (!of.is_open())
	    {
	      emit_prefix(argv[0], cerr)
		<< "could not open output file '"
		<< opts.out_file_path << "'\n";
	      return abigail::tools_utils::ABITOOL_ERROR;
	    }

	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitting the abixml output ...\n";
	  set_ostream(*ctxt, of);
	  t.start();
	  exit_code = write_corpus_group(*ctxt, group, 0)
	    ? abigail::tools_utils::ABITOOL_OK
	    : abigail::tools_utils::ABITOOL_ERROR;
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml output in: " << t << "\n";
	}
      else
	{
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitting the abixml output ...\n";
	  t.start();
	  exit_code = write_corpus_group(*ctxt, group, 0)
	    ? abigail::tools_utils::ABITOOL_OK
	    : abigail::tools_utils::ABITOOL_ERROR;
	  t.stop();
	  if (opts.do_log)
	    emit_prefix(argv[0], cerr)
	      << "emitted abixml output in: " << t << "\n";
	}
    }

  global_timer.stop();
  if (opts.do_log)
    emit_prefix(argv[0], cerr)
      << "total processing done in " << global_timer << "\n";
  return exit_code;
}

int
main(int argc, char* argv[])
{
  options opts;
  if (!parse_command_line(argc, argv, opts))
    {
      if (opts.missing_operand || !opts.wrong_option.empty())
	{
	  if (!opts.wrong_option.empty())
	    emit_prefix(argv[0], cerr)
	      << "unrecognized option: " << opts.wrong_option << "\n";
	  else if (opts.missing_operand)
	    emit_prefix(argv[0], cerr)
	      << "missing operand to option: " << opts.wrong_option <<"\n";
	  cerr << "try the --help option for more information\n";
	}
      else
	display_usage(argv[0], cerr);

      return (abigail::tools_utils::ABITOOL_USAGE_ERROR |
	      abigail::tools_utils::ABITOOL_ERROR);
    }

  auto exit_code = abigail::tools_utils::ABITOOL_OK;

  switch(abigail::tools_utils::guess_file_type(opts.reader_opts.elf_file_path))
    {
    case abigail::tools_utils::FILE_TYPE_ELF:
    case abigail::tools_utils::FILE_TYPE_AR:
      exit_code = load_corpus_and_write_abixml(argv, opts);
      break;
    case abigail::tools_utils::FILE_TYPE_DIR:
      exit_code = load_kernel_corpus_group_and_write_abixml(argv, opts);
      break;
    default:
      emit_prefix(argv[0], cerr)
	<< "files of the kind of "<< opts.reader_opts.elf_file_path
	<< " are not handled\n";
      exit_code = abigail::tools_utils::ABITOOL_ERROR;
    }

  return exit_code;
}
