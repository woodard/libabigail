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

FEDABIPKGDIFF_TEST =
if ENABLE_FEDABIPKGDIFF
FEDABIPKGDIFF_TEST += runtestfedabipkgdiff.py
endif

TESTS=				\
runtestreaddwarf		\
runtestcanonicalizetypes.sh	\
runtestdiffpkg			\
runtestdifffilter		\
$(ZIP_ARCHIVE_TESTS)		\
runtestdiffsuppr		\
runtestreadwrite		\
runtestabidiff			\
runtestdiffdwarfabixml		\
runtestabicompat		\
runtesttypesstability		\
runtestdiffdwarf		\
runtestlookupsyms		\
runtestaltdwarf			\
runtestcorediff			\
runtestabidiffexit		\
runtestdefaultsupprs.py		\
$(FEDABIPKGDIFF_TEST) $(CXX11_TESTS)


EXTRA_DIST = \
runtestcanonicalizetypes.sh.in \
runtestfedabipkgdiff.py.in \
mockfedabipkgdiff.in \
test-valgrind-suppressions.supp

CLEANFILES = \
runtestcanonicalizetypes.output.txt \
runtestcanonicalizetypes.output.final.txt

noinst_PROGRAMS= $(TESTS) testirwalker testdiff2 printdifftree
noinst_SCRIPTS = mockfedabipkgdiff
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
runtestreaddwarf_LDFLAGS=-pthread

runtestlookupsyms_SOURCES=test-lookup-syms.cc
runtestlookupsyms_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestaltdwarf_SOURCES=test-alt-dwarf-file.cc
runtestaltdwarf_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestcorediff_SOURCES=test-core-diff.cc
runtestcorediff_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestabidiff_SOURCES = test-abidiff.cc
runtestabidiff_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestabidiffexit_SOURCES = test-abidiff-exit.cc
runtestabidiffexit_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarf_SOURCES = test-diff-dwarf.cc
runtestdiffdwarf_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdifffilter_SOURCES = test-diff-filter.cc
runtestdifffilter_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffsuppr_SOURCES = test-diff-suppr.cc
runtestdiffsuppr_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarfabixml_SOURCES = test-diff-dwarf-abixml.cc
runtestdiffdwarfabixml_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestabicompat_SOURCES = test-abicompat.cc
runtestabicompat_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffpkg_SOURCES = test-diff-pkg.cc
runtestdiffpkg_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtesttypesstability_SOURCES = test-types-stability.cc
runtesttypesstability_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

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

runtestfedabipkgdiff_py_SOURCES =
runtestfedabipkgdiff.py$(EXEEXT):

runtestdefaultsupprs_py_SOURCES =
runtestdefaultsupprs.py$(EXEEXT):

AM_CPPFLAGS=-I${abs_top_srcdir}/include \
-I${abs_top_builddir}/include -I${abs_top_srcdir}/tools -fPIC

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv

@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = ${srcdir}/test-valgrind-suppressions.supp

# These are flags passed to Valgrind so that it follows children
# processes of the test programs.  This is because many test programs
# actually fork libabigail command line tools, and we want to valgrind
# those libabigail command line tools as well.
#
# The problem though is that there are many other command line
# programs that are forked too.  For instance, /bin/sh is forked
# because tests are using the system() function to execute commands.
# So we must tell Valgrind to avoid following a bunch of command line
# programs that we don't care about and that take a lot of time to
# valgrind.
RECURSIVE_VALGRIND_FLAGS = \
--num-callers=30 \
--trace-children=yes \
--trace-children-skip=/bin/diff,/bin/rm,/bin/mkdir,/bin/cd,*cpio,/bin/dpkg,/bin/rpm,/bin/test,/bin/tar,/bin/sed,/bin/ls,/bin/g++,/*collect*,/bin/ld

# Use the target below to run tests under Valgrind's memcheck tool by
# telling it to follow children process, so that it can also check
# libabigail tools that are forked by the tests.  This usually takes a
# while.  So, to launch just one test in this configuration, if you
# are in the top-most source directory you can do:
#
#  make -C <build-directory>/tests check-valgrind-memcheck-recursive TESTS=runtestdiffsuppr

check-valgrind-memcheck-recursive:
	$(MAKE) check-valgrind-memcheck VALGRIND_FLAGS="${RECURSIVE_VALGRIND_FLAGS}"
