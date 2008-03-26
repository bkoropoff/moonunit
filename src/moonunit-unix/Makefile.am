pkglib_LTLIBRARIES = unix.la

unix_la_SOURCES = \
	unix.c unixharness.c unixloader.c
unix_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
unix_la_LDFLAGS = -module

EXTRA_DIST = elfscan.h

AM_CPPFLAGS = -I$(top_srcdir)/include

pkglibdir=@plugindir@

if HAVE_LIBELF
unix_la_SOURCES += elfscan.c
unix_la_LIBADD += @LIBELF_LIBADD@
AM_CPPFLAGS += @LIBELF_INCLUDES@
endif