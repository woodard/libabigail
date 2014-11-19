m4_define([version_major],   [0])
m4_define([version_minor],   [1])
m4_define([version_revision],[0])

AC_INIT([libabigail],
	[version_major.version_minor.version_revision],
	[http://sourceware.org/bugzilla],
	[libabigail],
	[http://sourceware.org/libabigail])

AC_PREREQ([2.63])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1 foreign subdir-objects tar-ustar])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

VERSION_MAJOR=version_major
VERSION_MINOR=version_minor
VERSION_REVISION=version_revision

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

ENABLE_ZIP_ARCHIVE=auto
AC_ARG_ENABLE(zip-archive,
	      AS_HELP_STRING([--enable-zip-archive=yes|no|auto],
			     [enable bundling of TUs in zip archives (default is auto)]),
	      ENABLE_ZIP_ARCHIVE=$enableval,
	      ENABLE_ZIP_ARCHIVE=auto)


ENABLE_CXX11=no
AC_ARG_ENABLE(cxx11,
	      AS_HELP_STRING([--enable-cxx11=yes|no],
			     [enable features that use the C++11 compiler]),
	      ENABLE_CXX11=$enableval,
	      ENABLE_CXX11=no)

dnl *************************************************
dnl Here is the list of versions of the dependencies
dnl *************************************************

AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL

LT_PREREQ([2.2])
LT_INIT

AC_LANG([C++])
AC_LANG_COMPILER_REQUIRE

dnl Check for dependency: libdw (elfutils)
DW_LIBS=
AC_CHECK_LIB(dw, dwfl_begin, [DW_LIBS=-ldw])
AC_CHECK_HEADER(elfutils/libdwfl.h,
		[],
		[AC_MSG_ERROR([could not find elfutils/libdwfl.h installed])])
AC_CHECK_LIB([elf], [elf_end], [ELF_LIBS="-lelf"])
AC_CHECK_HEADER([libelf.h],
		[],
		[AC_MSG_ERROR([could not find libelf.h])])

if test x$DW_LIBS = x; then
   AC_MSG_ERROR([could not find elfutils dwarf library installed])
fi

AC_SUBST(DW_LIBS)
AC_SUBST([ELF_LIBS])

dnl Check for dependency: libxml
LIBXML2_VERSION=2.6.22
PKG_CHECK_MODULES(XML, libxml-2.0 >= $LIBXML2_VERSION)

AC_SUBST(LIBXML2_VERSION)
AC_SUBST(XML_LIBS)
AC_SUBST(XML_CFLAGS)

dnl Check for dependency: libzip
LIBZIP_VERSION=0.10

HAS_LIBZIP=no
# The below doesn't seem to work on my box for a reason.  Let's write
# the damn thing by hand:
# PKG_CHECK_EXISTS([libzip >= $LIBZIP_VERSION], [HAS_LIBZIP=yes], [HAS_LIBZIP=no])

if $PKG_CONFIG --exists --print-errors "libzip >= $LIBZIP_VERSION"; then
   AC_MSG_NOTICE(found libzip version $LIBZIP_VERSION)
   HAS_LIBZIP=yes
else
   AC_MSG_NOTICE(no libzip >= $LIBZIP_VERSION has been found)
   HAS_LIBZIP=no
fi

if test x$ENABLE_ZIP_ARCHIVE = xauto; then
   if test x$HAS_LIBZIP = xyes; then
      ENABLE_ZIP_ARCHIVE=yes;
   else
      ENABLE_ZIP_ARCHIVE=no
   fi
fi

if test x$ENABLE_ZIP_ARCHIVE = xyes; then
   AC_MSG_NOTICE(the zip-archive feature is enabled)
else
   AC_MSG_NOTICE(the zip-archive feature is disabled)
fi

FOUND_LIBZIP=no
if test x$ENABLE_ZIP_ARCHIVE = xyes; then
   PKG_CHECK_MODULES(LIBZIP, libzip >= $LIBZIP_VERSION)
   FOUND_LIBZIP=yes
   AC_SUBST(LIBZIP_VERSION)
   AC_SUBST(LIBZIP_LIBS)
   AC_SUBST(LIBZIP_CFLAGS)
   AC_DEFINE([WITH_ZIP_ARCHIVE], 1, [compile the zip archive support])
   AC_DEFINE([HAVE_LIBZIP], 1, [Defined to 1 if the libzip library is available])
fi

AM_CONDITIONAL(ENABLE_ZIP_ARCHIVE, test x$ENABLE_ZIP_ARCHIVE = xyes)
DEPS_CPPFLAGS="$XML_CFLAGS $LIBZIP_CFLAGS"
AC_SUBST(DEPS_CPPFLAGS)

dnl Handle conditional use of a C++11 compiler
if test x$ENABLE_CXX11 = xyes; then
   AC_DEFINE([WITH_CXX11], 1, [Defined to 1 if a C++11 compiler is used])
fi

AM_CONDITIONAL(ENABLE_CXX11, test x$ENABLE_CXX11 = xyes)

ENABLE_ZIP_ARCHIVE_AND_CXX11=no
if test x$ENABLE_CXX11 = xyes -a x$ENABLE_ZIP_ARCHIVE = xyes; then
   ENABLE_ZIP_ARCHIVE_AND_CXX11 = yes
fi

AM_CONDITIONAL(ENABLE_ZIP_ARCHIVE_AND_CXX11, test x$ENABLE_ZIP_ARCHIVE_AND_CXX11 = xyes)


DEPS_LIBS="$XML_LIBS $LIBZIP_LIBS $DW_LIBS $ELF_LIBS"
AC_SUBST(DEPS_LIBS)

if test x$ABIGAIL_DEVEL != x; then
   CFLAGS="-g -Wall -Wextra -Werror"
   CXXFLAGS="-g -Wall -Wextra -Werror"
fi

AC_CONFIG_FILES([Makefile
libabigail.pc
  include/Makefile
  include/abg-version.h
  doc/Makefile
    doc/manuals/Makefile
  src/Makefile
  tools/Makefile
  tests/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([
=====================================================================
	Libabigail: $VERSION_MAJOR.$VERSION_MINOR.$VERSION_REVISION
=====================================================================

		Here is the configuration of the package:

    Prefix                             : ${prefix}
    Source code location               : ${srcdir}
    C Compiler                         : ${CC}
    C++ Compiler		       : ${CXX}

 OPTIONAL FEATURES:
    Enable zip archives                : ${ENABLE_ZIP_ARCHIVE}
    Uses a C++-11 compiler             : ${ENABLE_CXX11}
])
