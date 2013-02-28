AC_INIT([libabigail],
	[0.1],
	[http://sourceware.org/bugzilla],
	[libabigail],
	[http://sourceware.org/libabigail])

AC_PREREQ([2.63])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.2])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL

LT_PREREQ([2.2])
LT_INIT

AC_LANG([C++])
AC_LANG_COMPILER_REQUIRE

AC_CONFIG_FILES([Makefile
src/Makefile])

AC_OUTPUT
