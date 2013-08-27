h=$(abs_srcdir)
libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

libabigail_la_SOURCES = $(headers)	\
$(h)/abg-ir.cc				\
$(h)/abg-corpus.cc			\
$(h)/abg-reader.cc			\
$(h)/abg-libxml-utils.cc		\
$(h)/abg-libzip-utils.cc		\
$(h)/abg-hash.cc			\
$(h)/abg-writer.cc			\
$(h)/abg-config.cc			\
$(h)/abg-viz-common.cc			\
$(h)/abg-viz-dot.cc			\
$(h)/abg-viz-svg.cc

libabigail_la_LDFLAGS=@DEPS_LIBS@ -Wl,--as-needed

AM_CPPFLAGS=@DEPS_CPPFLAGS@ -Wall -I$(abs_top_srcdir)/include -I$(abs_top_builddir)/include -I$(abs_top_builddir)
AM_CXXFLAGS="-std=gnu++11"
