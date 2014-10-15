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
      name = ^.*PrivateType

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

	  * ``name_regexp`` ``=`` <regular-expression>

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


	  * ``label`` ``=`` <some-value>

	    Define a label for the section.  A label is just an
	    informative string that might be used by abidiff to refer
	    to a type suppression in error messages.

    * Comments

      ``;`` or ``#`` ASCII character at the beginning of a line
      indicates a comment.  Comment lines are ignored.

  * Code examples

    Suppose we have a library named ``libtest1-v0.so`` which contains this
    very useful code: ::

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


.. _ELF: http://en.wikipedia.org/wiki/Executable_and_Linkable_Format

.. _Ini File Syntax: http://en.wikipedia.org/wiki/INI_file
