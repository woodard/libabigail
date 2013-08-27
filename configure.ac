m4_define([version_major],   [0])
m4_define([version_minor],   [1])
m4_define([version_revision],[0])

AC_INIT([libabigail],
	[version_major.version_minor.version_revision],
	[http://sourceware.org/bugzilla],
	[libabigail],
	[http://sourceware.org/libabigail])

AC_PREREQ([2.63])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

VERSION_MAJOR=version_major
VERSION_MINOR=version_minor
VERSION_REVISION=version_revision

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

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

dnl Check for dependency: libxml
LIBXML2_VERSION=2.6.22
PKG_CHECK_MODULES(XML, libxml-2.0 >= $LIBXML2_VERSION)

AC_SUBST(LIBXML2_VERSION)
AC_SUBST(XML_LIBS)
AC_SUBST(XML_CFLAGS)

dnl Check for dependency: libzip
LIBZIP_VERSION=0.10
PKG_CHECK_MODULES(LIBZIP, libzip >= $LIBZIP_VERSION)

AC_SUBST(LIBZIP_VERSION)
AC_SUBST(LIBZIP_LIBS)
AC_SUBST(LIBZIP_CFLAGS)

DEPS_CPPFLAGS="$XML_CFLAGS $LIBZIP_CFLAGS"
AC_SUBST(DEPS_CPPFLAGS)

DEPS_LIBS="$XML_LIBS $LIBZIP_LIBS"
AC_SUBST(DEPS_LIBS)

if test x$ABIGAIL_DEVEL != x; then
   DEVEL_CFLAGS="-g -Wall -Wextra -Werror"
   DEVEL_CXXFLAGS="-g -Wall -Wextra -Werror"
   CFLAGS=$DEVEL_CFLAGS
   CXXFLAGS=$DEVEL_CXXFLAGS
fi

AC_CONFIG_FILES([Makefile
libabigail.pc
include/Makefile
include/abg-version.h
doc/Makefile
src/Makefile
tests/Makefile])

AC_OUTPUT
