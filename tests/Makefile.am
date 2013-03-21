h=$(abs_srcdir)
hb=$(abs_builddir)

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
runtestreadwrite_LDADD=$(hb)/libtestutils.la $(top_builddir)/src/libabigail.la

AM_CPPFLAGS=-I${top_srcdir}/src

EXTRA_DIST= \
$(h)/data/test-read-write/input0.xml
