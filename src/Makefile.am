h=$(abs_srcdir)
lib_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)

libabigail_la_SOURCES = $(headers)	\
abg-traverse.cc				\
abg-ir.cc				\
abg-corpus.cc				\
abg-diff-utils.cc			\
abg-comparison.cc			\
abg-comp-filter.cc			\
abg-reader.cc				\
abg-dwarf-reader.cc			\
abg-libxml-utils.cc			\
abg-libzip-utils.cc			\
abg-hash.cc				\
abg-writer.cc				\
abg-config.cc				\
abg-viz-common.cc			\
abg-viz-dot.cc				\
abg-viz-svg.cc

libabigail_la_LDFLAGS=@DEPS_LIBS@ -Wl,--as-needed

AM_CPPFLAGS=@DEPS_CPPFLAGS@ -Wall -I$(abs_top_srcdir)/include -I$(abs_top_builddir)/include -I$(abs_top_builddir)
AM_CXXFLAGS="-std=gnu++11"
