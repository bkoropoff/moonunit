pkglib_LTLIBRARIES = xml.la console.la c.la

MODULE_LDFLAGS = -module -avoid-version -export-dynamic

xml_la_SOURCES = xml.c
xml_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
xml_la_LDFLAGS = $(MODULE_LDFLAGS)

console_la_SOURCES = console.c
console_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la
console_la_LDFLAGS = $(MODULE_LDFLAGS)

c_la_SOURCES = c.c c-run.c c-load.c backtrace.c
c_la_LIBADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @LIB_pthread@
c_la_LDFLAGS = $(MODULE_LDFLAGS)

if HAVE_LIBELF
c_la_SOURCES += elfscan.c
c_la_LIBADD += -lelf
endif

if CPLUSPLUS_ENABLED
c_la_SOURCES += cplusplus.cpp
c_la_LINK = $(CXXLINK) $(c_la_LDFLAGS)
else
# Automake will always try to link with the C++ linker
# since C++ sources are included in some conditions.  Force
# it to use the C linker when C++ support is disabled.  This
# fixes building on HP-UX
c_la_LINK = $(LINK) $(c_la_LDFLAGS)
endif

EXTRA_DIST = elfscan.h backtrace.h c-token.h cplusplus.h

AM_CPPFLAGS = -I$(top_srcdir)/include

pkglibdir=@plugindir@
