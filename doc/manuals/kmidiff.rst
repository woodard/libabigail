.. _kmidiff_label:

===========
kmidiff
===========

``kmidiff`` compares the binary Kernel Module Interfaces of two Linux
Kernel trees.  The binary KMI is the interface that the Linux Kernel
exposes to its modules.  The trees we are interested in here are the
result of the build of the Linux Kernel source tree.


General approach
=================

And example of how to build your kernel if you want to compare it to
another one using kmidiff is: ::

    git clone -b v4.5 git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux/v4.5
    cd linux/v4.5
    make allyesconfig all

Then install the modules into a directory, for instance, the
build/modules sub-directory of the your kernel source tree: ::

    mkdir build/modules
    make modules_install INSTALL_MOD_DIR=build/modules


Then construct a list of interfaces exported by the kernel, that you
want to compare: ::

    cat > kmi-whitelist << EOF
    [kernel_4.5_kmi_whitelist]
     init_task
     schedule
     dev_queue_xmit
     __kmalloc
     printk
    EOF

Suppose you've done something similar for the v4.6 branch of the Linux
kernel, you now have these two directories: ``linux/v4.5`` and ``linux/v4.6``.
Their modules are present under the directories
``linux/v4.5/build/modules`` and ``linux/v4.6/build/modules``.

To Comparing their KMI ``kmidiff`` needs to know where to find the
vmlinux binaries and their associated modules.  Here would be what the
command line looks like: ::

    kmidiff                                     \
      --kmi-whitelist  linux/v4.6/kmi-whitelist \
      --vmlinux1       linux/v4.5/vmlinux       \
      --vmlinux2       linux/v4.6/vmlinux       \
		       linux/v4.5/build/modules \
		       linux/v4.6/build/modules


.. include:: tools-use-libabigail.txt


Invocation
==========

More generally, ``kmidiff`` is invoked under the form: ::

    kmidiff [options] <first-modules-dir> <second-modules-dir>

Environment
===========

By default, ``kmidiff`` compares all the interfaces (exported
functions and variables) between the Kernel and its modules.  In
practice, though, some users might want to compare a subset of the
those interfaces.

By default, ``kmidiff`` uses debug information in the `DWARF`_ debug
info format, if present, otherwise it compares interfaces using `CTF`_
or `BTF`_ debug info formats, if present.  Finally, if no debug info
in these formats is found, it only considers `ELF`_ symbols and report
about their addition or removal.

Users can then define a "white list" of the interfaces to compare.
Such a white list is a just a file in the "INI" format that looks
like: ::

    [kernel_version_x86_64_whitelist]
      function1_name
      function2_name
      global_variable1_name
      ....


Note that the name of the section (the name that is between the two
brackets) of that INI file just has to end with the string
"whitelist".  So you can define the name you want, for instance
``[kernel_46_x86_64_whitelist]``.

Then each line of that whitelist file is the name of an exported
function or variable.  Only those interfaces along with the types
reachable from their signatures are going to be compared by
``kmidiff`` recursively.

Note that by default kmidiff analyzes the types reachable from the
interfaces associated with `ELF`_ symbols that are defined and
exported by the `Linux Kernel`_ as being the union of the ``vmlinux``
binary and all its compiled modules.  It then compares those
interfaces (along with their types).

