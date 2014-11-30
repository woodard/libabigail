.. _abicompat_label:

=========
abicompat
=========

abicompat checks that an application that links against a given shared
library is still ABI compatible with a subsequent version of that
library.  If the new version of the library introduces an ABI
incompatibility, then abicompat hints the user at what exactly that
incompatibility is.

Invocation
==========

::

  abicompat [options] [<application> <shared-library-first-version> <shared-library-second-version>]

Options
=======

  * --help

    Displays a short help about the command and exit.

  * --list-undefined-symbols|-u

    Display the list of undefined symbols of the application and exit.

  * --show-base-names|-b

    In the resulting report emitted by the tool, this option makes the
    application and libraries be referred to by their base names only;
    not by a full absolute name.  This can be useful for use in
    scripts that wants to compare names of the application and
    libraries independently of what their directory names are.

  * --app-debug-info-dir <path-to-app-debug-info-directory>

    Set the path to the directory under which the debug information of
    the application is supposed to be laid out.  This is useful for
    application binaries for which the debug info is in a separate set
    of files.

  * --lib-debug-info-dir1 <path-to-lib1-debug-info>

    Set the path to the directory under which the debug information of
    the first version of the shared library is supposed to be laid
    out.  This is useful for shared library binaries for which the
    debug info is in a separate set of files.

  * --lib-debug-info-dir2 <path-to-lib1-debug-info>

    Set the path to the directory under which the debug information of
    the second version of the shared library is supposed to be laid
    out.  This is useful for shared library binaries for which the
    debug info is in a separate set of files.

Usage examples
==============

  * Detecting a possible ABI incompatibility in a new shared library
    version: ::

	$ cat -n test-app.cc
	     1	// Compile with:
	     2	//  g++ -g -Wall -o test-app -L. -ltest-0 test-app.cc
	     3	
	     4	#include "test0.h"
	     5	
	     6	int
	     7	main()
	     8	{
	     9	  foo* f = first_func();
	    10	  second_func(*f);
	    11	  return 0;
	    12	}
	$
	$ cat -n test0.cc
	     1	// Compile this with:
	     2	//  g++ -g -Wall -shared -o libtest-0.so test0.cc
	     3	
	     4	#include "test0.h"
	     5	
	     6	foo*
	     7	first_func()
	     8	{
	     9	  return 0;
	    10	}
	    11	
	    12	void
	    13	second_func(foo&)
	    14	{
	    15	}
	    16	
	    17	void
	    18	third_func()
	    19	{
	    20	}
	$
	$ cat -n test1.cc
	     1	// Compile this with:
	     2	//  g++ -g -Wall -shared -o libtest-1.so test1.cc
	     3	
	     4	#include "test1.h"
	     5	
	     6	foo*
	     7	first_func()
	     8	{
	     9	  return 0;
	    10	}
	    11	
	    12	void
	    13	second_func(foo&)
	    14	{
	    15	}
	    16	
	    17	/* Let's comment out the definition of third_func()
	    18	    void
	    19	    third_func()
	    20	    {
	    21	    }
	    22	*/
	$ 


    * Compile the first and the second versions of the libraries:
      ``libtest-0.so`` and ``libtest-1.so``: ::

	$ g++ -g -Wall -shared -o libtest-0.so test0.cc
	$ g++ -g -Wall -shared -o libtest-1.so test1.cc

    * Compile the application and link it against the first version of
      the library, creating the ``test-app`` binary: ::

	$ g++ -g -Wall -o test-app -L. -ltest-0.so test-app.cc

    * Now, use ``abicompat`` to see if libtest-1.so is ABI compatible
      with app, with the respect to the ABI of libtest-0.so: ::

	$ abicompat test-app libtest-0.so libtest-1.so
	ELF file 'test-app' might not be ABI compatible with 'libtest-1.so' due to differences with 'libtest-0.so' below:
	Functions changes summary: 0 Removed, 1 Changed (1 filtered out), 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function foo* first_func()' has some indirect sub-type changes:
	    return type changed:
	      in pointed to type 'struct foo':
		size changed from 32 to 64 bits
		1 data member insertion:
		  'char foo::member_1', at offset 32 (in bits)

	$

