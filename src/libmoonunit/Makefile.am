AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=libmoonunit.la

libmoonunit_la_SOURCES = \
	util.c test.c harness.c runner.c unixloader.c unixharness.c

libmoonunit_la_LIBADD = -ldl $(top_builddir)/src/liburpc/liburpc.la

AM_CPPFLAGS = -I@top_srcdir@/include

if HAVE_LIBELF
libmoonunit_la_SOURCES += elfscan.c
libmoonunit_la_LIBADD += @LIBELF_LIBADD@
AM_CPPFLAGS += @LIBELF_INCLUDES@
endif



