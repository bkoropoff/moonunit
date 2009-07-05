pkglibdir=@plugindir@
pkglib_LTLIBRARIES = xml.la
AM_CPPFLAGS = -I$(top_srcdir)/include $(MU_CPPFLAGS)
AM_CFLAGS = $(MU_CFLAGS)
AM_LDFLAGS = $(MU_LDFLAGS)
MODULE_LDFLAGS = -shared -module -avoid-version

xml_la_SOURCES = xml.c
xml_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
xml_la_LDFLAGS = $(MODULE_LDFLAGS)
