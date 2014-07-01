AUTOMAKE_OPTIONS=subdir-objects

h=$(abs_srcdir)

TESTS=				\
runtestreadwrite		\
runtestwritereadarchive		\
runtestreaddwarf		\
runtestlookupsyms		\
runtestcorediff			\
runtestbidiff			\
runtestdiffdwarf		\
runtestdifffilter		\
runtestsvg			\
runtestdot 

noinst_PROGRAMS= $(TESTS) testirwalker testdiff2

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

runtestcorediff_SOURCES=test-core-diff.cc
runtestcorediff_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestbidiff_SOURCES = test-bidiff.cc
runtestbidiff_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdiffdwarf_SOURCES = test-diff-dwarf.cc
runtestdiffdwarf_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestdifffilter_SOURCES = test-diff-filter.cc
runtestdifffilter_LDADD = libtestutils.la $(top_builddir)/src/libabigail.la

runtestsvg_SOURCES=test-svg.cc
runtestsvg_LDADD=$(top_builddir)/src/libabigail.la

runtestdot_SOURCES=test-dot.cc
runtestdot_LDADD=$(top_builddir)/src/libabigail.la

testirwalker_SOURCES=test-ir-walker.cc
testirwalker_LDADD=$(top_builddir)/src/libabigail.la

testdiff2_SOURCES=test-diff2.cc
testdiff2_LDADD=$(top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I${abs_top_srcdir}/include \
-I${abs_top_builddir}/include -I${abs_top_srcdir}/tools
AM_CXXFLAGS="-std=gnu++11"

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
\
data/test-write-read-archive/test0.xml \
data/test-write-read-archive/test1.xml \
data/test-write-read-archive/test2.xml \
data/test-write-read-archive/test3.xml \
data/test-write-read-archive/test4.xml \
\
data/test-bidiff/test-enum0-v0.cc.bi		\
data/test-bidiff/test-enum0-v1.cc.bi		\
data/test-bidiff/test-enum0-report.txt		\
data/test-bidiff/test-enum1-v0.cc.bi		\
data/test-bidiff/test-enum1-v1.cc.bi		\
data/test-bidiff/test-enum1-report.txt		\
data/test-bidiff/test-qual-type0-v0.cc.bi	\
data/test-bidiff/test-qual-type0-v1.cc.bi	\
data/test-bidiff/test-qual-type0-report.txt	\
data/test-bidiff/test-struct0-v0.cc.bi		\
data/test-bidiff/test-struct0-v1.cc.bi		\
data/test-bidiff/test-struct0-report.txt	\
data/test-bidiff/test-struct1-v0.cc.bi	 	\
data/test-bidiff/test-struct1-v1.cc.bi		\
data/test-bidiff/test-struct1-report.txt	\
data/test-bidiff/test-var0-v0.cc.bi		\
data/test-bidiff/test-var0-v1.cc.bi		\
data/test-bidiff/test-var0-report.txt		\
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
tests/data/test-read-dwarf/test4.c		\
tests/data/test-read-dwarf/test4.so		\
tests/data/test-read-dwarf/test5.cc		\
tests/data/test-read-dwarf/test5.so		\
tests/data/test-read-dwarf/test6.cc		\
tests/data/test-read-dwarf/test6.so		\
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
data/test-lookup-syms/test1-3-report.txt

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv
