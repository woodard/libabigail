libabigail_LTLIBRARIES=libabigail.la
libabigaildir=$(libdir)/

headers =		\
abg-ir.h		\
abg-reader.h		\
abg-corpus.h		\
abg-libxml-utils.h	\
abg-hash.h		\
abg-writer.h		\
abg-config.h		\
abg-version.h

libabigail_la_SOURCES = $(headers)	\
abg-ir.cc				\
abg-reader.cc			\
abg-corpus.cc			\
abg-libxml-utils.cc		\
abg-hash.cc			\
abg-writer.cc			\
abg-config.cc

all-local: $(srcdir)/abg-version.h

$(srcdir)/abg-version.h: $(top_srcdir)/configure.ac
	@echo "Generating $(srcdir)/abg-version.h ..."
	@echo "#ifndef __ABG_VERSION_H__" > $(srcdir)/abg-version.h
	@echo "#define __ABG_VERSION_H__" >> $(srcdir)/abg-version.h
	@echo "#define ABIGAIL_VERSION_MAJOR @VERSION_MAJOR@" >> $(srcdir)/abg-version.h
	@echo "#define ABIGAIL_VERSION_MINOR @VERSION_MINOR@" >> $(srcdir)/abg-version.h
	@echo "#define ABIGAIL_VERSION_REVISION @VERSION_REVISION@" >> $(srcdir)/abg-version.h
	@echo "#endif" >> $(srcdir)/abg-version.h
	@echo "Done."


libabigail_la_LDFLAGS=@XML_LIBS@ -Wl,--as-needed

publicheaders_DATA=$(headers)
publicheadersdir=$(includedir)/libabigail

AM_CPPFLAGS=@XML_CFLAGS@ -Wall
