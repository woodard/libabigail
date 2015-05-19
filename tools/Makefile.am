h=$(abs_srcdir)

if ENABLE_ZIP_ARCHIVE
  bin_PROGRAMS = abiar abidiff abilint abidw abicompat abipkgdiff
else
  bin_PROGRAMS = abidiff abilint abidw abicompat abipkgdiff
endif

noinst_PROGRAMS = abisym abinilint

if ENABLE_ZIP_ARCHIVE
abiar_SOURCES = abiar.cc
abiardir = $(bindir)
abiar_LDADD = ../src/libabigail.la
endif

abidiff_SOURCES = abidiff.cc
abidiffdir = $(bindir)
abidiff_LDADD = ../src/libabigail.la

abilint_SOURCES = abilint.cc
abilintdir = $(bindir)
abilint_LDADD = ../src/libabigail.la

abidw_SOURCES = abidw.cc
abidwdir = $(bindir)
abidw_LDADD = ../src/libabigail.la

abisym_SOURCES = abisym.cc
abisymdir = $(bindir)
abisym_LDADD = ../src/libabigail.la

abinilint_SOURCES = binilint.cc
abinilintdir = $(bindir)
abinilint_LDFLAGS = $(abs_top_builddir)/src/libabigail.la

abicompat_SOURCES = abicompat.cc
abicompatdir = $(bindir)
abicompat_LDADD = $(abs_top_builddir)/src/libabigail.la

abipkgdiff_SOURCES = abipkgdiff.cc
abipkgdiffdir = $(bindir)
abipkgdiff_LDADD = $(abs_top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I$(abs_top_srcdir)/include -I$(abs_top_srcdir)/tools -fPIC
