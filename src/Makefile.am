libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

h=$(abs_srcdir)

headers =		\
abg-ir.h		\
abg-reader.h	\
abg-corpus.h	\
abg-libxml-utils.h	\
abg-hash.h		\
abg-writer.h	\
abg-config.h

libabigail_la_SOURCES = $(headers)	\
abg-ir.cc				\
abg-reader.cc			\
abg-corpus.cc			\
abg-libxml-utils.cc		\
abg-hash.cc			\
abg-writer.cc			\
abg-config.cc

libabigail_la_LDFLAGS=@XML_LIBS@ -Wl,--as-needed

publicheaders_DATA=$(headers)
publicheadersdir=$(includedir)/libabigail

AM_CPPFLAGS=@XML_CFLAGS@ -Wall
