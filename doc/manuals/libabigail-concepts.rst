#########
Concepts
#########

.. _abi_artifacts_label:

ABI artifacts
=============

An ABI artifact is a relevant part of the ABI of a shared library or
program.  Examples of ABI artifacts are exported types, variables,
functions, or `ELF`_ symbols exported by a shared library.

.. _harmfulchangeconcept_label:

Harmful changes
===============

A change in the diff report is considered harmful if it might cause
ABI compatibility issues.  That is, it might prevent an application
dynamically linked against a given version of a library to keep
working with the changed subsequent versions of the same library.

.. _harmlesschangeconcept_label:

Harmless changes
================

A change in the diff report is considered harmless if it will not
cause any ABI compatibility issue.  That is, it will not prevent an
application dynamically linked against given version of a library to
keep working with the changed subsequent versions of the same library.

By default, ``abidiff`` filters harmless changes from the diff report.

.. _suppr_spec_label:

Suppression specifications
==========================

  * Definition

 A suppression specification file is a way for a user to instruct
 :ref:`abidiff <abidiff_label>` to avoid emitting reports for changes
 involving certain :ref:`ABI artifacts<abi_artifacts_label>`.

 It contains directives (or specifications) that describe the set of
 ABI artifacts to avoid emitting change reports about.

  * Introductory examples

 Its syntax is based on a simplified form of `Ini File Syntax`_.  For
 instance, specify that change reports on a type named FooPrivateType
 should be suppressed, one could write this suppression specification:
 ::

    [suppress_type]
      name = FooPrivateType

 Maybe we want to ensure that only a change reports about structures
 named FooPrivateType should be suppressed, one could write: ::

    [suppress_type]
      type_kind = struct
      name = FooPrivateType

 But we could also want to suppress change reports avoid typedefs named
 FooPrivateType.  In that case we would write:  ::

    [suppress_type]
      type_kind = typedef
      name = FooPrivateType

 Or, we could want to suppress change reports about all struct which
 names ends with PrivateType: ::

    [suppress_type]
      type_kind = struct
      name_regexp = ^.*PrivateType

 Let's now look at the generic syntax of suppression specification
 files.

  * Syntax

    * Properties

     More generally, the format of suppression lists is organized
     around the concept of `property`.  Every property has a name and
     a value, delimited by the ``=`` sign.  E.g: ::

	 name = value

     Leading and trailing white spaces are ignored around property
     name and values.

.. _suppr_regexp_label:

    * Regular expressions

      The value of some properties might be a regular expression.  In
      that case, they must comply with the syntax of `extended POSIX
      regular expressions
      <http://www.gnu.org/software/findutils/manual/html_node/find_html/posix_002dextended-regular-expression-syntax.html#posix_002dextended-regular-expression-syntax>`_.
      Note that Libabigail uses the regular expression engine of the
      `GNU C Library`_.

    * Escaping a character in a regular expression

      When trying to match a string that contains a ``*`` character,
      like in the pointer type ``int*``, one must be careful to notice
      that the character ``*`` is a special character in the extended
      POSIX regular expression syntax.  And that character must be
      escaped for the regular expression engine.  Thus the regular
      expression that would match the string ``int*`` in a suppression
      file should be ::

        int\\*

      Wait; but then why the two ``\`` characters?  Well, because the
      ``\`` character is a special character in the `Ini File Syntax`_
      used for specifying suppressions.  So it must be escaped as
      well, so that the Ini File parser leaves a ``\`` character
      intact in the data stream that is handed to the regular
      expression engine.  Hence the ``\\`` targeted at the Ini File
      parser.

      So, in short, to escape a character in a regular expression,
      always prefix the character with the ``\\`` sequence.

    * Sections

     Properties are then grouped into arbitrarily named sections that
     shall not be nested.  The name of the section is on a line by
     itself and is surrounded by square brackets, i.e: ::

	 [section_name]
	 property1_name = property1_value
	 property2_name = property2_value


     A section might or might not have properties.  Sections that
     expect having properties and which are found nonetheless empty
     are just ignored.  Properties that are not recognized by the
     reader are ignored as well.

    * Section names

      * ``[suppress_type]``

	Suppresses report messages about a type change.  The potential
	properties of this sections are:

	  * ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	     Suppresses change reports involving types whose name
	     matches the regular expression specified as value of this
	     property.

	  * ``name`` ``=`` <a-value>

	    Suppresses change reports involving types whose name
	    equals the value of this property.

	  * ``type_kind`` ``=`` ``class`` | ``struct`` | ``union`` |
	    ``enum`` | ``array`` | ``typedef`` | ``builtin``

	    Suppresses change reports involving a certain kind of
	    type.  The kind of type to suppress change reports for is
	    specified by the possible values listed above:

	      - ``class``: suppress change reports for class types.
		Note that even if class types don't exist for C, this
		value still triggers the suppression of change reports
		for struct types, in C.  In C++ however, it should do
		what it suggests.

	      - ``struct``: suppress change reports for struct types
		in C or C++.  Note that the value ``class`` above is a
		super-set of this one.

	      - ``union``: suppress change reports for union types.

	      - ``enum``: suppress change reports for enum types.

	      - ``array``: suppress change reports for array types.

	      - ``typedef``: suppress change reports for typedef types.

	      - ``builtin``: suppress change reports for built-in (or
		native) types.  Example of built-in types are char,
		int, unsigned int, etc.

