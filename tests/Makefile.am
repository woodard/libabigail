h=$(abs_srcdir)

TESTS=runtestreadwrite

noinst_PROGRAMS= $(TESTS)

noinst_LTLIBRARIES = libtestutils.la

libtestutils_la_SOURCES=\
$(h)/test-utils.h \
$(h)/test-utils.cc

libtestutils_la_CXXFLAGS=		\
-DABIGAIL_SRC_DIR=\"${top_srcdir}\"  	\
-DABIGAIL_BUILD_DIR=\"${top_builddir}\"

runtestreadwrite_SOURCES=$(h)/test-read-write.cc
runtestreadwrite_LDADD=libtestutils.la $(top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I${top_srcdir}/src

EXTRA_DIST=				\
$(h)/data/test-read-write/test0.xml	\
$(h)/data/test-read-write/test1.xml	\
$(h)/data/test-read-write/test2.xml	\
$(h)/data/test-read-write/test3.xml	\
$(h)/data/test-read-write/test4.xml	\
$(h)/data/test-read-write/test5.xml	\
$(h)/data/test-read-write/test6.xml	\
$(h)/data/test-read-write/test7.xml	\
$(h)/data/test-read-write/test8.xml	\
$(h)/data/test-read-write/test9.xml	\
$(h)/data/test-read-write/test10.xml	\
$(h)/data/test-read-write/test11.xml	\
$(h)/data/test-read-write/test12.xml
