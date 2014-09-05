h=$(abs_srcdir)

noinst_LTLIBRARIES = libtoolsutils.la
libtoolsutils_la_SOURCES=	\
abg-tools-utils.h		\
abg-tools-utils.cc

if ENABLE_ZIP_ARCHIVE
  bin_PROGRAMS = abiar abidiff abilint abidw
else
  bin_PROGRAMS = abidiff abilint abidw  abinilint
endif

noinst_PROGRAMS = abisym

if ENABLE_ZIP_ARCHIVE
abiar_SOURCES = abiar.cc
abiardir = $(bindir)
abiar_LDADD = ../src/libabigail.la libtoolsutils.la
endif

abidiff_SOURCES = abidiff.cc
abidiffdir = $(bindir)
abidiff_LDADD = ../src/libabigail.la libtoolsutils.la

abilint_SOURCES = abilint.cc
abilintdir = $(bindir)
abilint_LDADD = ../src/libabigail.la libtoolsutils.la

abidw_SOURCES = abidw.cc
abidwdir = $(bindir)
abidw_LDADD = ../src/libabigail.la libtoolsutils.la

abisym_SOURCES = abisym.cc
abisymdir = $(bindir)
abisym_LDADD = ../src/libabigail.la

binilint_SOURCES = binilint.cc
binilint = $(bindir)
binilint_LDFLAGS = $(abs_top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools
