m4_define([version_major],   [1])
m4_define([version_minor],   [0])
m4_define([version_revision],[rc1])

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

AM_INIT_AUTOMAKE([1.11.1 foreign subdir-objects tar-ustar parallel-tests])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

VERSION_MAJOR=version_major
VERSION_MINOR=version_minor
VERSION_REVISION=version_revision

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)

AC_ARG_ENABLE(rpm,
	      AS_HELP_STRING([--enable-rpm=yes|no|auto],
			     [enable the support of rpm in abipkgdiff (default is auto)]),
	      ENABLE_RPM=$enableval,
	      ENABLE_RPM=auto)

AC_ARG_ENABLE(deb,
	      AS_HELP_STRING([--enable-rpm=yes|no|auto],
			     [enable the support of deb in abipkgdiff (default is auto)]),
	      ENABLE_DEB=$enableval,
	      ENABLE_DEB=auto)

AC_ARG_ENABLE(tar,
	      AS_HELP_STRING([--enable-tar=yes|no|auto],
			     [enable the support of GNU tar archives in abipkgdiff (default is auto)]),
	      ENABLE_TAR=$enableval,
	      ENABLE_TAR=auto)

AC_ARG_ENABLE(zip-archive,
	      AS_HELP_STRING([--enable-zip-archive=yes|no|auto],
			     [enable bundling of TUs in zip archives (default is no)]),
	      ENABLE_ZIP_ARCHIVE=$enableval,
	      ENABLE_ZIP_ARCHIVE=no)


AC_ARG_ENABLE(cxx11,
	      AS_HELP_STRING([--enable-cxx11=yes|no],
			     [enable features that use the C++11 compiler]),
	      ENABLE_CXX11=$enableval,
	      ENABLE_CXX11=no)

AC_ARG_ENABLE(apidoc,
	      AS_HELP_STRING([--enable-apidoc=yes|no|auto],
			     [enable generation of the apidoc in html]),
	      ENABLE_APIDOC=$enableval,
	      ENABLE_APIDOC=auto)

AC_ARG_ENABLE(manual,
	      AS_HELP_STRING([--enable-manual=yes|no|auto],
			     [enable generation of the manual in html]),
	      ENABLE_MANUAL=$enableval,
	      ENABLE_MANUAL=auto)

dnl *************************************************
dnl check for dependencies
dnl *************************************************

AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL

LT_PREREQ([2.2])
LT_INIT

AC_LANG([C++])
AC_LANG_COMPILER_REQUIRE

dnl Check for dependency: libelf, libdw, libebl (elfutils)
ELF_LIBS=
AC_CHECK_LIB([elf], [elf_end], [ELF_LIBS="-lelf"])
AC_CHECK_HEADER([libelf.h],
		[],
		[AC_MSG_ERROR([could not find libelf.h])])

DW_LIBS=
AC_CHECK_LIB(dw, dwfl_begin, [DW_LIBS=-ldw])
AC_CHECK_HEADER(elfutils/libdwfl.h,
		[],
		[AC_MSG_ERROR([could not find elfutils/libdwfl.h installed])])


if test x$ELF_LIBS = x; then
   AC_MSG_ERROR([could not find elfutils elf library installed])
fi

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

dnl Check for some programs like rm, mkdir, etc ...
AC_CHECK_PROG(HAS_RM, rm, yes, no)
if test x$HAS_RM = xno; then
   AC_MSG_ERROR([could not find the program 'rm' installed])
fi

AC_CHECK_PROG(HAS_MKDIR, mkdir, yes, no)
if test x$HAS_MKDIR = xno; then
   AC_MSG_ERROR([could not find the program 'mkdir' installed])
fi

dnl Check for the rpm2cpio and cpio programs
if test x$ENABLE_RPM = xyes -o x$ENABLE_RPM = xauto; then
  AC_CHECK_PROG(HAS_RPM2CPIO, rpm2cpio, yes, no)
  AC_CHECK_PROG(HAS_CPIO, cpio, yes, no)
  
  if test x$ENABLE_RPM = xauto; then
    if test x$HAS_RPM2CPIO = xyes -a x$HAS_CPIO = xyes; then
       ENABLE_RPM=yes
    fi
  fi
fi

if test x$ENABLE_RPM = xyes; then
   AC_DEFINE([WITH_RPM], 1, [compile the rpm package support in abipkgdiff])
   AC_MSG_NOTICE(rpm support in abipkgdiff is enabled);
else
   AC_MSG_NOTICE(rpm support in abipkgdiff is disabled);
fi

AM_CONDITIONAL(ENABLE_RPM, test x$ENABLE_RPM = xyes)

dnl Check for the dpkg program
if test x$ENABLE_DEB = xauto -o x$ENABLE_DEB = xyes; then
   AC_CHECK_PROG(HAS_DPKG, dpkg, yes, no)

   if test x$ENABLE_DEB = xauto; then
     if test x$HAS_DPKG = xyes; then
       ENABLE_DEB=yes
     else
       ENABLE_DEB=no
    fi
   fi
fi

if test x$ENABLE_DEB = xyes; then
   AC_DEFINE([WITH_DEB], 1, [compile the deb package support in abipkgdiff])
   AC_MSG_NOTICE(deb support in abipkgdiff is enabled);
else
   AC_MSG_NOTICE(deb support in abipkgdiff is disabled);
fi

AM_CONDITIONAL(ENABLE_DEB, test x$ENABLE_DEB = xyes)

dnl Check for the tar program
if test x$ENABLE_TAR = xauto -o x$ENABLE_TAR = xyes; then
   AC_CHECK_PROG(HAS_TAR, tar, yes, no)

   if test x$ENABLE_TAR = xauto; then
     if test x$HAS_TAR = xyes; then
       ENABLE_TAR=yes
    fi
   fi
fi

if test x$ENABLE_TAR = xyes; then
   AC_DEFINE([WITH_TAR], 1, [compile the GNU tar archive support in abipkgdiff])
   AC_MSG_NOTICE(GNU tar support in abipkgdiff is enabled);
else
   AC_MSG_NOTICE(GNU tar support in abipkgdiff is disabled);
fi

AM_CONDITIONAL(ENABLE_TAR, test x$ENABLE_TAR = xyes)

dnl Check for dependency: libzip
LIBZIP_VERSION=0.10.1

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

dnl Check for the presence of doxygen program

if test x$ENABLE_APIDOC != xno; then
  AC_CHECK_PROG(FOUND_DOXYGEN, doxygen, yes, no)
  if test x$ENABLE_APIDOC = xauto; then
    if test x$FOUND_DOXYGEN = xyes; then
      ENABLE_APIDOC=yes
    else
      ENABLE_APIDOC=no
    fi
  fi
fi
AM_CONDITIONAL(ENABLE_APIDOC, test x$ENABLE_APIDOC = xyes)

dnl Check for the presence of the sphinx-build program

if test x$ENABLE_MANUAL != xno; then
  AC_CHECK_PROG(FOUND_SPHINX_BUILD, sphinx-build, yes, no)
  if test x$ENABLE_MANUAL = xauto; then
    if test x$FOUND_SPHINX_BUILD = xyes; then
      ENABLE_MANUAL=yes
    else
      ENABLE_MANUAL=no
    fi
  fi
fi
AM_CONDITIONAL(ENABLE_MANUAL, test x$ENABLE_MANUAL = xyes)

dnl Set the list of libraries libabigail depends on

DEPS_LIBS="$XML_LIBS $LIBZIP_LIBS $ELF_LIBS $DW_LIBS"
AC_SUBST(DEPS_LIBS)

if test x$ABIGAIL_DEVEL != x; then
   CFLAGS="-g -Wall -Wextra -Werror"
   CXXFLAGS="-g -Wall -Wextra -Werror"
fi

HAS_EM_AARCH64=no
AC_CHECK_DECL([EM_AARCH64],
              [HAS_EM_AARCH64=yes],
              [HAS_EM_AARCH64=no],
              [[#include <elf.h>]])

if test x$HAS_EM_AARCH64 = xyes; then
  AC_DEFINE([HAVE_EM_AARCH64_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_AARCH64 macro defined])
fi

HAS_EM_TILEPRO=no
AC_CHECK_DECL([EM_TILEPRO],
              [HAS_EM_TILEPRO=yes],
              [HAS_EM_TILEPRO=no],
              [[#include <elf.h>]])

if test x$HAS_EM_TILEPRO = xyes; then
  AC_DEFINE([HAVE_EM_TILEPRO_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_TILEPR0 macro defined])
fi

HAS_EM_TILEGX=no
AC_CHECK_DECL([EM_TILEGX],
              [HAS_EM_TILEGX=yes],
              [HAS_EM_TILEGX=no],
              [[#include <elf.h>]])

if test x$HAS_EM_TILEGX = xyes; then
  AC_DEFINE([HAVE_EM_TILEGX_MACRO],
                 1,
            [Defined to 1 if elf.h has EM_TILEGX macro defined])
fi

dnl Set large files support
AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile
libabigail.pc
  include/Makefile
  include/abg-version.h
  doc/Makefile
    doc/manuals/Makefile
  src/Makefile
  tools/Makefile
  tests/Makefile
    tests/data/Makefile])

dnl Some test scripts are generated by autofoo.
AC_CONFIG_FILES([tests/runtestcanonicalizetypes.sh], [chmod +x tests/runtestcanonicalizetypes.sh])

AC_OUTPUT

AC_MSG_NOTICE([
=====================================================================
	Libabigail: $VERSION_MAJOR.$VERSION_MINOR.$VERSION_REVISION
=====================================================================

		Here is the configuration of the package:

    Prefix                                         : ${prefix}
    Source code location                           : ${srcdir}
    C Compiler                                     : ${CC}
    C++ Compiler		                   : ${CXX}

 OPTIONAL FEATURES:
    Enable zip archives                            : ${ENABLE_ZIP_ARCHIVE}
    Use a C++-11 compiler                          : ${ENABLE_CXX11}
    Enable rpm support in abipkgdiff               : ${ENABLE_RPM}
    Enable deb support in abipkgdiff               : ${ENABLE_DEB}
    Enable GNU tar archive support in abipkgdiff   : ${ENABLE_TAR}
    Generate html apidoc	                   : ${ENABLE_APIDOC}
    Generate html manual	                   : ${ENABLE_MANUAL}
])
