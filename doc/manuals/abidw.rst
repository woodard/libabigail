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

  * --help

    Display a short help about the command and exit.

  * --debug-info-dir <*dir-path*>

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

  * --out-file <*file-path*>

    This option instructs ``abidw`` to emit the XML representation of
    *path-to-elf-file* into the file *file-path*, rather than emitting
    it to its standard output.

  * --check-alternate-debug-info <*elf-path*>

    If the debug info for the file *elf-path* contains a reference to
    an `alternate debug info <alt-di-label>`_ file, ``abidw`` checks
    that it can find that alternate debug info file.  In that case, it
    emits a meaningful success message mentioning the full path to the
    alternate debug info file found.  Otherwise, it emits an error
    code.

  * --check-alternate-debug-info-base-name <*elf-path*>


    Like ``--check-alternate-debug-info``, but in the success message,
    only mention the base name of the debug info file; not its full path.


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

