bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-strip

check_LTLIBRARIES = libexample.la

moonunit_SOURCES = main.c option.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @POPT_LIBADD@

libexample_la_SOURCES = example.c

EXTRA_DIST = moonunit-lt.sh moonunit-strip.sh option.h

moonunit-lt: moonunit-lt.sh moonunit
	cp $< $@
	chmod +x $@

moonunit-strip: moonunit-strip.sh
	cp $< $@
	chmod +x $@


AM_CPPFLAGS = -I$(top_srcdir)/include @POPT_INCLUDES@
