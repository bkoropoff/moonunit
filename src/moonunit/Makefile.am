bin_PROGRAMS = moonunit

lib_LTLIBRARIES = libtest.la

moonunit_SOURCES = main.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la

libtest_la_SOURCES = test.c
libtest_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la

INCLUDES = -I$(top_srcdir)/include
