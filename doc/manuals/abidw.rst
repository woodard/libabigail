======
abidw
======

abidw reads a shared library in `ELF`_ format and emits an XML
representation of its ABI to standard output.  The emitted
representation includes all the globally defined functions and
variables, along with a complete representation of their types.  It
also includes a representation of the globally defined ELF symbols of
the file.  The input shared library must contain associated debug
information in `DWARF`_ format.

Invocation
==========

::

  abidw [options] [<path-to-elf-file>]

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--debug-info-dir | -d`` <*dir-path*>

    In cases where the debug info for *path-to-elf-file* is in a
    separate file that is located in a non-standard place, this tells
    ``abidw`` where to look for that debug info file.

    Note that *dir-path* must point to the root directory under which
    the debug information is arranged in a tree-like manner.  Under
    Red Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

    Note that this option is not mandatory for split debug information
    installed by your system's package manager because then
    ``abidw`` knows where to find it.

  * ``--out-file`` <*file-path*>

    This option instructs ``abidw`` to emit the XML representation of
    *path-to-elf-file* into the file *file-path*, rather than emitting
    it to its standard output.

  * ``--noout``

    This option instructs ``abidw`` to not emit the XML representation
    of the ABI.  So it only reads the ELF and debug information,
    builds the internal representation of the ABI and exits.  This
    option is usually useful for debugging purposes.

  * ``--suppressions | suppr`` <*path-to-suppression-specifications-file*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppression-specifications-file*.  Note that
    this option can appear multiple times on the command line.  In
    that case, all of the provided suppression specification files are
    taken into account.  ABI artifacts matched by the suppression
    specifications are suppressed from the output of this tool.

  * ``--headers-dir | --hd`` <headers-directory-path-1>

    Specifies where to find the public headers of the first shared
    library that the tool has to consider.  The tool will thus filter
    out types that are not defined in public headers.

  * ``--no-linux-kernel-mode``

    Without this option, if abipkgiff detects that the binaries it is
    looking at are Linux Kernel binaries (either vmlinux or modules)
    then it only considers functions and variables which ELF symbols
    are listed in the __ksymtab and __ksymtab_gpl sections.

    With this option, abipkgdiff considers the binary as a non-special
    ELF binary.  It thus considers functions and variables which are
    defined and exported in the ELF sense.

  * ``--check-alternate-debug-info`` <*elf-path*>

    If the debug info for the file *elf-path* contains a reference to
    an `alternate debug info <alt-di-label>`_ file, ``abidw`` checks
    that it can find that alternate debug info file.  In that case, it
    emits a meaningful success message mentioning the full path to the
    alternate debug info file found.  Otherwise, it emits an error
    code.

  * ``--no-show-locs``

   Do not show information about where in the *second shared library*
   the respective type was changed.

  * ``--check-alternate-debug-info-base-name`` <*elf-path*>


    Like ``--check-alternate-debug-info``, but in the success message,
    only mention the base name of the debug info file; not its full path.

  * ``--load-all-types``

    By default, ``libabigail`` (and thus ``abidw``) only loads types
    that are reachable from functions and variables declarations that
    are publicly defined and exported by the binary.  So only those
    types are present in the output of ``abidw``.  This option however
    makes ``abidw`` load *all* the types defined in the binaries, even
    those that are not reachable from public declarations.

  *  ``--abidiff``

    Load the ABI of the ELF binary given in argument, save it in
    libabigail's XML format in a temporary file; read the ABI from the
    temporary XML file and compare the ABI that has been read back
    against the ABI of the ELF binary given in argument.  The ABIs
    should compare equal.  If they don't, the program emits a
    diagnostic and exits with a non-zero code.

    This is a debugging and sanity check option.
 
  * ``--stats``

    Emit statistics about various internal things.

  * ``--verbose``

    Emit verbose logs about the progress of miscellaneous internal
    things.

Notes
=====

.. _alt-di-label:

Alternate debug info files
--------------------------

As of the version 4 of the DWARF specification, `Alternate debug
information <http://www.dwarfstd.org/ShowIssue.php?issue=120604.1>`_
is a `GNU`_ extension to the DWARF specification.  It has however been
proposed for inclusion into the upcoming version 5 of the DWARF
standard.  You can read more about the GNU extensions to the DWARF
standard `here
<https://fedorahosted.org/elfutils/wiki/DwarfExtensions>`_.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _DWARF: http://www.dwarfstd.org
.. _GNU: http://www.gnu.org

