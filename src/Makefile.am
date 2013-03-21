libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

h=$(abs_srcdir)

headers =		\
$(h)/abg-ir.h		\
$(h)/abg-reader.h	\
$(h)/abg-corpus.h	\
$(h)/abg-libxml-utils.h	\
$(h)/abg-hash.h		\
$(h)/abg-writer.h	\
$(h)/abg-config.h

libabigail_la_SOURCES = $(headers)	\
$(h)/abg-ir.cc				\
$(h)/abg-reader.cc			\
$(h)/abg-corpus.cc			\
$(h)/abg-libxml-utils.cc		\
$(h)/abg-hash.cc			\
$(h)/abg-writer.cc			\
$(h)/abg-config.cc

libabigail_la_LDFLAGS=@ABIGAIL_LIBS@ -Wl,--as-needed

publicheaders_DATA=$(headers)
publicheadersdir=$(includedir)/libabigail

AM_CPPFLAGS=@ABIGAIL_CFLAGS@ -Wall
