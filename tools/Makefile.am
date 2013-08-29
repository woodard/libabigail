h=$(abs_srcdir)



noinst_LTLIBRARIES = libtoolsutils.la
libtoolsutils_la_SOURCES=	\
$(h)/abg-tools-utils.h		\
$(h)/abg-tools-utils.cc

bin_PROGRAMS=biar
biar_SOURCES=$(h)/biar.cc
biardir=$(bindir)
biar_LDFLAGS=$(abs_top_builddir)/src/libabigail.la $(abs_top_builddir)/tools/libtoolsutils.la
biar_DEPENDENCIES = libtoolsutils.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools
