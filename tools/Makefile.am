h=$(abs_srcdir)

noinst_LTLIBRARIES = libtoolsutils.la
libtoolsutils_la_SOURCES=	\
abg-tools-utils.h		\
abg-tools-utils.cc

if ENABLE_ZIP_ARCHIVE
  bin_PROGRAMS = biar bidiff bilint bidw bisym
else
  bin_PROGRAMS = bidiff bilint bidw bisym
endif


if ENABLE_ZIP_ARCHIVE
biar_SOURCES = biar.cc
biardir = $(bindir)
biar_LDADD = ../src/libabigail.la libtoolsutils.la
endif

bidiff_SOURCES = bidiff.cc
bidiffdir = $(bindir)
bidiff_LDADD = ../src/libabigail.la libtoolsutils.la

bilint_SOURCES = bilint.cc
bilintdir = $(bindir)
bilint_LDADD = ../src/libabigail.la libtoolsutils.la

bidw_SOURCES = bidw.cc
bidwdir = $(bindir)
bidw_LDADD = ../src/libabigail.la libtoolsutils.la

bisym_SOURCES = bisym.cc
bisymdir = $(bindir)
bisym_LDADD = ../src/libabigail.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools
