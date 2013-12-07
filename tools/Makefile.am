h=$(abs_srcdir)



noinst_LTLIBRARIES = libtoolsutils.la
libtoolsutils_la_SOURCES=	\
$(h)/abg-tools-utils.h		\
$(h)/abg-tools-utils.cc

bin_PROGRAMS = biar bidiff bilint bidw
biar_SOURCES = $(h)/biar.cc
biardir = $(bindir)
biar_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
biar_DEPENDENCIES = libtoolsutils.la

bidiff_SOURCES = $(h)/bidiff.cc
bidiffdir = $(bindir)
bidiff_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bidiff_DEPENDENCIES = libtoolsutils.la

bilint_SOURCES = $(h)/bilint.cc
bilintdir = $(bindir)
bilint_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bilint_DEPENDENCIES = libtoolsutils.la

bidw_SOURCES = $(h)/bidw.cc
bidwdir = $(bindir)
bidw_LDFLAGS = $(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
bidw_DEPENDENCIES = libtoolsutils.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools
