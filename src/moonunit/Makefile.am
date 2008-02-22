bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-strip moonunit-stub

check_LTLIBRARIES = libexample.la libexample_mu.la

moonunit_SOURCES = main.c option.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @POPT_LIBADD@

libexample_la_SOURCES = example.c
libexample_mu_la_SOURCES = example_mu.c
libexample_mu_la_LIBADD = libexample.la
libexample_mu_la_LDFLAGS = -module -rpath $(libdir)

EXTRA_DIST = moonunit-lt.sh moonunit-strip.sh moonunit-stub.sh option.h
BUILT_SOURCES = example_mu.c

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

CHECK_PLUGINS= \
	--plugin $(top_builddir)/src/moonunit-unix/unix.la \
	--plugin $(top_builddir)/src/moonunit-misc/console.la \
	--plugin $(top_builddir)/src/moonunit-misc/xml.la	

check-local: moonunit-lt libexample_mu.la
	./moonunit-lt $(CHECK_ARGS) $(CHECK_PLUGINS) libexample_mu.la

