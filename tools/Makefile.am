AUTOMAKE_OPTIONS=subdir-objects

h=$(abs_srcdir)

noinst_LTLIBRARIES = libtoolsutils.la
libtoolsutils_la_SOURCES=	\
abg-tools-utils.h		\
abg-tools-utils.cc

bin_PROGRAMS = biar bidiff bilint bidw bisym

biar_SOURCES = biar.cc
biardir = $(bindir)
biar_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
biar_DEPENDENCIES = libtoolsutils.la

bidiff_SOURCES = bidiff.cc
bidiffdir = $(bindir)
bidiff_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bidiff_DEPENDENCIES = libtoolsutils.la

bilint_SOURCES = bilint.cc
bilintdir = $(bindir)
bilint_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bilint_DEPENDENCIES = libtoolsutils.la

bidw_SOURCES = bidw.cc
bidwdir = $(bindir)
bidw_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bidw_DEPENDENCIES = libtoolsutils.la

bisym_SOURCES = bisym.cc
bisymdir = $(bindir)
bisym_LDFLAGS = $(abs_top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools
