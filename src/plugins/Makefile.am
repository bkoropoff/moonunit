pkglib_LTLIBRARIES = xml.la console.la unix.la

xml_la_SOURCES = xml.c
xml_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
xml_la_LDFLAGS = -module

console_la_SOURCES = console.c
console_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
console_la_LDFLAGS = -module

unix_la_SOURCES = unix.c unixharness.c unixloader.c
if HAVE_LIBELF
	unix_la_SOURCES += elfscan.c
endif
unix_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
unix_la_LDFLAGS = -module

EXTRA_DIST = elfscan.h

AM_CPPFLAGS = -I$(top_srcdir)/include

pkglibdir=@plugindir@
