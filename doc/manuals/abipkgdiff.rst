.. _abipkgdiff_label:

===========
abipkgdiff
===========

``abipkgdiff`` compares the Application Binary Interfaces (ABI) of the
`ELF`_ binaries contained in two software packages.  The format of
software package that is currently supported is `RPM`_.

For a comprehensive ABI change report that includes changes about
function and variable sub-types, the two input packages must be
accompanied with their debug information packages that contain debug
information in `DWARF`_ format.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _RPM: https://en.wikipedia.org/wiki/RPM_Package_Manager
.. _DWARF: http://www.dwarfstd.org


.. _abipkgdiff_invocation_label:

Invocation
=========

::

  abipkgdiff [option] <package1> <package2>

.. _abipkgdiff_options_label:

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.

  * ``--debug-info-pkg1 | --d1`` <path>

    For cases where the debug information for *package1* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

  * ``--debug-info-pkg2 | --d2`` <path>

    For cases where the debug information for *package2* is split out
    into a separate file, tells ``abipkgdiff`` where to find that
    separate debug information package.

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

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line; all the suppression
    specification files are then taken into account.

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

