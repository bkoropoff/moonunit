AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=libmoonunit.la

libmoonunit_la_SOURCES = \
	util.c test.c harness.c runner.c logger.c plugin.c option.c

libmoonunit_la_LIBADD = -ldl $(top_builddir)/src/liburpc/liburpc.la

AM_CPPFLAGS = \
	-I@top_srcdir@/include \
	-DPLUGIN_PATH="\"$(plugindir)\"" \
	-DPLUGIN_EXTENSION="\"$(pluginext)\""




