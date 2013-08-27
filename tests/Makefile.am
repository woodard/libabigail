h=$(abs_srcdir)

TESTS=				 \
runtestreadwrite		 \
runtestwritereadarchive		 \
runtestsvg			 \
runtestdot 

noinst_PROGRAMS= $(TESTS) testwalker

noinst_LTLIBRARIES = libtestutils.la

libtestutils_la_SOURCES=	\
test-utils.h 			\
test-utils.cc

libtestutils_la_CXXFLAGS=		\
-DABIGAIL_SRC_DIR=\"${top_srcdir}\"  	\
-DABIGAIL_BUILD_DIR=\"${top_builddir}\"

runtestreadwrite_SOURCES=$(h)/test-read-write.cc
runtestreadwrite_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestwritereadarchive_SOURCES=$(h)/test-write-read-archive.cc
runtestwritereadarchive_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

runtestsvg_SOURCES=$(h)/test-svg.cc
runtestsvg_LDADD=$(top_builddir)/src/libabigail.la

runtestdot_SOURCES=$(h)/test-dot.cc
runtestdot_LDADD=$(top_builddir)/src/libabigail.la

testwalker_SOURCES=$(h)/test-walker.cc
testwalker_LDADD=$(top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I${top_srcdir}/include -I${top_builddir}/include
AM_CXXFLAGS="-std=gnu++11"

EXTRA_DIST=				\
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
\
data/test-write-read-archive/test0.xml \
data/test-write-read-archive/test1.xml \
data/test-write-read-archive/test2.xml \
data/test-write-read-archive/test3.xml \
data/test-write-read-archive/test4.xml

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	-rm -rf ${builddir}/output *.svg *.gv
