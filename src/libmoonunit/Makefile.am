AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=libmoonunit.la

libmoonunit_la_SOURCES= \
	util.c test.c harness.c runner.c unixloader.c unixharness.c

libmoonunit_la_LIBADD = -ldl $(top_builddir)/src/liburpc/liburpc.la

INCLUDES = -I@top_srcdir@/include
