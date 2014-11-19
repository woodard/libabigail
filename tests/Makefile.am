h=$(abs_srcdir)

if ENABLE_ZIP_ARCHIVE
ZIP_ARCHIVE_TESTS_FIRST_PART = runtestwritereadarchive
else
ZIP_ARCHIVE_TESTS_FIRST_PART =
endif

if ENABLE_CXX11
CXX11_TESTS = runtestsvg
AM_CXXFLAGS = "-std=gnu++11"
else
CXX11_TESTS =
endif

if ENABLE_ZIP_ARCHIVE_AND_CXX11
ZIP_ARCHIVE_TESTS_SECOND_PART = runtestdot
else
ZIP_ARCHIVE_TESTS_SECOND_PART =
endif

ZIP_ARCHIVE_TESTS = $(ZIP_ARCHIVE_TESTS_FIRST_PART) $(ZIP_ARCHIVE_TESTS_SECOND_PART)

TESTS=				\
runtestreadwrite		\
$(ZIP_ARCHIVE_TESTS)		\
runtestreaddwarf		\
runtestlookupsyms		\
runtestaltdwarf			\
runtestcorediff			\
runtestabidiff			\
runtestdiffdwarf		\
runtestdifffilter		\
runtestdiffsuppr		\
$(CXX11_TESTS)


noinst_PROGRAMS= $(TESTS) testirwalker testdiff2 printdifftree

noinst_LTLIBRARIES = libtestutils.la

libtestutils_la_SOURCES=	\
test-utils.h 			\
test-utils.cc

libtestutils_la_LIBADD=$(top_builddir)/tools/libtoolsutils.la
libtestutils_la_DEPENDENCIES=$(top_builddir)/tools/libtoolsutils.la

libtestutils_la_CXXFLAGS=		\
-DABIGAIL_SRC_DIR=\"${abs_top_srcdir}\"  	\
-DABIGAIL_BUILD_DIR=\"${abs_top_builddir}\"

runtestreadwrite_SOURCES=test-read-write.cc
runtestreadwrite_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestwritereadarchive_SOURCES=test-write-read-archive.cc
runtestwritereadarchive_LDADD= libtestutils.la $(top_builddir)/src/libabigail.la $(top_builddir)/tools/libtoolsutils.la

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

AM_CPPFLAGS=-I${abs_top_srcdir}/include \
-I${abs_top_builddir}/include -I${abs_top_srcdir}/tools

