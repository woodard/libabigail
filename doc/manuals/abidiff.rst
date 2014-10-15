.. _abidiff_label:

=======
abidiff
=======

abidiff compares the Application Binary Interfaces (ABI) of two shared
libraries in `ELF`_ format.  It emits a meaningful report describing the
differences between the two ABIs.

The two input shared libraries must be accompanied with their debug
information in `DWARF`_ format.

Invocation
==========

::

  abidiff [options] <first-shared-library> <second-shared-library>

Options
=======

  * --help

    Displays a short help about the command and exit.

  * --debug-info-dir1 <*di-path1*>

    For cases where the debug information for *first-shared-library*
    is split out into a separate file, tells ``abidiff`` where to find
    that separate debug information file.

    Note that *di-path* must point to the root directory under which
    the debug information is arranged in a tree-like manner.  Under
    Red Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

    Note also that this option is not mandatory for split debug
    information installed by your system's package manager because
    then ``abidiff`` knows where to find it.

  * --debug-info-dir2 <*di-path2*>

    Like ``--debug-info-dir1``, this options tells ``abidiff`` where
    to find the split debug information for the
    *second-shared-library* file.

  * --stat

    Rather than displaying the detailed ABI differences between
    *first-shared-library* and *second-shared-library*, just display
    some summary statistics about these differences.

  * --symtabs

    Only display the symbol tables of the *first-shared-library* and
    *second-shared-library*.

  * --deleted-fns

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the globally defined functions that got deleted from
    *first-shared-library*.

  * --changed-fns

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the changes in sub-types of the global functions defined in
    *first-shared-library*.

  * --added-fns

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the globally defined functions that were added to
    *second-shared-library*.

  * --deleted-vars

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the globally defined variables that were deleted from
    *first-shared-library*.

  * --changed-vars

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the changes in the sub-types of the global variables defined in
    *first-shared-library*

  * --added-vars

    In the resulting report about the differences between
    *first-shared-library* and *second-shared-library*, only display
    the global variables that were added (defined) to
    *second-shared-library*.

  * --no-linkage-name 

    In the resulting report, do not display the linkage names of
    the added, removed, or changed functions or variables.


  * --suppressions <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file located
    at *path-to-suppressions*.

  * --drop <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, drop the globally defined
    functions and variables which name match the regular expression
    *regex*.  As a result, no change involving these functions or
    variables will be emitted in the diff report.

  * --drop-fn <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, drop the globally defined
    functions which name match the regular expression *regex*.  As a
    result, no change involving these functions will be emitted in the
    diff report.

  * --drop-fn <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, drop the globally defined
    functions matching a the regular expression *regex*.  As a result,
    no change involving these functions will be emitted in the diff
    report.

  * --drop-var <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, drop the globally defined
    variables matching a the regular expression *regex*.

  * --keep <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, keep the globally defined
    functions and variables which names match the regular expression
    *regex*.  All other functions and variables are dropped on the
    floor and will thus not appear in the resulting diff report.

  * --keep-fn <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, keep the globally defined
    functions which name match the regular expression *regex*.  All
    other functions are dropped on the floor and will thus not appear
    in the resulting diff report.

  * --keep-var <*regex*>

    When reading the *first-shared-library* and
    *second-shared-library* ELF input files, keep the globally defined
    which names match the regular expression *regex*.  All other
    variables are dropped on the floor and will thus not appear in the
    resulting diff report.

  * --harmless

    In the diff report, display only the :ref:`harmless
    <harmlesschangeconcept_label>` changes.  By default, the harmless
    changes are filtered out of the diff report keep the clutter to a
    minimum and have a greater change to spot real ABI issues.

  * --no-harmful

    In the diff report, do not display the :ref:`harmful
    <harmfulchangeconcept_label>` changes.  By default, only the
    harmful changes are displayed in diff report.

  * --redundant

    In the diff report, do not display redundant changes.  A redundant
    change is a change that has been displayed elsewhere in the report.

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _DWARF: http://www.dwarfstd.org

