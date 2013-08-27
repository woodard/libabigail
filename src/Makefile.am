h=$(abs_srcdir)
libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

libabigail_la_SOURCES = $(headers)	\
$(h)/abg-ir.cc				\
$(h)/abg-reader.cc			\
$(h)/abg-libxml-utils.cc		\
$(h)/abg-hash.cc			\
$(h)/abg-writer.cc			\
$(h)/abg-config.cc			\
$(h)/abg-viz-common.cc			\
$(h)/abg-viz-dot.cc			\
$(h)/abg-viz-svg.cc

libabigail_la_LDFLAGS=@XML_LIBS@ -Wl,--as-needed

AM_CPPFLAGS=@XML_CFLAGS@ -Wall -I$(top_srcdir)/include -I$(top_builddir)/include
AM_CXXFLAGS="-std=gnu++11"