.. _suppr_label_property_label:

	  * ``label`` ``=`` <some-value>

	    Define a label for the section.  A label is just an
	    informative string that might be used by abidiff to refer
	    to a type suppression in error messages.

      * ``[suppress_function]``

	Suppresses report messages about changes on sub-types of a
	function.  The potential properties of this sections are:

	  * ``label`` ``=`` <some-value>

            This property is the same as the :ref:`label property
            <suppr_label_property_label>` defined above.

	  *  ``name`` ``=`` <some-value>

	    Suppresses change reports involving functions whose name
	    equals the value of this property.

	  *  ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving functions whose name
	    matches the regular expression specified as value of this
	    property.

          * ``parameter`` ``=`` <function-parameter-specification>

	    Suppresses change reports involving functions whose
	    parameters match the parameter specification indicated as
	    value of this property.

	    The format of the function parameter specification is:

	    ``'`` ``<parameter-index>`` ``<space>`` ``<type-name-or-regular-expression>``

	    That is, an apostrophe followed by a number that is the
	    index of the parameter, followed by one of several spaces,
	    followed by either the name of the type of the parameter,
	    or a regular expression describing a family of parameter
	    type names.

	    If the parameter type name is designated by a regular
	    expression, then said regular expression must be enclosed
	    between two slashes; like ``/some-regular-expression/``.

	    The index of the first parameter of the function is zero.
	    Note that for member functions (methods of classes), the
	    this is the first parameter that comes after the implicit
	    "this" pointer parameter.

	    Examples of function parameter specifications are: ::

	      '0 int

            Which means, the parameter at index 0, whose type name is
            ``int``. ::

	      '4 unsigned char*

	    Which means, the parameter at index 4, whose type name is
	    ``unsigned char*``.  ::

	      '2 /^foo.*&/

	    Which means, the parameter at index 2, whose type name
	    starts with the string "foo" and ends with an '&'.  In
	    other words, this is the third parameter and it's a
	    reference on a type that starts with the string "foo".

	  *  ``return_type_name`` ``=`` <some-value>

	    Suppresses change reports involving functions whose return
	    type name equals the value of this property.

	  *  ``return_type_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving functions whose return
	    type name matches the regular expression specified as
	    value of this property.

	  *  ``symbol_name`` ``=`` <some-value>

	    Suppresses change reports involving functions whose symbol
	    name equals the value of this property.

	  *  ``symbol_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving functions whose symbol
	    name matches the regular expression specified as value of
	    this property.

	  *  ``symbol_version`` ``=`` <some-value>

	    Suppresses change reports involving functions whose symbol
	    version equals the value of this property.

	  *  ``symbol_version_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving functions whose symbol
	    version matches the regular expression specified as value
	    of this property.

      * ``[suppress_variable]``

	Suppresses report messages about changes on sub-types of a
	variable.  The potential properties of this sections are:

	  * ``label`` ``=`` <some-value>

            This property is the same as the :ref:`label property
            <suppr_label_property_label>` defined above.

	  *  ``name`` ``=`` <some-value>

	    Suppresses change reports involving variables whose name
	    equals the value of this property.

	  *  ``name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving variables whose name
	    matches the regular expression specified as value of this
	    property.

	  *  ``symbol_name`` ``=`` <some-value>

	    Suppresses change reports involving variables whose symbol
	    name equals the value of this property.

	  *  ``symbol_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving variables whose symbol
	    name matches the regular expression specified as value of
	    this property.

	  *  ``symbol_version`` ``=`` <some-value>

	    Suppresses change reports involving variables whose symbol
	    version equals the value of this property.

	  *  ``symbol_version_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving variables whose symbol
	    version matches the regular expression specified as value
	    of this property.

	  *  ``type_name`` ``=`` <some-value>

	    Suppresses change reports involving variables whose type
	    name equals the value of this property.

	  *  ``type_name_regexp`` ``=`` <:ref:`regular-expression <suppr_regexp_label>`>

	    Suppresses change reports involving variables whose type
	    name matches the regular expression specified as value of
	    this property.

    * Comments

      ``;`` or ``#`` ASCII character at the beginning of a line
      indicates a comment.  Comment lines are ignored.

  * Code examples

    1. Suppressing change reports about types.

       Suppose we have a library named ``libtest1-v0.so`` which
       contains this very useful code: ::

	$ cat -n test1-v0.cc
	     1	// A forward declaration for a type considered to be opaque to
	     2	// function foo() below.
	     3	struct opaque_type;
	     4
	     5	// This function cannot touch any member of opaque_type.  Hence,
	     6	// changes to members of opaque_type should not impact foo, as far as
	     7	// ABI is concerned.
	     8	void
	     9	foo(opaque_type*)
	    10	{
	    11	}
	    12
	    13	struct opaque_type
	    14	{
	    15	  int member0;
	    16	  char member1;
	    17	};
	$

    Let's change the layout of struct opaque_type by inserting a data
    member around line 15, leading to a new version of the library,
    that we shall name ``libtest1-v1.so``: ::

	$ cat -n test1-v1.cc
	     1	// A forward declaration for a type considered to be opaque to
	     2	// function foo() below.
	     3	struct opaque_type;
	     4
	     5	// This function cannot touch any member of opaque_type;  Hence,
	     6	// changes to members of opaque_type should not impact foo, as far as
	     7	// ABI is concerned.
	     8	void
	     9	foo(opaque_type*)
	    10	{
	    11	}
	    12
	    13	struct opaque_type
	    14	{
	    15	  char added_member; // <-- a new member got added here now.
	    16	  int member0;
	    17	  char member1;
	    18	};
	$

    Let's compile both examples.  We shall not forget to compile them
    with debug information generation turned on: ::

	$ g++ -shared -g -Wall -o libtest1-v0.so test1-v0.cc
	$ g++ -shared -g -Wall -o libtest1-v1.so test1-v1.cc

    Let's ask :ref:`abidiff <abidiff_label>` which ABI differences it sees
    between ``libtest1-v0.so`` and ``libtest1-v1.so``: ::

	$ abidiff libtest1-v0.so libtest1-v1.so
	Functions changes summary: 0 Removed, 1 Changed, 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	1 function with some indirect sub-type change:

	  [C]'function void foo(opaque_type*)' has some indirect sub-type changes:
	    parameter 0 of type 'opaque_type*' has sub-type changes:
	      in pointed to type 'struct opaque_type':
		size changed from 64 to 96 bits
		1 data member insertion:
		  'char opaque_type::added_member', at offset 0 (in bits)
		2 data member changes:
		 'int opaque_type::member0' offset changed from 0 to 32
		 'char opaque_type::member1' offset changed from 32 to 64


    So ``abidiff`` reports that the opaque_type's layout has changed
    in a significant way, as far as ABI implications are concerned, in
    theory.  After all, a sub-type (``struct opaque_type``) of an
    exported function (``foo()``) has seen its layout change.  This
    might have non negligible ABI implications.  But in practice here,
    the programmer of the litest1-v1.so library knows that the "soft"
    contract between the function ``foo()`` and the type ``struct
    opaque_type`` is to stay away from the data members of the type.
    So layout changes of ``struct opaque_type`` should not impact
    ``foo()``.

    Now to teach ``abidiff`` about this soft contract and have it
    avoid emitting what amounts to false positives in this case, we
    write the suppression specification file below: ::

	$ cat test1.suppr
	[suppress_type]
	  type_kind = struct
	  name = opaque_type

    Translated in plain English, this suppression specification would
    read: "Do not emit change reports about a struct which name is
    opaque_type".

    Let's now invoke ``abidiff`` on the two versions of the library
    again, but this time with the suppression specification: ::

	$ abidiff --suppressions test1.suppr libtest1-v0.so libtest1-v1.so
	Functions changes summary: 0 Removed, 0 Changed (1 filtered out), 0 Added function
	Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

    As you can see, ``abidiff`` does not report the change anymore; it
    tells us that it was filtered out instead.

  2. Suppressing change reports about functions.

     Suppose we have a first version a library named
     ``libtest2-v0.so`` whose source code is: ::

	 $ cat -n test2-v0.cc

	  1	struct S1
	  2	{
	  3	  int m0;
	  4	
	  5	  S1()
	  6	    : m0()
	  7	  {}
	  8	};
	  9	
	 10	struct S2
	 11	{
	 12	  int m0;
	 13	
	 14	  S2()
	 15	    : m0()
	 16	  {}
	 17	};
	 18	
	 19	struct S3
	 20	{
	 21	  int m0;
	 22	
	 23	  S3()
	 24	    : m0()
	 25	  {}
	 26	};
	 27	
	 28	int
	 29	func(S1&)
	 30	{
	 31	  // suppose the code does something with the argument.
	 32	  return 0;
	 33	
	 34	}
	 35	
	 36	char
	 37	func(S2*)
	 38	{
	 39	  // suppose the code does something with the argument.
	 40	  return 0;
	 41	}
	 42	
	 43	unsigned
	 44	func(S3)
	 45	{
	 46	  // suppose the code does something with the argument.
	 47	  return 0;
	 48	}
	$
	
     And then we come up with a second version ``libtest2-v1.so`` of
     that library; the source code is modified by making the
     structures ``S1``, ``S2``, ``S3`` inherit another struct: ::

	$ cat -n test2-v1.cc
	      1	struct base_type
	      2	{
	      3	  int m_inserted;
	      4	};
	      5	
	      6	struct S1 : public base_type // <--- S1 now has base_type as its base
	      7				     // type.
	      8	{
	      9	  int m0;
	     10	
	     11	  S1()
	     12	    : m0()
	     13	  {}
	     14	};
	     15	
	     16	struct S2 : public base_type // <--- S2 now has base_type as its base
	     17				     // type.
	     18	{
	     19	  int m0;
	     20	
	     21	  S2()
	     22	    : m0()
	     23	  {}
	     24	};
	     25	
	     26	struct S3 : public base_type // <--- S3 now has base_type as its base
	     27				     // type.
	     28	{
	     29	  int m0;
	     30	
	     31	  S3()
	     32	    : m0()
	     33	  {}
	     34	};
	     35	
	     36	int
	     37	func(S1&)
	     38	{
	     39	  // suppose the code does something with the argument.
	     40	  return 0;
	     41	
	     42	}
	     43	
	     44	char
	     45	func(S2*)
	     46	{
	     47	  // suppose the code does something with the argument.
	     48	  return 0;
	     49	}
	     50	
	     51	unsigned
	     52	func(S3)
	     53	{
	     54	  // suppose the code does something with the argument.
	     55	  return 0;
	     56	}
	 $ 

     Now let's build the two libraries: ::

	 g++ -Wall -g -shared -o libtest2-v0.so test2-v0.cc
	 g++ -Wall -g -shared -o libtest2-v0.so test2-v0.cc

     Let's look at the output of ``abidiff``: ::

	 $ abidiff libtest2-v0.so libtest2-v1.so 
	 Functions changes summary: 0 Removed, 3 Changed, 0 Added functions
	 Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

	 3 functions with some indirect sub-type change:

	   [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	     parameter 0 of type 'struct S3' has sub-type changes:
	       size changed from 32 to 64 bits
	       1 base class insertion:
		 struct base_type
	       1 data member change:
		'int S3::m0' offset changed from 0 to 32

	   [C]'function char func(S2*)' has some indirect sub-type changes:
	     parameter 0 of type 'S2*' has sub-type changes:
	       in pointed to type 'struct S2':
		 size changed from 32 to 64 bits
		 1 base class insertion:
		   struct base_type
		 1 data member change:
		  'int S2::m0' offset changed from 0 to 32

	   [C]'function int func(S1&)' has some indirect sub-type changes:
	     parameter 0 of type 'S1&' has sub-type changes:
	       in referenced type 'struct S1':
		 size changed from 32 to 64 bits
		 1 base class insertion:
		   struct base_type
		 1 data member change:
		  'int S1::m0' offset changed from 0 to 32
	 $

     Let's tell ``abidiff`` to avoid showing us the differences on the
     overloads of ``func`` that takes either a pointer or a reference.
     For that, we author this simple suppression specification: ::

	 $ cat -n libtest2.suppr
	      1	[suppress_function]
	      2	  name = func
	      3	  parameter = '0 S1&
	      4	
	      5	[suppress_function]
	      6	  name = func
	      7	  parameter = '0 S2*
	 $
     
     And then let's invoke ``abidiff`` with the suppression
     specification: ::

       $ ../build/tools/abidiff --suppressions libtest2.suppr libtest2-v0.so libtest2-v1.so 
       Functions changes summary: 0 Removed, 1 Changed (2 filtered out), 0 Added function
       Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

       1 function with some indirect sub-type change:

	 [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	   parameter 0 of type 'struct S3' has sub-type changes:
	     size changed from 32 to 64 bits
	     1 base class insertion:
	       struct base_type
	     1 data member change:
	      'int S3::m0' offset changed from 0 to 32


     The suppression specification could be reduced using
     :ref:`regular expressions <suppr_regexp_label>`: ::

       $ cat -n libtest2-1.suppr
	    1	[suppress_function]
	    2	  name = func
	    3	  parameter = '0 /^S.(&|\\*)/
       $

       $ ../build/tools/abidiff --suppressions libtest2-1.suppr libtest2-v0.so libtest2-v1.so 
       Functions changes summary: 0 Removed, 1 Changed (2 filtered out), 0 Added function
       Variables changes summary: 0 Removed, 0 Changed, 0 Added variable

       1 function with some indirect sub-type change:

	 [C]'function unsigned int func(S3)' has some indirect sub-type changes:
	   parameter 0 of type 'struct S3' has sub-type changes:
	     size changed from 32 to 64 bits
	     1 base class insertion:
	       struct base_type
	     1 data member change:
	      'int S3::m0' offset changed from 0 to 32

       $

.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format

.. _Ini File Syntax: http://en.wikipedia.org/wiki/INI_file

.. _GNU C Library: http://www.gnu.org/software/libc