Options
=======

  * ``--help | -h``

    Display a short help about the command and exit.


  * ``--version | -v``

    Display the version of the program and exit.

  * ``--verbose``

    Display some verbose messages while executing.

  * ``--debug-info-dir1 | --d1`` <*di-path1*>

    For cases where the debug information for the binaries of the
    first Linux kernel is split out into separate files, tells
    ``kmidiff`` where to find those separate debug information files.

    Note that *di-path* must point to the root directory under which
    the debug information is arranged in a tree-like manner.  Under
    Red Hat based systems, that directory is usually
    ``<root>/usr/lib/debug``.

  * ``--debug-info-dir2 | --d2`` <*di-path2*>

    Like ``--debug-info-dir1``, this options tells ``kmidiff`` where
    to find the split debug information for the binaries of the second
    Linux kernel.

  * ``--vmlinux1 | --l1`` <*path-to-first-vmlinux*>

    Sets the path to the first ``vmlinux`` binary to consider.  This
    has to be the uncompressed vmlinux binary compiled with debug
    info.

  * ``--vmlinux2 | --l2`` <*path-to-first-vmlinux*>

    Sets the path to the second ``vmlinux`` binary to consider.  This
    has to be the uncompressed vmlinux binary compiled with debug
    info.

  * ``--kmi-whitelist | -w`` <*path-to-interface-whitelist*>

    Set the path to the white list of interfaces to compare while
    comparing the Kernel Module Interface of the first kernel against
    the one of the second kernel.

    If this option is not provided, *all* the exported interfaces of
    the two kernels are compared.  That takes a lot of times and is
    not necessarily meaningful because many interface are probably
    meant to see their reachable types change.

    So please, make sure you always use this option unless you really
    know what you  are doing.

  * ``--suppressions | --suppr`` <*path-to-suppressions*>

    Use a :ref:`suppression specification <suppr_spec_label>` file
    located at *path-to-suppressions*.  Note that this option can
    appear multiple times on the command line.  In that case, all of
    the provided suppression specification files are taken into
    account.

    Please note that, by default, if this option is not provided, then
    the :ref:`default suppression specification files
    <abidiff_default_supprs_label>` are loaded .

  * ``--no-change-categorization | -x``

    This option disables the categorization of changes into harmless
    and harmful changes.  Note that this categorization is a
    pre-requisite for the filtering of changes so this option disables
    that filtering.  The goal of this option is to speed-up the
    execution of the program for cases where the graph of changes is
    huge and where the user is just interested in looking at, for
    instance, leaf node changes without caring about their possible
    impact on interfaces.

  * ``--ctf``

    Extract ABI information from `CTF`_ debug information, if present,
    in the Kernel and Modules.

  * ``--btf``

    Extract ABI information from `BTF`_ debug information, if present,
    in the Kernel and Modules.

  * ``--impacted-interfaces | -i``

    Tell what interfaces got impacted by each individual ABI change.

  * ``--full-impact | -f``

    Emit a change report that shows the full impact of each change on
    exported interfaces.  This is the default kind of report emitted
    by tools like ``abidiff`` or ``abipkgdiff``.

  * ``--exported-interfaces-only``

    When using this option, this tool analyzes the descriptions of the
    types reachable by the interfaces (functions and variables)
    associated with `ELF`_ symbols that are defined and exported by
    the `Linux Kernel`_.

    Otherwise, the tool also has the ability to analyze the
    descriptions of the types reachable by the interfaces associated
    with `ELF`_ symbols that are visible outside their translation
    unit.  This later possibility is however much more resource
    intensive and results in much slower operations.

    That is why this option is enabled by default.


  * ``--allow-non-exported-interfaces``

    When using this option, this tool analyzes the descriptions of the
    types reachable by the interfaces (functions and variables) that
    are visible outside of their translation unit.  Once that analysis
    is done, an ABI Corpus is constructed by only considering the
    subset of types reachable from interfaces associated to `ELF`_
    symbols that are defined and exported by the binary.  It's that
    final ABI corpus which is compared against another one.

    The problem with that approach however is that analyzing all the
    interfaces that are visible from outside their translation unit
    can amount to a lot of data, leading to very slow operations.

    Note that this option is turned off by default.

  * ``--show-bytes``

    Show sizes and offsets in bytes, not bits.  This option is
    activated by default.

  * ``--show-bits``

    Show sizes and offsets in bits, not bytes.  By default, sizes and
    offsets are shown in bytes.

  * ``--show-hex``

    Show sizes and offsets in hexadecimal base.  This option is
    activated by default.

  * ``--show-dec``

    Show sizes and offsets in decimal base.


.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _ksymtab: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format
.. _Linux Kernel: https://kernel.org
.. _DWARF: http://www.dwarfstd.org
.. _CTF: https://raw.githubusercontent.com/wiki/oracle/binutils-gdb/files/ctf-spec.pdf
.. _BTF: https://docs.kernel.org/bpf/btf.html
