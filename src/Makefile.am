libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

libabigail_la_SOURCES = $(headers)	\
abg-ir.cc				\
abg-reader.cc			\
abg-libxml-utils.cc		\
abg-hash.cc			\
abg-writer.cc			\
abg-config.cc			\
abg-viz-common.cc		\
abg-viz-dot.cc			\
abg-viz-svg.cc			

libabigail_la_LDFLAGS=@XML_LIBS@ -Wl,--as-needed

AM_CPPFLAGS=@XML_CFLAGS@ -Wall -I$(top_srcdir)/include -I$(top_builddir)/include
AM_CXXFLAGS="-std=gnu++11"
