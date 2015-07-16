SUBDIRS = data

ZIP_ARCHIVE_TESTS =
if ENABLE_ZIP_ARCHIVE
ZIP_ARCHIVE_TESTS += runtestwritereadarchive
if ENABLE_CXX11
ZIP_ARCHIVE_TESTS += runtestdot
endif
endif

CXX11_TESTS =
if ENABLE_CXX11
CXX11_TESTS += runtestsvg
AM_CXXFLAGS = "-std=gnu++11"
endif

TESTS=				\
runtestreadwrite		\
$(ZIP_ARCHIVE_TESTS)		\
runtestreaddwarf		\
runtestlookupsyms		\
runtestaltdwarf			\
runtestcorediff			\
runtestabidiff			\
runtestdiffdwarf		\
runtestcanonicalizetypes.sh	\
runtestdifffilter		\
runtestdiffsuppr		\
runtestabicompat		\
runtestdiffpkg			\
$(CXX11_TESTS)

EXTRA_DIST = runtestcanonicalizetypes.sh.in
CLEANFILES = \
 runtestcanonicalizetypes.output.txt \
 runtestcanonicalizetypes.output.final.txt

noinst_PROGRAMS= $(TESTS) testirwalker testdiff2 printdifftree

noinst_LTLIBRARIES = libtestutils.la

libtestutils_la_SOURCES=	\
test-utils.h 			\
test-utils.cc

libtestutils_la_CXXFLAGS=		\
-DABIGAIL_SRC_DIR=\"${abs_top_srcdir}\"  	\
-DABIGAIL_BUILD_DIR=\"${abs_top_builddir}\"

runtestreadwrite_SOURCES=test-read-write.cc
runtestreadwrite_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestwritereadarchive_SOURCES=test-write-read-archive.cc
runtestwritereadarchive_LDADD= libtestutils.la $(top_builddir)/src/libabigail.la

runtestreaddwarf_SOURCES=test-read-dwarf.cc
runtestreaddwarf_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestlookupsyms_SOURCES=test-lookup-syms.cc
runtestlookupsyms_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestaltdwarf_SOURCES=test-alt-dwarf-file.cc
runtestaltdwarf_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestcorediff_SOURCES=test-core-diff.cc
runtestcorediff_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestabidiff_SOURCES = test-abidiff.cc
runtestabidiff_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarf_SOURCES = test-diff-dwarf.cc
runtestdiffdwarf_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdifffilter_SOURCES = test-diff-filter.cc
runtestdifffilter_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffsuppr_SOURCES = test-diff-suppr.cc
runtestdiffsuppr_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestabicompat_SOURCES = test-abicompat.cc
runtestabicompat_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffpkg_SOURCES = test-diff-pkg.cc
runtestdiffpkg_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestsvg_SOURCES=test-svg.cc
runtestsvg_LDADD=$(top_builddir)/src/libabigail.la

runtestdot_SOURCES = test-dot.cc
runtestdot_LDADD = $(top_builddir)/src/libabigail.la

testirwalker_SOURCES=test-ir-walker.cc
testirwalker_LDADD=$(top_builddir)/src/libabigail.la

testdiff2_SOURCES=test-diff2.cc
testdiff2_LDADD=$(top_builddir)/src/libabigail.la

printdifftree_SOURCES = print-diff-tree.cc
printdifftree_LDADD = $(top_builddir)/src/libabigail.la

runtestcanonicalizetypes_sh_SOURCES =
runtestcanonicalizetypes.sh$(EXEEXT):

AM_CPPFLAGS=-I${abs_top_srcdir}/include \
-I${abs_top_builddir}/include -I${abs_top_srcdir}/tools -fPIC

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv
