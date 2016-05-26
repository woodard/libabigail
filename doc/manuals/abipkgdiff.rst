.. _abipkgdiff_label:

===========
abipkgdiff
===========

``abipkgdiff`` compares the Application Binary Interfaces (ABI) of the
`ELF`_ binaries contained in two software packages.  The software
package formats currently supported are `Deb`_, `RPM`_, `tar`_
archives (either compressed or not) and plain directories that contain
binaries.

For a comprehensive ABI change report that includes changes about
function and variable sub-types, the two input packages must be
accompanied with their debug information packages that contain debug
information in `DWARF`_ format.


.. _abipkgdiff_invocation_label:

Invocation
==========

::

  abipkgdiff [option] <package1> <package2>

``package1`` and ``package2`` are the packages that contain the
binaries to be compared.

.. _abipkgdiff_options_label:

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * `--version | -v`

    Display the version of the program and exit.

  * ``--debug-info-pkg1 | --d1`` <path>

    For cases where the debug information for *package1* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

  * ``--debug-info-pkg2 | --d2`` <path>

    For cases where the debug information for *package2* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

  * ``--devel-pkg1 | --devel1`` <path>

    Specifies where to find the `Development Package`_ associated with
    the first package to be compared.  That `Development Package`_ at
    ``path`` should at least contain header files in which public
    types exposed by the libraries (of the first package to be
    compared) are defined.  When this option is provided, the tool
    filters out reports about ABI changes to types that are *NOT*
    defined in these header files.

  * ``--devel-pkg2 | --devel2`` <path>

    Specifies where to find the `Development Package`_ associated with
    the second package to be compared.  That `Development Package`_ at
    ``path`` should at least contains header files in which public
    types exposed by the libraries (of the second package to be
    compared) are defined.  When this option is provided, the tool
    filters out reports about ABI changes to types that are *NOT*
    defined in these header files.

  * ``--dso-only``

    Compare ELF files that are shared libraries, only.  Do not compare
    executable files, for instance.

  *  ``--redundant``

    In the diff reports, do display redundant changes.  A redundant
    change is a change that has been displayed elsewhere in a given
    report.

  * ``--no-linkage-name``

    In the resulting report, do not display the linkage names of
    the added, removed, or changed functions or variables.

  * ``--no-added-syms``

    Do not show the list of functions, variables, or any symbol that
    was added.

  * ``--no-added-binaries``

    Do not show the list of binaries that got added to the second
    package.

    Please note that the presence of such added binaries is not
    considered like an ABI change by this tool; as such, it doesn't
    have any impact on the exit code of the tool.  It does only have
    an informational value.  Removed binaries are, however, considered
    as an ABI change.

  * ``--no-abignore``

    Do not search the *package2* for the presence of suppression
    files.

  * ``--no-parallel``

    By default, ``abipkgdiff`` will use all the processors it has available to
    execute concurrently.  This option tells it not to extract packages or run
    comparisons in parallel.

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line.  In that case, all of
    the suppression specification files are taken into account.

  * ``--no-show-locs``

   Do not show information about where in the *second shared library*
   the respective type was changed.

  * ``--fail-no-dbg``

    Make the program fail and return a non-zero exit code if couldn't
    read any of the debug information that comes from the debug info
    packages that were given on the command line.  If no debug info
    package were provided on the command line then this option is not
    active.

    Note that the non-zero exit code returned by the program as a
    result of this option is the constant ``ABIDIFF_ERROR``.  To know
    the numerical value of that constant, please refer to the
    :ref:`exit code documentation <abidiff_return_value_label>`.

  * ``--keep-tmp-files``

    Do not erase the temporary directory files that are created during
    the execution of the tool.

  * ``--verbose``

    Emit verbose progress messages.

.. _abipkgdiff_return_value_label:

Return value
============

The exit code of the ``abipkgdiff`` command is either 0 if the ABI of
the binaries compared are equal, or non-zero if they differ or if the
tool encountered an error.

In the later case, the value of the exit code is the same as for the
:ref:`abidiff tool <abidiff_return_value_label>`.


.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _RPM: https://en.wikipedia.org/wiki/RPM_Package_Manager
.. _Deb: https://en.wikipedia.org/wiki/Deb_%28file_format%29
.. _tar: https://en.wikipedia.org/wiki/Tar_%28computing%29
.. _DWARF: http://www.dwarfstd.org
.. _Development Package: https://fedoraproject.org/wiki/Packaging:Guidelines?rd=Packaging/Guidelines#Devel_Packages
