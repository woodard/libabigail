## SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
h=$(abs_srcdir)

bin_PROGRAMS = abidiff abilint abidw abicompat abipkgdiff kmidiff

if ENABLE_FEDABIPKGDIFF
  dist_bin_SCRIPTS = fedabipkgdiff
else
  noinst_SCRIPTS = fedabipkgdiff
endif

noinst_PROGRAMS = abisym abinilint
LDADD = $(abs_top_builddir)/src/libabigail.la

abidiff_SOURCES = abidiff.cc
abidiffdir = $(bindir)

abilint_SOURCES = abilint.cc
abilintdir = $(bindir)

abidw_SOURCES = abidw.cc
abidwdir = $(bindir)

abisym_SOURCES = abisym.cc
abisymdir = $(bindir)

abinilint_SOURCES = binilint.cc
abinilintdir = $(bindir)

abicompat_SOURCES = abicompat.cc
abicompatdir = $(bindir)

abipkgdiff_SOURCES = abipkgdiff.cc
abipkgdiffdir = $(bindir)
abipkgdiff_CPPFLAGS = $(FTS_CFLAGS)
abipkgdiff_LDADD = $(FTS_LIBS)
abipkgdiff_LDFLAGS = $(abs_top_builddir)/src/libabigail.la -pthread

kmidiff_SOURCES = kmidiff.cc
kmidiffdir = $(bindir)
kmidiff_LDADD = $(abs_top_builddir)/src/libabigail.la
kmidiff_LDFLAGS = $(abs_top_builddir)/src/libabigail.la -pthread

AM_CXXFLAGS = \
$(VISIBILITY_FLAGS) -I$(abs_top_srcdir)/include \
-I$(abs_top_srcdir)/tools -fPIC