EXTRA_DIST=			\
data/test-read-write/test0.xml	\
data/test-read-write/test1.xml	\
data/test-read-write/test2.xml	\
data/test-read-write/test3.xml	\
data/test-read-write/test4.xml	\
data/test-read-write/test5.xml	\
data/test-read-write/test6.xml	\
data/test-read-write/test7.xml	\
data/test-read-write/test8.xml	\
data/test-read-write/test9.xml	\
data/test-read-write/test10.xml	\
data/test-read-write/test11.xml	\
data/test-read-write/test12.xml	\
data/test-read-write/test13.xml	\
data/test-read-write/test14.xml	\
data/test-read-write/test15.xml	\
data/test-read-write/test16.xml \
data/test-read-write/test17.xml \
data/test-read-write/test18.xml \
data/test-read-write/test19.xml \
data/test-read-write/test20.xml \
data/test-read-write/test21.xml \
data/test-read-write/test22.xml \
data/test-read-write/test23.xml \
data/test-read-write/test24.xml \
data/test-read-write/test25.xml \
\
data/test-write-read-archive/test0.xml \
data/test-write-read-archive/test1.xml \
data/test-write-read-archive/test2.xml \
data/test-write-read-archive/test3.xml \
data/test-write-read-archive/test4.xml \
\
data/test-abidiff/test-enum0-v0.cc.bi		\
data/test-abidiff/test-enum0-v1.cc.bi		\
data/test-abidiff/test-enum0-report.txt		\
data/test-abidiff/test-enum1-v0.cc.bi		\
data/test-abidiff/test-enum1-v1.cc.bi		\
data/test-abidiff/test-enum1-report.txt		\
data/test-abidiff/test-qual-type0-v0.cc.bi	\
data/test-abidiff/test-qual-type0-v1.cc.bi	\
data/test-abidiff/test-qual-type0-report.txt	\
data/test-abidiff/test-struct0-v0.cc.bi		\
data/test-abidiff/test-struct0-v1.cc.bi		\
data/test-abidiff/test-struct0-report.txt	\
data/test-abidiff/test-struct1-v0.cc.bi	 	\
data/test-abidiff/test-struct1-v1.cc.bi		\
data/test-abidiff/test-struct1-report.txt	\
data/test-abidiff/test-var0-v0.cc.bi		\
data/test-abidiff/test-var0-v1.cc.bi		\
data/test-abidiff/test-var0-report.txt		\
\
data/test-diff-dwarf/test0-v0.cc		\
data/test-diff-dwarf/test0-v0.o			\
data/test-diff-dwarf/test0-v1.cc		\
data/test-diff-dwarf/test0-v1.o			\
data/test-diff-dwarf/test0-report.txt		\
data/test-diff-dwarf/test1-v0.cc		\
data/test-diff-dwarf/test1-v0.o			\
data/test-diff-dwarf/test1-v1.cc		\
data/test-diff-dwarf/test1-v1.o			\
data/test-diff-dwarf/test1-report.txt		\
data/test-diff-dwarf/test2-v0.cc		\
data/test-diff-dwarf/test2-v0.o			\
data/test-diff-dwarf/test2-v1.cc		\
data/test-diff-dwarf/test2-v1.o			\
data/test-diff-dwarf/test2-report.txt		\
data/test-diff-dwarf/test3-v0.cc		\
data/test-diff-dwarf/test3-v0.o			\
data/test-diff-dwarf/test3-v1.cc		\
data/test-diff-dwarf/test3-v1.o			\
data/test-diff-dwarf/test3-report.txt		\
data/test-diff-dwarf/test4-v0.cc		\
data/test-diff-dwarf/test4-v0.o			\
data/test-diff-dwarf/test4-v1.cc		\
data/test-diff-dwarf/test4-v1.o			\
data/test-diff-dwarf/test4-report.txt		\
data/test-diff-dwarf/test5-v0.cc		\
data/test-diff-dwarf/test5-v0.o			\
data/test-diff-dwarf/test5-v1.cc		\
data/test-diff-dwarf/test5-v1.o			\
data/test-diff-dwarf/test5-report.txt		\
data/test-diff-dwarf/test6-v0.cc		\
data/test-diff-dwarf/test6-v0.o			\
data/test-diff-dwarf/test6-v1.cc		\
data/test-diff-dwarf/test6-v1.o			\
data/test-diff-dwarf/test6-report.txt		\
data/test-diff-dwarf/test7-v0.cc		\
data/test-diff-dwarf/test7-v0.o			\
data/test-diff-dwarf/test7-v1.cc		\
data/test-diff-dwarf/test7-v1.o			\
data/test-diff-dwarf/test7-report.txt		\
data/test-diff-dwarf/test8-v0.cc		\
data/test-diff-dwarf/test8-v0.o			\
data/test-diff-dwarf/test8-v1.cc		\
data/test-diff-dwarf/test8-v1.o			\
data/test-diff-dwarf/test8-report.txt		\
data/test-diff-dwarf/test9-v0.cc		\
data/test-diff-dwarf/libtest9-v0.so		\
data/test-diff-dwarf/test9-v1.cc		\
data/test-diff-dwarf/libtest9-v1.so		\
data/test-diff-dwarf/test9-report.txt		\
data/test-diff-dwarf/test10-v0.cc		\
data/test-diff-dwarf/test10-v0.o		\
data/test-diff-dwarf/test10-v1.cc		\
data/test-diff-dwarf/test10-v1.o		\
data/test-diff-dwarf/test10-report.txt		\
data/test-diff-dwarf/test11-v0.cc		\
data/test-diff-dwarf/test11-v0.o		\
data/test-diff-dwarf/test11-v1.cc		\
data/test-diff-dwarf/test11-v1.o		\
data/test-diff-dwarf/test11-report.txt		\
data/test-diff-dwarf/test12-v0.c		\
data/test-diff-dwarf/libtest12-v0.so		\
data/test-diff-dwarf/test12-v1.c		\
data/test-diff-dwarf/libtest12-v1.so		\
data/test-diff-dwarf/test12-version-script	\
data/test-diff-dwarf/test12-report.txt		\
data/test-diff-dwarf/test13-v0.cc		\
data/test-diff-dwarf/test13-v0.o		\
data/test-diff-dwarf/test13-v1.cc		\
data/test-diff-dwarf/test13-v1.o		\
data/test-diff-dwarf/test13-report.txt		\
data/test-diff-dwarf/test14-inline-v0.cc	\
data/test-diff-dwarf/test14-inline-v0.o		\
data/test-diff-dwarf/test14-inline-v1.cc	\
data/test-diff-dwarf/test14-inline-v1.o		\
data/test-diff-dwarf/test14-inline-report.txt	\
data/test-diff-dwarf/test15-enum-v0.cc		\
data/test-diff-dwarf/test15-enum-v0.o		\
data/test-diff-dwarf/test15-enum-v1.cc		\
data/test-diff-dwarf/test15-enum-v1.o		\
data/test-diff-dwarf/test15-enum-report.txt	\
tests/data/test-diff-dwarf/test16-syms-only-v0.o \
tests/data/test-diff-dwarf/test16-syms-only-v1.o \
tests/data/test-diff-dwarf/test16-syms-only-v0.cc \
tests/data/test-diff-dwarf/test16-syms-only-v1.cc \
tests/data/test-diff-dwarf/test17-non-refed-syms-v0.o \
tests/data/test-diff-dwarf/test17-non-refed-syms-v1.o \
tests/data/test-diff-dwarf/test17-non-refed-syms-v0.cc \
tests/data/test-diff-dwarf/test17-non-refed-syms-v1.cc \
tests/data/test-diff-dwarf/libtest18-alias-sym-v0.so \
tests/data/test-diff-dwarf/libtest18-alias-sym-v1.so \
tests/data/test-diff-dwarf/test18-alias-sym-report-0.txt \
tests/data/test-diff-dwarf/test18-alias-sym-v0.cc \
tests/data/test-diff-dwarf/test18-alias-sym-v1.cc \
tests/data/test-diff-dwarf/test18-alias-sym-version-script \
\
data/test-read-dwarf/test0			\
data/test-read-dwarf/test0.abi			\
data/test-read-dwarf/test0.cc			\
data/test-read-dwarf/test1			\
data/test-read-dwarf/test1.abi			\
data/test-read-dwarf/test1.cc			\
tests/data/test-read-dwarf/test2.h		\
tests/data/test-read-dwarf/test2-0.cc		\
tests/data/test-read-dwarf/test2-1.cc		\
tests/data/test-read-dwarf/test2.so		\
tests/data/test-read-dwarf/test2.so.abi		\
tests/data/test-read-dwarf/test3.c		\
tests/data/test-read-dwarf/test3.so		\
tests/data/test-read-dwarf/test3.so.abi		\
tests/data/test-read-dwarf/test4.c		\
tests/data/test-read-dwarf/test4.so		\
tests/data/test-read-dwarf/test4.so.abi		\
tests/data/test-read-dwarf/test5.cc		\
tests/data/test-read-dwarf/test5.so		\
tests/data/test-read-dwarf/test5.so.abi		\
tests/data/test-read-dwarf/test6.cc		\
tests/data/test-read-dwarf/test6.so		\
tests/data/test-read-dwarf/test6.so.abi		\
tests/data/test-read-dwarf/test7.cc		\
tests/data/test-read-dwarf/test7.so		\
tests/data/test-read-dwarf/test7.so.abi		\
tests/data/test-read-dwarf/test8-qualified-this-pointer.cc		\
tests/data/test-read-dwarf/test8-qualified-this-pointer.so		\
tests/data/test-read-dwarf/test8-qualified-this-pointer.so.abi		\
\
data/test-diff-filter/test0-v0.cc		\
data/test-diff-filter/test0-v1.cc		\
data/test-diff-filter/test0-v0.o		\
data/test-diff-filter/test0-v1.o		\
data/test-diff-filter/test0-report.txt		\
data/test-diff-filter/test1-v0.cc		\
data/test-diff-filter/test1-v1.cc		\
data/test-diff-filter/test1-v0.o		\
data/test-diff-filter/test1-v1.o		\
data/test-diff-filter/test1-report.txt		\
data/test-diff-filter/test2-v0.cc		\
data/test-diff-filter/test2-v1.cc		\
data/test-diff-filter/test2-v0.o		\
data/test-diff-filter/test2-v1.o		\
data/test-diff-filter/test2-report.txt		\
data/test-diff-filter/test3-v0.cc		\
data/test-diff-filter/test3-v1.cc		\
data/test-diff-filter/test3-v0.o		\
data/test-diff-filter/test3-v1.o		\
data/test-diff-filter/test3-report.txt		\
data/test-diff-filter/test4-v0.cc		\
data/test-diff-filter/test4-v1.cc		\
data/test-diff-filter/test4-v0.o		\
data/test-diff-filter/test4-v1.o		\
data/test-diff-filter/test4-report.txt		\
data/test-diff-filter/test5-v0.cc		\
data/test-diff-filter/test5-v1.cc		\
data/test-diff-filter/test5-v0.o		\
data/test-diff-filter/test5-v1.o		\
data/test-diff-filter/test5-report.txt		\
data/test-diff-filter/test6-v0.cc		\
data/test-diff-filter/test6-v1.cc		\
data/test-diff-filter/test6-v0.o		\
data/test-diff-filter/test6-v1.o		\
data/test-diff-filter/test6-report.txt		\
data/test-diff-filter/test7-v0.cc		\
data/test-diff-filter/test7-v1.cc		\
data/test-diff-filter/test7-v0.o		\
data/test-diff-filter/test7-v1.o		\
data/test-diff-filter/test7-report.txt		\
data/test-diff-filter/test8-v0.cc		\
data/test-diff-filter/test8-v1.cc		\
data/test-diff-filter/test8-v0.o		\
data/test-diff-filter/test8-v1.o		\
data/test-diff-filter/test8-report.txt		\
data/test-diff-filter/test9-v0.cc		\
data/test-diff-filter/test9-v1.cc		\
data/test-diff-filter/test9-v0.o		\
data/test-diff-filter/test9-v1.o		\
data/test-diff-filter/test9-report.txt		\
data/test-diff-filter/test10-v0.cc		\
data/test-diff-filter/test10-v1.cc		\
data/test-diff-filter/test10-v0.o		\
data/test-diff-filter/test10-v1.o		\
data/test-diff-filter/test10-report.txt		\
data/test-diff-filter/test11-v0.cc		\
data/test-diff-filter/test11-v1.cc		\
data/test-diff-filter/test11-v0.o		\
data/test-diff-filter/test11-v1.o		\
data/test-diff-filter/test11-report.txt		\
data/test-diff-filter/test12-v0.cc		\
data/test-diff-filter/test12-v1.cc		\
data/test-diff-filter/test12-v0.o		\
data/test-diff-filter/test12-v1.o		\
data/test-diff-filter/test12-report.txt		\
data/test-diff-filter/test13-v0.cc		\
data/test-diff-filter/test13-v1.cc		\
data/test-diff-filter/test13-v0.o		\
data/test-diff-filter/test13-v1.o		\
data/test-diff-filter/test13-report.txt		\
data/test-diff-filter/test14-v0.cc		\
data/test-diff-filter/test14-v1.cc		\
data/test-diff-filter/test14-v0.o		\
data/test-diff-filter/test14-v1.o		\
data/test-diff-filter/test14-0-report.txt	\
data/test-diff-filter/test14-1-report.txt	\
data/test-diff-filter/test15-v0.cc		\
data/test-diff-filter/test15-v1.cc		\
data/test-diff-filter/test15-v0.o		\
data/test-diff-filter/test15-v1.o		\
data/test-diff-filter/test15-0-report.txt	\
data/test-diff-filter/test15-1-report.txt	\
data/test-diff-filter/test16-v0.cc		\
data/test-diff-filter/test16-v1.cc		\
data/test-diff-filter/test16-v0.o		\
data/test-diff-filter/test16-v1.o		\
data/test-diff-filter/test16-report.txt		\
data/test-diff-filter/test17-v0.cc		\
data/test-diff-filter/test17-v1.cc		\
data/test-diff-filter/test17-v0.o		\
data/test-diff-filter/test17-v1.o		\
data/test-diff-filter/test17-0-report.txt	\
data/test-diff-filter/test17-1-report.txt	\
data/test-diff-filter/test18-v0.cc		\
data/test-diff-filter/test18-v1.cc		\
data/test-diff-filter/test18-v0.o		\
data/test-diff-filter/test18-v1.o		\
data/test-diff-filter/test18-report.txt		\
data/test-diff-filter/test19-v0.cc		\
data/test-diff-filter/test19-v1.cc		\
data/test-diff-filter/test19-v0.o		\
data/test-diff-filter/test19-v1.o		\
data/test-diff-filter/test19-report-0.txt	\
data/test-diff-filter/test19-report-1.txt	\
data/test-diff-filter/test20-inline-v0.cc	\
data/test-diff-filter/test20-inline-v1.cc	\
data/test-diff-filter/test20-inline-v0.o	\
data/test-diff-filter/test20-inline-v1.o	\
data/test-diff-filter/test20-inline-report-0.txt \
data/test-diff-filter/test20-inline-report-1.txt \
\
tests/data/test-diff-suppr/test0-type-suppr-v0.cc	\
tests/data/test-diff-suppr/test0-type-suppr-v1.cc	\
tests/data/test-diff-suppr/test0-type-suppr-v0.o	\
tests/data/test-diff-suppr/test0-type-suppr-v1.o	\
tests/data/test-diff-suppr/test0-type-suppr-0.suppr	\
tests/data/test-diff-suppr/test0-type-suppr-1.suppr	\
tests/data/test-diff-suppr/test0-type-suppr-2.suppr	\
tests/data/test-diff-suppr/test0-type-suppr-report-0.txt \
tests/data/test-diff-suppr/test0-type-suppr-report-1.txt \
tests/data/test-diff-suppr/test0-type-suppr-report-2.txt \
tests/data/test-diff-suppr/test0-type-suppr-report-3.txt \
tests/data/test-diff-suppr/test1-typedef-suppr.h	 \
tests/data/test-diff-suppr/test1-typedef-suppr-v0.c	 \
tests/data/test-diff-suppr/test1-typedef-suppr-v1.c	 \
tests/data/test-diff-suppr/test1-typedef-suppr-v0.o	 \
tests/data/test-diff-suppr/test1-typedef-suppr-v1.o	 \
tests/data/test-diff-suppr/test1-typedef-suppr-0.suppr	 \
tests/data/test-diff-suppr/test1-typedef-suppr-1.suppr 	 \
tests/data/test-diff-suppr/test1-typedef-suppr-report-0.txt \
tests/data/test-diff-suppr/test1-typedef-suppr-report-1.txt \
tests/data/test-diff-suppr/test1-typedef-suppr-report-2.txt \
tests/data/test-diff-suppr/test2-struct-suppr-0.suppr \
tests/data/test-diff-suppr/test2-struct-suppr-1.suppr \
tests/data/test-diff-suppr/test2-struct-suppr-report-0.txt \
tests/data/test-diff-suppr/test2-struct-suppr-report-1.txt \
tests/data/test-diff-suppr/test2-struct-suppr-v0.cc \
tests/data/test-diff-suppr/test2-struct-suppr-v1.cc \
tests/data/test-diff-suppr/test2-struct-suppr-v0.o \
tests/data/test-diff-suppr/test2-struct-suppr-v0.o \
tests/data/test-diff-suppr/test3-struct-suppr-0.suppr \
tests/data/test-diff-suppr/test3-struct-suppr-1.suppr \
tests/data/test-diff-suppr/test3-struct-suppr-report-0.txt \
tests/data/test-diff-suppr/test3-struct-suppr-report-1.txt \
tests/data/test-diff-suppr/test3-struct-suppr-report-2.txt \
tests/data/test-diff-suppr/test3-struct-suppr-v0.cc \
tests/data/test-diff-suppr/test3-struct-suppr-v0.o \
tests/data/test-diff-suppr/test3-struct-suppr-v1.cc \
tests/data/test-diff-suppr/test3-struct-suppr-v1.o \
tests/data/test-diff-suppr/libtest4-local-suppr-v0.so \
tests/data/test-diff-suppr/libtest4-local-suppr-v1.so \
tests/data/test-diff-suppr/test4-local-suppr-0.suppr \
tests/data/test-diff-suppr/test4-local-suppr-report-0.txt \
tests/data/test-diff-suppr/test4-local-suppr-report-1.txt \
tests/data/test-diff-suppr/test4-local-suppr-v0.c \
tests/data/test-diff-suppr/test4-local-suppr-v0.h \
tests/data/test-diff-suppr/test4-local-suppr-v1.c \
tests/data/test-diff-suppr/test4-local-suppr-v1.h \
tests/data/test-diff-suppr/libtest5-fn-suppr-v0.so \
tests/data/test-diff-suppr/libtest5-fn-suppr-v1.so \
tests/data/test-diff-suppr/test5-fn-suppr-0.suppr \
tests/data/test-diff-suppr/test5-fn-suppr-1.suppr \
tests/data/test-diff-suppr/test5-fn-suppr-2.suppr \
tests/data/test-diff-suppr/test5-fn-suppr-3.suppr \
tests/data/test-diff-suppr/test5-fn-suppr-4.suppr \
tests/data/test-diff-suppr/test5-fn-suppr-report-0.txt \
tests/data/test-diff-suppr/test5-fn-suppr-report-1.txt \
tests/data/test-diff-suppr/test5-fn-suppr-report-2.txt \
tests/data/test-diff-suppr/test5-fn-suppr-report-3.txt \
tests/data/test-diff-suppr/test5-fn-suppr-report-4.txt \
tests/data/test-diff-suppr/test5-fn-suppr-report-5.txt \
tests/data/test-diff-suppr/test5-fn-suppr-v0.cc \
tests/data/test-diff-suppr/test5-fn-suppr-v1.cc \
tests/data/test-diff-suppr/libtest6-fn-suppr-v0.so \
tests/data/test-diff-suppr/libtest6-fn-suppr-v1.so \
tests/data/test-diff-suppr/test6-fn-suppr-0.suppr \
tests/data/test-diff-suppr/test6-fn-suppr-1.suppr \
tests/data/test-diff-suppr/test6-fn-suppr-2.suppr \
tests/data/test-diff-suppr/test6-fn-suppr-3.suppr \
tests/data/test-diff-suppr/test6-fn-suppr-report-0.txt \
tests/data/test-diff-suppr/test6-fn-suppr-report-1.txt \
tests/data/test-diff-suppr/test6-fn-suppr-report-2.txt \
tests/data/test-diff-suppr/test6-fn-suppr-report-3.txt \
tests/data/test-diff-suppr/test6-fn-suppr-report-4.txt \
tests/data/test-diff-suppr/test6-fn-suppr-v0.cc \
tests/data/test-diff-suppr/test6-fn-suppr-v1.cc \
tests/data/test-diff-suppr/test6-fn-suppr-version-script \
tests/data/test-diff-suppr/libtest7-var-suppr-v0.so \
tests/data/test-diff-suppr/libtest7-var-suppr-v1.so \
tests/data/test-diff-suppr/test7-var-suppr-v0.cc \
tests/data/test-diff-suppr/test7-var-suppr-v1.cc \
tests/data/test-diff-suppr/test7-var-suppr-1.suppr \
tests/data/test-diff-suppr/test7-var-suppr-2.suppr \
tests/data/test-diff-suppr/test7-var-suppr-3.suppr \
tests/data/test-diff-suppr/test7-var-suppr-4.suppr \
tests/data/test-diff-suppr/test7-var-suppr-5.suppr \
tests/data/test-diff-suppr/test7-var-suppr-6.suppr \
tests/data/test-diff-suppr/test7-var-suppr-7.suppr \
tests/data/test-diff-suppr/test7-var-suppr-8.suppr \
tests/data/test-diff-suppr/test7-var-suppr-report-0.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-1.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-2.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-3.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-4.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-5.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-6.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-7.txt \
tests/data/test-diff-suppr/test7-var-suppr-report-8.txt \
tests/data/test-diff-suppr/test7-var-suppr-version-script \
\
data/test-lookup-syms/test0.cc			\
data/test-lookup-syms/test0.o			\
data/test-lookup-syms/test0-report.txt		\
data/test-lookup-syms/test01-report.txt		\
data/test-lookup-syms/test02-report.txt		\
data/test-lookup-syms/test1.c			\
data/test-lookup-syms/test1.version-script	\
data/test-lookup-syms/test1.so			\
data/test-lookup-syms/test1-1-report.txt	\
data/test-lookup-syms/test1-2-report.txt	\
data/test-lookup-syms/test1-3-report.txt	\
\
data/test-alt-dwarf-file/test0.cc		\
data/test-alt-dwarf-file/libtest0.so		\
data/test-alt-dwarf-file/test0-common.cc	\
data/test-alt-dwarf-file/libtest0-common.so	\
data/test-alt-dwarf-file/test0-report.txt	\
data/test-alt-dwarf-file/test0-debug-dir/test0-common-dwz.debug \
data/test-alt-dwarf-file/test0-debug-dir/.build-id/16/7088580c513b439c9ed95fe6a8b29496495f26.debug


clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv
