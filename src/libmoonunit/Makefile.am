AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=libmoonunit.la

libmoonunit_la_SOURCES = \
	error.c util.c test.c harness.c logger.c loader.c plugin.c option.c

libmoonunit_la_LIBADD = -ldl $(top_builddir)/src/libuipc/libuipc.la

AM_CPPFLAGS = \
	-I@top_srcdir@/include \
	-DPLUGIN_PATH="\"$(plugindir)\"" \
	-DPLUGIN_EXTENSION="\"$(pluginext)\""




