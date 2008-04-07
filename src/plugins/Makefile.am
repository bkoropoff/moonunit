pkglib_LTLIBRARIES = xml.la console.la unix.la

MODULE_LDFLAGS = -module -avoid-version -export-dynamic

xml_la_SOURCES = xml.c
xml_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
xml_la_LDFLAGS = $(MODULE_LDFLAGS)

console_la_SOURCES = console.c
console_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
console_la_LDFLAGS = $(MODULE_LDFLAGS)

unix_la_SOURCES = unix.c unixharness.c unixloader.c backtrace.c
unix_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
unix_la_LDFLAGS = $(MODULE_LDFLAGS)

if HAVE_LIBELF
unix_la_SOURCES += elfscan.c
unix_la_LIBADD += -lelf
endif

if CPLUSPLUS_ENABLED
unix_la_SOURCES += cplusplus.cpp
endif

EXTRA_DIST = elfscan.h

AM_CPPFLAGS = -I$(top_srcdir)/include

pkglibdir=@plugindir@
