bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-strip moonunit-stub

noinst_LTLIBRARIES = libexample.la
lib_LTLIBRARIES = libexample_mu.la

moonunit_SOURCES = main.c option.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @POPT_LIBADD@

libexample_la_SOURCES = example.c
libexample_mu_la_SOURCES = example_mu.c
libexample_mu_la_LIBADD = libexample.la

EXTRA_DIST = moonunit-lt.sh moonunit-strip.sh moonunit-stub.sh option.h
BUILT_SOURCES = example_stub.c

moonunit-lt: moonunit-lt.sh moonunit
	cp $< $@
	chmod +x $@

moonunit-strip: moonunit-strip.sh
	cp $< $@
	chmod +x $@

moonunit-stub: moonunit-stub.sh
	cp $< $@
	chmod +x $@

example_mu.c: example.c moonunit-stub
	CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" CPP="$(CPP)" $(builddir)/moonunit-stub $(srcdir)/example.c > $@

AM_CPPFLAGS = -I$(top_srcdir)/include @POPT_INCLUDES@
